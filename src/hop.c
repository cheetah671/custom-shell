#include "hop.h"
#include "shell.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void hop(char *args) {
    // If no arguments, treat as "hop ~"
    if (args == NULL || strlen(args) == 0) {
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        strcpy(prev_dir, cwd);
        if (chdir(initial_home) != 0) {
            printf("No such directory!\n");
        }
        return;
    }

    // Make a copy of arguments since we need to tokenize
    char *args_copy = strdup(args);
    char *token = strtok(args_copy, " \t\n");
    
    while (token != NULL) {
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        
        if (strcmp(token, "~") == 0 || strlen(token) == 0) {
            strcpy(prev_dir, cwd);
            if (chdir(initial_home) != 0) {
                printf("No such directory!\n");
            }
        }
        else if (strcmp(token, ".") == 0) {
            // Do nothing, stay in same directory
        }
        else if (strcmp(token, "..") == 0) {
            strcpy(prev_dir, cwd);
            if (strcmp(cwd, "/") != 0) {
                if (chdir("..") != 0) {
                    printf("No such directory!\n");
                }
            }
        }
        else if (strcmp(token, "-") == 0) {
            if (strlen(prev_dir) > 0) {
                char temp[PATH_MAX];
                strcpy(temp, cwd);
                if (chdir(prev_dir) != 0) {
                    printf("No such directory!\n");
                } else {
                    strcpy(prev_dir, temp);
                }
            } else {
                printf("No previous directory\n");
            }
        }
        else {
            strcpy(prev_dir, cwd);
            if (chdir(token) != 0) {
                printf("No such directory!\n");
            }
        }
        
        token = strtok(NULL, " \t\n");
    }
    
    free(args_copy);
}