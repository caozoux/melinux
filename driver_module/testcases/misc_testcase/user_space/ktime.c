#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include "../template_iocmd.h"
#include "common_head.h"

extern int misc_fd;

static void help(void)
{
	printf("ktimer --scan scan all the clockevent device\n");
}

int ktime_usage(int argc, char **argv)
{
	static const struct option long_options[] = {
		{"help",     no_argument, 0,  0 },
		{"scan",     no_argument, 0,  0 },
		{"output",   required_argument, 0,  0 },
		{0,0,0,0}};
	int c;
	struct ioctl_data data;
	int __attribute__ ((unused)) ret;
	unsigned long zone_vm_stat[64];
	unsigned long numa_vm_stat[64];
	unsigned long node_vm_stat[64];

	data.type = IOCTL_USEKTIME;

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);

		if (c == -1) {
			help();
			break;
		}

		switch (option_index) {
			case 0:
				help();
				break;
			case 1:
				data.cmdcode = IOCTL_USEKTIME_DEV_SCAN;
				return ioctl(misc_fd, sizeof(struct ioctl_data), data);
				break;
			default:
				break;
		}
	}

	
	return 0;
}
