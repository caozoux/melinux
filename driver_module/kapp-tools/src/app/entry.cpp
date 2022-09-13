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

#define __FILECMD__ "kunit"
#include "kapp_unit.h"

using namespace std;
//DEFINE_bool(isvip, false, "If Is VIP");
//DEFINE_string(kinject, "", "connect ip");
//DEFINE_bool(kinject, false, "kinject");
//DECLARE_int32(port);
//DEFINE_int32(port, 80, "listen port");
DEFINE_string(unit, "", "\n"
"inject\n"
"kprobe\n"
"ktrace\n"
""
);
DEFINE_bool(init, false, "initail module");
 
MODULE_ENTRY_ARGS_LIST();
struct unit_func {
    //const char* name;
    string name;
    unit_fp func;
};

#define UNIT_ITEM(name) {"##name", unit_##name}

static struct unit_func all_funcs[] = {
    //{"help", usage_help},
    {"inject", unit_kinject},
    {"kprobe", unit_kprobe},
    {"ktrace", unit_ktrace},
};

int ktools_ioctl::mFd = 0;
static int test_code = 6;

string globle_unit;

static unsigned long get_kallsyms_lookup_address(void)
{
#define BUF_SIZE (0x1000000)
    char *buf;
    int fd;
    unsigned long ret = 0;
    int i, len = 0;
    char kallsyms_str[17]={0};
    char *str;
	
    str = buf = (char*)malloc(BUF_SIZE);
    if (!buf)
        return ret;

    fd=open("/proc/kallsyms", O_RDONLY);
    if (fd<0) {
        free(buf);
        return ret;
    }

    do {
        len = read(fd, str, BUF_SIZE);
        str += len;
    } while(len > 0);

    str=strstr(buf, " T kallsyms_lookup_name");
    if (!str)
        goto out;

    str -= 16;
    strncpy(kallsyms_str, str, 16);
	kallsyms_str[16] = 0;

    ret = strtoull(kallsyms_str, NULL, 16);

out:
    free(buf);
    close(fd);
    return ret;
}

int env_init(void)
{
	struct ioctl_ksdata data;
	unsigned long address;
	int ret;
	int check;

	ret = ktools_ioctl::kioctl(IOCTL_INIT, IOCTL_USEINIT_CHECK, NULL, 0);
	if (ret == 0)
		return 0;

	//ioctl_data_init(&data);
	data.cmd = IOCTL_INIT;
	address = get_kallsyms_lookup_address();
	if (!address)
		return 0;
	
	data.data = (void*)address;
	data.len = 0;
	return ktools_ioctl::kioctl(IOCTL_INIT, IOCTL_USEINIT_INIT, (void*)address, 8);
}

int main(int argc, char** argv)
{
	unsigned long i;
	char *operation;
	string command;

	//google::SetUsageMessage("./gflags");
	if (argc < 2) {
		google::ShowUsageWithFlags("");
		return 0;
	}

	if (!strstr(argv[1], "help"))  {
		argc--;
		command.append(argv[1]);
		argv++;
	} else {
		google::ShowUsageWithFlags("");
		return 0;
	}

	google::ParseCommandLineFlags(&argc, &argv, true);
	google::ShutDownCommandLineFlags();

	if (FLAGS_unit.length())
		globle_unit = FLAGS_unit;

	env_init();

	if (FLAGS_init)
		return env_init();

	for (i = 0; i < sizeof(all_funcs) / sizeof(struct unit_func); i++) {
		if(command == all_funcs[i].name) {
			return all_funcs[i].func(argc, argv);
		}
	}

	return 0;
}

