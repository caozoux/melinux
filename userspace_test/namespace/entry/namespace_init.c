/* simple_init.c
   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2or later
   A simple init(1)-style program to be used asthe init program in
   a PID namespace. The program reaps thestatus of its children and
   provides a simple shell facility forexecuting commands.
*/

#define _GNU_SOURCE
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<wordexp.h>
#include<errno.h>
#include<sys/wait.h>

#define errExit(msg)    do { perror(msg);exit(EXIT_FAILURE); \
                        } while (0)

static int verbose= 0;
/* Display waitstatus (from waitpid() or similar) given in 'status' */
/* SIGCHLDhandler: reap child processes as they change state */

 

static void
child_handler(intsig)
{
    pid_t pid;
    int status;

    /* WUNTRACED and WCONTINUED allow waitpid()to catch stopped and
       continued children (in addition toterminated children) */

    while ((pid = waitpid(-1, &status,
                          WNOHANG | WUNTRACED |WCONTINUED)) != 0) {
        if (pid == -1) {
            if (errno == ECHILD)        /* No more children */
                break;
            else
               perror("waitpid");     /* Unexpected error */
        }

        if (verbose)
            printf("\tinit: SIGCHLDhandler: PID %ld terminated\n",
                    (long) pid);
    }
}

 

/* Perform wordexpansion on string in 'cmd', allocating and

   returning a vector of words on success orNULL on failure */

 

static char **
expand_words(char*cmd)
{
    char **arg_vec;
    int s;
    wordexp_t pwordexp;

    s = wordexp(cmd, &pwordexp, 0);

    if (s != 0) {
        fprintf(stderr, "Word expansionfailed\n");
        return NULL;
    }

    arg_vec = calloc(pwordexp.we_wordc + 1,sizeof(char *));
    if (arg_vec == NULL)
        errExit("calloc");

    for (s = 0; s < pwordexp.we_wordc; s++)
        arg_vec[s] = pwordexp.we_wordv[s];

    arg_vec[pwordexp.we_wordc] = NULL;

    return arg_vec;
}

static void
usage(char *pname)
{
    fprintf(stderr, "Usage: %s[-q]\n", pname);
    fprintf(stderr, "\t-v\tProvide verboselogging\n");

    exit(EXIT_FAILURE);
}

int
main(int argc,char *argv[])
{
    struct sigaction sa;
#define CMD_SIZE 10000
    char cmd[CMD_SIZE];
    pid_t pid;
    int opt;

    while ((opt = getopt(argc, argv,"v")) != -1) {
        switch (opt) {
			case 'v': verbose = 1;          break;
			default:  usage(argv[0]);
        }
    }

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = child_handler;

    if (sigaction(SIGCHLD, &sa, NULL) ==-1)
        errExit("sigaction");

    if (verbose)
        printf("\tinit: my PID is%ld\n", (long) getpid());

    /* Performing terminal operations while notbeing the foreground
       process group for the terminal generatesa SIGTTOU that stops the
       process. However our init "shell" needs to be able to perform
       such operations (just like a normalshell), so we ignore that
       signal, which allows the operations toproceed successfully. */

    signal(SIGTTOU, SIG_IGN);

    /* Become leader of a new process group andmake that process
       group the foreground process group forthe terminal */

    if (setpgid(0, 0) == -1)
        errExit("setpgid");;

    if (tcsetpgrp(STDIN_FILENO, getpgrp()) ==-1)
        errExit("tcsetpgrp-child");

    while (1) {
        /* Read a shell command; exit on end offile */
        printf("init$ ");
        if (fgets(cmd, CMD_SIZE, stdin) ==NULL) {
            if (verbose)
                printf("\tinit:exiting");
            printf("\n");
            exit(EXIT_SUCCESS);
        }

        if (cmd[strlen(cmd) - 1] == '\n')
            cmd[strlen(cmd) - 1] = '\0';        /* Strip trailing '\n' */

        if (strlen(cmd) == 0)
            continue;           /* Ignore empty commands */

        pid = fork();           /* Create child process */
        if (pid == -1)
            errExit("fork");

        if (pid == 0) {         /* Child */
            char **arg_vec;
            arg_vec = expand_words(cmd);

            if (arg_vec == NULL)        /* Word expansion failed */
                continue;

            /* Make child the leader of a newprocess group and
               make that process group theforeground process
               group for the terminal */

            if (setpgid(0, 0) == -1)
                errExit("setpgid");;

            if (tcsetpgrp(STDIN_FILENO,getpgrp()) == -1)
               errExit("tcsetpgrp-child");

            /* Child executes shell command andterminates */

            execvp(arg_vec[0], arg_vec);
            errExit("execvp");          /* Only reached if execvp() fails */
        }

        /* Parent falls through to here */

        if (verbose)
            printf("\tinit: created child%ld\n", (long) pid);

        pause();                /* Will be interrupted by signalhandler */

        /* After child changes state, ensurethat the 'init' program
           is the foreground process group forthe terminal */

        if (tcsetpgrp(STDIN_FILENO, getpgrp())== -1)
            errExit("tcsetpgrp-parent");
    }
}
