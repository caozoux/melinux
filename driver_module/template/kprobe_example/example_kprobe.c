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


static int kprobe_udp_unicast_rcv_skb(struct kprobe *kp, struct pt_regs *regs)
{
	struct sock *sk = (struct sock*)regs->di;
	struct sk_buff *skb = (struct sk_buff*)regs->si;;
	struct udphdr *uh   = (struct udphdr*)regs->dx;
	printk("zz %s sk:%lx skb:%lx uh:%lx source %d dest %d len %d\n",__func__, 
			(unsigned long)sk, (unsigned long)skb, (unsigned long)uh
			, ntohs(uh->source)
			, ntohs(uh->dest)
			, ntohs(uh->len)
			);
	return 0;

}

static int kprobe_udp_gro_receive(struct kprobe *kp, struct pt_regs *regs)
{
	struct list_head *head = (struct list_head*) regs->di;
	struct sk_buff *skb = (struct sk_buff*) regs->si;;
	struct udphdr *uh   = (struct udphdr*) regs->dx;
	struct sk_buff *p;
	printk("zz %s head:%lx skb:%lx uh:%lx source %d dest %d len %d\n",__func__, 
			(unsigned long)head, (unsigned long)skb, (unsigned long)uh
			, ntohs(uh->source)
			, ntohs(uh->dest)
			, ntohs(uh->len)
			);

	//list_for_each_entry(p, head, list) {
	//	printk("zz %s p:%lx \n",__func__, (unsigned long)p);
	//}
    return 0;
}

int kprobe_ip_local_deliver(struct kprobe *kp, struct pt_regs *regs)
{
	struct sk_buff *skb = (struct sk_buff*) regs->di;;
	printk("zz %s skb:%lx data:%lx\n",__func__, (unsigned long)skb, (unsigned long)skb->data);
	return 0;
}

int kprobe_ip_rcv(struct kprobe *kp, struct pt_regs *regs)
{
	struct sk_buff *skb = (struct sk_buff*) regs->di;;
	printk("zz %s skb:%lx data:%lx\n",__func__, (unsigned long)skb, (unsigned long)skb->data);
	return 0;
}

int kprobe_skb_clone(struct kprobe *kp, struct pt_regs *regs)
{
	struct sk_buff *skb = (struct sk_buff*) regs->di;;
	printk("zz %s skb:%lx data:%lx\n",__func__, (unsigned long)skb, (unsigned long)skb->data);
	return 0;
}

static int kprobe___udp4_lib_rcv(struct kprobe *kp, struct pt_regs *regs)
{
	struct sk_buff *skb = (struct sk_buff*) regs->di;;
	struct udphdr *uh;
	unsigned short ulen;
	 __be32 saddr, daddr;

	uh   = udp_hdr(skb);
	ulen = ntohs(uh->len);
	saddr = ip_hdr(skb)->saddr;
	daddr = ip_hdr(skb)->daddr;

	printk("zz %s skb:%lx data:%lx protocol:%x\n",__func__, (unsigned long)skb, (unsigned long)skb->data, skb->protocol);
	printk("zz %s uh:%lx ulen:%lx saddr:%lx daddr:%lx \n",__func__, (unsigned long)uh, (unsigned long)ulen, (unsigned long)saddr, (unsigned long)daddr);

	//dump_stack();
	return 0;
}

struct list_head *orig_ptype_all;

static int kprobe___netif_receive_skb_core(struct kprobe *kp, struct pt_regs *regs)
{
	struct packet_type *ptype, *pt_prev;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0)
	struct sk_buff *skb = (struct sk_buff*) regs->di;;
	printk("zz %s skb:%lx data:%lx protocol:%x\n",__func__, (unsigned long)skb, (unsigned long)skb->data, skb->protocol);
#else
	struct sk_buff **pskb = (struct sk_buff**) regs->di;
	struct sk_buff *skb = *pskb;
	printk("zz %s skb:%lx data:%lx protocol:%x\n",__func__, (unsigned long)skb, (unsigned long)skb->data, skb->protocol);
#endif
#if 0
	orig_ptype_all = (struct list_head *)0xffffffffac8432e0;

	printk("zz %s orig_ptype_all:%lx %lx\n",__func__, (unsigned long)orig_ptype_all->next, (unsigned long)orig_ptype_all->next);
	printk("zz %s orig_ptype_all:%lx %lx\n",__func__, (unsigned long)skb->dev->ptype_all.next, (unsigned long)skb->dev->ptype_all.next);
	ptype = list_entry(orig_ptype_all, struct packet_type, list)
	list_for_each_entry(ptype, orig_ptype_all, list) {
		printk("zz %s ptype->func:%lx \n",__func__, (unsigned long)ptype->func);
	}

	list_for_each_entry(ptype, &skb->dev->ptype_all, list) {
		printk("zz %s ptype->func:%lx \n",__func__, (unsigned long)ptype->func);
	}
#endif
	return 0;
}

struct kprobe kplist[] = {
	{
        .symbol_name = "udp_unicast_rcv_skb",
        .pre_handler = kprobe_udp_unicast_rcv_skb,
	},
	{
        .symbol_name = "udp_gro_receive",
        .pre_handler = kprobe_udp_gro_receive,
	},
	{
        .symbol_name = "__netif_receive_skb_core.constprop.0",
        .pre_handler = kprobe___netif_receive_skb_core,
	},
	{
        .symbol_name = "__udp4_lib_rcv",
        .pre_handler = kprobe___udp4_lib_rcv,
	},
	{
        .symbol_name = "ip_local_deliver",
        .pre_handler = kprobe_ip_local_deliver,
	},
	{
        .symbol_name = "ip_rcv",
        .pre_handler = kprobe_ip_rcv,
	},
	{
        .symbol_name = "skb_clone",
        .pre_handler = kprobe_skb_clone,
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
