#include <iostream>
#include <getopt.h>
#include <ostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include <ksioctl/kprobe_ioctl.h>
#include "kapp_unit.h"

#define BUF_SIZE (MAX_SAVE * sizeof(ksys_callchain))

static int kprobe_args_handle(int argc, char **argv);

struct cmdargs kprobe_args[] = {
	{"--dump",kprobe_args_handle, 
		"\n"
		"     --dis disable \n"
		"     --en enable\n"
		"     -f function name\n"
		"     -l delay us\n"
		"     -t type: \n"
		"     	 0: dumpstack\n"
		"     	 1: delay\n"
		"     for_example: \n"
		"     kprobe  --dump -f \"blk_account_io_completion\" --en -d 20 -t 1 \n"

	},
};

static int kprobe_args_handle(int argc, char **argv)
{
	static int key_enable, key_disable, key_clean;
	unsigned long delay;
	int type = -1, ret;
	struct ioctl_ksdata data;
	struct kprobe_ioctl kprobe_data;
	static struct option kprobe_opts[] = {
		{ "en",no_argument,&key_enable,1},
		{ "dis",no_argument,&key_disable,1},
		{ "clean",no_argument,&key_clean,1},
		{ "func",required_argument,NULL,'f'},
		{ "delay",required_argument,NULL,'d'},
		{ "type",required_argument,NULL,'t'},
		{     0,    0,    0,    0},
	};
	int c;

	data.data = &kprobe_data;
	data.len = sizeof(struct kprobe_ioctl);
	kprobe_data.enable = -1;
	kprobe_data.delay = 0;

	while((c = getopt_long(argc, argv, "f:d:t:", kprobe_opts, NULL)) != -1)
	{
		switch(c) {
			case 'f':
				strcpy(kprobe_data.name, optarg);
				break;
			case 'd':
				delay= atoi(optarg);
				kprobe_data.delay = delay;
				break;
			case 't':
				type = atoi(optarg);
				break;
			default:
				break;
		}
	}


	if (key_enable == 1) {
		kprobe_data.enable=1;
		switch (type) {
			case 0:
				ret = ktools_ioctl::kioctl(IOCTL_KPROBE, (int)IOCTL_KRPOBE_FUNC_DUMP,
						&data, sizeof(struct ioctl_ksdata));
				break;
			case 1:
				ret = ktools_ioctl::kioctl(IOCTL_KPROBE, (int)IOCTL_KPROBE_SUB_DELAY,
						&data, sizeof(struct ioctl_ksdata));
				break;
			default:
				break;
		}
	} else if (key_enable == 0) {
		kprobe_data.enable=0;
		switch (type) {
			case 0:
				ret = ktools_ioctl::kioctl(IOCTL_KPROBE, (int)IOCTL_KRPOBE_FUNC_DUMP,
						&data, sizeof(struct ioctl_ksdata));
				break;
			case 1:
				ret = ktools_ioctl::kioctl(IOCTL_KPROBE, (int)IOCTL_KPROBE_SUB_DELAY,
						&data, sizeof(struct ioctl_ksdata));
				break;
			default:
				break;
		}
	}

	return ret;
}

int unit_kprobe(int argc, char** argv)
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
	for (i = 0; i < sizeof(kprobe_args) / sizeof(struct cmdargs); i++) {
		if (is_help) {
			printf("    %s: %s\n", kprobe_args[i].name, kprobe_args[i].help);
		} else {
			if(!strcmp(command, kprobe_args[i].name)) {
				argc--;
				argv++;
				return kprobe_args[i].func(argc, argv);
			}
		}
	}

	return 0;
}

