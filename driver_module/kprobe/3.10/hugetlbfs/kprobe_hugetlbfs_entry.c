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
	struct inode *inode=(struct inode *)regs->di;
	loff_t lstart=(loff_t)regs->si;
	struct hstate *h = hstate_inode(inode);
	struct address_space *mapping = &inode->i_data;
	const pgoff_t start = lstart >> huge_page_shift(h);
	struct pagevec pvec;
	pgoff_t next;
	int i, freed = 0;

	dump_stack();
	pagevec_init(&pvec, 0);
	next = start;         
	printk("next:%lx\n", next);
	while(1) {
		if (!pagevec_lookup(&pvec, mapping, next, PAGEVEC_SIZE)) {	
			if (next == start) 
				break;
			next = start;
			continue; 
		}
			
		printk("zz %s  nr_page:%d \n", __func__, pagevec_count(&pvec));

		for (i = 0; i < pagevec_count(&pvec); ++i) {
            		struct page *page = pvec.pages[i];
			printk("zz %s  page->mapping:%lx  page->_mapcount:%lx flags:%lx\n", __func__,
				 page->mapping,
				 page_mapped(page), page->flags);
        	}
		break;
	}
 
		
	printk("zz %s mapping:%lx\n", __func__, (unsigned long) mapping);
	printk("zz %s  %d node->i_count: \n", __func__, atomic_read(&inode->i_count));
	return 0;
}

struct kprobe kp1 = {
        .symbol_name = "hugetlbfs_evict_inode",
        .pre_handler = hugetlbfs_evict_inode_handler,
};

int hugetlbfs_statfs_handle(struct kprobe *p, struct pt_regs *regs)
{
	struct dentry *dentry=(struct inode *)regs->di;
	struct inode *inode=dentry->d_inode;
	struct address_space *mapping = &inode->i_data;
	struct pagevec pvec;
	pgoff_t next;
	int i, freed = 0;

	dump_stack();
	pagevec_init(&pvec, 0);
	next = 0;         
	printk("next:%lx\n", next);
	while(1) {
		if (!pagevec_lookup(&pvec, mapping, next, PAGEVEC_SIZE)) {	
			if (next == 0) 
				break;
			next = 0;
			continue; 
		}
			
		printk("zz %s  nr_page:%d \n", __func__, pagevec_count(&pvec));

		for (i = 0; i < pagevec_count(&pvec); ++i) {
            		struct page *page = pvec.pages[i];
			printk("zz %s  page->mapping:%lx  page->_mapcount:%lx flags:%lx mapping->nrpages:%d\n", __func__,
				 page->mapping,
				page_mapped(page), page->flags
				, mapping->nrpages);
        	}
		break;
	 
	}
 
		
	printk("zz %s mapping:%lx\n", __func__, (unsigned long) mapping);
	printk("zz %s  node->i_count:%d inode->i_block:%d \n", __func__, atomic_read(&inode->i_count), inode->i_blocks);
		
	return 0;
}
struct kprobe kp2 = {
        .symbol_name = "hugetlbfs_setattr",
        .pre_handler = hugetlbfs_statfs_handle,
};

static int __init kprobedriver_init(void)
{
    	register_kprobe(&kp1);
    	register_kprobe(&kp2);
	printk("zz %s \n", __func__);
	return 0;
}

static void __exit kprobedriver_exit(void)
{
    	unregister_kprobe(&kp1);
    	unregister_kprobe(&kp2);
	printk("zz %s \n", __func__);
}

module_init(kprobedriver_init);
module_exit(kprobedriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zou cao<zoucaox@outlook.com>");
