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
	u64 r12;
	u64 rbp;
	u64 rbx;
};

static struct check_regs __percpu *per_regs;

static int kprobe_page_add_file_rmap(struct kprobe *kp, struct pt_regs *regs)
{
	struct check_regs *percpu_regs = this_cpu_ptr(per_regs);

	if (!percpu_regs) {
		trace_printk("percpu is NULL \n");
		goto out;
	}

	percpu_regs->r12 = regs->r12;
	percpu_regs->rbp = regs->bp;
	percpu_regs->rbx = regs->bx;

out:
	return 0;
}

static int kretprobe_page_add_file_rmap(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct check_regs *percpu_regs = this_cpu_ptr(per_regs);
	if (!percpu_regs) {
		trace_printk("percpu is NULL \n");
		goto out;
	}

	if (percpu_regs->r12 != regs->r12)
		trace_printk("r12 is diff\n");
	if (percpu_regs->rbp != regs->bp)
		trace_printk("rbp is diff\n");
	if (percpu_regs->rbx != regs->bx)
		trace_printk("rbx is diff\n");
out:
	return 0;
}

struct kprobe kplist[] = {
	{
        .symbol_name = "page_add_file_rmap",
        .pre_handler = kprobe_page_add_file_rmap,
	},
};

struct kretprobe kretlist[]  = {
	{
		.kp.symbol_name = "page_add_file_rmap",
		.handler= kretprobe_page_add_file_rmap,
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
