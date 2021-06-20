#include<semaphore.h>
#include<stdio.h>
#include<memory.h>
#include<time.h>
#include<sys/mman.h>
#include<sys/time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

typedef unsigned long long cycles_t;

unsigned long *buf1, *buf2;

#define NULL (0)

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;

void *thread_buf1;
void *thread_buf2;
void *buf;

long thread_time1;
long thread_time2;

long thread_stop = 0;
long thread2_stop = 0;

unsigned long long data = 1UL<<32;
unsigned long cache_page_size = 1024;
//unsigned long cache_page_size = 1;
int thread_loop=200;


#define DEV_NAME "/dev/misc_template"

struct misc_data {
        unsigned long tbr0_el1;
        unsigned long cpu;
};

static void get_cpu_tlb(void)
{
        struct misc_data data;
        char ch;
        int fd;
        int ret;

        memset(&data, 0, sizeof(struct misc_data));

        fd = open(DEV_NAME, O_RDWR);
        if (fd<=0)
           printf("open %s failed\n", DEV_NAME);

        read(fd, &data, sizeof(struct misc_data));
        printf("tbr0_el1:%lx cpu:%d\n", data.tbr0_el1, data.cpu);
        close(fd);
}

static long unsigned g_off=0;
static long unsigned thread1_loop=0;
static long unsigned thread2_loop=0;
static long unsigned thread1_cnt=0;
void *thread2Func(void *param)
{

	while(thread_stop == 0) {
		if (buf1[((g_off++)*cache_page_size)] == 1)
		//if (buf1[0] == 1)
			break;
		if (g_off>=thread_loop)
			g_off = 0;
		thread2_loop++;
	}

    	return NULL;  // Never returns
};


void *thread1Func(void *param)
{
    //get_cpu_tlb();	
	while(thread_stop == 0) {
		//if (buf1[((thread1_cnt++)*cache_page_size)] == 1)
		if (buf1[0] == 1)
			break;
		if (thread1_cnt>=thread_loop)
			thread1_cnt = 0;
		thread1_loop++;
	}

    return NULL;  // Never returns
};

#define M128  (1024*1024*128)
int main()
{

	unsigned long cnt = 0, test_cnt=2000, i=0;
	unsigned long  time1, time2;
	unsigned long  max_cache_offset = 10;
	struct timespec ts1, ts2;
	int fd;
	int *temptr; 
    	pthread_t thread1, thread2;

#if 1
	buf1 = malloc(data);
	if (!buf1) {
		printf("malloc failed\n");
		return 0;
	}
	memset(buf1, 0, data);
#else
	fd = open("/media/huge/test", O_CREAT | O_RDWR);
	buf1 = mmap(0, data, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(!buf1){
		printf("malloc failed\n");
		return 0;
	}
#endif
	printf("start test\n");

	thread_buf1 = buf;
	thread_buf2 = buf;


	pthread_create(&thread1, NULL, thread1Func, NULL);
	pthread_create(&thread2, NULL, thread2Func, NULL);

	cnt = 0;

	sleep(10);
	thread_stop = 1;
	pthread_join(thread1,(void **)&temptr);
	pthread_join(thread2,(void **)&temptr);
	printf("thread1:%ld thread2:%ld\n", thread1_loop, thread2_loop);
}
