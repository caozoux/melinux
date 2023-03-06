#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <uys/stat.h>

#include "kapp_unit.h"
#include "krunlog_ioctl.h"

using namespace std;
 
struct unit_func {
    //const char* name;
    const char *name;
    unit_fp func;
};

#define UNIT_ITEM(name) {"##name", unit_##name}

int unit_krunlog(int argc, char** argv);
static struct unit_func all_funcs[] = {
	//{"help", usage_help},
    {"inject", unit_kinject},
    {"kmem", unit_kmem},
    {"kblock", unit_kblock},
    {"ksched", unit_ksched},
    {"klog", unit_klog},
//    {"kprobe", unit_kprobe},
//    {"ktrace", unit_ktrace},
};

int ktools_ioctl::mFd = 0;

void unit_krunlog_help(void)
{

}

#define BUF_SIZE (1024*10)
int unit_krunlog(int argc, char** argv)
{
	int choice;
	struct ioctl_ksdata data;
	void dump_buf[BUF_SIZE];
	int ret;

	while (1)
	{
		static struct option long_options[] =
		{
			/* Use flags like so:
			{"verbose",	no_argument,	&verbose_flag, 'V'}*/
			/* Argument styles: no_argument, required_argument, optional_argument */
			{"version", no_argument,	0,	'v'},
			{"help",	no_argument,	0,	'h'},
			{"dump",	no_argument,	0,	'd'},
			{"clean",	no_argument,	0,	'c'},
			{0,0,0,0}
		};
	
		int option_index = 0;
	
		/* Argument parameters:
			no_argument: " "
			required_argument: ":"
			optional_argument: "::" */
	
		choice = getopt_long( argc, argv, "vhdc",
					long_options, &option_index);
	
		if (choice == -1)
			break;
	
		switch( choice )
		{
			case 'v':
				break;
	
			case 'h':
				break;
	
			case 'd':
				memset(dump_buf, 0, BUF_SIZE);
				ret = ktools_ioctl::kioctl(IOCTL_KRUNLOG, (int)IOCTL_USERUNLOG_DUMP, (void*)&data, sizeof(struct ioctl_ksdata_krunlog));
				if (!ret)
					printf("%s\n", dump_buf);
				break;
	
			case 'c':
				return ktools_ioctl::kioctl(IOCTL_KRUNLOG, (int)IOCTL_USERUNLOG_CLEAN, (void*)&data, 0);
				break;
	
			case '?':
				/* getopt_long will have already printed an error */
				break;
	
			default:
				/* Not sure how to get here... */
				return EXIT_FAILURE;
		}
	}
}

int main(int argc, char** argv)
{
	unsigned long i;
	char *operation;
	char *command;

	//google::SetUsageMessage("./gflags");
	if (argc < 2) {
		return 0;
	}

	if (strstr(argv[1], "help"))  {
		char *hargv[] = {"kapp","help"};
		for (i = 0; i < sizeof(all_funcs) / sizeof(struct unit_func); i++) {
			printf("%s\n", all_funcs[i].name);
			all_funcs[i].func(2, hargv);
		}
		return 0;
	} else {
	}
		
	command = argv[1];

	for (i = 0; i < sizeof(all_funcs) / sizeof(struct unit_func); i++) {
		if(!strcmp(command, all_funcs[i].name)) {
			argc--;
			argv++;
			return all_funcs[i].func(argc, argv);
		}
	}

	return 0;
}

