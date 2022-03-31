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
	printf("kprobe --func      specify function\n");
	printf("kprobe --hook      hook specify function and no call the real function\n");
	printf("kprobe --printk    kprobe specify function and printk function call\n");
	printf("kprobe --free      hook specify function and no call the real function\n");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"dumpstack",     no_argument, 0,  0 },
	{"func",     required_argument, 0,  0 },
	{"hook",     no_argument, 0,  0 },
	{"cpu",      required_argument, 0,  0 },
	{"free",     no_argument, 0,  0 },
	{"printk",     no_argument, 0,  0 },
	{0,0,0,0}};

int kprobe_usage(int argc, char **argv)
{
	struct ioctl_data data;
	int c, ret;
	char func_name[128]="";
	int op_dump = 0;
	int hook=0, free = 0, printk=0;
	int cpu = -1;

	if (argc <= 1) { 
		help();
		return 0;
	}

	data.type = IOCTL_USEKPROBE;
	data.kp_data.dump_buf= NULL;
	data.kp_data.dump_len= 0;
	data.kp_data.hook = 0;
	data.kp_data.free = 0;
	data.kp_data.cpu = -1;

	while (1) {
		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;

		switch (option_index) {
			case 0:
				help();
				break;

			case 1:
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
			case 2:
				strcpy(func_name, optarg);
				data.kp_data.name = func_name;
				data.kp_data.len = strlen(func_name);
				break;

			case 3:
				hook=1;
				break;

			case 4:
				free=1;
				break;

			case 5:
				free=1;
				break;

			case 6:
				printk=1;
				break;

			default:
				break;
		}
	}

	printf("printk %d name:%s\n", printk, func_name);
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
	} else if (hook) {
		data.cmdcode = IOCTL_USEKRPOBE_KPROBE_HOOK;
	} else if (printk) {
		data.cmdcode = IOCTL_USEKRPOBE_KPROBE_FUNC;
	} else if (free) {
		data.cmdcode = IOCTL_USEKRPOBE_KPROBE_FREE;
	}
	
	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}

