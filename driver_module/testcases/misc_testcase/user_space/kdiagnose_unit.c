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
	printf("diagnose --irq_trace irq trace for long time irq\n");
	printf("diagnose --enable    enable irq trace for long time irq\n");
	printf("diagnose --disable   enable irq trace for long time irq\n");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"irq_trace",     no_argument, 0,  0 },
	{"enable", no_argument, 0,  0 },
	{"disable", no_argument, 0,  0 },
	{"hwirq", required_argument, 0,  0 },
	{"softirq", required_argument, 0,  0 },
	{"htrimer", required_argument, 0,  0 },
	{"softtimer", required_argument, 0,  0 },
	{0,0,0,0}
};

int kdiagnose_usage(int argc, char **argv)
{
	struct ioctl_data data;
	struct trace_ioctl *p_data;
	int ret, c;
	int enable, hwirq_thro, softirq_thro;
	int htrimer_thro, softtimer_thro;

	if (argc <= 1) { 
		help();
		return 0;
	}

	p_data = &data.trace_data;
	data.type = IOCTL_TRACE;
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
			case 1:
				data.cmdcode = IOCTL_TRACE_IRQ_ALL;
				//ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;

			case 2:
				p_data->enable = 1;
				break;

			case 3:
				p_data->enable = 0;
				break;

			case 4:
				p_data->thro_set.hwirq= atoi(optarg);
				break;

			case 5:
				p_data->thro_set.softirq= atoi(optarg);
				break;

			case 6:
				p_data->thro_set.hrtimer= atoi(optarg);
				break;
			case 7:
				p_data->thro_set.softtimer= atoi(optarg);
				break;

			default:
				break;
		}
	}

	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}

