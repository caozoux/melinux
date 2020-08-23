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

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

static void dump_request_queue(struct request_queue *req_queue)
{

}

static void dump_request(struct request *req)
{
	if ((req->cmd_flags & REQ_OP_WRITE) && (req->cmd_flags & REQ_BACKGROUND)
		 
		) {
		printk("write|blockgroup");
		//dump_stack();
	}

	//printk("cmd_flags:%lx rq_flags:%lx ", (unsigned long)req->cmd_flags, (unsigned long)req->rq_flags);
	//printk("len:%lx sector:%lx\n", (unsigned long)req->__data_len, (unsigned long)req->__sector);

	//printk("s_ns:%lx io_s_ns:%lx timeout:%lx \n",(unsigned long)req->start_time_ns, (unsigned long)req->io_start_time_ns, (unsigned long)req->timeout);
#if 0
	struct request_queue *q; 
	truct blk_mq_ctx *mq_ctx;
	int cpu;
	unsigned int cmd_flags;
	req_flags_t rq_flags;
	unsigned int __data_len;
	int tag;
	sector_t __sector;
	struct bio *bio;
	struct bio *biotail;
#endif
}

void merequest_list_dump(struct request *req)
{
	struct request *next_rq;
	int i;
	struct gendisk *rq_disk;

	rq_disk = req->rq_disk;
	if (!strcmp(rq_disk->disk_name, "vdb")) {
		//printk("zz %s req:%lx \n",__func__, (unsigned long)req);
		dump_request(req);
		list_for_each_entry(next_rq, &req->queuelist, queuelist) {
			printk("req:%lx %d@%lx \n", next_rq, req);
		}
	}
}

