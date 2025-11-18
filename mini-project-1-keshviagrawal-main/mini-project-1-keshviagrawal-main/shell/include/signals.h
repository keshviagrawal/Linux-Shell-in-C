#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

// Function declarations
void ping(pid_t pid, int signal_number);
void handle_sigint(int signo);
void handle_sigtstp(int signo);
void setup_signal_handlers(void);

#endif // SIGNALS_H
