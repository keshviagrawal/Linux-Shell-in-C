#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>

// Enum to classify built-in commands
enum BuiltinType { NOT_BUILTIN, SPECIAL_BUILTIN, REGULAR_BUILTIN };

// The main execution function that parses and runs commands.
bool execute(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR);

// These are needed by multiple modules, so they are declared here.
enum BuiltinType get_builtin_type(const char* cmd);
void execute_builtin(char** tokens, int token_count, char** prev_dir, char* SHELL_HOME_DIR);

#endif // EXECUTOR_H