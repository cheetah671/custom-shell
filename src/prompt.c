#include "prompt.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
char* get_username() {
    char *user = getlogin();
    if (user == NULL) {
        exit(1);
    }
    return user;
}

void get_system_name(char *hostname, size_t size) {
    if (gethostname(hostname, size) != 0) {
        exit(1);
    }
}

void get_current_dir(char *cwd, size_t size) {
    if (getcwd(cwd, size) == NULL) {
        exit(1);
    }
}

void print_prompt(const char *username, const char *hostname, const char *home) {
    char cwd[1024];
    char display_path[1024];
    get_current_dir(cwd, sizeof(cwd));
    if (strncmp(cwd, home, strlen(home)) == 0 && cwd[strlen(home)] == '/') {
        snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(home));
    } 
    else if (strcmp(cwd, home) == 0) {
        strcpy(display_path, "~");
    } 
    else {
        strcpy(display_path, cwd);
    }
    printf("<%s@%s:%s> ", username, hostname, display_path);
    fflush(stdout);
}