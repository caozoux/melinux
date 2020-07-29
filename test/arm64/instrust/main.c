#include <semaphore.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>

// nop
#define ARM64_ASM_NOP 		(0xd503201f) 
//dmb oshst
#define ARM64_ASM_DMB 		(0xd50332bf)
//dmb oshst
#define ARM64_ASM_DMB 		(0xd50332bf)
//dsb sy
#define ARM64_ASM_DSB_SY 	(0xd5033f9f)
//dsb ld 
#define ARM64_ASM_DSB_LD 	(0xd5033d9f)
//dsb st
#define ARM64_ASM_DSB_ST 	(0xd5033e9f)

//casal
#define ARM64_ASM_LOAD_CASAL    (0xc8e4fc62)
//ldxr
#define ARM64_ASM_LOAD_LDXR     (0xc85f7c64)

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

#define LOOP_VAL 0x20000000
typedef float run_func(void);

static unsigned long load_test1, load_test2;
float run_dmb_oshst(void)
{
	unsigned long long v_cnt = 0;
	unsigned long long time_cnt_old = 0, time_cnt_new = 0, time_loop;
	float time_val = 0;

	time_cnt_old = get_cnt();
	while(v_cnt++<LOOP_VAL);
	time_loop = get_cnt() - time_cnt_old;

	v_cnt = 0;

	time_cnt_old = get_cnt();

	while(v_cnt++<(LOOP_VAL)) {
		asm ("dmb oshst\n");
	}

	time_cnt_new = get_cnt() - time_cnt_old;
	time_val = (float)(time_cnt_new-time_loop)/(float)LOOP_VAL;

	return time_val;
}

float run_load_instr(void)
{
	unsigned long long v_cnt = 0;
	unsigned long long time_cnt_old = 0, time_cnt_new = 0, time_loop;
	float time_val = 0;

	load_test1 = 1;
	time_cnt_old = get_cnt();

	while(v_cnt++<LOOP_VAL){
		asm (
			
		     "ldr x3, =load_test1\n"
		     "mov x4,#1\n"
		     "mov x2,#2\n"
		     "str x4, [x3]\n"
		    :
		    :
		    : "memory", "x2", "x3","x4"
		);
		//asm ("":::"memory");
	}

	time_loop = get_cnt() - time_cnt_old;

	v_cnt = 0;

	time_cnt_old = get_cnt();

	while (v_cnt++<(LOOP_VAL)) {
		asm (
			
		     "ldr x3, =load_test1\n"
		     "mov x4,#1\n"
		     "mov x2,#2\n"
		     "str x4, [x3]\n"
		     "ldxr x4,[x3]\n"
		    :
		    :
		    : "memory", "x2", "x3","x4"
		);
	}

	time_cnt_new = get_cnt() - time_cnt_old;
	time_val = (float)(time_cnt_new-time_loop)/(float)LOOP_VAL;

	return time_val*10;
}

float run_ldar(void)
{
	unsigned long long v_cnt = 0;
	unsigned long long time_cnt_old = 0, time_cnt_new = 0, time_loop;
	float time_val = 0;

	time_cnt_old = get_cnt();

	while(v_cnt++<LOOP_VAL)

	time_loop = get_cnt() - time_cnt_old;

	v_cnt = 0;

	time_cnt_old = get_cnt();

	while(v_cnt++<(LOOP_VAL)) {
		asm ("dmb oshst\n");
	}

	time_cnt_new = get_cnt() - time_cnt_old;
	time_val = (float)(time_cnt_new-time_loop)/(float)LOOP_VAL;

	return time_val*10;
}

float run_signal_instruct(unsigned int inst, run_func *run_addr, int off)
{
	volatile unsigned int * instruct_addr;
	float time_val;
	int ret;
    	int pagesize = 4096;
    	uintptr_t addr = (((uintptr_t)run_addr) / pagesize) * pagesize;
	int len = 256;
	unsigned int instruct = inst;
	float time_l ;
	
	instruct_addr = (unsigned int*) ((char*) run_addr + off);

	if (mprotect((void*)addr, (uintptr_t)run_addr - addr + 256, PROT_WRITE|PROT_READ|PROT_EXEC) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}

    	memcpy((void*)instruct_addr, (void*)&instruct, 4);

	if (mprotect((void*)addr, (uintptr_t)run_addr - addr + len, PROT_READ|PROT_EXEC) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	time_l = run_addr();
	//printf("time_val:%f\n", time_l);
	return time_l;
}

int main()
{
	float time;

	time = run_signal_instruct(ARM64_ASM_NOP, run_ldar, 96);
	printf("ARM64_ASM_NOP:%f \n", time);

	time = run_signal_instruct(ARM64_ASM_DMB, run_ldar, 96);
	printf("ARM64_ASM_DMB:%f \n", time);

	time = run_signal_instruct(ARM64_ASM_DSB_SY, run_ldar, 96);
	printf("ARM64_ASM_DSB_SY:%f \n", time);

	time = run_signal_instruct(ARM64_ASM_DSB_LD, run_ldar, 96);
	printf("ARM64_ASM_DSB_LD:%f \n", time);

	time = run_signal_instruct(ARM64_ASM_DSB_ST, run_ldar, 96);
	printf("ARM64_ASM_DSB_ST:%f \n", time);

	time = run_signal_instruct(ARM64_ASM_LOAD_CASAL, run_load_instr, 144);
	printf("ARM64_ASM_LOAD_CASAL:%f \n", time, run_ldar);

	time = run_signal_instruct(ARM64_ASM_LOAD_LDXR, run_load_instr, 144);
	printf("ARM64_ASM_LOAD_LDXR:%f \n", time, run_ldar);


	return 0;
}

