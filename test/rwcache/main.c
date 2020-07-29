#include<semaphore.h>
#include<malloc.h>
#include<string.h>
#include<stdio.h>
#include<pthread.h>
#include <unistd.h>

#define CPU_FREQ (2600000000)

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;

struct thread_cnt {
	unsigned long read_cnt;
	//unsigned long var1[15];
};

int thread1_stop = 0;
int thread2_stop = 0;
int X, Y;
int r1, r2;
unsigned long *buf_rw;
//unsigned long long read_cnt[14];
struct thread_cnt read_cnt[14];
unsigned long write_cnt;

struct seqlock {
	volatile unsigned long lock;	
};

struct seqlock slock;

void udelay(unsigned long var)
{
	var = var*((CPU_FREQ)/1000000);
	asm (   "mov x1, %0\n"
		"loop2: sub x1, x1, #1\n"
		"mov   x2, x1\n"
		"cmp   x1, #0x0\n"
		"b.ne  loop2\n"
		: "=r" (var)
	);
}

void mdelay(unsigned long var)
{
	var = var*((CPU_FREQ)/2000);
	asm (   "mov x1, %0\n"
		"loop1: sub x1, x1, #1\n"
		"mov   x2, x1\n"
		"cmp   x1, #0x0\n"
		"b.ne  loop1\n"
		: "=r" (var)
	);
}

void seqlock_lockw_init(struct seqlock *seqlock)
{
	seqlock->lock = 0;
}

void seqlock_lockw(struct seqlock *seqlock)
{
	seqlock->lock++;	
}

void seqlock_unlockw(struct seqlock *seqlock)
{
	seqlock->lock++;	
	//asm("sev");
}

unsigned long seqlock_lockr(struct seqlock *seqlock)
{
	int i=0;
	while((seqlock->lock & 1)) {
#if 0
		for(i=0;i<400;i++);
#else
		//asm("wfe");
		//asm("sev");
		//asm("nop");
		//udelay(1);
#endif
	}
	return seqlock->lock;
}

int seqlock_unlockr(struct seqlock *seqlock, unsigned long seq)
{
	return seqlock->lock != seq;	
}

void *writethread2Func(void *param)
{
	while(thread2_stop==0) {
		seqlock_lockw(&slock);
		mdelay(1);
		seqlock_unlockw(&slock);
		mdelay(1);
	}

    	return NULL;  // Never returns
};

void *thread1noshare(void *param)
{
	unsigned long seq;
	struct thread_cnt *data;

	data = (struct thread_cnt *) param;
	while(thread1_stop==0) {
	}
    	return NULL;  // Never returns
};
void *thread1Func(void *param)
{
	unsigned long seq;
	struct thread_cnt *data;

	data = (struct thread_cnt *) param;
	while(thread1_stop==0) {
#if 1
		do
		{
			seq=seqlock_lockr(&slock);
			asm ("dmb oshst");
			data->read_cnt++;	
		}while(seqlock_unlockr(&slock, seq));
#else
			//read_cnt++;	
			//if (read_cnt > 0x0e000000)
			//	printf("over\n");
#endif
	}
    	return NULL;  // Never returns
};

static inline unsigned long get_cnt(void)
{
	unsigned long timer_val;	
	asm (   " mrs %0, cntvct_el0"
		: "=r" (timer_val)
		:
		: "memory"
	);
	return timer_val;
}

int main()
{
    	pthread_t thread1;
    	pthread_t threadlist[14];
	int i, thread_create_nums=15;

	write_cnt = 0;

    	printf("start\n");
	memset(read_cnt, 0, sizeof(read_cnt));

	seqlock_lockw_init(&slock);
	buf_rw = malloc(1024);
  	memset(buf_rw,0,1024);
	// Initialize the semaphores
	sem_init(&beginSema1, 0, 0);
	sem_init(&beginSema2, 0, 0);
	sem_init(&endSema, 0, 0);

	buf_rw[0]++;
	buf_rw[124]++;
	// Spawn the threads
	pthread_create(&thread1, NULL, writethread2Func, NULL);

#if 1
	for(i=0;i<thread_create_nums;i++) {
		pthread_create(&threadlist[i], NULL, thread1Func, &read_cnt[i]);
	}
#endif

	sleep(7);
	thread1_stop=1;
	thread2_stop=1;
	printf("rec_core:%lld\n", read_cnt[0].read_cnt);

    return 0;  // Never returns
}

