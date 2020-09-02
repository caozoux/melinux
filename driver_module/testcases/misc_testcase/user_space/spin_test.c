#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "common_head.h"

int spin_test(int fd, struct ioctl_data *data)
{
	int ret;
	ret = ioctl(fd, 1, data);
	return 0;
}

