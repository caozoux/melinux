#include <linux/init.h>
#include <linux/hugetlb.h>                                                                                                                                              |||     HUGETLBFS_I
#include <linux/pagevec.h>
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
#include "kprobe_hugetlb.h"

int hugetlbfs_evict_inode_handler(struct kprobe *p, struct pt_regs *regs)
{
	return 0;
}

struct kprobe kp1 = {
        .symbol_name = "hugetlbfs_evict_inode",
        .pre_handler = hugetlbfs_evict_inode_handler,
};
