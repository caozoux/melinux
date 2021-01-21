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
	struct address_space *mapping = inode->i_mapping;
	struct radix_tree_root *radix = &mapping->i_pages;
}

void drop_inode_page_cache(char *filename)
{
	int lenth;
	struct inode *inode;
	struct address_space *mapping;
	unsigned long start_index = 0;
	unsigned long end_index;
	struct file *filp;

	filp = filp_open(filename, O_RDWR , 0600);
	if (!filp) {
		pr_warning("open %s failed\n", filename);
		return;
	}

	mapping = filp->f_mapping;
	inode = mapping->host;
	end_index = inode->i_size;

	vfs_fadvise(filp, start_index, end_index, POSIX_FADV_DONTNEED);
}
