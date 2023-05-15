#include <iostream>
#include <ostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include <ksioctl/kprobe_ioctl.h>
#include "kapp_unit.h"
#include "lib/symbole.h"

#define BUF_SIZE (MAX_SAVE * sizeof(ksys_callchain))

static int args_handle(int argc, char **argv);

struct cmdargs kprobe_args[] = {
	{"--dump",dump_args_handle, 
		"\n"
		"     -m mode\n"
		"     -d disable \n"
		"     -e enable\n"
	},
};

static int args_handle(int argc, char **argv)
{
	static int key_enable, key_disable, key_clean;
	static struct option key_opts[] = {
		{ "enable",no_argument,&key_enable,'e'},
		{ "disable",no_argument,&key_disable,'d'},
		{ "clean",no_argument,&key_clean,'c'},
		{ "func",required_argument,NULL,'f'},
		{     0,    0,    0,    0},
	};
	int c;

	while((c = getopt_long(argc, argv, ":f:", key_opts, NULL)) != -1)
	{
		switch(c) {
			case 'f':
				break;
			default:
				break;
		}
	}

	return 0;
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

