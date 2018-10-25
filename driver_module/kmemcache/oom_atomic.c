#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/ftrace.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include "kmemcache.h"

int shrink_slab_handler(struct kprobe *p, struct pt_regs *regs)
{
	printk("%s\n",__func__);
	dump_stack();

	return 0;
}

struct kprobe kp2 = {
        .symbol_name = "shrink_slab",
        .pre_handler = shrink_slab_handler,
};

int shrink_zone_handler(struct kprobe *p, struct pt_regs *regs)
{
	printk("shrink_zone\n");
	dump_stack();
	return 0;
}

struct kprobe kp1 = {
        .symbol_name = "shrink_zone",
        .pre_handler = shrink_zone_handler,
};

void oom_atomic_test_init()
{
	void *addr;		
    	register_kprobe(&kp1);
    	register_kprobe(&kp2);
	while(1) {
		addr = kmalloc(1024*1024*4, GFP_ATOMIC);
	}
}

void oom_atomic_test_exit()
{
    	unregister_kprobe(&kp1);
    	unregister_kprobe(&kp2);
}

