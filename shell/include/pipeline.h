#ifndef PIPELINE_H
#define PIPELINE_H

#include <sys/types.h>
#include <stdbool.h>

// Function declarations
pid_t execute_pipeline(char** tokens, int token_count, bool run_in_background, const char* command_name, char** prev_dir, char* SHELL_HOME_DIR);

#endif // PIPELINE_H
