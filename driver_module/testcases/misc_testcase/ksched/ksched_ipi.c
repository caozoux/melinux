#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/timekeeper_internal.h>
#include <kernel/sched/sched.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "kschedlocal.h"
#include "debug_ctrl.h"
#include "medelay.h"

#define 	LOOP_NUM   	(100000)
static void smp_call_func_test(void *type)
{
	//int cur_cpu = smp_processor_id();
	//printk("zz %s cur_cpu:%lx \n",__func__, (unsigned long)cur_cpu);
}

int ksched_sched_ipi_call_test(void)
{
	int cpu, cur_cpu = smp_processor_id();
	u64 smp_cyces, test_loop=LOOP_NUM;
	u64 befor_ts, comple_ts;
	
	printk("zz %s cur_cpu:%lx \n",__func__, (unsigned long)cur_cpu);
	for_each_cpu(cpu, cpu_online_mask) {
		if (cpu != cur_cpu) {
			befor_ts = get_time_tick();
			while(test_loop-->0) {
				smp_call_function_single(cpu, smp_call_func_test, NULL, 1);
			}
			comple_ts = get_time_tick();
			printk("zz %s comple_ts:%lx befor_ts:%lx \n",__func__, (unsigned long)comple_ts, (unsigned long)befor_ts);
			printk("ipi cycles:%lld\n", (comple_ts - befor_ts)%LOOP_NUM);
			break;
		}
	}
	return 0;
}
