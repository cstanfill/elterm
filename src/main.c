#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include "display.h"
#include "main.h"
#include "pty.h"

int master_pty = -1;
int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    pid_t child = fork_shell(&master_pty);
    fd_set rfds;
    int rv;
    if ((rv = display_init()) < 0) {
        printf("Error setting up display. Quitting ...\r\n");
        exit(1);
    }
    add_screen(&all_screens, new_screen(master_pty));

    char inbuf[8192];
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(master_pty, &rfds);
        FD_SET(x11fd, &rfds);
        if ((rv = select(FD_SETSIZE, &rfds, NULL, NULL, NULL)) <= 0) {
            if (rv == 0) {
                exit(0);
            } else {
                perror("error in select");
                exit(-1);
            }
        }
        if (FD_ISSET(0, &rfds)) {
            int ct = read(0, inbuf, 8192);
            if (ct > 0) {
                write(master_pty, inbuf, ct);
            } else if (ct ==0) {
                inbuf[0] = 4;
                write(master_pty, inbuf, 1);
            } else {
                perror("error reading from stdin");
            }
        }
        if (FD_ISSET(master_pty, &rfds)) {
            int ct = read(master_pty, inbuf, 8192);
            if (ct > 0) {
                inbuf[ct] = 0;
                write_string(all_screens.screens[0].buffer, inbuf, ct);
                refresh(all_screens.screens);
            } else if (ct == 0) {
                waitpid(child, NULL, 0);
                exit(0);
            } else {
                printf("Shell pty closed; exiting\n");
                waitpid(child, NULL, 0);
                exit(0);
            }
        }

        if (FD_ISSET(x11fd, &rfds)) {
            handle_x11evs();
        }
    }
}
