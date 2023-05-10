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
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>

struct check_regs {
	u64 r15;
	u64 rbp;
	u64 rsp;
};

static struct check_regs __percpu *per_regs;
//#define HOOK_FUN "show_cpuinfo"
#define HOOK_FUN "vsnprintf"

static int kprobe_pre_handle(struct kprobe *kp, struct pt_regs *regs)
{
	struct check_regs *percpu_regs = this_cpu_ptr(per_regs);

	if (!percpu_regs) {
		printk("percpu is NULL \n");
		goto out;
	}

	percpu_regs->r15 = regs->r15;
	percpu_regs->rsp = regs->sp;
	//percpu_regs->rbx = regs->bx;

out:
	return 0;
}

static int kretprobe_handle(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct check_regs *percpu_regs = this_cpu_ptr(per_regs);
	if (!percpu_regs) {
		printk("percpu is NULL \n");
		goto out;
	}

	if (percpu_regs->r15 != regs->r15)
		trace_printk("r15 is diff\n");
	if (percpu_regs->rsp != regs->sp)
		printk("rbp is diff\n");
#if 0
	if (percpu_regs->rbx != regs->bx)
		printk("rbx is diff\n");
#endif
out:
	return 0;
}

struct kprobe kplist[] = {
	{
        .symbol_name = HOOK_FUN,
		.offset = 2,
        .pre_handler = kprobe_pre_handle,
	},
};

struct kretprobe kretlist[]  = {
	{
		.kp.symbol_name = HOOK_FUN,
		.handler= kretprobe_handle,
	},
};


static int __init kprobe_driver_init(void)
{
	int kpnum, kpretnum;

	per_regs = alloc_percpu(struct check_regs);
	if (!per_regs)
		return -EINVAL;

	for (kpnum = 0; kpnum < sizeof(kplist)/sizeof(struct kprobe); kpnum++)
		if (register_kprobe(&kplist[kpnum]))
			goto out1;

	for (kpretnum = 0; kpretnum < sizeof(kretlist)/sizeof(struct kretprobe); kpretnum++)
		if (register_kretprobe(&kretlist[kpretnum]))
			goto out2;

	return 0;
out2:
	while (kpretnum>0) {
		kpretnum--;
		unregister_kretprobe(&kretlist[kpretnum]);
	}
out1:
	while (kpnum>0) {
		kpnum--;
		unregister_kprobe(&kplist[kpnum]);
	}
	return -EINVAL;
}

static void __exit kprobe_driver_exit(void)
{
	int i;
	for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i)
		unregister_kprobe(&kplist[i]);

	for (i = 0; i < sizeof(kretlist)/sizeof(struct kretprobe); ++i)
		unregister_kretprobe(&kretlist[i]);

	free_percpu(per_regs);
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
