#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include "../include/jobs.h"

// Global job management variables
BackgroundJob background_jobs[256];
int background_job_count = 0;
static int next_job_number = 1;

// External global variables (declared in main.c or elsewhere)
extern bool is_interactive_mode;

const char* get_job_state_string(JobState state) {
    switch (state) {
        case RUNNING:
            return "Running";
        case STOPPED:
            return "Stopped";
        default:
            return "Unknown";
    }
}

void add_background_job(pid_t pid, const char* command_name, JobState state) {
    if (background_job_count >= (int)(sizeof(background_jobs) / sizeof(background_jobs[0]))) {
        return;
    }
    BackgroundJob* job = &background_jobs[background_job_count++];
    job->job_number = next_job_number++;
    job->pid = pid;
    job->state = state;
    if (command_name != NULL) {
        strncpy(job->command_name, command_name, sizeof(job->command_name) - 1);
        job->command_name[sizeof(job->command_name) - 1] = '\0';
    } else {
        job->command_name[0] = '\0';
    }

    if (is_interactive_mode && state == RUNNING) {
        fprintf(stderr, "[%d] %d\n", job->job_number, (int)job->pid);
        fflush(stderr);
    }
}

static void remove_background_job_index(int index) {
    if (index < 0 || index >= background_job_count) {
        return;
    }
    for (int i = index; i < background_job_count - 1; i++) {
        background_jobs[i] = background_jobs[i + 1];
    }
    background_job_count--;
}

void remove_job_by_pid(pid_t pid) {
    for (int i = 0; i < background_job_count; i++) {
        if (background_jobs[i].pid == pid) {
            remove_background_job_index(i);
            return;
        }
    }
}

BackgroundJob* find_job_by_number(int job_number) {
    if (job_number <= 0) return NULL;
    for (int i = 0; i < background_job_count; i++) {
        if (background_jobs[i].job_number == job_number) {
            return &background_jobs[i];
        }
    }
    return NULL;
}

BackgroundJob* find_most_recent_job() {
    if (background_job_count > 0) {
        return &background_jobs[background_job_count - 1];
    }
    return NULL;
}

void check_background_jobs(void) {
    int status;
    for (int i = 0; i < background_job_count; ) {
        pid_t pid = background_jobs[i].pid;
        pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        
        if (result == 0) {
            i++;
            continue;
        } else if (result == -1) {
            if (errno == ECHILD) {
                if (kill(pid, 0) == -1 && errno == ESRCH) {
                    remove_background_job_index(i);
                } else {
                    i++;
                }
            } else {
                i++;
            }
            continue;
        } else {
            const char* name = background_jobs[i].command_name[0] != '\0' ? background_jobs[i].command_name : "";

            if (WIFEXITED(status)) {
                if (is_interactive_mode) {
                    if (WEXITSTATUS(status) == 0) {
                        fprintf(stderr, "%s with pid %d exited normally\n", name, (int)pid);
                    } else {
                        fprintf(stderr, "%s with pid %d exited abnormally\n", name, (int)pid);
                    }
                    fflush(stderr);
                }
                remove_background_job_index(i);
            } else if (WIFSIGNALED(status)) {
                if (is_interactive_mode) {
                    printf("[%d] Terminated %s\n", background_jobs[i].job_number, name);
                    fflush(stdout);
                }
                remove_background_job_index(i);
            } else if (WIFSTOPPED(status)) {
                background_jobs[i].state = STOPPED;
                i++;
            } else if (WIFCONTINUED(status)) {
                background_jobs[i].state = RUNNING;
                i++;
            }
        }
    }
}

static int compare_background_jobs(const void* a, const void* b) {
    return strcmp(((const BackgroundJob*)a)->command_name, ((const BackgroundJob*)b)->command_name);
}

void list_activities(void) {
    check_background_jobs();
    BackgroundJob active_jobs[256];
    int active_jobs_count = 0;
    
    for(int i = 0; i < background_job_count; i++) {
        active_jobs[active_jobs_count++] = background_jobs[i];
    }
    qsort(active_jobs, active_jobs_count, sizeof(BackgroundJob), compare_background_jobs);
    for (int i = 0; i < active_jobs_count; i++) {
        printf("[%d] : %s - %s\n", active_jobs[i].pid, active_jobs[i].command_name, get_job_state_string(active_jobs[i].state));
    }
}

void check_and_kill_all_jobs(void) {
    for (int i = 0; i < background_job_count; i++) {
        kill(-background_jobs[i].pid, SIGKILL);
    }
    while (waitpid(-1, NULL, 0) > 0);
}
