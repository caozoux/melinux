#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "../template_iocmd.h"

extern int misc_fd;

static void help(void)
{
	printf("kprobe --dumpstack  dump the specify function of stack_dump\n");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"dumpstack",     no_argument, 0,  0 },
	{"funcname",     required_argument, 0,  0 },
	{0,0,0,0}};

int kprobe_usage(int argc, char **argv)
{
	struct ioctl_data data;
	int c, ret;
	char func_name[128];
	int op_dump = 0;

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1) {
			help();
			break;
		}

		switch (option_index) {
			case '0':
				help();
				break;

			case '1':
				data.type = IOCTL_USEKPROBE;
				data.cmdcode = IOCTL_USEKRPOBE_FUNC_DUMP;
				data.kp_data.dump_buf= malloc(4096);
				data.kp_data.dump_len= 4096;
				op_dump = 1;
				ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				if (ret) {
					printf("kprobe failed\n");
					return 0;
				}
				printf("%s\n", data.kp_data.dump_buf);

				return;
			case '2':
				strcpy(func_name, optarg);
				break;

			default:
				printf("hardlock operation not support\n");
				return -1;
		}
	}

	if (op_dump) {
		data.kp_data.name = func_name;
		data.kp_data.len = strlen(func_name);
		ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
		if (ret) {
			printf("kprobe failed\n");
			return 0;
		} 
		printf("%s\n", data.kp_data.dump_buf);
		return 0;
	}
	
	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}

