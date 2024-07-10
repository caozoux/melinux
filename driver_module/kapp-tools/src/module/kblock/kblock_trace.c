#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <ksioctl/kblock_ioctl.h>

#include "hotfix_util.h"
#include "ktrace.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kblock_local.h"

typedef struct {
	char *name;
	void *func;
	int enable;
} ktrace_array_t;

static LIST_HEAD(trace_head);

static void kbtrace_blk_rq_insert(void *ignore, struct request_queue *q, struct request *rq)
{
	trace_printk("zz rq:%lx\n", (unsigned long)rq);
}

static void block_balance_dirty_pages(void *ignore,
			unsigned long thresh, unsigned long bg_thresh,
			unsigned long dirty, unsigned long bdi_thresh,
			unsigned long bdi_dirty, unsigned long dirty_ratelimit,
			unsigned long task_ratelimit, unsigned long dirtied,
			unsigned long period, long pause,
			unsigned long start_time)
{
	trace_printk("zz \n");
}

static void kbtrace_block_bio_queue(void *ignore, struct request_queue *q, struct bio *bio)
{
	trace_printk("sector:%ld block:%d\n", bio->bi_iter.bi_sector , bio_sectors(bio));
	printk("sector:%ld block:%d\n", bio->bi_iter.bi_sector , bio_sectors(bio));
	dump_stack();
}

static void kbtrace_block_bio_backmerge(void *ignore, struct request_queue *q, struct request *rq, struct bio *bio)
{
	trace_printk("sector:%ld block:%d\n", bio->bi_iter.bi_sector , bio_sectors(bio));
}

static void kbtrace_block_bio_fontmerge(void *ignore, struct request_queue *q, struct request *rq, struct bio *bio)
{
	trace_printk("sector:%ld block:%d\n", bio->bi_iter.bi_sector , bio_sectors(bio));
}

static void kbtrace_block_bio_complete(void *ignore, struct request_queue *q, struct bio *bio, int error)
{
	trace_printk("zz \n");
}

static void kbtrace_block_rq_complete(void *ignore, struct request *rq, int error, unsigned int nr_bytes)
{
	struct bio *bio_iter;

	bio_iter = rq->bio;

	trace_printk("rq:%lx sector:%ld block:%d error:%d\n", (unsigned long)rq, blk_rq_pos(rq), nr_bytes>>9, error);

#if 0
	printk("zz %s %d +\n", __func__, __LINE__);
	while(bio_iter) {
		printk("zz %s bio:%lx \n",__func__, (unsigned long)bio_iter);
		bio_iter = bio_iter->bi_next;
	}
	printk("zz %s %d -\n", __func__, __LINE__);
#endif
}

static void kbtrace_block_rq_issue(void *ignore, struct request_queue *q, struct request *rq)
{
	trace_printk("rq:%lx %d\n", (unsigned long)rq, blk_rq_pos(rq));
}

static void kbtrace_block_rq_requeue(void *ignore, struct request_queue *q, struct request *rq)
{
	trace_printk("rq:%lx\n", (unsigned long)rq);
}

/*
block_bio_backmerge
block_bio_complete
block_bio_queue  
block_dirty_buffer  
block_plug         
block_rq_insert  
block_rq_remap    
block_sleeprq  
block_touch_buffer  enable
block_bio_bounce     
block_bio_frontmerge  
block_bio_remap  
block_getrq         
block_rq_complete  
block_rq_issue   
block_rq_requeue  
block_split
block_unplug
*/
ktrace_array_t  trace_list[] = {
	//{"block_rq_insert", (void*)kbtrace_block_rq_insert, 0},
	{"balance_dirty_pages", (void*)block_balance_dirty_pages, 0},
	{"block_bio_backmerge", (void*)kbtrace_block_bio_backmerge,0},
	{"block_bio_frontmerge  ", (void*)kbtrace_block_bio_fontmerge,0},
	{"block_bio_complete", (void*)kbtrace_block_bio_complete,0},
	{"block_bio_queue", (void*)kbtrace_block_bio_queue,0},
	{"block_rq_complete", (void*)kbtrace_block_rq_complete, 0},
	{"block_rq_issue", (void*)kbtrace_block_rq_issue, 0},
	{"block_rq_requeue", (void*)kbtrace_block_rq_requeue, 0},
#if 0
	{"block_dirty_buffer  ", (void*)aa,0},
	{"block_plug         ", (void*)aa,0},
	{"block_rq_remap    ", (void*)aa,0},
	{"block_sleeprq  ", (void*)aa,0},
	{"block_touch_buffer", (void*)aa,0},
	{"block_bio_bounce     ", (void*)aa,0},
	{"block_bio_remap  ", (void*)aa,0},
	{"block_getrq         ", (void*)aa,0},
	{"block_split", (void*)aa,0},
	{"block_unplug", (void*)aa,0},
#endif
};



int kblock_trace_init(void)
{
	int ret, i; 
	for (i = 0; i < ARRAY_SIZE(trace_list); ++i) {
		//ret = register_tracepoint(trace_list[i].name, blk_add_trace_rq_insert, NULL);
		ret = register_tracepoint(trace_list[i].name, trace_list[i].func, NULL);
		printk("zz %s name:%lx \n",__func__, (unsigned long)trace_list[i].name);
		if (!ret) 
			trace_list[i].enable = 1;
		else
			ERR("%s trace register err\n", trace_list[i].name);
	}

	return 0;
}

int kblock_trace_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(trace_list); ++i) {
		if (trace_list[i].enable) {
			unregister_tracepoint(trace_list[i].name, trace_list[i].func, NULL);
			trace_list[i].enable = 0;
		}
	}
	return 0;
}

