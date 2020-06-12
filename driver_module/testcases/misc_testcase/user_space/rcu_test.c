#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
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

int ruc_test(int fd)
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
