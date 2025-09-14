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


struct mutex *orig_text_mutex;
struct timekeeper *orig_timekeeper;

DEFINE_ORIG_FUNC(ktime_t, tick_nohz_next_event, 2, struct tick_sched *, ts, int, cpu);
TEXT_DECLARE()
ktime_t (*old_tick_nohz_next_event)(struct tick_sched *ts, int cpu);

ktime_t new_tick_nohz_next_event(struct tick_sched *ts, int cpu)
{
	ktime_t  kt_s;
	kt_s = old_tick_nohz_next_event(ts,cpu);
	printk("zz %s %d cpu%d ts:%lx\n", __func__, __LINE__, cpu, kt_s);
	return kt_s;	 
}

static int init_syms(void)
{
	TEXT_SYMS()
	LOOKUP_SYMS(tick_nohz_next_event);
	return 0;
}

static int __init cpuset_trick_init(void)
{
	int ret = 0;
	struct timespec64 boot;
    boot.tv_nsec = 0;
	if (init_syms())
		return -EINVAL;

	JUMP_INIT(tick_nohz_next_event);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(tick_nohz_next_event);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	return ret;
}

static void __exit cpuset_trick_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(tick_nohz_next_event);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
}

module_init(cpuset_trick_init);
module_exit(cpuset_trick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

