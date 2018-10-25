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

int dev_queue_xmit_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct request_queue *q = (struct request_queu*)regs->di;
	if (q)
		printk("zz %s q->queue_flags \n", __func__, q->queue_flags);
		
	return 0;
}

struct kprobe kp1 = {
        .symbol_name = "__generic_unplug_device",
        .pre_handler = dev_queue_xmit_handler,
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
