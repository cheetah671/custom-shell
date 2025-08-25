#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

// Parses the entire input line into the global 'commands' array.
// Returns true on success, false on syntax error.
bool parse_input(char *input);

// Executes the pipeline of commands stored in the global 'commands' array.
void execute_pipeline();

// Parse a command group and return true if syntax is valid
bool parse_command_group(char *group_str);

// Execute a command group with background flag
void execute_command_group(bool is_background);

#endif