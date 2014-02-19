#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <pty.h>
#include "pty.h"

pid_t fork_shell(int *master_out) {
   int master, slave; 
   struct winsize sz = { 20, 80, 0, 0 };
   pid_t pid;
   openpty(&master, &slave, NULL, NULL, &sz);
   pid = fork();
   if (pid == -1) {
       perror("fork failure");
       exit(-1);
   } else if (pid == 0) {
       close(master);

       int rv;
       if((rv = start_shell(slave)) != 0) {
           perror("Error starting shell");
           close(master);
           exit(-1);
       }

       // never returns
       return pid;
   } else {
       close(slave);

       if (master_out != NULL) {
           *master_out = master;
       }
       
       return pid;
   }
}

// only returns if there's an error
int start_shell(int term_pty) {
    struct termios tsettings;
    tcgetattr(term_pty, &tsettings);
    // what modifications do we actually need to make?
    /* tsettings.c_lflag &= ~ECHO; // only for term wrapper */
    tcsetattr(term_pty, TCSANOW, &tsettings);
    setsid();
    ioctl(term_pty, TIOCSCTTY, 1);
    dup2(term_pty, 0); // stdin
    dup2(term_pty, 1); // stdout
    dup2(term_pty, 2); // stderr
    // TODO: signal that pipes are set up
    close(term_pty);
    char *argv[] = { config.shell, NULL };
    printf("%s\n", config.shell);
    return execvp(argv[0], argv);
}
