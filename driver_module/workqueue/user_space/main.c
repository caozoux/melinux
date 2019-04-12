#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define DEV_NAME "/dev/misc_template"

struct ping_data {
	short unsigned int  s_send;
	short unsigned int  s_recv;
};

int main(int argc, char *argv[])
{
	struct ping_data data;
	int fd;	
	int ret;

	fd = open(DEV_NAME, O_RDWR);
	if (fd <= 0) {
		printf("%s open failed", DEV_NAME);
		return 0;
	}

	ret = ioctl(fd, 1, &data);
	printf("zz %s ret:%08x data.s_send:%08x data.s_recv:%08x \n",__func__, (int)ret, (int)data.s_send, (int)data.s_recv);
	while(1) {
		sleep(1);
	}

	close(fd);
	return 0;
}
