#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 100


typedef enum { JOB_RUNNING, JOB_STOPPED } JobState;

typedef struct {
    pid_t pid;
    char command_name[256];
    int job_number;
    JobState state;
} Job;

extern Job jobs[MAX_JOBS];
extern int job_count;

Job* add_job(pid_t pid, const char *command_name, JobState state);
void remove_job(pid_t pid);
Job* get_job_by_num(int job_number);
Job* get_job_by_pid(pid_t pid);
Job* get_most_recent_job();
void set_job_state(pid_t pid, JobState state);
void print_jobs();
void sigchld_handler(int signo); // New signal handler function
#endif