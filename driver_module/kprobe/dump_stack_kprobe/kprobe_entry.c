#include <linux/init.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include <linux/ftrace.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/mm.h>

char *sym = NULL;
module_param (sym, charp, S_IRUGO);

static int kprobe_handler(struct kprobe *p, struct pt_regs *regs);
static int kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags);

struct kprobe kp1 = {
        .pre_handler = kprobe_handler,
	//.post_handler = kprobe_post_handler,
};

static int kprobe_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	return 0;
}

static int kprobe_handler(struct kprobe *p, struct pt_regs *regs)
{
	dump_stack();
	p->flags |=KPROBE_FLAG_DISABLED;
	return 0;
}

static int __init kprobedriver_init(void)
{
	int ret;
	if (!sym) {
		printk("no symbole specified\n");
		return -EINVAL;
	}
	kp1.symbol_name = sym;
    	ret = register_kprobe(&kp1);
	return 0;
}

static void __exit kprobedriver_exit(void)
{
    	unregister_kprobe(&kp1);
}

module_init(kprobedriver_init);
module_exit(kprobedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zou cao<zoucaox@outlook.com>");
