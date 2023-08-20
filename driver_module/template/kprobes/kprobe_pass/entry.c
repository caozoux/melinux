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
#include <linux/proc_fs.h>
#include <linux/jbd2.h>

static void pass_func(void)
{

}

static int kprobe_passthrough_func(struct kprobe *kp, struct pt_regs *regs)
{
	regs->ip = (unsigned long)pass_func;	
	return 1;
}

struct kprobe kplist[] = {
	{
        .symbol_name = "css_release_work_fn",
        .pre_handler = kprobe_passthrough_func,
	},
	{
        .symbol_name = "cgroup_bpf_release",
        .pre_handler = kprobe_passthrough_func,
	},
};

static int __init kprobe_driver_init(void)
{
	int i;
	for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
		if (register_kprobe(&kplist[i])) {
			printk("register kprobe failed:%s \n", kplist[i].symbol_name);
			while (i>0) {
				i--;
				unregister_kprobe(&kplist[i]);
			}
			goto out;
		}
	}
	return 0;
out:
	return -EINVAL;
}

static void __exit kprobe_driver_exit(void)
{
	int i;
	for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
		unregister_kprobe(&kplist[i]);
	}
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
