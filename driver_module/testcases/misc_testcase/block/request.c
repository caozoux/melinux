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
#include <linux/bio.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

void request_flag_dump(struct request *req)
{
	printk("start\n");
	if (req_op(req) == REQ_OP_READ)
	{
		printk("     REQ_OP_READ	/* read sectors from the device */	\n");
	}
	if (req_op(req) == REQ_OP_WRITE)
	{
		printk("     REQ_OP_WRITE	/* write sectors to the device */	\n");
	}
	if (req_op(req) == REQ_OP_FLUSH)
	{
		printk("     REQ_OP_FLUSH	/* flush the volatile write cache */	\n");
	}
	if (req_op(req) == REQ_OP_DISCARD)
	{
		printk("     REQ_OP_DISCARD	/* discard sectors */	\n");
	}
	if (req_op(req) == REQ_OP_ZONE_REPORT)
	{
		printk("     REQ_OP_ZONE_REPORT/* get zone information */	\n");
	}
	if (req_op(req) == REQ_OP_SECURE_ERASE)
	{
		printk("     REQ_OP_SECURE_ERASE/* securely erase sectors */	\n");
	}
	if (req_op(req) == REQ_OP_ZONE_RESET)
	{
		printk("     REQ_OP_ZONE_RESET/* seset a zone write pointer */	\n");
	}
	if (req_op(req) == REQ_OP_WRITE_SAME)
	{
		printk("     REQ_OP_WRITE_SAME/* write the same sector many times */	\n");
	}
	if (req_op(req) == REQ_OP_WRITE_ZEROES)
	{
		printk("     REQ_OP_WRITE_ZEROES/* write the zero filled sector many times */	\n");
	}
	if (req_op(req) == REQ_OP_SCSI_IN)
	{
		printk("     REQ_OP_SCSI_IN	/* SCSI passthrough using struct scsi_request */	\n");
	}
	if (req_op(req) == REQ_OP_SCSI_OUT)
	{
		printk("     REQ_OP_SCSI_OUT	/* SCSI passthrough using struct scsi_request */	\n");
	}
	if (req_op(req) == REQ_OP_DRV_IN)
	{
		printk("     REQ_OP_DRV_IN	/* Driver private requests */	\n");
	}
	if (req_op(req) == REQ_OP_DRV_OUT)
	{
		printk("     REQ_OP_DRV_OUT	/* Driver private requests */	\n");
	}
	if (req->cmd_flags & REQ_FAILFAST_DEV) {
		printk("     REQ_FAILFAST_DEV /* no driver retries of device errors */\n");
	}
	if (req->cmd_flags & REQ_FAILFAST_TRANSPORT) {
		printk("     REQ_FAILFAST_TRANSPORT /* no driver retries of transport errors */\n");
	}
	if (req->cmd_flags & REQ_FAILFAST_DRIVER) {
		printk("     REQ_FAILFAST_DRIVER /* no driver retries of driver errors */\n");
	}
	if (req->cmd_flags & REQ_SYNC) {
		printk("     REQ_SYNC /* request is sync (sync write or read) */\n");
	}
	if (req->cmd_flags & REQ_META) {
		printk("     REQ_META /* metadata io request */\n");
	}
	if (req->cmd_flags & REQ_PRIO) {
		printk("     REQ_PRIO /* boost priority in cfq */\n");
	}
	if (req->cmd_flags & REQ_NOMERGE) {
		printk("     REQ_NOMERGE /* don't touch this for merging */\n");
	}
	if (req->cmd_flags & REQ_IDLE) {
		printk("     REQ_IDLE /* anticipate more IO after this one */\n");
	}
	if (req->cmd_flags & REQ_INTEGRITY) {
		printk("     REQ_INTEGRITY /* I/O includes block integrity payload */\n");
	}
	if (req->cmd_flags & REQ_FUA) {
		printk("     REQ_FUA /* forced unit access */\n");
	}
	if (req->cmd_flags & REQ_PREFLUSH) {
		printk("     REQ_PREFLUSH /* request for cache flush */\n");
	}
	if (req->cmd_flags & REQ_RAHEAD) {
		printk("     REQ_RAHEAD /* read ahead, can fail anytime */\n");
	}
	if (req->cmd_flags & REQ_BACKGROUND) {
		printk("     REQ_BACKGROUND /* background IO */\n");
	}
	if (req->cmd_flags & REQ_NOWAIT) {
		printk("     REQ_NOWAIT /* Don't wait if request will block */\n");
	}
	if (req->cmd_flags & REQ_NOUNMAP) {
		printk("     REQ_NOUNMAP /* do not free blocks when zeroing */\n");
	}
	if (req->cmd_flags & REQ_DRV) {
		printk("     REQ_DRV /* for driver use */\n");
	}
	if (req->cmd_flags & REQ_SWAP) {
		printk("     REQ_SWAP /* swapping request. */\n");
	}
	printk("end\n");
}

static void dump_request_queue(struct request_queue *req_queue)
{

}

static void dump_request(struct request *req)
{
	 struct bio_vec iv;
	 struct bvec_iter iter;
#if 0
	if ((req->cmd_flags & REQ_OP_WRITE) && (req->cmd_flags & REQ_BACKGROUND)
		 
		) {
		printk("write|blockgroup");
		//dump_stack();
	}
#endif
	printk("len:%lx sector:%lx\n", (unsigned long)req->__data_len, (unsigned long)req->__sector);
	printk("cmd_flags:%lx rq_flags:%lx ", (unsigned long)req->cmd_flags, (unsigned long)req->rq_flags);
	printk("bio:%lx bio_tail:%lx ", (unsigned long)req->bio, (unsigned long)req->biotail);
	if (req->bio)
		bio_dump_data(req->bio);
	request_flag_dump(req);

		for_each_bio(req->bio)
			bio_for_each_segment(iv, req->bio, iter)
				printk("iv_len:%x\n", iv.bv_len);

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

