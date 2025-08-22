
#include "log.h"
#include "shell.h"
#include "parser.h"
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define HISTORY_FILE ".shell_history"

void get_history_file_path(char *history_file_path) {
    char cwd[PATH_MAX];
    if (readlink("/proc/self/exe", cwd, sizeof(cwd)) == -1) {
        perror("Failed to get the executable path");
        exit(EXIT_FAILURE);
    }

    // Get the directory of the executable
    char *last_slash = strrchr(cwd, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // Terminate the string at the last slash
    }

    // Construct the full path to the .shell_history file
    int ret = snprintf(history_file_path, PATH_MAX, "%s/%s", cwd, HISTORY_FILE);
    if (ret >= PATH_MAX) {
        fprintf(stderr, "Warning: History file path truncated\n");
    }
}

void save_history() {
    char history_file_path[PATH_MAX];
    get_history_file_path(history_file_path);

    FILE *file = fopen(history_file_path, "w");
    if (file == NULL) {
        perror("Failed to save history");
        return;
    }

    for (int i = 0; i < history_count; i++) {
        fprintf(file, "%s\n", command_history[i]);
    }

    fclose(file);
}


void load_history() {
    
    char history_file_path[PATH_MAX];
    get_history_file_path(history_file_path);

    FILE *file = fopen(history_file_path, "r");
    if (file == NULL) {
        return; // No history file yet
    }

    char line[1024];
    while (history_count < MAX_HISTORY && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline
        if (strlen(line) > 0) {
            command_history[history_count] = strdup(line);
            history_count++;
        }
    }

    fclose(file);
}


void add_to_history(const char *cmd) {
    // Don't add if it's the same as the last command
    if (history_count > 0 && strcmp(command_history[history_count - 1], cmd) == 0) {
        return;
    }

    // Don't add if it's the log command itself
    if (strncmp(cmd, "log", 3) == 0 && (cmd[3] == '\0' || cmd[3] == ' ')) {
        return;
    }

    // If we've reached max capacity, remove the oldest command
    if (history_count >= MAX_HISTORY) {
        free(command_history[0]);
        // Shift all commands down
        for (int i = 1; i < MAX_HISTORY; i++) {
            command_history[i - 1] = command_history[i];
        }
        history_count--;
    }

    // Add the new command
    command_history[history_count] = strdup(cmd);
    history_count++;

    // Save to file
    save_history();
}

void log_command(char *args) {
    if (args == NULL || strlen(args) == 0) {
        // Print history (oldest to newest)
        for (int i = 0; i < history_count; i++) {
            printf("%s\n", command_history[i]);
        }
    } else {
        // Parse arguments
        char args_copy[1024];
        strncpy(args_copy, args, sizeof(args_copy));
        args_copy[sizeof(args_copy) - 1] = '\0';
        
        char *token = strtok(args_copy, " \t\n");
        
        if (strcmp(token, "purge") == 0) {
            // Clear history
            for (int i = 0; i < history_count; i++) {
                free(command_history[i]);
            }
            history_count = 0;
            save_history();
        } else if (strcmp(token, "execute") == 0) {
            // log execute is handled in main.c - this shouldn't be reached
            printf("Usage: log execute <index>\n");
        } else {
            printf("Invalid log command. Usage: log [purge|execute <index>]\n");
        }
    }
}
