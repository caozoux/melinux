#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include "template_iocmd.h"
#include "common_head.h"

extern int misc_fd;

static void help(void)
{
	printf("--kmem --mode get  get the vm_state of zone/none/numa");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"lock",     no_argument, 0,  0 },
	{"unlock",   no_argument, 0,  0 },
	{"tryloc",   no_argument, 0,  0 },
	{"hwlock",   no_argument, 0,  0 },
	{"hwunlock",   no_argument, 0,  0 },
	{"hwtryloc",   no_argument, 0,  0 },
	{0,0,0,0}};

int lock_usage(int argc, char **argv)
{
	struct ioctl_data data;
	int c;

	while (1) {

		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;

		switch (option_index) {
			case '0':
				help();
				break;
			case '1':
				data.cmdcode = IOCTL_HARDLOCK_LOCK;
				break;
			case '2':
				data.cmdcode = IOCTL_HARDLOCK_UNLOCK;
				break;
			case '3':
				data.cmdcode = IOCTL_HARDLOCK_TRYLOCK;
				break;
			case '4':
				data.cmdcode = IOCTL_HARDLOCK_IRQLOCK;
				break;
			case '5':
				data.cmdcode = IOCTL_HARDLOCK_IRQUNLOCK;
				break;
			case '6':
				data.cmdcode = IOCTL_HARDLOCK_IRQTRYLOCK;
				break;
			default:
				printf("hardlock operation not support\n");
				return -1;
		}
	}

	
	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}
