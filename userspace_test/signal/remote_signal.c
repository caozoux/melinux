#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	int pid;
	if (argc == 1)
		printf("no pid  \n");

	pid = atoi(argv[1]);

	abort();
	kill(pid, 6);
	return 0;
}

