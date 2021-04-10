#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/bvec.h>
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
#include <linux/uio.h>
#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"
#include "blocklocal.h"


void file_readwrite_test(void)
{
	char file_name[] = "/root/aa";
	struct file *file;
	struct page *page;
	int len = PAGE_SIZE;
	void *buf;
	struct address_space *mapping;
	struct iovec iov;
	struct kiocb kiocb;
	struct iov_iter iter;
	struct inode *inode;
	struct file_ra_state *ra;
	unsigned long offset;
	loff_t ppos = 0;
	pgoff_t index, last_index;
	pgoff_t prev_index;
	pgoff_t prev_offset;
	ssize_t ret;
	int error = 0;
	gfp_t gfp;

	file = filp_open(file_name, O_RDWR , 0600);

	if (!file) {
		printk("open %s failed\n", file_name);
		return;
	}

	buf = kmalloc(PAGE_SIZE * 2, GFP_KERNEL);
	iov.iov_base = buf;
	iov.iov_len = len;
	ra = &file->f_ra;
	mapping = file->f_mapping;
	inode = mapping->host;
	gfp = mapping_gfp_mask(mapping);

	{
		struct dentry *dentry;
		//list_entry(head, struct inode, i_io_list);
		//dentry = list_entry(inode->i_dentry, &inode->i_dentry, d_u.d_alias);
		dentry = hlist_entry(inode->i_dentry.first, struct dentry, d_u.d_alias);

		printk("zz %s name:%s \n",__func__, (int)dentry->d_iname);
	}

	init_sync_kiocb(&kiocb, file);
	kiocb.ki_pos = &ppos;
	iov_iter_init(&iter, READ, &iov, 1, len);

	index = ppos >> PAGE_SHIFT;
	prev_index = ra->prev_pos >> PAGE_SHIFT;
	prev_offset = ra->prev_pos & (PAGE_SIZE-1);
	last_index = (ppos + iter.count + PAGE_SIZE-1) >> PAGE_SHIFT;
	offset = ppos & ~PAGE_MASK;

	page = find_get_page(mapping, index);
	if (!page) {
		int i;
		//page = page_cache_alloc(mapping);
		page = alloc_pages(gfp, 9);
		if (!page) {
			pr_err("alloc page memory failed\n");
			goto failed;
		}

		for (i = 0; i < 512; i++) {
			error = add_to_page_cache_lru(page, mapping, index + i,
					mapping_gfp_constraint(mapping, GFP_KERNEL));
			if (error) {
				put_page(page);
				goto failed;
			}
			error = mapping->a_ops->readpage(file, page);
			if (error) {
				put_page(page);
				goto failed;
			}
			page++;
		}
	}



	//generic_file_read_iter(iocb, to);
	//file->f_op->read_iter(kio, iter);

	filp_close(file, NULL);
	return;

failed:
	printk("zz %s %d failed\n", __func__, __LINE__);
	filp_close(file, NULL);

}

void readhead_test(void)
{

}

