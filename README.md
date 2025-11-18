### A Mini Linux Shell in C

*   **From `parser.c`**: Tokenizes the command line input and builds the pipeline structure.
*   **From `executor.c`**: Executes a single command in a new process using `fork` and `exec`.
*   **From `pipeline.c`**: Manages the creation of pipes to connect multiple commands.
*   **From `jobs.c`**: Handles the bookkeeping of all background and stopped jobs.
*   **From `fg_bg.c`**: Implements the logic for the built-in `fg` and `bg` commands.
*   **From `signals.c`**: Installs custom handlers for signals like `SIGINT`, `SIGTSTP`, and `SIGCHLD`.
*   **From `main.c`**: Provides the main entry point and the primary loop for the shell.
