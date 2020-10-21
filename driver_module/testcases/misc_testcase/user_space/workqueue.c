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
	printf("workqueue --wk_sig_cpu\n");
	printf("workqueue --wk_sig_lock\n");
	printf("workqueue --wk_sig_hwlock\n");
	printf("workqueue --wk_sig_percpu_hwlock\n");
	printf("workqueue --wk_sig_cpu_hwlock_race\n");
	printf("workqueue --wk_perfermance_delay\n");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"wk_sig_cpu",     no_argument, 0,  0 },
	{"wk_sig_cpu_lock",     no_argument, 0,  0 },
	{"wk_sig_cpu_hwock",     no_argument, 0,  0 },
	{"wk_sig_percpu_hwlock",     no_argument, 0,  0 },
	{"wk_sig_cpu_hwlock_race",     no_argument, 0,  0 },
	{"wk_perfermance_delay",     no_argument, 0,  0 },
	{"runtime",     required_argument, 0,  0 },
	{0,0,0,0}};

int workqueue_usage(int argc, char **argv)
{
	int runtime, ret;
	int c;
	struct ioctl_data data;
	unsigned long workqueue_wakeup_time;

	while (1) {

		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1) {
			help();
			break;
		}

		data.type = IOCTL_USEWORKQUEUE;
		data.wq_data.workqueue_performance = &workqueue_wakeup_time;
		data.wq_data.workqueue_performance = 0;
		switch (option_index) {
			case '0':
				help();
			case '1':
				data.cmdcode = IOCTL_USEWORKQUEUE_SIG;
				break;
			case '2':
				data.cmdcode = IOCTL_USEWORKQUEUE_SIG_SPINLOCK;
				break;
			case '3':
				data.cmdcode = IOCTL_USEWORKQUEUE_SIG_SPINLOCKIRQ;
				break;
			case '4':
				data.cmdcode =  IOCTL_USEWORKQUEUE_PERCPU_SPINLOCKIRQ_RACE;
				break;
			case '5':
				data.cmdcode =  IOCTL_USEWORKQUEUE_PEFORMANCE_DELAY;
				break;
			case '6':
				runtime = atoi(optarg);
				data.wq_data.runtime = runtime;
				break;
			default:
				printf("hardlock operation not support\n");
				return -1;
		}
	}

	ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);

	if (data.cmdcode ==  IOCTL_USEWORKQUEUE_PEFORMANCE_DELAY)
		printf("the workqueue wakeup time:%ld \n", *(data.wq_data.workqueue_performance));
	
	return ret;
}
