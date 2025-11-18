#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include "../include/pipeline.h"
#include "../include/process.h"
#include "../include/jobs.h"
#include "../include/executor.h"

// External global variables
extern pid_t foreground_pid;
extern bool is_interactive_mode;

// This function is declared in executor.c but used here
void execute_builtin(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR);
enum BuiltinType get_builtin_type(const char* cmd);

pid_t execute_pipeline(char** tokens, int token_count, bool run_in_background, const char* command_name, char** prev_dir, char* SHELL_HOME_DIR) {
    if (token_count <= 0) return -1;

    int pipe_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "|") == 0) pipe_count++;
    }

    if (pipe_count == 0) {
        char* cmd_args[1024];
        int arg_count = 0;
        bool has_redirection = false;
        for (int i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
                has_redirection = true;
                i++;
            } else {
                cmd_args[arg_count++] = tokens[i];
            }
        }
        cmd_args[arg_count] = NULL;

        if (arg_count > 0 && get_builtin_type(cmd_args[0]) == SPECIAL_BUILTIN) {
            if (has_redirection) {
                fprintf(stderr, "shell: redirection is not supported for %s\n", cmd_args[0]);
                return 0;
            }
            execute_builtin(cmd_args, arg_count, prev_dir, SHELL_HOME_DIR);
            return 0;
        }

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }
        if (pid == 0) {
            if (is_interactive_mode) {
                setpgid(0, 0);
                if (!run_in_background) tcsetpgrp(STDIN_FILENO, getpgrp());
            }
            run_command_in_child(tokens, token_count, run_in_background, prev_dir, SHELL_HOME_DIR);
        }

        setpgid(pid, pid);
        if (run_in_background) {
            add_background_job(pid, command_name, RUNNING);
            return pid;
        }
        
        foreground_pid = pid;
        if (is_interactive_mode) tcsetpgrp(STDIN_FILENO, pid);
        
        int status;
        if (waitpid(pid, &status, WUNTRACED) != -1) {
            if (WIFSTOPPED(status)) {
                add_background_job(pid, command_name, STOPPED);
                fprintf(stderr, "\n[%d] Stopped %s\n", find_most_recent_job()->job_number, command_name);
            }
        }
        
        if (is_interactive_mode) tcsetpgrp(STDIN_FILENO, getpgrp());
        foreground_pid = -1;
        return pid;
    }

    // Pipeline execution
    int start = 0, num_cmds = pipe_count + 1;
    pid_t pgid = 0, pids[num_cmds];
    int pid_count = 0, fds[2], in_fd = -1;

    for (int i = 0; i < num_cmds; i++) {
        int end = start;
        while(end < token_count && strcmp(tokens[end], "|") != 0) end++;
        
        bool is_last = (i == num_cmds - 1);
        if (!is_last && pipe(fds) == -1) { perror("pipe"); return -1; }

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); return -1; }
        if (pid == 0) { // Child
            if (is_interactive_mode) {
                setpgid(0, pgid == 0 ? 0 : pgid);
                if (!run_in_background) tcsetpgrp(STDIN_FILENO, getpgrp());
            }
            if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
            if (!is_last) { dup2(fds[1], STDOUT_FILENO); close(fds[0]); close(fds[1]); }
            
            char** child_tokens = &tokens[start];
            int child_token_count = end - start;
            run_command_in_child(child_tokens, child_token_count, run_in_background, prev_dir, SHELL_HOME_DIR);
        }

        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);
        pids[pid_count++] = pid;

        if (in_fd != -1) close(in_fd);
        if (!is_last) { close(fds[1]); in_fd = fds[0]; }
        start = end + 1;
    }

    if (run_in_background) {
        if (pgid != 0) add_background_job(pgid, command_name, RUNNING);
        return pgid;
    }
    
    foreground_pid = pgid;
    if (is_interactive_mode && pgid != 0) tcsetpgrp(STDIN_FILENO, pgid);
    
    int status;
    bool stopped = false;
    int processes_to_wait_for = pid_count;
    while (processes_to_wait_for > 0) {
        pid_t child_pid = waitpid(-pgid, &status, WUNTRACED);
        if (child_pid > 0) {
            if (WIFSTOPPED(status)) {
                stopped = true;
                break;
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                processes_to_wait_for--;
            }
        } else if (errno == ECHILD) {
            break;
        }
    }

    if (stopped) {
        add_background_job(pgid, command_name, STOPPED);
        fprintf(stderr, "\n[%d] Stopped %s\n", find_most_recent_job()->job_number, command_name);
    }
    
    if (is_interactive_mode) tcsetpgrp(STDIN_FILENO, getpgrp());
    foreground_pid = -1;
    return pgid;
}
