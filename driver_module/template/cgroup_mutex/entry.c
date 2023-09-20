#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>
#include <trace/events/block.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

unsigned long (*cust_kallsyms_lookup_name)(const char *name);

struct percpu_rw_semaphore *orig_cgroup_threadgroup_rwsem;

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) {
	return 0;
}

static int sym_init(void)
{
	LOOKUP_SYMS(cgroup_threadgroup_rwsem);
	return 0;
}

static int sysm_lookup_init(void)
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

int proc_debug_show(struct seq_file *m, void *v)
{
	percpu_down_write(&cgroup_threadgroup_rwsem);
	percpu_up_write(&cgroup_threadgroup_rwsem);
	return 0;
}

static int __init kprobe_driver_init(void)
{
	if (sysm_lookup_init())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	proc_create_single("test", 0, NULL, proc_debug_show);

	return 0;

}

static void __exit kprobe_driver_exit(void)
{

}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
