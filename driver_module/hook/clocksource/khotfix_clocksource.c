#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/cpuset.h>
#include <linux/slab.h>
#include <linux/time64.h>
#include <linux/rwlock.h>
#include <linux/pid_namespace.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>
#include <linux/timekeeper_internal.h>
#include <linux/time.h>
#include <linux/tick.h>
#include "hotfix_util.h"
#include "kernel/time/tick-sched.h"


#define TSC_DIVISOR  8
struct mutex *orig_text_mutex;
struct timekeeper *orig_timekeeper;

static int hook_cpu = 2;
DEFINE_ORIG_FUNC(int, lapic_next_deadline, 2, unsigned long ,delta, struct clock_event_device, *evt);
TEXT_DECLARE()

int (*orig_idle_cpu)(int cpu);
int (*old_lapic_next_deadline)(unsigned long delta, struct clock_event_device  *evt);

void set_new_deadline(void *data)
{
	u64 tsc;
	tsc = rdtsc();
	wrmsrl(MSR_IA32_TSC_DEADLINE, tsc + (((u64) 1000000) * TSC_DIVISOR));
	trace_printk("zz %s %d wakeup cpu%d  timer task:%s\n", __func__, __LINE__, smp_processor_id(), current->comm);
}

void wakepu_cpu_tsc(int cpu)
{
	smp_call_function_single(cpu, set_new_deadline, NULL, 1);
}

int new_lapic_next_deadline(unsigned long delta, struct clock_event_device  *evt)
{
	if (smp_processor_id() == hook_cpu) {
		if (!orig_idle_cpu(hook_cpu))
			printk("cpu busye\n");
		else
			printk("cpu idle\n");

#if 0
		if (orig_idle_cpu(hook_cpu)) {
			printk("zz %s %d ignore cpu 1 timer task:%s\n", __func__, __LINE__,current->comm);
			trace_printk("zz %s %d ignore cpu 1 timer task:%s\n", __func__, __LINE__,current->comm);
			return 0;
		}
#else
			printk("zz %s %d ignore cpu %d timer task:%s\n", __func__, __LINE__, hook_cpu, current->comm);
			trace_printk("zz %s %d ignore cpu 1 timer task:%s\n", __func__, __LINE__,current->comm);
			return 0;
#endif
	}
	return old_lapic_next_deadline(delta, evt);
}

static int init_syms(void)
{
	TEXT_SYMS()
	LOOKUP_SYMS(lapic_next_deadline);
	LOOKUP_SYMS(idle_cpu);
	return 0;
}

static int __init hook_func_entry(void)
{
	int ret = 0;
	struct timespec64 boot;
    boot.tv_nsec = 0;
	if (init_syms())
		return -EINVAL;

	JUMP_INIT(lapic_next_deadline);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(lapic_next_deadline);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	return ret;
}

static void __exit hook_func_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(lapic_next_deadline);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	wakepu_cpu_tsc(1);
}

module_init(hook_func_entry);
module_exit(hook_func_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

