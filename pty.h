#ifndef PTY_H
#define PTY_H
#include "config.h"
int fork_shell();
int start_shell(int slave_pty);
#endif
