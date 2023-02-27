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

typedef struct {
	char *name;
	void *func;
	int enable;
} ktrace_array_t;

static LIST_HEAD(trace_head);

static void blk_add_trace_rq_insert(void *ignore,
                    struct request_queue *q, struct request *rq)
{
	//blk_add_trace_rq(rq, 0, blk_rq_bytes(rq), BLK_TA_INSERT,
	//        blk_trace_request_get_cgid(q, rq));
	printk("zz %s %d \n", __func__, __LINE__);
}

static void block_balance_dirty_pages(void *ignore,
			unsigned long thresh, unsigned long bg_thresh,
			unsigned long dirty, unsigned long bdi_thresh,
			unsigned long bdi_dirty, unsigned long dirty_ratelimit,
			unsigned long task_ratelimit, unsigned long dirtied,
			unsigned long period, long pause,
			unsigned long start_time)
{
	printk("zz %s pause:%lx \n",__func__, (unsigned long)pause);
}

ktrace_array_t  trace_list[] = {
	//{"block_rq_insert", (void*)blk_add_trace_rq_insert, 0},
	{"balance_dirty_pages", (void*)block_balance_dirty_pages, 0},
};



int kblock_trace_init(void)
{
	int ret, i; 
	for (i = 0; i < ARRAY_SIZE(trace_list); ++i) {
		ret = register_tracepoint(trace_list[i].name, blk_add_trace_rq_insert, NULL);
		printk("zz %s ret:%lx \n",__func__, (unsigned long)ret);
		if (!ret)
			trace_list[i].enable = 1;
	}

	return 0;
}

int kblock_trace_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(trace_list); ++i) {
		if (trace_list[i].enable) {
			unregister_tracepoint(trace_list[i].name, blk_add_trace_rq_insert, NULL);
			trace_list[i].enable = 0;
		}
	}
	return 0;
}

