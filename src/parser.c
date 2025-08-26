
#include "parser.h"
#include "shell.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "jobs.h"
#include "activities.h"
#include "ping.h"
#include "fgbg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

// External reference to foreground process group ID for signal handling
extern pid_t foreground_pgid;

// --- CHANGE 1: ADD THIS HELPER FUNCTION ---
// This function checks for and removes surrounding quotes from a string.
char* strip_quotes(char *str) {
    int len = strlen(str);
    // Handle double quotes
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        // Use memmove to shift the string content one position to the left
        memmove(str, str + 1, len - 2);
        // Add the new null terminator
        str[len - 2] = '\0';
    }
    // Handle single quotes
    else if (len >= 2 && str[0] == '\'' && str[len - 1] == '\'') {
        // Use memmove to shift the string content one position to the left  
        memmove(str, str + 1, len - 2);
        // Add the new null terminator
        str[len - 2] = '\0';
    }
    return str;
}
// ------------------------------------------

void clear_commands() {
    for (int i = 0; i < num_commands; i++) {
        for (int j = 0; commands[i].argv[j] != NULL; j++) {
            free(commands[i].argv[j]);
        }
        if (commands[i].input_file) free(commands[i].input_file);
        if (commands[i].output_file) free(commands[i].output_file);
    }
    memset(commands, 0, sizeof(commands));
    num_commands = 0;
}

bool parse_command_group(char *group_str) {
    clear_commands();

    // Check for invalid syntax patterns within a single command group
    // Note: semicolons and ampersands are handled by main.c before calling this function
    if (strstr(group_str, "||") || strstr(group_str, ";;")) {
        return false;
    }
    
    // Check for leading or trailing pipe
    char *trimmed = group_str;
    while (isspace(*trimmed)) trimmed++;
    char *end = trimmed + strlen(trimmed) - 1;
    while (end > trimmed && isspace(*end)) end--;
    
    // Invalid: starts with pipe or ends with pipe
    if (*trimmed == '|' || *end == '|') {
        return false;
    }

    char* pipe_saveptr;
    char* command_str = strtok_r(group_str, "|", &pipe_saveptr);
    int cmd_idx = 0;

    while (command_str != NULL && cmd_idx < MAX_COMMANDS_IN_PIPELINE) {
        char* arg_saveptr;
        char* token = strtok_r(command_str, " \t\n", &arg_saveptr);
        int arg_idx = 0;

        // Check if command starts or ends with invalid tokens
        if (token == NULL) {
            return false; // Empty command in pipeline
        }

        while (token != NULL) {
            if (strcmp(token, "<") == 0) {
                token = strtok_r(NULL, " \t\n", &arg_saveptr);
                if (token) {
                    // Check if this input file exists (for multiple redirects)
                    if (access(token, R_OK) != 0) {
                        printf("No such file or directory\n");
                        return false;
                    }
                    if (commands[cmd_idx].input_file) {
                        // Free previous input file and use the new one
                        free(commands[cmd_idx].input_file);
                    }
                    commands[cmd_idx].input_file = strdup(token);
                } else {
                    return false; // Invalid: < without filename
                }
            } else if (strcmp(token, ">") == 0) {
                token = strtok_r(NULL, " \t\n", &arg_saveptr);
                if (token) {
                    // Test file creation immediately for multiple redirects
                    int test_fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (test_fd == -1) {
                        printf("Unable to create file for writing\n");
                        // Don't return false - clear commands and exit gracefully
                        clear_commands();
                        return true; // Return true to avoid "Invalid Syntax" message
                    }
                    close(test_fd);
                    
                    if (commands[cmd_idx].output_file) {
                        // Free previous output file and use the new one
                        free(commands[cmd_idx].output_file);
                    }
                    commands[cmd_idx].output_file = strdup(token);
                    commands[cmd_idx].append = false;
                } else {
                    return false; // Invalid: > without filename
                }
            } else if (strcmp(token, ">>") == 0) {
                token = strtok_r(NULL, " \t\n", &arg_saveptr);
                if (token) {
                    // Test file creation immediately for multiple redirects
                    int test_fd = open(token, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (test_fd == -1) {
                        printf("Unable to create file for writing\n");
                        // Don't return false - clear commands and exit gracefully
                        clear_commands();
                        return true; // Return true to avoid "Invalid Syntax" message
                    }
                    close(test_fd);
                    
                    if (commands[cmd_idx].output_file) {
                        // Free previous output file and use the new one
                        free(commands[cmd_idx].output_file);
                    }
                    commands[cmd_idx].output_file = strdup(token);
                    commands[cmd_idx].append = true;
                } else {
                    return false; // Invalid: >> without filename
                }
            } else {
                if (arg_idx < MAX_ARGS - 1) {
                    commands[cmd_idx].argv[arg_idx++] = strdup(strip_quotes(token));
                }
            }
            token = strtok_r(NULL, " \t\n", &arg_saveptr);
        }
        
        // Check if this command has any arguments
        if (arg_idx == 0) {
            return false;
        }
        
        commands[cmd_idx].argv[arg_idx] = NULL;
        cmd_idx++;
        command_str = strtok_r(NULL, "|", &pipe_saveptr);
    }
    num_commands = cmd_idx;
    return true;
}

int handle_intrinsic(SimpleCommand *cmd) {
    char *command_name = cmd->argv[0];
    if (!command_name) return 0;

    // Handle output redirection for builtin commands
    int saved_stdout = -1;
    if (cmd->output_file) {
        saved_stdout = dup(STDOUT_FILENO);
        int flags = cmd->append ? (O_WRONLY | O_CREAT | O_APPEND) : (O_WRONLY | O_CREAT | O_TRUNC);
        int fd_out = open(cmd->output_file, flags, 0644);
        if (fd_out == -1) {
            fprintf(stderr, "Unable to create file for writing\n");
            return 1;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    char args_str[1024] = "";
    for (int i = 1; cmd->argv[i] != NULL; i++) {
        strcat(args_str, cmd->argv[i]);
        strcat(args_str, " ");
    }

    int result = 0;
    if (strcmp(command_name, "hop") == 0) { hop(args_str); result = 1; }
    else if (strcmp(command_name, "reveal") == 0) { reveal(args_str); result = 1; }
    else if (strcmp(command_name, "log") == 0) { log_command(args_str); result = 1; }
    else if (strcmp(command_name, "activities") == 0) { activities(); result = 1; }
    else if (strcmp(command_name, "ping") == 0) { ping(args_str); result = 1; }
    else if (strcmp(command_name, "fg") == 0) { fg(args_str); result = 1; }
    else if (strcmp(command_name, "bg") == 0) { bg(args_str); result = 1; }
    else if (strcmp(command_name, "exit") == 0 || strcmp(command_name, "logout") == 0) { 
        printf("logout\n");
        exit(0);
        result = 1; 
    }

    // Restore stdout if it was redirected
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }

    return result;
}


int handle_intrinsic_in_child(SimpleCommand *cmd) {
    char *command_name = cmd->argv[0];
    if (!command_name) return 0;

    char args_str[1024] = "";
    for (int i = 1; cmd->argv[i] != NULL; i++) {
        strcat(args_str, cmd->argv[i]);
        strcat(args_str, " ");
    }

    if (strcmp(command_name, "reveal") == 0) { reveal(args_str); return 1; }
    if (strcmp(command_name, "log") == 0) { log_command(args_str); return 1; }
    if (strcmp(command_name, "activities") == 0) { activities(); return 1; }
    if (strcmp(command_name, "ping") == 0) { ping(args_str); return 1; }
    if (strcmp(command_name, "exit") == 0) { 
        // In child process, just exit normally
        _exit(0);
        return 1; 
    }
    return 0;
}

void execute_command_group(bool is_background) {
    // This function remains the same
    if (num_commands == 0 || commands[0].argv[0] == NULL) return;

    if (num_commands == 1 && !is_background && handle_intrinsic(&commands[0])) {
        return;
    }

    int prev_pipe_fd_read = -1;
    pid_t pids[num_commands];
    pid_t group_pid = -1;

    for (int i = 0; i < num_commands; i++) {
        int pipe_fd[2];
        if (i < num_commands - 1) {
            if (pipe(pipe_fd) == -1) { perror("pipe"); return; }
        }

        pids[i] = fork();
        if (pids[i] == -1) { perror("fork"); return; }
        
        if (i == 0) group_pid = pids[i];

        if (pids[i] == 0) { // --- Child Process ---
            // Create new process group for foreground jobs
            if (!is_background) {
                if (i == 0) {
                    setpgid(0, 0); // Make first process the group leader
                } else {
                    setpgid(0, pids[0]); // Join the first process's group
                }
                // Reset signal handlers to default for foreground processes
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
            }
            
            if (is_background && commands[i].input_file == NULL) {
                int dev_null = open("/dev/null", O_RDONLY);
                if (dev_null != -1) {
                    dup2(dev_null, STDIN_FILENO);
                    close(dev_null);
                }
            }
            
            if (prev_pipe_fd_read != -1) {
                dup2(prev_pipe_fd_read, STDIN_FILENO);
                close(prev_pipe_fd_read);
            }
            if (commands[i].input_file) {
                int fd_in = open(commands[i].input_file, O_RDONLY);
                if (fd_in == -1) { 
                    fprintf(stderr, "No such file or directory!\n");
                    exit(EXIT_FAILURE); 
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            if (i < num_commands - 1) {
                close(pipe_fd[0]);
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
            }
            if (commands[i].output_file) {
                int flags = O_WRONLY | O_CREAT | (commands[i].append ? O_APPEND : O_TRUNC);
                int fd_out = open(commands[i].output_file, flags, 0644);
                if (fd_out == -1) { 
                    fprintf(stderr, "Unable to create file for writing\n"); 
                    exit(EXIT_FAILURE); 
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            if (handle_intrinsic_in_child(&commands[i])) {
                _exit(0);
            }

            execvp(commands[i].argv[0], commands[i].argv);
            fprintf(stderr, "Command not found!\n");
            exit(EXIT_FAILURE);
        } else {
            // Parent process - set up process group
            if (!is_background) {
                if (i == 0) {
                    setpgid(pids[i], pids[i]); // Make first process the group leader
                } else {
                    setpgid(pids[i], pids[0]); // Join the first process's group
                }
            }
        }

        if (prev_pipe_fd_read != -1) close(prev_pipe_fd_read);
        if (i < num_commands - 1) {
            close(pipe_fd[1]);
            prev_pipe_fd_read = pipe_fd[0];
        }
    }

    if (is_background) {
        char bg_cmd_name[256];
        int len = 0;
        // Build full command with arguments
        for (int i = 0; commands[0].argv[i] && len < sizeof(bg_cmd_name) - 3; i++) {
            if (i > 0) len += snprintf(bg_cmd_name + len, sizeof(bg_cmd_name) - len, " ");
            len += snprintf(bg_cmd_name + len, sizeof(bg_cmd_name) - len, "%s", commands[0].argv[i]);
        }
        len += snprintf(bg_cmd_name + len, sizeof(bg_cmd_name) - len, " &");
        Job *job = add_job(group_pid, bg_cmd_name, JOB_RUNNING);
        // Print job notification when background job starts
        printf("[%d] %s\n", job->job_number, bg_cmd_name);
    } else {
        // Set foreground process group for signal handling
        foreground_pgid = group_pid;
        
        // Add foreground job temporarily for signal handling
        Job *fg_job = add_job(group_pid, commands[0].argv[0], JOB_RUNNING);
        
        // Wait for all processes in the foreground job
        for (int i = 0; i < num_commands; i++) {
            int status;
            pid_t result = waitpid(pids[i], &status, WUNTRACED);
            if (result > 0) {
                if (WIFSTOPPED(status)) {
                    // Process was stopped, keep it in job list and return immediately
                    if (fg_job) {
                        fg_job->state = JOB_STOPPED;
                        printf("[%d] Stopped %s\n", fg_job->job_number, fg_job->command_name);
                        fflush(stdout);
                        usleep(100000); // Small delay to ensure output is processed
                    }
                    break; // Return to prompt immediately
                }
            }
        }
        
        // Clear foreground process group
        foreground_pgid = 0;
        
        // Remove foreground job if it completed (not stopped)
        if (fg_job && fg_job->state != JOB_STOPPED) {
            remove_job(group_pid);
        }
    }
}

