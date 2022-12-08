#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <ksioctl/kdevice_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kdevice_local.h"

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

struct perf_event * __percpu *sample_hbp;

struct list_head *orig_net_namespace_list;

static void sample_hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	dump_stack();
	printk(KERN_INFO "Dump stack from sample_hbp_handler\n");
}


static void kdevice_net_rx_dropped_watchpoint(struct net_device *dev)
{
	struct perf_event_attr attr;
	int ret;
	
	hw_breakpoint_init(&attr);

	attr.bp_addr = &dev->rx_dropped;
	attr.bp_len = HW_BREAKPOINT_LEN_4;
	//attr.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
	attr.bp_type = HW_BREAKPOINT_W;

	sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
	if (IS_ERR((void __force *)sample_hbp)) {
		ret = PTR_ERR((void __force *)sample_hbp);
		sample_hbp = NULL;
		goto fail;
	}
	
	return 0;
fail:
	return -EINVAL;
}

static void kdevice_dump_net_device(struct net_device *dev)
{

	struct net_device_stats *stats;
	if (!dev)
		return;

	stats = &dev->stats;

	atomic_long_inc(&dev->rx_dropped);
	printk("netdev:%s\n", dev->name);
	printk("   rx_dropped:%ld\n", atomic_long_read(&dev->rx_dropped));
	printk("   tx_dropped:%ld\n", atomic_long_read(&dev->tx_dropped));
	printk("   rx_nohandler:%ld\n", atomic_long_read(&dev->rx_nohandler));
	printk("stats:\n");
	printk("   rx_errors:%ld\n", stats->rx_errors);
	printk("   tx_errors:%ld\n", stats->tx_errors);
	printk("   rx_dropped:%ld\n", stats->rx_dropped);
	printk("   tx_dropped:%ld\n", stats->tx_dropped);
	printk("   rx_length_errors:%ld\n", stats->rx_length_errors);
	printk("   rx_over_errors:%ld\n",   stats->rx_over_errors);
	printk("   rx_crc_errors:%ld\n",    stats->rx_crc_errors);
	printk("   rx_frame_errors:%ld\n",  stats->rx_frame_errors);
	printk("   rx_fifo_errors:%ld\n",   stats->rx_fifo_errors);
	printk("   rx_missed_errors:%ld\n", stats->rx_missed_errors);

};

static int kdevice_scan_net_device(void)
{
	struct net *net;
	struct net_device *dev;

	list_for_each_entry(net, orig_net_namespace_list, list) {
		for_each_netdev(net, dev) {
			if (dev) {
				if (strstr("ens3", dev->name)) {
					kdevice_net_rx_dropped_watchpoint(dev);	
				}
			}
			kdevice_dump_net_device(dev);
			//printk("zz %s net:%lx dev:%lx \n",__func__, (unsigned long)net, (unsigned long)dev);
		}
	}
	return 0;
}

int kdevice_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *ksdata)
{
	struct kdevice_ioctl kioctl;
	int ret;

	if (copy_from_user(&kioctl, (char __user *)ksdata->data, ksdata->len)) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (ksdata->subcmd) {
		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

static  int kdevice_unit_sym_init(void)
{
	LOOKUP_SYMS(net_namespace_list);
	return 0;
}

int kdevice_unit_init(void)
{
	if (kdevice_unit_sym_init())	
		return -EINVAL;

	kdevice_scan_net_device();
	return 0;
}

int kdevice_unit_exit(void)
{
	if (sample_hbp)
		unregister_wide_hw_breakpoint(sample_hbp);
	return 0;
}

