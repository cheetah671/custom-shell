
#include "../include/fgbg.h"
#include "../include/jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

void fg(char* args) {
    int job_num = -1;
    if (args && strlen(args) > 0) {
        job_num = atoi(args);
    }
    Job* job = job_num > 0 ? get_job_by_num(job_num) : get_most_recent_job();
    if (!job) {
        printf("No such job\n");
        return;
    }
    printf("Bringing job to foreground: %s\n", job->command_name);
    if (job->state == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
        set_job_state(job->pid, JOB_RUNNING);
    }
    int status;
    waitpid(job->pid, &status, WUNTRACED);
    remove_job(job->pid);
}

void bg(char* args) {
    int job_num = -1;
    if (args && strlen(args) > 0) {
        job_num = atoi(args);
    }
    Job* job = job_num > 0 ? get_job_by_num(job_num) : get_most_recent_job();
    if (!job) {
        printf("No such job\n");
        return;
    }
    if (job->state == JOB_RUNNING) {
        printf("Job already running\n");
        return;
    }
    kill(job->pid, SIGCONT);
    set_job_state(job->pid, JOB_RUNNING);
    printf("[%d] %s &\n", job->job_number, job->command_name);
}
