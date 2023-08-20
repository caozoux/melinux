#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <trace/events/block.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

struct tracepoint *orig___tracepoint_block_rq_insert;
struct tracepoint *orig___tracepoint_balance_dirty_pages;

unsigned long (*cust_kallsyms_lookup_name)(const char *name);

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

static int symbol_walk_callback(void *data, const char *name,
        struct module *mod, unsigned long addr)
{
    if (strcmp(name, "kallsyms_lookup_name") == 0) {
        cust_kallsyms_lookup_name = (void *)addr;
        return addr;
    }

    return 0;
}

static int get_kallsyms_lookup_name(void)
{
    int ret;
    ksys_kallsyms_on_each_symbol = &kallsyms_on_each_symbol;
    ret = ksys_kallsyms_on_each_symbol(symbol_walk_callback, NULL);
    if (!ret || !cust_kallsyms_lookup_name)
        return -EINVAL;

    return 0;
}

int sym_init(void)
{
	LOOKUP_SYMS(__tracepoint_block_rq_insert);
	LOOKUP_SYMS(__tracepoint_balance_dirty_pages);
	return 0;
}

static void blk_tracepoint_balance_dirty_pages(void *ignore,
			struct bdi_writeback *wb,
			unsigned long dirty,
			unsigned long thresh,
			unsigned long bg_thresh,
			unsigned long avail,
			int memcg_id,
			unsigned long m_dirty,
			unsigned long m_thresh,
			unsigned long m_bg_thresh,
			unsigned long m_avail)
{
	trace_printk("zz\n");
}
static void blk_add_trace_rq_insert(void *ignore,
                    struct request_queue *q, struct request *rq)
{
	trace_printk("zz\n");
#if 0
    blk_add_trace_rq(rq, 0, blk_rq_bytes(rq), BLK_TA_INSERT,
             blk_trace_request_get_cgid(q, rq));
#endif
}

static int block_trace_init(void)
{

	int ret;
	ret = tracepoint_probe_register(orig___tracepoint_block_rq_insert, blk_add_trace_rq_insert, NULL);
	if (ret)
		goto out;

	ret = tracepoint_probe_register(orig___tracepoint_balance_dirty_pages, blk_tracepoint_balance_dirty_pages, NULL);
	if (ret)
		goto out1;

	return -1;

out1:
	tracepoint_probe_unregister(orig___tracepoint_block_rq_insert, blk_add_trace_rq_insert, NULL);
out:
	return ret;
}

static void block_trace_exit(void)
{
	tracepoint_probe_unregister(orig___tracepoint_block_rq_insert, blk_add_trace_rq_insert, NULL);
	tracepoint_probe_unregister(orig___tracepoint_balance_dirty_pages, blk_tracepoint_balance_dirty_pages, NULL);
}

static int __init percpu_hrtimer_init(void)
{
	if (get_kallsyms_lookup_name())
		return -EINVAL;

	if (sym_init())
 		return -EINVAL;

	block_trace_init();

	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
  block_trace_exit();	
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
