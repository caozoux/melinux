
#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <sched.h>
#include <cpuid.h>
#include <linux/capability.h>
#include <errno.h>
#include "msr-index.h"

int get_msr(int cpu, off_t offset, unsigned long long *msr)
{
    ssize_t retval;
    char pathname[32];
    int fd;

    sprintf(pathname, "/dev/cpu/%d/msr", cpu);
    fd = open(pathname, O_RDONLY);
    if (fd < 0)
        err(-1, "%s open failed, try chown or chmod +r /dev/cpu/*/msr, or run as root", pathname);

    retval = pread(fd, msr, sizeof *msr, offset);
    close(fd);

    if (retval != sizeof *msr)
        err(-1, "%s offset 0x%llx read failed", pathname, (unsigned long long)offset);

    return 0;
}

int main(int argc, char *argv[])
{
	unsigned long long msr_val;
	int i;
	printf("zz %s MSR_SMI_COUNT:%08x \n",__func__, (int)MSR_SMI_COUNT);
	for (i=0; i<23;i++) {
		get_msr(1, MSR_SMI_COUNT, &msr_val);
		printf("zz %s cpu%d MSR_SMI_COUNT:%lx \n",__func__, i, msr_val);
	}
	
	return 0;
}
