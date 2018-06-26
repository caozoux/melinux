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
	struct timeval cur_time;
	struct timeval *ping_time;

	if (current->pid != hook_ping_pid)
		return 0;

	skb = (struct sk_buff *) regs->di;
	icp=icmp_hdr(skb);
	iph = (struct iphdr *)skb->data;
	if (iph->protocol != 0x6E)
		return 0;

	do_gettimeofday(&cur_time);
	ping_time=(struct timeval *)(icp+1);
	printk("%s skb=%lx id=%d seq=%d time=%ld.%06ld\n", 
			__func__,
			(long int)skb,
			htons(icp->un.echo.id),
			htons(icp->un.echo.sequence),
			cur_time.tv_sec-ping_time->tv_sec,
			cur_time.tv_usec-ping_time->tv_usec);

	data_dump(skb);
    return 0;
}

struct kprobe kp1 = {
        .symbol_name = "dev_queue_xmit",
        .pre_handler = dev_queue_xmit_handler,
};

static int __init kprobedriver_init(void)
{
	int ret;
    ret = register_kprobe(&kp1);
	if (hook_ping_pid != -1)
		printk("hook task pid:%d\n", hook_ping_pid);

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
