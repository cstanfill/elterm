#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "display.h"
#include "main.h"
#include "pty.h"

int master_pty = -1;
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char *argv[]) {
    pid_t child = fork_shell(&master_pty);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    FD_SET(master_pty, &rfds);
    display_init();

    char inbuf[512];
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(master_pty, &rfds);
        int rv;
        if ((rv = select(master_pty+1, &rfds, NULL, NULL, NULL)) <= 0) {
            if (rv == 0) {
                exit(0);
            } else {
                perror("error in select");
                exit(-1);
            }
        }
        if (FD_ISSET(0, &rfds)) {
            int ct = read(0, inbuf, 512);
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
            int ct = read(master_pty, inbuf, 512);
            if (ct > 0) {
                inbuf[ct] = 0;
                printf("%s", inbuf);
                fflush(stdout);
            } else if (ct == 0) {
                waitpid(child, NULL, 0);
                exit(0);
            } else { 
                waitpid(child, NULL, 0);
                exit(0);
            }
        }
    }
} 
