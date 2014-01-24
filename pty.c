#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "pty.h"

int init_shell() {
    
}

// only returns if there's an error
int start_shell(int term_pty) {
    struct termios tsettings;
    tcgetattr(term_pty, &tsettings);
    // what modifications do we actually need to make?
    tcsetattr(term_pty, TCSANOW, &tsettings);
    setsid();
    ioctl(term_pty, TIOCSCTTY, 1);
    dup2(term_pty, 0); // stdin
    dup2(term_pty, 1); // stdout
    dup2(term_pty, 2); // stderr
    // TODO: signal that pipes are set up
    close(term_pty);
    char *argv[] = { config.font_pattern, "" };
    return execvp(argv[0], argv);
}
