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

static unsigned int hook_ping_pid = -1;
module_param(hook_ping_pid, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hook_ping_pid, "hook ping pid");

static void icmp_header_dump(struct icmphdr *icp, struct sk_buff *skb)
{

}

static void data_dump(struct sk_buff *skb)
{
	int i,j;
	unsigned int len;
	unsigned int *p;
	len =  skb->len;
	p = (unsigned int *) skb->data;

	for (i = 0; i < len/4; i=i+4) {
		for (j = i; j < len/4 && j < i+4 ; ++j) {
			printk("%08x ", p[j]);
		}
		printk("\n");
	}
	printk("\n");
}


int dev_queue_xmit_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct sk_buff *skb;
	struct icmphdr *icp;
	struct iphdr *iph;

	if (current->pid != hook_ping_pid)
		return 0;

	skb = (struct sk_buff *) regs->di;
	icp=icmp_hdr(skb);
	iph = (struct iphdr *)skb->data;
	if (iph->protocol != 0x6E)
		return 0;

	printk("%s skb=%lx type:%ld code:%ld id:%lx seq:%lx\n", 
			__func__,
			(long int)skb,
			(long int)icp->type,
			(long int)icp->code,
			(long int)(icp->un.echo.id),
			(long int)(icp->un.echo.sequence));

	printk("zz %s skb->head:%lx skb->transport_header:%lx data:%lx icmp:%lx\n",__func__, (long int)skb->head, (long int)skb->transport_header, skb->data, icp);
	data_dump(skb);
    return 0;
}

struct kprobe kp1 = {
        .symbol_name = "dev_queue_xmit",
        .pre_handler = dev_queue_xmit_handler,
};

int skb_copy_datagram_iovec_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct sk_buff *skb;
	struct icmphdr *icp;
	struct iphdr *iph;

	if (current->pid != hook_ping_pid)
		return 0;

	skb = (struct sk_buff *) regs->di;
	icp=icmp_hdr(skb);
	iph = (struct iphdr *)skb->data;
	if (iph->protocol != 1)
		return 0;

	trace_printk("%s skb=%lx type:%ld code:%ld id:%ld seq:%ld\n", 
			__func__,
			(long int)skb,
			(long int)icp->type,
			(long int)icp->code,
			(long int)ntohs(icp->un.echo.id),
			(long int)ntohs(icp->un.echo.sequence));
		
    return 0;
}

struct kprobe kp_sys_sendmsg = {
        .symbol_name = "skb_copy_datagram_iovec",
        .pre_handler = skb_copy_datagram_iovec_handler,
};
static int __init kprobedriver_init(void)
{
	int ret;
    //ret = register_kprobe(&kp_sys_sendmsg);
    ret = register_kprobe(&kp1);
	if (hook_ping_pid != -1)
		printk("hook task pid:%d\n", hook_ping_pid);

	printk("zz %s %d %d\n", __func__, sizeof(struct icmphdr), sizeof(struct iphdr));
	return 0;
}

static void __exit kprobedriver_exit(void)
{
    unregister_kprobe(&kp1);
    //unregister_kprobe(&kp_sys_sendmsg);
	printk("zz %s \n", __func__);
}

module_init(kprobedriver_init);
module_exit(kprobedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zou cao<zoucaox@outlook.com>");
