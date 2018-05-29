#include<stdlib.h>  
#include<stdio.h>  
#include<sys/types.h>  
#include<sys/sysinfo.h>  
#include<unistd.h>  
   
#define __USE_GNU  
#include<sched.h>  
#include<ctype.h>  
#include<string.h>  
#include<pthread.h>
      #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
static unsigned long rdpmc(unsigned int counter)
{
   unsigned int low, high;
 
   asm volatile("rdpmc" : "=a" (low), "=d" (high) : "c" (counter));
 
   return low | ((unsigned long)high) << 32;
}

static unsigned long rdmsr(unsigned int msr)
{
   unsigned int low, high;

   asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
   return low | ((unsigned long)high) << 32;
}

// rdpmc_instructions uses a "fixed-function" performance counter to return the count of retired instructions on
//       the current core in the low-order 48 bits of an unsigned 64-bit integer.
unsigned long rdpmc_instructions()
{
   unsigned a, d, c;
 
   c = (1<<30);
   __asm__ volatile("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
 
   return ((unsigned long)a) | (((unsigned long)d) << 32);;
}
 
// rdpmc_actual_cycles uses a "fixed-function" performance counter to return the count of actual CPU core cycles
//       executed by the current core.  Core cycles are not accumulated while the processor is in the "HALT" state,
//       which is used when the operating system has no task(s) to run on a processor core.
unsigned long rdpmc_actual_cycles()
{
   unsigned a, d, c;
 
   c = (1<<30)+1;
   __asm__ volatile("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
 
   return ((unsigned long)a) | (((unsigned long)d) << 32);;
}
 
// rdpmc_reference_cycles uses a "fixed-function" performance counter to return the count of "reference" (or "nominal")
//       CPU core cycles executed by the current core.  This counts at the same rate as the TSC, but does not count
//       when the core is in the "HALT" state.  If a timed section of code shows a larger change in TSC than in
//       rdpmc_reference_cycles, the processor probably spent some time in a HALT state.
unsigned long rdpmc_reference_cycles()
{
   unsigned a, d, c;
 
   c = (1<<30)+2;
   __asm__ volatile("rdpmc" : "=a" (a), "=d" (d) : "c" (c));
 
   return ((unsigned long)a) | (((unsigned long)d) << 32);;
}

#define MSR_IA32_MPERF          0x000000e7
#define MSR_CORE_PERF_FIXED_CTR0    0x00000309
#define MSR_CORE_PERF_FIXED_CTR1    0x0000030a
#define MSR_CORE_PERF_FIXED_CTR2    0x0000030b
#define MSR_CORE_PERF_FIXED_CTR_CTRL    0x0000038d
#define MSR_CORE_PERF_GLOBAL_STATUS 0x0000038e
#define MSR_CORE_PERF_GLOBAL_CTRL   0x0000038f
#define MSR_CORE_PERF_GLOBAL_OVF_CTRL   0x00000390

int cpu_migrate(int pid)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);    //置空
	CPU_SET(1, &mask);
    if (sched_setaffinity(pid, sizeof(mask), &mask) == -1)
        return -1;
    else
        return 0;
}

void cpuid_read(unsigned int request)
{
	unsigned int cpu_vendor[3];
	asm("cpuid"
		: "=a" (request),
		"=b" (cpu_vendor[0]),
		"=d" (cpu_vendor[1]),
		"=c" (cpu_vendor[2])
		: "a" (0));

}
int main(int argc, char *argv[])
{
	unsigned long val=0;	
	char pathname[32];

	unsigned long long msrlist[]={
		  MSR_CORE_PERF_FIXED_CTR0
		, MSR_CORE_PERF_FIXED_CTR1
		, MSR_CORE_PERF_FIXED_CTR2
		, MSR_CORE_PERF_FIXED_CTR_CTRL
		, MSR_CORE_PERF_GLOBAL_STATUS
		, MSR_CORE_PERF_GLOBAL_CTRL
		, MSR_CORE_PERF_GLOBAL_OVF_CTRL
	};

	unsigned long long msr=MSR_CORE_PERF_FIXED_CTR0;
	unsigned long long msr_val;
	int fd, retval = 0;
	pid_t pid;
	int i=0;


	cpuid_read(10);
	pid=fork();
	if (pid == 0) {
		sprintf(pathname, "/dev/cpu/%d/msr", 0);
		fd = open(pathname, O_RDONLY);

		if (!fd) {
			printf("error open failed\n");
			return 0;	
		}

		for (i = 0; i < sizeof(msrlist)/8; ++i) {
			msr = msrlist[i];
			retval = pread(fd, &msr_val, sizeof(msr_val),msr);
			if (msr == MSR_CORE_PERF_GLOBAL_CTRL) {
				retval = pwrite(fd, &msr_val, sizeof(msr_val),msr);
				printf("zz %s %d write retval:%lx \n",__func__, __LINE__, (unsigned long)retval);
			}
			val=rdpmc_actual_cycles();
			if (retval != sizeof &msr_val) {
				printf("error read failed\n");
				goto out;
			}
			printf("zz %s %d msr:0x%lx msr_val:0x%lx val:%lx %lx\n",__func__, __LINE__, (unsigned long)msr, (unsigned long)msr_val
					, rdpmc_actual_cycles()
					, rdpmc_reference_cycles());
		}
	}  else {
		cpu_migrate(pid);
		printf("zz %s %dpid:%lx \n",__func__, __LINE__, (unsigned long)pid);
		wait(&pid);
	}
out:
	close(fd);
	return 0;
}
