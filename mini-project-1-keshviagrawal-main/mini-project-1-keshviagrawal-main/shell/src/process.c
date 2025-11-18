#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/process.h"
#include "../include/executor.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"
#include "../include/fg_bg.h"
#include "../include/signals.h"

// This function is declared in executor.c but used here
void execute_builtin(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR);
enum BuiltinType get_builtin_type(const char* cmd);

void run_command_in_child(char** tokens, int token_count, bool run_in_background, char** prev_dir, char* SHELL_HOME_DIR) {
    char* cmd_args[1024];
    int arg_count = 0;
    int in_fd = -1, out_fd = -1;

    for (int i = 0; i < token_count && tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (!tokens[++i]) { fprintf(stderr, "Syntax error near `<`\n"); exit(1); }
            if ((in_fd = open(tokens[i], O_RDONLY)) == -1) { printf("No such file or directory\n"); exit(1); }
        } else if (strcmp(tokens[i], ">") == 0) {
            if (!tokens[++i]) { fprintf(stderr, "Syntax error near `>`\n"); exit(1); }
            if ((out_fd = open(tokens[i], O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) { printf("Unable to create file for writing\n"); exit(1); }
        } else if (strcmp(tokens[i], ">>") == 0) {
            if (!tokens[++i]) { fprintf(stderr, "Syntax error near `>>`\n"); exit(1); }
            if ((out_fd = open(tokens[i], O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1) { printf("Unable to create file for writing\n"); exit(1); }
        } else {
            cmd_args[arg_count++] = tokens[i];
        }
    }
    cmd_args[arg_count] = NULL;

    if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
    if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
    if (run_in_background && in_fd == -1) {
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull != -1) { dup2(devnull, STDIN_FILENO); close(devnull); }
    }

    if (arg_count > 0 && get_builtin_type(cmd_args[0]) != NOT_BUILTIN) {
        if (strcmp(cmd_args[0], "fg") == 0 || strcmp(cmd_args[0], "bg") == 0) {
            fprintf(stderr, "%s: no job control\n", cmd_args[0]);
            exit(1);
        }
        execute_builtin(cmd_args, arg_count, prev_dir, SHELL_HOME_DIR);
        exit(0);
    }
    
    if (arg_count > 0) {
        execvp(cmd_args[0], cmd_args);
        fprintf(stderr, "Command not found!\n");
    }
    exit(127);
}
