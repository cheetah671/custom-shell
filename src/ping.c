#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

void ping(char* args) {
    int pid, sig;
    if (sscanf(args, "%d %d", &pid, &sig) != 2) {
        printf("ping: Invalid Syntax!\n");
        return;
    }
    int actual_signal = sig % 32;
    if (kill(pid, 0) == -1) {
        printf("No such process found\n");
        return;
    }
    if (kill(pid, actual_signal) == 0) {
        printf("Sent signal %d to process with pid %d\n", sig, pid);
    } else {
        perror("ping");
    }
}
