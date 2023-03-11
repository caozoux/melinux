#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include <ksioctl/krunlog_ioctl.h>
#include "kapp_unit.h"

#define BUF_SIZE (1024*10)
int unit_krunlog(int argc, char** argv)
{
	int ret;
	struct ioctl_ksdata data;
	struct krunlog_ioctl krunlog_data;
	char dump_buf[BUF_SIZE];

	data.data = &krunlog_data;
	data.len = sizeof(struct krunlog_ioctl);
	krunlog_data.buf = dump_buf;
	krunlog_data.len = BUF_SIZE;

	if (argc != 2)
		return -1;

	if (strstr(argv[1], "help"))  {
		printf("     -d dump log\n");
		printf("     -c clean log\n");
		return 0;
	}

	if (!strcmp(argv[1], "-d")) {
		ret = ktools_ioctl::kioctl(IOCTL_KRUNLOG, (int)IOCTL_RUNLOG_DUMP, &data, sizeof(struct ioctl_ksdata));
		if (!ret)
			printf("%s\n", dump_buf);
		dump_buf[ret]  = 0;
	} else if (!strcmp(argv[1], "-c")) {
		ktools_ioctl::kioctl(IOCTL_KRUNLOG, (int)IOCTL_RUNLOG_CLEAN, &data, sizeof(struct ioctl_ksdata));
	}

	return 0;
}

