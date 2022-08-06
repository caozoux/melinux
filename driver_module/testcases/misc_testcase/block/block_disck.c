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
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/blkdev.h>

#if LINUX_VERSION_CODE >  KERNEL_VERSION(5,0,0)
#include <linux/backing-dev-defs.h>
#endif

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"


extern struct list_head *orig_all_bdevs;
extern spinlock_t *orig_bdev_lock;
extern struct list_head *orig_bdi_list;

static inline struct inode *wb_inode(struct list_head *head)
{
	return list_entry(head, struct inode, i_io_list);
}

static void scan_wb_info(struct bdi_writeback *wb)
{
	while (!list_empty(&wb->b_io)) {
		struct inode *inode = wb_inode(wb->b_io.prev);
		dump_inode_data(inode, 1);
	}
}

static void __maybe_unused scan_disk_bdi_wb_list(struct backing_dev_info *bdi)
{
	 struct bdi_writeback *wb;

	 if (!bdi)
		 return;
	 //if (bdi_has_dirty_io(bdi)) 
	 list_for_each_entry_rcu(wb, &bdi->wb_list, bdi_node) {
#if 1
		 if (wb) {
			spin_lock(&wb->list_lock);
			scan_wb_info(wb);
			spin_unlock(&wb->list_lock);
		 }
#else
		 printk("zz %s wb:%lx \n",__func__, (unsigned long)wb);
#endif
	 }
}

static void __maybe_unused scan_disk_bdi_list(void)
{
	struct backing_dev_info *bdi;
	rcu_read_lock();
	list_for_each_entry_rcu(bdi, orig_bdi_list, bdi_list) {

	}
	rcu_read_unlock();
}

static void  dump_disk_info(struct gendisk *disk)
{
#if 0
	struct backing_dev_info *info;
	struct request_queue *rq;


	info = disk->queue->backing_dev_info;
	rq = disk->queue;

	//printk("zz %s info:%lx \n",__func__, (unsigned long)info);
	scan_disk_bdi_wb_list(info);
#else
	printk("zz %s disk:%lx \n",__func__, (unsigned long)disk);
#endif
}

void scan_block_dev_disk(void)
{
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	struct block_device *bdev;
	spin_lock(orig_bdev_lock);
	list_for_each_entry(bdev, orig_all_bdevs, bd_list) { 	
		if (bdev->bd_disk->disk_name) {
			//printk("%s%d\n", bdev->bd_disk->disk_name, bdev->bd_partno);	
			if (bdev->bd_disk)
				if (!strcmp(bdev->bd_disk->disk_name,"vdb"))
					dump_disk_info(bdev->bd_disk);

		}
	}
	spin_unlock(orig_bdev_lock);
#endif
}

