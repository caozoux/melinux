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
#include "../hotfix_util.h"
#include "kernel/sched/sched.h"


struct mutex *orig_text_mutex;
struct timekeeper *orig_timekeeper;

DEFINE_ORIG_FUNC(void, put_prev_entity, 2, struct cfs_rq*, cfs_rq, struct sched_entity *, prev);
TEXT_DECLARE()
unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void (*old_put_prev_entity)(struct cfs_rq* cfs_rq, struct sched_entity * prev);

void new_put_prev_entity(struct cfs_rq* cfs_rq, struct sched_entity * prev)
{
	if (smp_processor_id() == 1) {
		if (prev->task_type != TASK_BE)
			dump_stack();
	}
	old_put_prev_entity(cfs_rq, prev);
}

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) { return 0; }

static int get_kallsyms_lookup_name(void)
{
	int ret;
	struct kprobe kp;

	memset(&kp, 0, sizeof(struct kprobe));

	ret = -1;
	kp.symbol_name = "kallsyms_lookup_name";
	kp.pre_handler = noop_pre_handler;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		printk("Err: find kallsyms_lookup_name failed \n");
		return -EINVAL;
	}

	cust_kallsyms_lookup_name = (void*)kp.addr;
	unregister_kprobe(&kp);

	return 0;
}

static int init_syms(void)
{
	if (get_kallsyms_lookup_name()) {
		printk("get_kallsyms_lookup_name failed\n");
		return -EINVAL;
	}
	TEXT_SYMS()
	LOOKUP_SYMS(put_prev_entity);
	return 0;
}

static int __init cpuset_trick_init(void)
{
	int ret = 0;
	struct timespec64 boot;
    boot.tv_nsec = 0;
	if (init_syms())
		return -EINVAL;

	JUMP_INIT(put_prev_entity);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(put_prev_entity);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();

	return ret;
}

static void __exit cpuset_trick_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(put_prev_entity);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
}

module_init(cpuset_trick_init);
module_exit(cpuset_trick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

