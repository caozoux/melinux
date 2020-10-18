#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include "template_iocmd.h"

static int s_fd;
static int exit_code = 0;
void* thread1()
{
	int ret;
	struct ioctl_data data;
	data.type = IOCTL_USERCU;

	while(1) {
		data.args[0] = 12;

		ret = ioctl(s_fd, 1, &data);
		if (s_fd <= 0) {
			printf("ioctl failed\n");
			return NULL;
		}
		printf("%d\n", data.args[0]);
		if (data.args[0] != 12) {
			printf("find sync error val:%d get:%d\n", 8, data.args[0]);
			exit_code = 1;
			break;
		}

		if(exit_code)
			break;
	}

	return NULL;
}

void* thread2()
{
	int ret;
	struct ioctl_data data;
	data.type = IOCTL_USERCU;

	while(1) {
		data.args[0] = 8;

		ret = ioctl(s_fd, 1, &data);
		if (s_fd <= 0) {
			printf("ioctl failed\n");
			return NULL;
		}
		if (data.args[0] != 8) {
			printf("find sync error val:%d get:%d\n", 8, data.args[0]);
			exit_code = 1;
			break;
		}
		if(exit_code)
			break;
	}

	return NULL;
}

int rcu_test(int fd)
{

	pthread_t tid1;
	pthread_t tid2;
	struct ioctl_data data;
	int ret;

	data.type = IOCTL_USERCU;
	data.cmdcode = IOCTL_USERCU_READTEST_START;
	s_fd = fd;
	printf("rcu readlock test start\n");
	ret = ioctl(s_fd, 1, &data);
	sleep(10);
	data.cmdcode = IOCTL_USERCU_READTEST_END;
	ret = ioctl(s_fd, 1, &data);
	printf("rcu readlock test stop\n");
#if 0
	pthread_create(&tid1, NULL, thread1, NULL);
	//pthread_create(&tid2, NULL, thread2, NULL);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
#endif

}

extern int misc_fd;

static void help(void)
{
	printf("rcu --rcu_read_sync  test the rcu read for rcu sync");
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"rcu_read_sync",     no_argument, 0,  0 },
	{0,0,0,0}};

int rcu_usage(int argc, char **argv)
{
	int c;
	struct ioctl_data data;

	while (1) {

		int option_index = -1;
		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;

		switch (option_index) {
			case '0':
				help();
				break;
			case '1':
				rcu_test(misc_fd);
				return;
			default:
				printf("hardlock operation not support\n");
				return -1;
		}
	}

	
	return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
}
