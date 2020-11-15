#include <unistd.h>
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
	printf("radixtree --add  index raidixtree add inode\n");
	printf("radixtree --del  index raidixtree del inode\n");
	printf("radixtree --get  index raidixtree get inode\n");
	printf("radixtree --index set the radixed index\n");
	printf("radixtree --dump index raidixtree dump inode\n");
}

static const struct option long_options[] = {
	{"help",    no_argument, 0,  0 },
	{"add",     no_argument, 0,  0 },
	{"del",     no_argument, 0,  0 },
	{"get",   	no_argument, 0,  0 },
	{"index",   required_argument, 0,  0 },
	{"count",   required_argument, 0,  0 },
	{"dump",   	no_argument, 0,  0 },
	{0,0,0,0}
};

int radixtree_usage(int argc, char **argv)
{
	struct ioctl_data data;
	struct raidixtree_ioctl  *radixtree;
	int  __attribute__ ((unused)) ret = 0;
	int c;

	if (argc <= 1) { 
		help();
		return 0;
	}

	radixtree = &data.radix_data;
	radixtree->index = -1;
	radixtree->count = 1;
	data.type = IOCTL_USERAIDIXTREE;

	while (1) {
		int option_index = -1;

		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;

		switch (option_index) {
			case 0:
				help();
				return 0;

			case 1:
				radixtree->buf = malloc(4096);
				radixtree->buf_len = 4096;
				memset(radixtree->buf, 0, 4096);
				data.cmdcode = IOCTL_USERAIDIXTREE_ADD;
				break;

			case 2:
				data.cmdcode = IOCTL_USERAIDIXTREE_DEL;
				break;

			case 3:
				data.cmdcode = IOCTL_USERAIDIXTREE_GET;
				break;

			case 4:
				radixtree->index = atoi(optarg);
				break;

			case 5:
				data.cmdcode = IOCTL_USERAIDIXTREE_DUMP;
				break;

			case 6:
				radixtree->count = atoi(optarg);
				break;

			default:
				return -1;
		}
	}

	ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);

	return ret;
}

