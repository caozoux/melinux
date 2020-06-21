#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "template_iocmd.h"
#include "common_head.h"


#define DEV_NAME "/dev/misc_template"

struct ping_data {
	short unsigned int  s_send;
	short unsigned int  s_recv;
};

void usage_help()
{
	printf("help: -t [optcode:softlock/hwlock/mem/rcu] -o optcode\n");
	printf("       -s softlock: test softlock \n");
	printf("       -w hwlock:   hw lock \n");
	printf("       -r rcu:      hw lock \n");
	printf("       -m mem:      test mem\n");
	printf("       -k funcname  dumpstack of kernel function \n");
	printf("       mem:         -o  dump    show the page pgd pud pmd pte info\n");
	printf("       mem:         -o  vmmax   test the max vmlloc support \n");
}


static int kprobe_test(int fd ,struct ioctl_data *data)
{
	return ioctl(fd, sizeof(struct ioctl_data), data);
}

int main(int argc, char *argv[])
{
	struct ioctl_data data;
	char ch;
	int fd;	
	int ret;
	char name[128];

	if (argc == 1) {
		usage_help();
		return 0;
	}

	fd = open(DEV_NAME, O_RDWR);
	if (fd <= 0) {
		printf("%s open failed", DEV_NAME);
		return 0;
	}

    while((ch=getopt(argc,argv,"hswrm:k:"))!=-1)
  	{
		switch (ch) {
			case 'h':
				usage_help();
				break;
			case 's':
				data.type = IOCTL_SOFTLOCK;
				break;
			case 'w':
				data.type = IOCTL_HARDLOCK;
				break;
			case 'r':
				ruc_test(fd);
				goto out;
			case 'm':
				data.type = IOCTL_MEM;
				data.cmdcode = IOCTL_TYPE_VMALLOC_MAX;
				break;

			case 'k':
				data.type = IOCTL_USEKPROBE;
				data.cmdcode = IOCTL_USEKRPOBE_FUNC_DUMP;
				printf("%s\n", optarg);
				strcpy(name, optarg);
				data.kp_data.name = name;
				data.kp_data.len = strlen(name);
				data.kp_data.dump_buf= malloc(4096);
				data.kp_data.dump_len= 4096;
				ret = kprobe_test(fd, &data);
				if (ret) {
					printf("kprobe failed\n");
					return 0;
				}

				printf("%s\n", data.kp_data.dump_buf);

				break;
			default:
				usage_help();
				return 0;
		}
	}


out:
	close(fd);
	return 0;
}
