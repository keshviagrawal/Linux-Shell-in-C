#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "../include/signals.h"

// External global variables
extern pid_t foreground_pid;

void ping(pid_t pid, int signal_number) {
    int actual_signal = ((signal_number % 32) + 32) % 32;
    if (kill(pid, actual_signal) == -1) {
        if (errno == ESRCH) fprintf(stderr, "No such process found\n");
        else perror("kill failed");
    } else {
        printf("Sent signal %d to process with pid %d\n", signal_number, (int)pid);
    }
}

void handle_sigint(int signo) {
    (void)signo;
    if (foreground_pid != -1) {
        if (kill(-foreground_pid, SIGINT) == -1) {
            perror("kill failed in SIGINT handler");
        }
    }
}

void handle_sigtstp(int signo) {
    (void)signo;
    if (foreground_pid != -1) {
        if (kill(-foreground_pid, SIGTSTP) == -1) {
            perror("kill failed in SIGTSTP handler");
        }
    }
}

void setup_signal_handlers(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}
