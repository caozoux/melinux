#include <iostream>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ksioctl/kinject_ioctl.h>

#include "kapp_unit.h"

static int dump_args_handle(int argc, char **argv);

struct cmdargs kblock_args[] = {
	{"--dump",dump_args_handle, 
		"\n"
		"     -m mode\n"
		"     -d disable \n"
		"     -e enable\n"
	},
};

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

int unit_kblock(int argc, char** argv)
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
	for (i = 0; i < sizeof(kblock_args) / sizeof(struct cmdargs); i++) {
		if (is_help) {
			printf("    %s: %s\n", kblock_args[i].name, kblock_args[i].help);
		} else {
			if(!strcmp(command, kblock_args[i].name)) {
				argc--;
				argv++;
				return kblock_args[i].func(argc, argv);
			}
		}
	}

	return 0;
}

