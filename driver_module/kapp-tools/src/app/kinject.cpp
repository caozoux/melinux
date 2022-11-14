#include <iostream>
#include <getopt.h>
#include <string.h>
#include <ksioctl/kinject_ioctl.h>

#include "kapp_unit.h"

int hrtimer_args_handle(int argc, char **argv);
int statickey_args_handle(int argc, char **argv);
int slub_args_handle(int argc, char **argv);
struct cmdargs kinject_args[] = {
	{"--statickey",statickey_args_handle, 
		"\n"
		"     -e enable \n"
		"     -d disable \n"
	},
	{"--hrtimer", hrtimer_args_handle,
		"\n"
		"     --en enable \n"
		"     --dis disable\n"
		"     -t timer mis vaul\n"
	},
	{"--slub", slub_args_handle,
		"\n"
		"     --en enable \n"
		"     --dis disable\n"
		"     --overwrite inject slub overwrite\n"
	},
};

int slub_args_handle(int argc, char **argv)
{
	int c;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	static int slub_enable, slub_disable;
	static int slub_overwrite;
	int time=0;
	int ret;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option slub_opts[] = {
		{ "en",no_argument,&slub_enable,1},
		{ "dis",no_argument,&slub_disable,1},
		{ "overwrite",no_argument,&slub_overwrite,1},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", slub_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				time = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (slub_enable) {
		kinject_data.enable = 1;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_CTRL,
				(void*)&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject start slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (slub_disable) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_CTRL,
				(void*)&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (slub_overwrite) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_INJECT_SLUB_OVERWRITE,
				(void*)&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop slub failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	return 0;
}
int statickey_args_handle(int argc, char **argv)
{
	static int key_enable, key_disable;
	static struct option key_opts[] = {
		{ "enable",no_argument,&key_enable,'e'},
		{ "disable",no_argument,&key_disable,'d'},
		{     0,    0,    0,    0},
	};
	int c;

	while((c = getopt_long(argc, argv, ":l:", key_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				break;
			default:
				break;
		}
	}
	return 0;
}

int hrtimer_args_handle(int argc, char **argv)
{
	int c;
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	static int hrtimer_enable, hrtimer_disable;
	int time=0;
	int ret;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	static struct option hrtimer_opts[] = {
		{ "en",no_argument,&hrtimer_enable,1},
		{ "dis",no_argument,&hrtimer_disable,1},
		{ "time",required_argument,NULL,'t'},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", hrtimer_opts, NULL)) != -1)
	{
		switch(c) {
			case 't':
				time = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (hrtimer_enable) {
		kinject_data.enable = 1;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER,
				(void*)&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject start hrtimer failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (hrtimer_disable) {
		kinject_data.enable = 0;
		ret = ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER,
				(void*)&data, sizeof(struct ioctl_ksdata));
		if (ret) {
			printf("inject stop hrtimer failed\n");
			return -1;
		} else {
			return 0;
		}
	}

	if (time)
		printf("zz %s %d %d\n", __func__, __LINE__, time);

	return 0;
}

int unit_kinject(int argc, char** argv)
{
#if 0
	struct ioctl_ksdata data;
	struct kinject_ioctl kinject_data;

	kinject_data.enable = 0;

	data.data = &kinject_data;
	data.len = sizeof(struct kinject_ioctl);

	if (FLAGS_kinjectunit == "statickey") {
		kinject_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_TEST , (void*)&data, sizeof(struct ioctl_ksdata));
	}

	if (FLAGS_kinjectunit == "htitmer_s") {
		kinject_data.enable = 1;
		return ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER, (void*)&data, sizeof(struct ioctl_ksdata));
	}

	if (FLAGS_kinjectunit== "htitmer_d") {
		kinject_data.enable = 0;
		return ktools_ioctl::kioctl(IOCTL_INJECT, (int)IOCTL_USEKINJECT_HRTIMER, (void*)&data, sizeof(struct ioctl_ksdata));
	}
#else
#endif
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
	for (i = 0; i < sizeof(kinject_args) / sizeof(struct cmdargs); i++) {
		if (is_help) {
			printf("    %s: %s\n", kinject_args[i].name, kinject_args[i].help);
		} else {
			if(!strcmp(command, kinject_args[i].name)) {
				argc--;
				argv++;
				return kinject_args[i].func(argc, argv);
			}
		}
	}

	return 0;
}
