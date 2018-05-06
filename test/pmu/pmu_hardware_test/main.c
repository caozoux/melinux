#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#define __USE_GNU  
#include<sched.h>  
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/types.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#define barrier() asm volatile ("" ::: "memory")

unsigned long perf_event_open_cloexec_flag(void)
{
	return 0;
}

static int cpu_migrate(int pid, int cpu)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);    //置空
	CPU_SET(cpu, &mask);
    if (sched_setaffinity(pid, sizeof(mask), &mask) == -1)
        return -1;
    else
        return 0;
}

static inline int
sys_perf_event_open(struct perf_event_attr *attr,
              pid_t pid, int cpu, int group_fd,
              unsigned long flags)
{
    int fd;

    fd = syscall(__NR_perf_event_open, attr, pid, cpu,
             group_fd, flags);

    return fd;
}

static unsigned long rdtsc(void)
{
    unsigned int low, high;

    asm volatile("rdtsc" : "=a" (low), "=d" (high));

    return low | ((unsigned long)high) << 32;
}

static unsigned long rdpmc(unsigned int counter)
{
   unsigned int low, high;
 
   asm volatile("rdpmc" : "=a" (low), "=d" (high) : "c" (counter));
 
   return low | ((unsigned long)high) << 32;
}
 
/*
 PERF_COUNT_HW_CPU_CYCLES        = 0,
 PERF_COUNT_HW_INSTRUCTIONS      = 1,
 PERF_COUNT_HW_CACHE_REFERENCES      = 2,
 PERF_COUNT_HW_CACHE_MISSES      = 3,
 PERF_COUNT_HW_BRANCH_INSTRUCTIONS   = 4,
 PERF_COUNT_HW_BRANCH_MISSES     = 5,
 PERF_COUNT_HW_BUS_CYCLES        = 6,
 PERF_COUNT_HW_STALLED_CYCLES_FRONTEND   = 7,
 PERF_COUNT_HW_STALLED_CYCLES_BACKEND    = 8,
 PERF_COUNT_HW_REF_CPU_CYCLES        = 9,
 PERF_COUNT_HW_MAX, 
*/

static int perf_event_start(int hw_type)
{
	int fd;
	struct perf_event_attr attr = {
		.type = PERF_TYPE_HARDWARE,
		.config = hw_type,
		.exclude_kernel = 1,
	};
	fd = sys_perf_event_open(&attr, 0, -1, -1,
				 perf_event_open_cloexec_flag());
	if (fd < 0) {
		printf("Error: sys_perf_event_open() syscall returned "
		       "with %d failed\n", fd);
	}

	return fd;

}


unsigned long read_pevent_counter_with_map(struct perf_event_mmap_page *pc)
{
	unsigned int seq, idx, time_mult = 0, time_shift = 0;;
	unsigned long enabled, running, count, cyc = 0, time_offset = 0;
	do {
		seq = pc->lock;
		enabled = pc->time_enabled;
		running = pc->time_running;
		if (enabled != running) {
			cyc = rdtsc();
			time_mult = pc->time_mult;
			time_shift = pc->time_shift;
			time_offset = pc->time_offset;
		}

		idx = pc->index;
		count = pc->offset;
		if (idx)
			count += rdpmc(idx - 1);

		barrier();
	} while (seq != pc->lock);

	printf("zz %s %d pc->index:%lx \n",__func__, __LINE__, (unsigned long)pc->index);
	return count;
}

/* use mmap to read pmu counter */
static int perf_mmap_inist(int hw_type)
{
	int fd;
	void *addr;
	struct perf_event_mmap_page *pc;
	unsigned long count;
	unsigned long delay_count = 0x1000000*2;

	fd = perf_event_start(hw_type);
	if (fd<0)
		return  -1;
	
	addr = mmap(NULL, 0x1000, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == (void *)(-1)) {
		printf("Error: mmap() syscall returned with \n");
		goto out_close;
	}
	pc = addr;

	count = read_pevent_counter_with_map(pc);
	printf("zz %s %d count:%d \n",__func__, __LINE__, (unsigned long)count);

	while(delay_count-->0);
	count = read_pevent_counter_with_map(pc);
	printf("zz %s %d count:%d \n",__func__, __LINE__, (unsigned long)count);

	munmap((void*)pc, 0x1000);
	close(fd);

	return 0;

out_close:
	close(fd);

	return -1;
}

static int perf_counter_read_inist(int hw_type)
{
	int fd;
	unsigned long values[4];
	int size;
	unsigned long delay_count = 0x1000000*2;

	fd = perf_event_start(hw_type);
	if (fd<0) {
		printf("open event failed\n");
		return  -1;
	}
		
	while(delay_count-->0){
		size=read(fd, values, 64*4);
	}
	close(fd);
}



static int bind_cpu(int cpu)
{
    pid_t pid = getpid();
	cpu_migrate(pid, cpu);
	return 0;
}

int main(int argc, char *argv[])
{
	bind_cpu(0);
	perf_counter_read_inist(PERF_COUNT_HW_INSTRUCTIONS);
	return 0;
}
