#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include "../include/hotfix_util.h"

#define PROC_PARENT "ctrlproc"

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void *(*orig_text_poke_bp)(void *addr, const void *opcode, size_t len, void *handler);

struct list_head *orig_net_namespace_list;

static int kdevice_scan_net_device(void);

int kprobe_func(struct kprobe *kp, struct pt_regs *regs)
{
    return 0;
}

struct kprobe kplist = {
	.symbol_name = "kallsyms_lookup_name",
	.pre_handler = kprobe_func,
};

static int sampling_period_show(struct seq_file *m, void *ptr)
{
    seq_printf(m, "%llums\n", 10);
	kdevice_scan_net_device();

    return 0;
}

static int sampling_open(struct inode *inode, struct file *file)                                                                                                                                                                                 
{
    return single_open(file, sampling_period_show, inode->i_private);
}

static ssize_t ctrl_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    unsigned long val;

	if (kstrtoul_from_user(buf, count, 0, &val))
        return -EINVAL;

	return count;
}

static const struct file_operations sampling_period_fops = {
    .open       = sampling_open,
    .read       = seq_read,
    .write      = ctrl_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static void kdevice_dump_net_device(struct net_device *dev)
{
	struct net_device_stats *stats;
	if (!dev)
		return;

	stats = &dev->stats;

	//atomic_long_inc(&dev->rx_dropped);
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
				if (strstr(dev->name, "enp0s3")) {
					struct net_device_stats *stats;
					stats = &dev->stats;
					atomic_long_inc(&dev->rx_dropped);
#if 1
					stats->rx_dropped += 8;
					stats->rx_errors += 1;
					stats->rx_length_errors += 2;
					stats->rx_over_errors += 3;
					stats->rx_frame_errors += 4;
					stats->rx_fifo_errors += 5;
					stats->rx_missed_errors += 6;
					stats->rx_crc_errors += 7;
#endif
					kdevice_dump_net_device(dev);
				}
			}
		}
	}
	return 0;
}

static int get_kallsyms_lookup_name(void)
{
    if (register_kprobe(&kplist))
        return -EINVAL;

    unregister_kprobe(&kplist);

    cust_kallsyms_lookup_name = (void *)kplist.addr;

    return 0;
}

static int proc_dir_init(void)
{
	struct proc_dir_entry *parent_dir;
	parent_dir = proc_mkdir(PROC_PARENT, NULL);
	if (!parent_dir) 
		return -ENOMEM;

	if (!proc_create(PROC_PARENT, S_IRUSR | S_IWUSR, parent_dir,
             &sampling_period_fops))                                                                                                                                                                                                                      
        goto remove_proc;

	return 0;

remove_proc:
    remove_proc_subtree(PROC_PARENT, NULL);

    return -ENOMEM;
}

static void proc_dir_exit(void)
{
    remove_proc_subtree(PROC_PARENT, NULL);
}

static  int sym_init(void)
{
	if (get_kallsyms_lookup_name())
		goto out;

	LOOKUP_SYMS(net_namespace_list);
	return 0;

out:
	return -EINVAL;
}

static int __init entry_init(void)
{
	if (sym_init())
		goto out;

	if (proc_dir_init())
		goto out;

	return 0;
out:
	return -EINVAL;
}

static void __exit entry_exit(void)
{
	proc_dir_exit();
}

module_init(entry_init);
module_exit(entry_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

