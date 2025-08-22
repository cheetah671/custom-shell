#ifndef LOG_H
#define LOG_H

void log_command(char *args);
void save_history();
void load_history();
void add_to_history(const char *cmd);

#endif