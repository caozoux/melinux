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

static void block_help(void)
{
	printf("block --file 0 enum task file fd\n");
}

static int block_file(int fd ,struct ioctl_data *data, char *arg)
{
	char ch = *arg;
	printf("zz %s %d \n", __func__, __LINE__);
	switch (ch) {
		case '0':
			data->cmdcode = IOCTL_USEBLOCK_FILE;
			return ioctl(misc_fd, sizeof(struct ioctl_data), data);
		default:
			block_help();
			break;
	}
}

int block_usage(int argc, char **argv)
{
	static const struct option long_options[] = {
		{"help",     no_argument, 0,  0 },
		{"file",     required_argument, 0,  0 },
		{"inode",     required_argument, 0,  0 },
		{0,0,0,0}};
	int c;
	struct ioctl_data data;
	int __attribute__ ((unused)) ret;

	data.type = IOCTL_USEBLOCK;

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if ( c == -1) {
			//block_help();
			break;
		}
		switch (option_index) {
			case 0:
				//data.cmdcode = IOCTL_USEBLOCK_INDOE;
				//return ioctl(misc_fd, sizeof(struct ioctl_data), data);
				block_help();
				break;
			case 1:
				block_file(misc_fd, &data, optarg);
				break;
			case 2:
				data.cmdcode = IOCTL_USEBLOCK_INDOE;
				return ioctl(misc_fd, sizeof(struct ioctl_data), data);
				break;
			default:
				break;
		}
	}

	
	return 0;
}
