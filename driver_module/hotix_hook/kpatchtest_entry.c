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


struct mutex *orig_text_mutex;
struct timekeeper *orig_timekeeper;
TEXT_DECLARE()

DEFINE_ORIG_FUNC(void,  getboottime, 1,
		 struct timespec*, ts);

static int init_syms(void)
{
	TEXT_SYMS()
	LOOKUP_SYMS(timekeeper);
	LOOKUP_SYMS(getboottime);
	return 0;
}


void new_getboottime(struct timespec *ts)
{
	printk("zz %s %d \n", __func__, __LINE__);
}

static int __init cpuset_trick_init(void)
{
	int ret = 0;

	if (init_syms())
		return -EINVAL;

	JUMP_INIT(getboottime);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALL(getboottime);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	return ret;
}

static void __exit cpuset_trick_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(getboottime);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
}

module_init(cpuset_trick_init);
module_exit(cpuset_trick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

