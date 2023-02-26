#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <ksioctl/kblock_ioctl.h>

#include "hotfix_util.h"
#include "ktrace.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kblock_local.h"

static void blk_add_trace_rq_insert(void *ignore,
                    struct request_queue *q, struct request *rq)
{
    //blk_add_trace_rq(rq, 0, blk_rq_bytes(rq), BLK_TA_INSERT,
     //        blk_trace_request_get_cgid(q, rq));
	 printk("zz %s %d \n", __func__, __LINE__);
}

int kblock_trace_init(void)
{
	int ret; 

	ret = register_tracepoint("block_rq_insert", blk_add_trace_rq_insert, NULL);
	if (ret)
		printk("Err: block_rq_insert trace failed\n");
	return 0;
}

int kblock_trace_exit(void)
{
	unregister_tracepoint("block_rq_insert", blk_add_trace_rq_insert, NULL);
	return 0;
}

