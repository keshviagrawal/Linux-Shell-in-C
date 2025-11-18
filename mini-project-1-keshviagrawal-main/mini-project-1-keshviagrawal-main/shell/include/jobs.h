#ifndef JOBS_H
#define JOBS_H

#include <stdbool.h>
#include <sys/types.h>

// Enum for job states
typedef enum {
    RUNNING,
    STOPPED
} JobState;

// Struct for representing a background job
typedef struct {
    int job_number;
    pid_t pid; // This is the process group ID (pgid)
    char command_name[256];
    JobState state;
} BackgroundJob;

// Function declarations
void check_background_jobs(void);
void list_activities(void);
void check_and_kill_all_jobs(void);
void remove_job_by_pid(pid_t pid);
void add_background_job(pid_t pid, const char* command_name, JobState state);
BackgroundJob* find_job_by_number(int job_number);
BackgroundJob* find_most_recent_job();
const char* get_job_state_string(JobState state);

extern BackgroundJob background_jobs[256];
extern int background_job_count;

#endif // JOBS_H
