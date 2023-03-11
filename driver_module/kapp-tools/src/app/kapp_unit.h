#ifndef __KAPP_UNIT_H
#define __KAPP_UNIT_H__
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ksysd_ioctl.h>
#include <ksysdata.h>
#include <list>

#define MODULE_ENTRY(name) \
	if (FLAGS_##name) \
		return  unit_##name(argc, argv)
		

#define MODULE_ENTRY_ARGS_LIST() \
	DEFINE_bool(kinject, false, "kinject");

#define MODULE_ENTRY_LIST() \
	MODULE_ENTRY(kinject); 

#define DEV_NAME "/dev/ksysd"

typedef int (*cmd_func)(int argc, char **argv);
struct kapp_func {
	const char* name;
	cmd_func func;
};

struct cmdargs {
	const char* name;
	cmd_func func;
	const char* help;
};


typedef struct kapp_func struct_func;

class ktools_ioctl {
	static int mFd;
public:
	static int getFd(void)
	{
		if (ktools_ioctl::mFd <= 0)
			ktools_ioctl::mFd = open(DEV_NAME, O_RDWR);

		if (ktools_ioctl::mFd <= 0)
			return -1;
		return ktools_ioctl::mFd;
	}

	static int kioctl(ioctl_cmdtype cmd, int subcmd, struct ioctl_ksdata *data, int len)
	{
		int fd = ktools_ioctl::getFd();
		//struct ioctl_ksdata iodata;
		//struct ioctl_ksdata *pdata = data;
		if (fd <= 0) {
			fprintf(stderr, "open %s failed\n", DEV_NAME);
			return -1;
		}

		data->cmd= cmd;
		data->subcmd= subcmd;
		data->len = len;

		return ioctl(fd, sizeof(struct ioctl_ksdata), (void*)data);
	}
};

typedef int (*unit_fp)(int argc, char **argv);
int unit_kinject(int argc, char** argv);
int unit_kprobe(int argc, char** argv);
int unit_ktrace(int argc, char** argv);
int unit_kmem(int argc, char** argv);
int unit_kblock(int argc, char** argv);
int unit_ksched(int argc, char** argv);
int unit_krunlog(int argc, char** argv);

#endif /* ifndef __KAPP_UNIT_H */
