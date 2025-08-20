#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_TOKENS 1024
#define MAX_ARGS 128
#define MAX_COMMANDS_IN_PIPELINE 100
#define MAX_HISTORY 15
#define HISTORY_FILE ".shell_history"
#define PATH_MAX 4096

// Structure for a single command (e.g., "ls -l")
typedef struct {
    char *argv[MAX_ARGS];
    char *input_file;
    char *output_file;
    bool append;
} SimpleCommand;


// --- FIX: DECLARE GLOBAL VARIABLES HERE ---
// These are now visible to any .c file that includes shell.h
extern SimpleCommand commands[MAX_COMMANDS_IN_PIPELINE];
extern int num_commands;
extern pid_t foreground_pgid;
// ------------------------------------------

// Other existing global variable declarations
extern char initial_home[];
extern char prev_dir[];
extern char *command_history[];
extern int history_count;
extern pid_t foreground_pgid;

#endif