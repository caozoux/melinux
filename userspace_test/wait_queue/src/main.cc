#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <sys/time.h>
#include <sys/wait.h>

static void child_run(void)
{
	char buf[128];
	int ret;
	int fd;
	pid_t child_pid;

	fd = open("/dev/mewaitqueue",  O_RDWR);
	if (fd<0) {
		printf("zz %s %d open failed\n", __func__, __LINE__);
	}
	ret = write(fd, buf,64);
	printf("zz %s child write:%08x \n",__func__, (int)ret);
	close(fd);
}

int main()
{
	int fd;
	char buf[128];
	int ret;
	pid_t child_pid;

	fd = open("/dev/mewaitqueue",  O_RDWR);
	if (fd<0) {
		printf("zz %s %d open failed\n", __func__, __LINE__);
		return 0;
	}

	child_pid=fork();
	if (child_pid != 0) {
		child_run();
	} else {
		ret = read(fd, buf,64);
		printf("zz %s ret:%08x \n",__func__, (int)ret);
		close(fd);
		wait(NULL);
	}

	return 0;
}

