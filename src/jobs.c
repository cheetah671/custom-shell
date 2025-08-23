
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h> // For write()


Job jobs[MAX_JOBS];
int job_count = 0;


Job* add_job(pid_t pid, const char *command_name, JobState state) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command_name, command_name, sizeof(jobs[job_count].command_name) - 1);
        jobs[job_count].job_number = job_count + 1;
        jobs[job_count].state = state;
        job_count++;
        return &jobs[job_count - 1];
    }
    return NULL;
}


void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

Job* get_job_by_num(int job_number) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_number == job_number) return &jobs[i];
    }
    return NULL;
}

Job* get_job_by_pid(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) return &jobs[i];
    }
    return NULL;
}

Job* get_most_recent_job() {
    if (job_count > 0) return &jobs[job_count - 1];
    return NULL;
}

void set_job_state(pid_t pid, JobState state) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].state = state;
            break;
        }
    }
}

void print_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] : %s - %s\n", jobs[i].pid, jobs[i].command_name, jobs[i].state == JOB_RUNNING ? "Running" : "Stopped");
    }
}

void sigchld_handler(int signo) {
    (void)signo; // Suppress unused parameter warning
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        Job* job = get_job_by_pid(pid);
        if (job) {
            char buffer[256];
            int len;
            if (WIFEXITED(status)) {
                len = snprintf(buffer, sizeof(buffer), "\n%s with pid %d exited normally\n", job->command_name, pid);
                write(STDOUT_FILENO, buffer, len);
                remove_job(pid);
            } else if (WIFSIGNALED(status)) {
                len = snprintf(buffer, sizeof(buffer), "\n%s with pid %d exited abnormally\n", job->command_name, pid);
                write(STDOUT_FILENO, buffer, len);
                remove_job(pid);
            } else if (WIFSTOPPED(status)) {
                // Only handle stopped background jobs here
                // Foreground job stops are handled in the parser
                if (job->state == JOB_RUNNING) { // This was a background job
                    len = snprintf(buffer, sizeof(buffer), "\n[%d] Stopped %s\n", job->job_number, job->command_name);
                    write(STDOUT_FILENO, buffer, len);
                    job->state = JOB_STOPPED;
                }
            }
        }
    }
}
