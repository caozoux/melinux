/* multi_pidns.c

 

   Copyright 2013, Michael Kerrisk

   Licensed under GNU General Public License v2or later

 

   Create a series of child processes in nestedPID namespaces.

*/

#define _GNU_SOURCE

#include<sched.h>

#include<unistd.h>

#include<stdlib.h>

#include<sys/wait.h>

#include<string.h>

#include<signal.h>

#include<stdio.h>

#include<limits.h>

#include<sys/mount.h>

#include<sys/types.h>

#include<sys/stat.h>

 

/* A simpleerror-handling function: print an error message based

   on the value in 'errno' and terminate thecalling process */

 

#define errExit(msg)    do { perror(msg);exit(EXIT_FAILURE); \
                        } while (0)

 

#define STACK_SIZE (1024 * 1024)

 
static char child_stack[STACK_SIZE];    /* Space forchild's stack */
                /* Since each child gets a copyof virtual memory, this
                   buffer can be reused as eachchild creates its child */

/* Recursivelycreate a series of child process in nested PID namespaces.
   'arg' is an integer that counts down to 0during the recursion.
   When the counter reaches 0, recursion stopsand the tail child
   executes the sleep(1) program. */

static int
childFunc(void*arg)
{
    static int first_call = 1;
    long level = (long) arg;

    if (!first_call) {

        /* Unless this is the first recursivecall to childFunc()
           (i.e., we were invoked from main()),mount a procfs
           for the current PID namespace */

        char mount_point[PATH_MAX];

        snprintf(mount_point, PATH_MAX,"/proc%c", (char) ('0' + level));
        mkdir(mount_point, 0555);       /* Create directory for mount point */
        if (mount("proc",mount_point, "proc", 0, NULL) == -1)
            errExit("mount");
        printf("Mounting procfs at %s\n",mount_point);

    }

    first_call = 0;
    if (level > 0) {
        /* Recursively invoke childFunc() tocreate another child in a
           nested PID namespace */
        level--;
        pid_t child_pid;

        child_pid = clone(childFunc,
                    child_stack +STACK_SIZE,   /* Points to start of downwardly growing stack */
                    CLONE_NEWPID | SIGCHLD,(void *) level);

        if (child_pid == -1)
            errExit("clone");

        if (waitpid(child_pid, NULL, 0) ==-1)  /* Wait for child */
            errExit("waitpid");
    } else {
        /* Tail end of recursion: executesleep(1) */
        printf("Final childsleeping\n");
        execlp("sleep","sleep", "1000", (char *) NULL);
        errExit("execlp");

    }

 
    return 0;

}

 

int

main(int argc,char *argv[])

{

    long levels;


    levels = (argc > 1) ? atoi(argv[1]) : 5;
    childFunc((void *) levels);

 

    exit(EXIT_SUCCESS);

}
