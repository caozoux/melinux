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
	printf("lock --lock spin lock\n");
	printf("lock --unlock spin unlock\n");
	printf("lock --trylock try lock\n");
	printf("lock --hwlock spin hw lock\n");
	printf("lock --hwunlock spin hw unlock\n");
	printf("lock --hwtrylock spin hw try lock\n");
	printf("lock --semaup semaphore up\n");
	printf("lock --semadown semaphore down\n");
	printf("lock --read_semaup semaphore up\n");
	printf("lock --read_semadown semaphore down\n");
	printf("lock --write_semaup semaphore up\n");
	printf("lock --write_semadown semaphore down\n");
}

static const struct option long_options[] = {
	{"help",      no_argument, 0,  0 },
	{"lock",      no_argument, 0,  0 },
	{"unlock",    no_argument, 0,  0 },
	{"trylock",   no_argument, 0,  0 },
	{"hwlock",    no_argument, 0,  0 },
	{"hwunlock",  no_argument, 0,  0 },
	{"hwtrylock", no_argument, 0,  0 },
	{"semadown",    no_argument, 0,  0 },
	{"semaup",  no_argument, 0,  0 },
	{"read_semadown",    no_argument, 0,  0 },
	{"read_semaup",  no_argument, 0,  0 },
	{"write_semadown",    no_argument, 0,  0 },
	{"write_semaup",  no_argument, 0,  0 },
	{0,0,0,0}};

int lock_usage(int argc, char **argv)
{
	struct ioctl_data data;
	int c;

	if (argc <= 1) { 
		help();
		return 0;
	}

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		//printf("zz %s c:%08x \n",__func__, (int)c);
		if (c == -1) {
			break;
		} else if (c == 0xcf){
			break;
		}

		data.type=IOCTL_LOCK;
		//printf("zz %s option_index:%08x \n",__func__, (int)option_index);
		switch (option_index) {
			case 0:
				help();
				break;
			case 1:
				data.cmdcode = IOCTL_HARDLOCK_LOCK;
				break;
			case 2:
				data.cmdcode = IOCTL_HARDLOCK_UNLOCK;
				break;
			case 3:
				data.cmdcode = IOCTL_HARDLOCK_TRYLOCK;
				break;
			case 4:
				data.cmdcode = IOCTL_HARDLOCK_IRQLOCK;
				break;
			case 5:
				data.cmdcode = IOCTL_HARDLOCK_IRQUNLOCK;
				break;
			case 6:
				data.cmdcode = IOCTL_HARDLOCK_IRQTRYLOCK;
				break;
			case 7:
				data.cmdcode = IOCTL_SEMAPHORE_DOWN;
				break;
			case 8:
				data.cmdcode = IOCTL_SEMAPHORE_UP;
				break;
			case 9:
				data.cmdcode = IOCTL_SEMAPHORE_READ_DOWN;
				break;
			case 10:
				data.cmdcode = IOCTL_SEMAPHORE_READ_UP;
				break;
			case 11:
				data.cmdcode = IOCTL_SEMAPHORE_WRITE_DOWN;
				break;
			case 12:
				data.cmdcode = IOCTL_SEMAPHORE_WRITE_UP;
				break;
			default:
				printf("hardlock operation not support\n");
				return -1;
		}
	}
	printf("zz %s %d %lx\n", __func__, __LINE__, data.type);
	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}
