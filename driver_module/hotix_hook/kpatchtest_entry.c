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

DEFINE_ORIG_FUNC(void,  getboottime64, 1,
		 struct timespec64*, ts);

static int init_syms(void)
{
	TEXT_SYMS()
	LOOKUP_SYMS(getboottime64);
	return 0;
}

void (*old_getboottime64)(struct timespec64 *ts);

void new_getboottime64(struct timespec64 *ts)
{
}

static int __init cpuset_trick_init(void)
{
	int ret = 0;
	struct timespec64 boot;
    boot.tv_nsec = 0;
	if (init_syms())
		return -EINVAL;

	JUMP_INIT(getboottime64);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(getboottime64);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	return ret;
}

static void __exit cpuset_trick_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(getboottime64);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
}

module_init(cpuset_trick_init);
module_exit(cpuset_trick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

