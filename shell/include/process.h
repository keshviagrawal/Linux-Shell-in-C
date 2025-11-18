#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>

// Function declarations
void run_command_in_child(char** tokens, int token_count, bool run_in_background, char** prev_dir, char* SHELL_HOME_DIR);

#endif // PROCESS_H
