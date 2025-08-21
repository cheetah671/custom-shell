#ifndef PROMPT_H
#define PROMPT_H

#include <stddef.h>  // Add this line

char* get_username();
void get_system_name(char *hostname, size_t size);
void get_current_dir(char *cwd, size_t size);
void print_prompt(const char *username, const char *hostname, const char *home);

#endif