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
#include <linux/blk-mq.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

typedef void (*virtio_hook)(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd);

static virtio_hook *orig_virtio_queue_rq_hook;
struct list_head *orig_all_bdevs;
struct list_head *orig_bdi_list;
spinlock_t *orig_bdev_lock;

void misc_virtio_queue_rq_hook(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	struct request *req = bd->rq;
	merequest_list_dump(req);
}


int block_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	
	int ret = -1;
	int i;
	int cpu = smp_processor_id();

	switch (data->cmdcode) {
		case  IOCTL_USEBLOCK_INDOE:
			break;

		case  IOCTL_USEBLOCK_FILE:
			task_file_enum(current);
			break;

		default:
			break;
	}

	return 0;
}

int block_unit_init(void)
{
	LOOKUP_SYMS(all_bdevs);
	LOOKUP_SYMS(bdev_lock);
	LOOKUP_SYMS(bdi_list);

	//printk("zz %s orig_virtio_queue_rq_hook:%lx \n",__func__, (unsigned long)orig_virtio_queue_rq_hook);
	//*orig_virtio_queue_rq_hook = (virtio_hook) misc_virtio_queue_rq_hook;
	scan_block_dev_disk();
	return 0;
}

int block_unit_exit(void)
{
	//*orig_virtio_queue_rq_hook = NULL;
	return 0;
}

