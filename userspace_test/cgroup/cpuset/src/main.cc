#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h> 
#include <sys/mman.h> 
#include <getopt.h>
#include <sstream>
#include <string>

#define MAP_LENGTH      (1UL<<34) 
using namespace std;

void *func(void *arg)
{
	while(1);
	return NULL;
}

static void help()
{
	printf("-t|--type:    busy\n");	
	printf("-d|--thread:  page count\n");	
	printf("-c|--cpus:    busy cpus\n");	
}

static struct option long_options[] =
{
	/* Use flags like so:
	{"verbose",	no_argument,	&verbose_flag, 'V'}*/
	/* Argument styles: no_argument, required_argument, optional_argument */
	{"version", no_argument,	0,	'v'},
	{"help",	no_argument,	0,	'h'},
	{"type",	required_argument,	0,	't'},
	{"cpu",required_argument,	0,	'c'},
	{"thread",required_argument,	0,	'd'},
	{0,0,0,0}
};

int main(int argc, char *argv[])
{
	int args_size = 0;

	char args_type[64] = {0};
	int choice;
	int fd;
	int threads = 0;

	while (1) {
		int option_index = 0;
		choice = getopt_long( argc, argv, "vht:d:c:",
					long_options, &option_index);
		if (choice == -1)
			break;
		switch( choice )
		{
			case 'h':
				help();
				return 0;
			case 'v':
				break;
			case 't':
				memcpy(args_type, optarg, strlen(optarg));
				break;
			case 'c':
				threads = atoi(optarg);
				break;

			default:
				return -1;
		}
	}

	if (!strcmp(args_type, "busy")) {
		int i;
		int cnt = threads ? threads: 1;
		for (i = 0; i < cnt; ++i) {
			pthread_t tid;
			while(cnt--)
				pthread_create(&tid, NULL, func, NULL);
		}
	} else {
			printf("Err: type is lost\n");
			goto out;
	}

	while(1)
		sleep(1);
	return 0;
out:
	return -1;
}

