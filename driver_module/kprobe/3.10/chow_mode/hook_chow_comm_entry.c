#include <linux/init.h>
#include <linux/hugetlb.h>                                                                                                                                              |||     HUGETLBFS_I
#include <linux/pagevec.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include <linux/ftrace.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/mm.h>

int chmod_common_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct path *path = (struct path *) regs->di;
	if (path->dentry && path->dentry->d_iname) {
		if (strstr("local_tags", path->dentry->d_iname)) {
			trace_printk("tag:%s task:%s pid:%d\n", 
				path->dentry->d_iname
				, current->comm?current->comm:current->comm
				, current->pid);
		}
	}
	return 0;
}

struct kprobe kp1 = {
        .symbol_name = "chmod_common",
        .pre_handler = chmod_common_handler,
};


static int __init kprobedriver_init(void)
{
    	register_kprobe(&kp1);
	printk("zz %s \n", __func__);
	return 0;
}

static void __exit kprobedriver_exit(void)
{
    	unregister_kprobe(&kp1);
	printk("zz %s \n", __func__);
}

module_init(kprobedriver_init);
module_exit(kprobedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zou cao<zoucaox@outlook.com>");
