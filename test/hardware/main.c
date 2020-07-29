#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

void usage_help()
{
	printf("help:  -t [optcode:softlock/hwlock/mem/rcu] -o optcode\n");
}

int main(int argc, char *argv[])
{
	char ch;
    	while((ch=getopt(argc,argv,"hsw:rm:k:q:"))!=-1)
    	{
		case 'h':
			usage_help();
			break;
		default:
			break;
	}
	return 0;
}
