
#include "reveal.h"
#include "hop.h"
#include "prompt.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

int cmp(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void reveal(char *args) {
    int show_all = 0, line_by_line = 0;
    char dir_arg[PATH_MAX] = "";
    char *token = strtok(args, " ");
    int arg_count = 0;
    while (token) {
        if (token[0] == '-' && strlen(token) > 1) {
            // This is a flag like -a, -l, -al, etc.
            for (int i = 1; token[i]; i++) {
                if (token[i] == 'a') show_all = 1;
                if (token[i] == 'l') line_by_line = 1;
            }
        } else {
            // This is a directory argument (including "-")
            arg_count++;
            strncpy(dir_arg, token, sizeof(dir_arg)-1);
        }
        token = strtok(NULL, " ");
    }
    if (arg_count > 1) {
        printf("reveal: Invalid Syntax!\n");
        return;
    }
    char target[PATH_MAX];
    if (arg_count == 0 || strcmp(dir_arg, "~") == 0) {
        get_current_dir(target, sizeof(target));
    } else if (strcmp(dir_arg, ".") == 0) {
        getcwd(target, sizeof(target));
    } else if (strcmp(dir_arg, "..") == 0) {
        getcwd(target, sizeof(target));
        char *last = strrchr(target, '/');
        if (last && last != target) *last = '\0';
        else strcpy(target, "/");
    } else if (strcmp(dir_arg, "-") == 0) {
        extern char prev_dir[PATH_MAX];
        if (strlen(prev_dir) == 0) {
            printf("No such directory!\n");
            return;
        }
        strcpy(target, prev_dir);
    } else {
        strcpy(target, dir_arg);
    }
    DIR *dir = opendir(target);
    if (!dir) {
        printf("No such directory!\n");
        return;
    }
    struct dirent *entry;
    char *names[1024];
    int count = 0;
    while ((entry = readdir(dir))) {
        // If show_all is false, skip hidden files (starting with .)
        // If show_all is true, include all files including . and ..
        if (!show_all && entry->d_name[0] == '.') continue;
        names[count++] = strdup(entry->d_name);
    }
    closedir(dir);
    qsort(names, count, sizeof(char*), cmp);
    if (line_by_line) {
        for (int i = 0; i < count; i++) {
            printf("%s\n", names[i]);
            free(names[i]);
        }
    } else {
        for (int i = 0; i < count; i++) {
            printf("%s", names[i]);
            if (i != count-1) printf(" ");
            free(names[i]);
        }
        printf("\n");
    }
}
