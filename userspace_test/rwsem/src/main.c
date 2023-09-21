#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<getopt.h>
#include<unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include<semaphore.h>
#include <signal.h>
#include<linux/sched.h>

#define MAX_TRHEAD  (128)

unsigned long thread_cnt;

struct thread_data {
	unsigned long id;
	unsigned long cnt;
};

static int ioctl_fd;
static int exit_code;

pthread_t read_thread_array[MAX_TRHEAD];
pthread_t write_thread_array[MAX_TRHEAD];
struct thread_data read_thread_data_array[MAX_TRHEAD];
struct thread_data write_thread_data_array[MAX_TRHEAD];

void *write_threadFunc(void *param)
{
	struct thread_data *data = (struct thread_data *)param;
	printf("zz %s %d \n", __func__, __LINE__);
	while(exit_code) {
		ioctl(ioctl_fd, 1, 3); //percpu_down_write
		data->cnt +=1;
		ioctl(ioctl_fd, 1, 4);//percpu_up_write
	}
	printf("zz %s %d \n", __func__, __LINE__);
}
void *read_threadFunc(void *param)
{
	struct thread_data *data = (struct thread_data *)param;
	while(exit_code) {
		ioctl(ioctl_fd, 1, 1); //percpu_down_read
		data->cnt +=1;
		ioctl(ioctl_fd, 1, 2);//percpu_up_read
	}
}

void help(void)
{
	printf("--threads   run thread numbers\n");
	printf("--run_cnt   run runtime count\n");
	printf("--loop_cnt  run loop runtime count\n");
	printf("--sleep_cnt sleep run count\n");
	printf("--sched_policy fifo/deadline/cpuidle/qos\n");
	printf("--sched_priority  sched priority vaule -20 - 120\n");
}

void sig_alarm_handler(int data)
{
	printf("zz %s %d \n", __func__, __LINE__);
	exit_code = 0;
}

int main(int argc, char *argv[])
{
	int i;
	unsigned long loop = 1UL<<20;
	int ret;
	int remaing;
	int read_cnt = 0;
	int write_cnt = 0;
	exit_code = 1;

	signal(SIGALRM, sig_alarm_handler);
	remaing = alarm(3);


    ioctl_fd  = open("/dev//misc_template", O_RDWR);
    if(ioctl_fd < 0){ 
       perror("Err: dev"); 
       return -1; 
    }

	for (i = 0; i < 2; ++i) {
		read_thread_data_array[i].id = i;
		read_thread_data_array[i].cnt= 0;
		pthread_create(&read_thread_array[i], NULL, read_threadFunc, &read_thread_data_array[i]);
	}

	for (i = 0; i < 1; ++i) {
		write_thread_data_array[i].id = i;
		write_thread_data_array[i].cnt = 0;
		pthread_create(&write_thread_array[i], NULL, write_threadFunc, &write_thread_data_array[i]);
	}

	for (i = 0; i < 2; ++i) {
		pthread_join(read_thread_array[i], NULL);
	}
	for (i = 0; i < 1; ++i) {
		pthread_join(write_thread_array[i], NULL);
	}

	for (i = 0; i < 2; ++i) {
			read_cnt += read_thread_data_array[i].cnt;
	}

	for (i = 0; i < 1; ++i) {
			write_cnt += write_thread_data_array[i].cnt;
	}

	printf("zz %s read_cnt:%ld write_cnt:%ld \n",__func__, (unsigned long)read_cnt, (unsigned long)write_cnt);
	return 0;
}

