
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void usage_help()
{
	printf("help:  -t [optcode:softlock/hwlock/mem/rcu] -o optcode\n");
	printf("       -s cnt   sleep mis\n");
	printf("       -c cnt   loop cnt \n");
}


int main(int argc, char *argv[])
{
	char ch;
	int ret;
	unsigned long loop, sleep_us, ccly_cnt;

    while((ch=getopt(argc,argv,"hs:c:"))!=-1)
  	{
		switch (ch) {
			case 'h':
				usage_help();
				break;
			case 's':
				sleep_us = atoi(optarg);
				break;
			case 'c':
				loop = atoi(optarg);
				break;
			default:
				usage_help();
				return 0;
		}
	}

	if (loop > 0) {
		while(loop--) {
			ccly_cnt++;
			usleep(sleep_us);
		}
	}

	printf("ccly_cnt");

out:
	return 0;
}
