#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/mman.h>
#include "template_iocmd.h"
#include "common_head.h"

extern int misc_fd;

static void help(void)
{
	
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"init",     no_argument, 0,  0 },
	{0,0,0,0}
};

int kmem_usage(int argc, char **argv)
{
	struct ioctl_data data;

	if (argc <= 1) { 
		help();
		return 0;
	}

	ioctl_data_init(&data);

	while (1) {
		int option_index = -1;

		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (option_index) {
			case 0:
				help();
				return 0;
			default:
				break;
		}
	}

	return 0;
}
