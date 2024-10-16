#include <iostream>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <linux/sched.h>
#include <ksioctl/ksched_ioctl.h>

#include "kapp_unit.h"

static int dump_args_handle(int argc, char **argv);
int monitor_args_handle(int argc, char **argv);

struct cmdargs ksched_args[] = {
	{"--dump",dump_args_handle, 
		"\n"
		"     -m mode\n"
		"     -d disable \n"
		"     -e enable\n"
	},
	{"--monitor", monitor_args_handle,
		"\n"
		"     -p pid\n"
		"     -i interat\n"
	},
};

static void dump_task_load(struct ksched_ioctl *ksched_data)
{
	printf("task:\n");
	printf("  weight:         %ld\n", ksched_data->se.weight>>10);
	printf("  load_sum:       %ld\n", ksched_data->se.load_sum);
	printf("  runnable_sum:   %ld\n", ksched_data->se.runnable_sum);
	printf("  util_sum:       %d\n", ksched_data->se.util_sum);
	printf("  period_contrib: %d\n", ksched_data->se.period_contrib);
	printf("  load_avg:       %ld\n", ksched_data->se.load_avg);
	printf("  runnable_avg:   %ld\n", ksched_data->se.runnable_avg);
	printf("  util_avg:       %ld\n", ksched_data->se.util_avg);
}

int monitor_args_handle(int argc, char **argv)
{
	static int monitor_args_pid,  monitor_args_interat;
	static struct option kmonit_opts[] = {
		{ "help",required_argument,NULL,'h'},
		{     0,    0,    0,    0},
	};

	int c, ret;
	struct ioctl_ksdata data;
	struct ksched_ioctl ksched_data;

	data.data = &ksched_data;
	data.len = sizeof(struct ksched_ioctl);

	while((c = getopt_long(argc, argv, ":p:i:", kmonit_opts, NULL)) != -1)
	{
		switch(c) {
			case 'p':
				monitor_args_pid = atoi(optarg);
				break;
			case 'i':
				monitor_args_interat = atoi(optarg);
				break;
			case 'm':
				break;
			default:
				break;
		}
	}

	if (monitor_args_pid) {
		ksched_data.pid = monitor_args_pid;
		if (monitor_args_interat) {
			while (1) {
				ret = ktools_ioctl::kioctl(IOCTL_KSCHED, (int)IOCTL_KSCHED_MONITOR_PID,
						&data, sizeof(struct ioctl_ksdata));
				dump_task_load(&ksched_data);
				usleep(monitor_args_interat * 1000);
			}

		} else {
			ret = ktools_ioctl::kioctl(IOCTL_KSCHED, (int)IOCTL_KSCHED_MONITOR_PID,
					&data, sizeof(struct ioctl_ksdata));
			dump_task_load(&ksched_data);
		}
			

	}

	return 0;
}

int dump_args_handle(int argc, char **argv)
{
	static int key_enable, key_disable;
	static struct option key_opts[] = {
		{ "enable",no_argument,&key_enable,'e'},
		{ "disable",no_argument,&key_disable,'d'},
		{ "mode",required_argument,NULL,'e'},
		{     0,    0,    0,    0},
	};
	int c;

	while((c = getopt_long(argc, argv, ":l:", key_opts, NULL)) != -1)
	{
		switch(c) {
			case 'd':
				break;
			case 'e':
				break;
			case 'm':
				break;
			default:
				break;
		}
	}
	return 0;
}

int unit_ksched(int argc, char** argv)
{
	int i;
	int is_help = 0;
	char *command;

	if (argc < 2) {
		return 0;
	}

	if (strstr(argv[1], "help"))  {
		is_help=1;
	}

	command = argv[1];
	for (i = 0; i < sizeof(ksched_args) / sizeof(struct cmdargs); i++) {
		if (is_help) {
			printf("    %s: %s\n", ksched_args[i].name, ksched_args[i].help);
		} else {
			if(!strcmp(command, ksched_args[i].name)) {
				argc--;
				argv++;
				return ksched_args[i].func(argc, argv);
			}
		}
	}

	return 0;
}

