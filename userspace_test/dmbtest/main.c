#include<semaphore.h>
#include<malloc.h>
#include<string.h>
#include<stdio.h>
#include<pthread.h>
#include <unistd.h>

static unsigned long rec_cnt;

static unsigned long get_cnt(void)
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
#if 0
	//volatile unsigned long *p=&rec_cnt;
	unsigned long *p=rec_cnt;
	unsigned long old_time, new_time;

	rec_cnt = 0;

	old_time = get_cnt();
	while (rec_cnt<0x20000000) {
		//asm (   "dmb oshst\n"
		//asm (   "dmb ishst\n"
		//"dmb oshst\n"
		//"dmb oshld\n"
		//"dsb ld\n"
		//"dsb sy\n"
		asm (
		     "ldr x0, =rec_cnt\n"
		     "ldr x1, [x0]\n"
		     "dmb oshld\n"
		     "add x1, x1, #1\n"
		     "str x1, [x0]\n"
		    :
		    :
		    : "memory"	
		);
	}
	new_time = get_cnt();
	printf("time %ld\n", new_time - old_time);
#else
	while(1) {
		asm ("dmb oshst\n");
	}
#endif
	return 0;
}
