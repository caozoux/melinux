#include <fcntl.h> 
#include <sys/mman.h> 
#include <errno.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
 
static int fd;
static unsigned long read_cnt =0;
static unsigned long write_cnt =0;

#define RCU_SYNC_WRITE 		3
#define RCU_NO_SYNC_WRITE 	4

void* thread1()
{

	printf("pid is: %ld, tid is: %ld\n", getpid(),pthread_self);
	while(1) {
		ioctl(fd, RCU_NO_SYNC_WRITE, 0);
		write_cnt++;
	}
	//pthread_detach(pthread_self();//分离后仍可被等待
	return NULL;
}

void timer(int sig)
{
        if(SIGALRM == sig)
        {
                printf("read:%ld write:%ld\n", read_cnt, write_cnt);
                alarm(1);       //重新继续定时1s
        }

        return ;
}

static int delay(int cnt)
{

	unsigned int i, j;
	for (j = 0; j < cnt; ++j) {
		for (i = 0; i < 0xfffffff; ++i) {
		}
	}
}

static int rcu_read(void)
{

	pthread_t tid;
	struct sched_param param;  
	int tmp, err;
	int maxpri; 
	int cnt;
	void *ret;

	//signal(SIGALRM, timer); //注册安装信号
			
	//alarm(10);       //触发定时器

    fd = open("/dev/rcu_hood", O_RDWR);
    if(fd < 0){ 
       perror("Err: dev"); 
       return -1; 
    }   


  
	param.sched_priority = 1;  
	if(maxpri == -1) 
    { 
            perror("sched_get_priority_max() failed"); 
            exit(1); 
    }
 	maxpri = sched_get_priority_max(SCHED_FIFO);
	tmp = sched_setscheduler( 0, SCHED_FIFO, &param);  
	if (tmp == -1 ) {
            perror("sched_get_priority_max() failed"); 
            exit(1); 
	}

#if 1
	//pthread_create(&tid, NULL, thread1, NULL);

	if (err) {
       perror("Err: thread"); 
       return -1; 
	}
#endif

	while(1) {
		ioctl(fd, 1, 0);
		//printf("zz %s %d \n", __func__, __LINE__);
		//kdelay(1);
		//printf("zz %s %d \n", __func__, __LINE__);
		ioctl(fd, 2, 0);
		read_cnt++;
	}

	pthread_join(tid, &ret);
	close(fd);

	return 0;
}


int main(int argc, char *argv[])
{
	rcu_read();
	return 0;
}
