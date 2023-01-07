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


int kprobe_func(struct kprobe *kp, struct pt_regs *regs)
{
	return 0;
}

struct kprobe kplist = {
        .symbol_name = "kallsyms_lookup_name",
        .pre_handler = kprobe_func,
};

static int __init kprobe_driver_init(void)
{
	int i;

	if (register_kprobe(&kplist))
		goto out;

	printk("zz %s addr:%lx \n",__func__, (unsigned long)kplist.addr);

	return 0;
out:
	return -EINVAL;
}

static void __exit kprobe_driver_exit(void)
{
	unregister_kprobe(&kplist);
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
