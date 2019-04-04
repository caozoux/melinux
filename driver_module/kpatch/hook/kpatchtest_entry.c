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
void *__kprobes (*orig_text_poke_smp)(void *addr,
					const void *opcode, size_t len);


DEFINE_ORIG_FUNC(void,  getboottime, 1,
		 struct timespec*, ts);
static int init_syms(void)
{
	LOOKUP_SYMS(text_mutex);
	LOOKUP_SYMS(text_poke_smp);
	LOOKUP_SYMS(timekeeper);
	LOOKUP_SYMS(getboottime);
	printk("zz %s orig_text_mutex:%lx orig_text_poke_smp:%lx orig_timekeeper:%lx \n",__func__, (unsigned long)orig_text_mutex, (unsigned long)orig_text_poke_smp, (unsigned long)orig_timekeeper);
	return 0;
}

#if 0
//struct cgroup_subsys_state *
//new_cpuset_css_alloc(struct cgroup_subsys_state *parent_css);

//DEFINE_ORIG_FUNC(struct cgroup_subsys_state *, cpuset_css_alloc, 1,
//		 struct cgroup_subsys_state *, parent_css);
/*
 *	cpuset_css_alloc - allocate a cpuset css
 *	cgrp:	control group that the new cpuset will be part of
 */

struct cgroup_subsys_state *
new_cpuset_css_alloc(struct cgroup_subsys_state *parent_css)
{
	return NULL;
}
#endif

void new_getboottime(struct timespec *ts)
{
    struct timekeeper *tk = orig_timekeeper;
    struct timespec boottime = {
        .tv_sec = tk->wall_to_monotonic.tv_sec +
                tk->total_sleep_time.tv_sec,
        .tv_nsec = tk->wall_to_monotonic.tv_nsec +
                tk->total_sleep_time.tv_nsec
    };
	printk("zz %s %d \n", __func__, __LINE__);

    set_normalized_timespec(ts, -boottime.tv_sec, -boottime.tv_nsec);
}

static int __init cpuset_trick_init(void)
{
	int ret = 0;

	if (init_syms())
		return -EINVAL;

	JUMP_INIT(getboottime);

#if 1
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALL(getboottime);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
#else
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	printk("zz %s new_getboottime:%lx inst_getboottime:%lx orig_getboottime:%lx e9_getboottime:%lx \n",__func__, (unsigned long)new_getboottime, (unsigned long)inst_getboottime, (unsigned long)orig_getboottime, (unsigned long)e9_getboottime);
#endif

	pr_info("cpuset_trick loaded: %d\n", ret);
	return ret;
}

static void __exit cpuset_trick_exit(void)
{
#if 1
	struct timespec ts;
	getboottime(&ts);
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(getboottime);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	pr_info("cpuset_trick unloaded.\n");
#endif
}

module_init(cpuset_trick_init);
module_exit(cpuset_trick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

