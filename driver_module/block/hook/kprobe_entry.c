#include <linux/init.h>
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
#include "medriver_kprone.h"

int kpro_blk_mq_flush_plug_list(struct kprobe *p, struct pt_regs *regs)
{

	struct blk_plug *plug = (struct blk_plug *) regs->di;
	if (plug)
		printk("zz %s plug:%p\n", __func__, plug);
		
	return 0;
}

struct kprobe_item pro_blk_plug = {
	.enable = 1,
	.symbol = "blk_mq_flush_plug_list",
	.pre_handler = kpro_blk_mq_flush_plug_list,
};

static int __init kprobedriver_init(void)
{
    	medriver_register_kprobe(&pro_blk_plug);
	printk("zz %s \n", __func__);
	return 0;
}

static void __exit kprobedriver_exit(void)
{
    	medriver_unregister_kprobe(&pro_blk_plug);
	printk("zz %s \n", __func__);
}

module_init(kprobedriver_init);
module_exit(kprobedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zou cao<zoucaox@outlook.com>");
