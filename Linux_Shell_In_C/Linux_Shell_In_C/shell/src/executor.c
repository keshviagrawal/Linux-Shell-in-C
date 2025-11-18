#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/executor.h"
#include "../include/pipeline.h"
#include "../include/jobs.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"
#include "../include/fg_bg.h"
#include "../include/signals.h"

// Global variables defined in main.c, declared here for use
extern pid_t foreground_pid;
extern bool is_interactive_mode;

enum BuiltinType get_builtin_type(const char* cmd) {
    if (!cmd) return NOT_BUILTIN;
    if (strcmp(cmd, "hop") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0 || strcmp(cmd, "log") == 0) {
        return SPECIAL_BUILTIN;
    }
    if (strcmp(cmd, "reveal") == 0 || strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0) {
        return REGULAR_BUILTIN;
    }
    return NOT_BUILTIN;
}

void execute_builtin(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR) {
    if (strcmp(tokens[0], "hop") == 0) {
        hop(&tokens[1], token_count - 1, prev_dir, SHELL_HOME_DIR);
    } else if (strcmp(tokens[0], "exit") == 0) {
        check_and_kill_all_jobs();
        printf("logout\n");
        exit(0);
    } else if (strcmp(tokens[0], "fg") == 0) {
        fg_command(tokens, token_count);
    } else if (strcmp(tokens[0], "bg") == 0) {
        bg_command(tokens, token_count);
    } else if (strcmp(tokens[0], "reveal") == 0) {
        reveal(&tokens[1], token_count - 1, prev_dir, SHELL_HOME_DIR);
    } else if (strcmp(tokens[0], "log") == 0) {
        handle_log_command(&tokens[1], token_count - 1, prev_dir, SHELL_HOME_DIR);
    } else if (strcmp(tokens[0], "activities") == 0) {
        list_activities();
    } else if (strcmp(tokens[0], "ping") == 0) {
        if (token_count != 3) fprintf(stderr, "Syntax: ping <pid> <signal_number>\n");
        else ping((pid_t)strtol(tokens[1], NULL, 10), (int)strtol(tokens[2], NULL, 10));
    }
}

bool execute(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR) {
    if (token_count <= 0) return false;

    if (is_interactive_mode) {
        check_background_jobs();
    }

    int cmd_start = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ";") == 0 || strcmp(tokens[i], "&") == 0) {
            bool background = (strcmp(tokens[i], "&") == 0);
            tokens[i] = NULL;
            const char* command_name = (cmd_start < i && tokens[cmd_start]) ? tokens[cmd_start] : "";
            execute_pipeline(&tokens[cmd_start], i - cmd_start, background, command_name, prev_dir, SHELL_HOME_DIR);
            cmd_start = i + 1;
        }
    }

    if (cmd_start < token_count) {
        const char* command_name = (tokens[cmd_start]) ? tokens[cmd_start] : "";
        execute_pipeline(&tokens[cmd_start], token_count - cmd_start, false, command_name, prev_dir, SHELL_HOME_DIR);
    }

    return true;
}
