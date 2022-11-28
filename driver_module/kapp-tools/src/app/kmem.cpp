#include <iostream>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ksioctl/kmem_ioctl.h>

#include "kapp_unit.h"

int kmemdump_args_handle(int argc, char **argv);

struct cmdargs kmem_args[] = {
	{"--dump", kmemdump_args_handle,
		"\n"
		"     --water dump water\n"
		"     --memcss  scan memory cgroup \n"
	},
};

int kmemdump_args_handle(int argc, char **argv)
{
	int c;
	struct kmem_ioctl kmem_data;
	struct kmem_dump  dump_data;

	static int dump_water,dump_memcss;
	int ret;
	char *msg = NULL;

	kmem_data.enable = 0;

	static struct option slub_opts[] = {
		{ "water",no_argument,&dump_water,1},
		{ "memcss",no_argument,&dump_memcss,1},
		{     0,    0,    0,    0},
	};

	while((c = getopt_long(argc, argv, "", slub_opts, NULL)) != -1)
	{
		switch(c) {
			case 'm':
				msg = optarg;
				break;
			default:
				break;
		}
	}

	if (dump_water) {

		kmem_data.data = &dump_data;
		kmem_data.len = sizeof(struct kmem_dump);
		printf("zz %s data:%lx len:%lx \n",__func__, (unsigned long)kmem_data.data, (unsigned long)kmem_data.len);

		dump_data.dumpcmd = IOCTL_USEKMEM_DUMP_MEMORYWARTER;
		ktools_ioctl::kioctl(IOCTL_KMEM, (int)IOCTL_USEKMEM_DUMP,
				(void*)&kmem_data, sizeof(struct kmem_ioctl));
	}

	if (dump_memcss) {
		kmem_data.subcmd = IOCTL_USEKMEM_DUMP;

		kmem_data.data = &dump_data;
		kmem_data.len = sizeof(struct kmem_dump);

		dump_data.dumpcmd = IOCTL_USEKMEM_DUMP_EACH_CSS;
		ktools_ioctl::kioctl(IOCTL_KMEM, (int)IOCTL_USEKMEM_DUMP,
				(void*)&kmem_data, sizeof(struct kmem_ioctl));
	}

	return 0;
}

int unit_kmem(int argc, char** argv)
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
	for (i = 0; i < sizeof(kmem_args) / sizeof(struct cmdargs); i++) {
		if (is_help) {
			printf("    %s: %s\n", kmem_args[i].name, kmem_args[i].help);
		} else {
			if(!strcmp(command, kmem_args[i].name)) {
				argc--;
				argv++;
				return kmem_args[i].func(argc, argv);
			}
		}
	}

	return 0;
}
