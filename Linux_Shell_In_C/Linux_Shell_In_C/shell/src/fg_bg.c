#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "../include/fg_bg.h"
#include "../include/jobs.h"

// External global variables
extern pid_t foreground_pid;

void fg_command(char** tokens, int token_count) {
    BackgroundJob* job = NULL;
    if (token_count > 2) {
        fprintf(stderr, "Syntax: fg [job_number]\n");
        return;
    }
    job = (token_count == 2) ? find_job_by_number(atoi(tokens[1])) : find_most_recent_job();
    if (job == NULL) {
        fprintf(stderr, "No such job\n");
        return;
    }
    
    printf("%s\n", job->command_name);
    pid_t pgid = job->pid;
    
    foreground_pid = pgid;
    if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
        perror("tcsetpgrp failed");
        foreground_pid = -1;
        return;
    }
    
    if (job->state == STOPPED) {
        if (kill(-pgid, SIGCONT) == -1) {
            perror("kill failed");
            tcsetpgrp(STDIN_FILENO, getpgrp());
            foreground_pid = -1;
            return;
        }
    }

    int status;
    pid_t wait_result = waitpid(-pgid, &status, WUNTRACED);
    
    if (wait_result != -1) {
        if (WIFSTOPPED(status)) {
            fprintf(stderr, "\n[%d] Stopped %s\n", job->job_number, job->command_name);
            job->state = STOPPED;
        } else if (WIFSIGNALED(status)) {
            printf("\n");
            remove_job_by_pid(job->pid);
        } else if (WIFEXITED(status)) {
            remove_job_by_pid(job->pid);
        }
    } else if (errno == ECHILD) {
        remove_job_by_pid(job->pid);
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = -1;
}

void bg_command(char** tokens, int token_count) {
    BackgroundJob* job = NULL;
    if (token_count > 2) {
        fprintf(stderr, "Syntax: bg [job_number]\n");
        return;
    }
    job = (token_count == 2) ? find_job_by_number(atoi(tokens[1])) : find_most_recent_job();
    if (job == NULL) {
        fprintf(stderr, "No such job\n");
        return;
    }
    if (job->state == RUNNING) {
        fprintf(stderr, "Job already running\n");
        return;
    }
    if (kill(-job->pid, SIGCONT) == -1) {
        perror("kill failed");
        return;
    }
    job->state = RUNNING;
    printf("[%d] %s &\n", job->job_number, job->command_name);
}
