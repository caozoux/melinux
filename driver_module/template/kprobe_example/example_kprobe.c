#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/syscore_ops.h>


static int kprobe_udp_unicast_rcv_skb(struct kprobe *kp, struct pt_regs *regs)
{
	struct sock *sk = regs->di;
	struct sk_buff *skb = regs->si;;
	struct udphdr *uh   = regs->dx;
	printk("zz %s sk:%lx skb:%lx uh:%lx source %d dest %d len %d\n",__func__, 
			(unsigned long)sk, (unsigned long)skb, (unsigned long)uh
			, uh->source
			, uh->dest
			, uh->len
			);
	dump_stack();
	return 0;

}

static int kprobe_udp_gro_receive(struct kprobe *kp, struct pt_regs *regs)
{
	struct list_head *head = regs->di;
	struct sk_buff *skb = regs->si;;
	struct udphdr *uh   = regs->dx;
	struct sk_buff *p;
	printk("zz %s head:%lx skb:%lx uh:%lx source %d dest %d len %d\n",__func__, 
			(unsigned long)head, (unsigned long)skb, (unsigned long)uh
			, uh->source
			, uh->dest
			, uh->len
			);

	list_for_each_entry(p, head, list) {
		printk("zz %s p:%lx \n",__func__, (unsigned long)p);
	}
	dump_stack();
    return 0;
}

static int __init kprobe_driver_init(void)
{
	int i;
	for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
		if (register_kprobe(&kplist[i])) {
			printk("register kprobe failed:%s \n", kp.symbol_name);
			goto out;
		}
	}
	printk("zz %s %d \n", __func__, __LINE__);
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
	printk("zz %s %d \n", __func__, __LINE__);
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
