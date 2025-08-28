
#include "shell.h"
#include "prompt.h"
#include "parser.h"
#include "log.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h> // For signal handling

// Global variables
char initial_home[PATH_MAX] = "";
char prev_dir[PATH_MAX] = "";
char *command_history[MAX_HISTORY];
int history_count = 0;
SimpleCommand commands[MAX_COMMANDS_IN_PIPELINE];
int num_commands = 0;
pid_t foreground_pgid = 0; // Current foreground process group ID

// Signal handlers
void sigint_handler(int signo) {
    (void)signo;
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGINT); // Send SIGINT to foreground process group
    }
}

void sigtstp_handler(int signo) {
    (void)signo;
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGTSTP); // Send SIGTSTP to foreground process group
    }
}

char* read_input() {
    char *input = NULL;
    size_t bufsize = 0;
    ssize_t len = getline(&input, &bufsize, stdin);
    if (len == -1) { 
        if (input) free(input); 
        return NULL;
    }
    if (len > 0 && input[len-1] == '\n') { 
        input[len-1] = '\0'; 
    }
    return input;
}

int main() {
    // Setup signal handlers
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);   // Handle Ctrl+C
    signal(SIGTSTP, sigtstp_handler); // Handle Ctrl+Z

    get_current_dir(initial_home, sizeof(initial_home));
    // Leave prev_dir empty until first hop command
    load_history();

    char *username = get_username();
    char hostname[256];
    get_system_name(hostname, sizeof(hostname));

    while (1) {
        // The signal handler now deals with job completion messages automatically
        print_prompt(username, hostname, initial_home);

        char *input = read_input();
        if (input == NULL) { 
            // EOF received when shell is waiting for input (no foreground process)
            // Clean up background processes and exit
            for (int i = 0; i < job_count; i++) {
                if (jobs[i].pid > 0) {
                    kill(-jobs[i].pid, SIGKILL); // Kill process group
                }
            }
            printf("logout\n"); 
            fflush(stdout);
            exit(0); // Exit with status 0
        }
        if (strlen(input) == 0) { free(input); continue; }
        
        // Only add to history if it's not a 'log' command
        if (strncmp(input, "log", 3) != 0 || (input[3] != '\0' && !isspace(input[3]))) {
            add_to_history(input);
        }

        // Handle log execute command - always execute the command
        if (strncmp(input, "log execute", 11) == 0) {
            int index;
            char remaining_cmd[4096] = "";
            
            // Check if there's a pipeline or other operators after log execute
            char *pipe_pos = strchr(input, '|');
            if (pipe_pos) {
                strcpy(remaining_cmd, pipe_pos); // Save everything from | onwards
            }
            
            if (sscanf(input, "log execute %d", &index) == 1) {
                if (index > 0 && index <= history_count) {
                    // Get command from history (reverse indexing: 1 = most recent)
                    char* executed_command = strdup(command_history[history_count - index]);
                    
                    // Create new input with executed command + remaining pipeline
                    char* new_input = malloc(strlen(executed_command) + strlen(remaining_cmd) + 2);
                    strcpy(new_input, executed_command);
                    if (strlen(remaining_cmd) > 0) {
                        strcat(new_input, " ");
                        strcat(new_input, remaining_cmd);
                    }
                    
                    free(executed_command);
                    free(input);
                    input = new_input;
                } else {
                    fprintf(stderr, "log execute: Invalid index\n");
                    free(input);
                    continue; // Skip the rest of the loop
                }
            }
        }

        // Safer parsing loop from the previous step
        char *input_copy = strdup(input);
        
        // First, check syntax for all command chunks
        bool all_valid = true;
        char *syntax_copy = strdup(input);
        char *syntax_pos = syntax_copy;
        while (*syntax_pos) {
            char *delimiter = strpbrk(syntax_pos, ";&");
            if (delimiter) *delimiter = '\0';
            while (isspace(*syntax_pos)) syntax_pos++;
            
            // Handle background commands during syntax check too
            char *syntax_chunk = strdup(syntax_pos);
            size_t len = strlen(syntax_chunk);
            if (len > 0 && syntax_chunk[len-1] == '&') {
                syntax_chunk[len-1] = '\0';
                // Trim trailing spaces after removing &
                len--;
                while (len > 0 && isspace(syntax_chunk[len-1])) {
                    syntax_chunk[len-1] = '\0';
                    len--;
                }
            }
            
            if (*syntax_chunk != '\0' && !parse_command_group(syntax_chunk)) {
                all_valid = false;
                free(syntax_chunk);
                break;
            }
            free(syntax_chunk);
            
            if (delimiter) syntax_pos = delimiter + 1;
            else break;
        }
        free(syntax_copy);
        if (!all_valid) {
            printf("Invalid Syntax!\n");
        } else {
            // Now execute all command chunks
            char *exec_copy = strdup(input);
            char *exec_pos = exec_copy;
            while (*exec_pos) {
                char *delimiter = strpbrk(exec_pos, ";&");
                bool is_background = false;
                if (delimiter) {
                    if (*delimiter == '&') is_background = true;
                    *delimiter = '\0';
                } else {
                    size_t len = strlen(exec_pos);
                    if (len > 0 && exec_pos[len-1] == '&') {
                        is_background = true;
                        exec_pos[len-1] = '\0';
                    }
                }
                while (isspace(*exec_pos)) exec_pos++;
                if (*exec_pos != '\0') {
                    parse_command_group(strdup(exec_pos));
                    execute_command_group(is_background);
                }
                if (delimiter) exec_pos = delimiter + 1;
                else break;
            }
            free(exec_copy);
        }

        free(input_copy);
        free(input);
    }

    return 0;
}
