#ifndef PTY_H
#define PTY_H
#include "config.h"
int init_shell();
int start_shell(int slave_pty);
#endif
