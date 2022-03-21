#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <uapi/linux/fadvise.h>
#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "blocklocal.h"
#include "medelay.h"
#include "mekernel.h"

void dump_inode_data(struct inode *inode, int leve)
{
	//struct address_space    *i_mapping = inode->i_mapping;
	//struct file_lock    *i_flock;//文件锁链表
	printk("node:%ld flag:%x size:%lld", inode->i_ino, inode->i_flags, inode->i_size);
}

void inode_mapping_dump(struct inode *inode)
{
	//struct radix_tree_iter iter;
	//void __rcu **slot;
	struct address_space *mapping = inode->i_mapping;
	struct radix_tree_root *radix = &mapping->i_pages;
	//radix_tree_for_each_slot(slot, root, &iter, first_index) {
	printk("zz %s mapping:%lx radix:%lx \n",__func__, (unsigned long)mapping, (unsigned long)radix);
}

void scan_inode_radix_pagecache(struct inode *inode)
{
	struct address_space *mapping;
	//struct dentry *dentry;
	//dentry = hlist_entry(inode->i_dentry.first, struct dentry, d_u.d_alias)
	mapping = inode->i_mapping;

	//page = radix_tree_lookup(&mapping->i_pages, page_offset);

}

struct inode *get_inode_with_filename(char *filename)
{
	struct file *filp;
	struct inode *inode;
	filp = filp_open(filename, O_RDWR , 0600);
	inode = filp->f_inode;
	filp_close(filp,NULL);
	return inode;
}

void drop_inode_page_cache(char *filename)
{
	struct inode *inode;
	struct address_space *mapping;
	unsigned long start_index = 0;
	unsigned long end_index;
	struct file *filp;

	filp = filp_open(filename, O_RDWR , 0600);
	if (!filp) {
		pr_warn("open %s failed\n", filename);
		return;
	}

	mapping = filp->f_mapping;
	inode = mapping->host;
	end_index = inode->i_size;

	vfs_fadvise(filp, start_index, end_index, POSIX_FADV_DONTNEED);
}
