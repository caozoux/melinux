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

static void help()
{
	printf("-t|--type:  anon/dirty/shem/mmap/thp\n");	
	printf("-f|--file:  specify file name\n");	
	printf("-s|--size:  page count\n");	
	printf("-k|--fork:  fork proccessor\n");	
}

static struct option long_options[] =
{
	/* Use flags like so:
	{"verbose",	no_argument,	&verbose_flag, 'V'}*/
	/* Argument styles: no_argument, required_argument, optional_argument */
	{"version", no_argument,	0,	'v'},
	{"help",	no_argument,	0,	'h'},
	{"fork",	no_argument,	0,	'k'},
	{"type",	required_argument,	0,	't'},
	{"file",required_argument,	0,	'f'},
	{"size",required_argument,	0,	's'},
	{0,0,0,0}
};


int main(int argc, char *argv[])
{
	int args_size = 0;
	int args_fork = 0;
	char args_type[64] = {0};
	char args_file[256] = {0};
	char pagebuf[4096] = {0};
	int choice;
	int fd;

	while (1) {
		int option_index = 0;
		choice = getopt_long( argc, argv, "vht:-f:s:k",
					long_options, &option_index);
		if (choice == -1)
			break;
		switch( choice )
		{
			case 'v':
				break;
	
			case 'h':
				help();
				return 0;
	
			case 't':
				memcpy(args_type, optarg, strlen(optarg));
				break;

			case 'f':
				memcpy(args_file, optarg, strlen(optarg));
				break;

			case 's':
				args_size = atoi(optarg);
				break;

			case 'k':
				args_fork= 1;
				break;

			default:
				return -1;
		}
	}

	if (!strcmp(args_type, "anon")) {
		unsigned long cnt = args_size;
		while(cnt--) {
			void *buf;
			buf = malloc(4096);
			if (buf)
				memset(buf, 0, 4096);
		}
	} else if (!strcmp(args_type, "dirty")) {
		unsigned long cnt = args_size;
   		fd = open(args_file, O_CREAT | O_RDWR); 
		while(cnt--) {
			write(fd,pagebuf, 4096);
		}
	} else if (!strcmp(args_type, "thp")) {
		void *buf;
		int ret;
		unsigned long cnt = args_size;
		unsigned long size = cnt * 4096;

		buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (buf == MAP_FAILED)
			exit(1);
		//ret = madvise(buf, size, MADV_HUGEPAGE);
		if (ret)
			exit(1);
		//memset(buf, 0, size);
	} else if (!strcmp(args_type, "mmap")) {
		unsigned long cnt = args_size;
		char  *addr;
   		fd = open(args_file, O_CREAT | O_RDWR); 
		if (fd < 0) {
			printf("Err: open %s failed\n", args_file);
			goto out;
		}
		addr = (char *)mmap(0, cnt, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		printf("zz %s addr:%lx cnt:%lx \n",__func__, (unsigned long)addr, (unsigned long)cnt);
		//while(cnt--) {
		//	memset(addr, 0, 4096);
		//	addr += 4096;
		//}
	} else {
			printf("Err: type is lost\n");
			goto out;
	}


	if (args_fork) {
				
		if(fork()) // 父进程
		{ 
			while(1)
				sleep(1);
		} else { // 子进程读
			while(1)
				sleep(1);
		}
	}
	while(1)
		sleep(1);
	return 0;
out:
	return -1;
}

