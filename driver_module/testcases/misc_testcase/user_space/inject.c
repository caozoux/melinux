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
	printf("inject --NULL inject NULL pionter write except\n");
	printf("inject --write_proctect inject write proctect except\n");
	printf("inject --mutex_deplock inject mutext deplock error\n");
	printf("inject --spinlock_deplock inject mutext deplock error\n");
	printf("inject --irqspinlock_deplock  inject error\n");
	printf("inject --rcu_hang  inject error\n");
	printf("inject --softwatchdog_timeout  inject error\n");
}

int inject_usage(int argc, char **argv)
{
	static const struct option long_options[] = {
		{"help",     no_argument, 0,  0 },
		{"NULL",     no_argument, 0,  0 },
		{"write_proctect",   no_argument, 0,  0 },
		{"mutex_deplock",   required_argument, 0,  0 },
		{"spinlock_deplock",   required_argument, 0,  0 },
		{"irqspinlock_deplock",   no_argument, 0,  0 },
		{"rcu_hang",   no_argument, 0,  0 },
		{"softwatchdog_timeout",   no_argument, 0,  0 },
		{0,0,0,0}};
	int c;
	struct ioctl_data data;
	int __attribute__ ((unused)) ret;
	unsigned long zone_vm_stat[64];
	unsigned long numa_vm_stat[64];
	unsigned long node_vm_stat[64];

	data.type = IOCTL_INJECT;

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
				data.cmdcode = IOCTL_INJECT_NULL;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			case 2:
				data.cmdcode = IOCTL_INJECT_WRITE_PROTECT;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			case 3:
				data.cmdcode = IOCTL_INJECT_MUTET_DEPLOCK;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			case 4:
				data.cmdcode = IOCTL_USEKTIME_DEV_SCAN;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			case 5:
				data.cmdcode = IOCTL_INJECT_IRQSPINLOCK_DEPLOCK;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			case 6:
				data.cmdcode =IOCTL_INJECT_IRQSPINLOCK_DEPLOCK;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			case 7:
				data.cmdcode = IOCTL_INJECT_SOFTWATCHDOG_TIMEOUT;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
			default:
				break;
		}
	}

	return 0;
}

