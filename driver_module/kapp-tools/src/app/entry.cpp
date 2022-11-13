#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "kapp_unit.h"

using namespace std;
 
struct unit_func {
    //const char* name;
    const char *name;
    unit_fp func;
};

#define UNIT_ITEM(name) {"##name", unit_##name}

static struct unit_func all_funcs[] = {
	//{"help", usage_help},
    {"inject", unit_kinject},
//    {"kprobe", unit_kprobe},
//    {"ktrace", unit_ktrace},
};

int ktools_ioctl::mFd = 0;

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

	printf("zz %s command:%s \n",__func__, command);
	for (i = 0; i < sizeof(all_funcs) / sizeof(struct unit_func); i++) {
		if(!strcmp(command, all_funcs[i].name)) {
			argc--;
			argv++;
			return all_funcs[i].func(argc, argv);
		}
	}

	return 0;
}

