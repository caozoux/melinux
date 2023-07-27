#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dcache.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

struct blk_mq_tags {
    unsigned int nr_tags;
    unsigned int nr_reserved_tags;

    atomic_t active_queues;

    struct sbitmap_queue bitmap_tags;
    struct sbitmap_queue breserved_tags;

    struct request **rqs;
    struct request **static_rqs;
    struct list_head page_list;
};

void (*orig_blk_mq_queue_tag_busy_iter)(struct request_queue *q, busy_iter_fn *fn,  void *priv);
struct list_head *orig_super_blocks;
spinlock_t *orig_sb_lock;

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

  orig_blk_mq_queue_tag_busy_iter = (void *)cust_kallsyms_lookup_name("blk_mq_queue_tag_busy_iter");
  if (!orig_blk_mq_queue_tag_busy_iter)
	return -EINVAL;

  orig_sb_lock = (void *)cust_kallsyms_lookup_name("sb_lock");
  if (!orig_sb_lock)
	return -EINVAL;

  orig_super_blocks = (void *)cust_kallsyms_lookup_name("super_blocks");
  if (!orig_super_blocks)
	return -EINVAL;
  return 0;
}

static int hrtimer_pr_init(void)
{
	return 0;
}

static void hrtimer_pr_exit(void)
{
}

#define REQ_OP_NAME(name) [REQ_OP_##name] = #name
static const char *const op_name[] = {
    REQ_OP_NAME(READ),
    REQ_OP_NAME(WRITE),
    REQ_OP_NAME(FLUSH),
    REQ_OP_NAME(DISCARD),
    REQ_OP_NAME(ZONE_REPORT),
    REQ_OP_NAME(SECURE_ERASE),
    REQ_OP_NAME(ZONE_RESET),
    REQ_OP_NAME(WRITE_SAME),
    REQ_OP_NAME(WRITE_ZEROES),
    REQ_OP_NAME(SCSI_IN),
    REQ_OP_NAME(SCSI_OUT),
    REQ_OP_NAME(DRV_IN),
    REQ_OP_NAME(DRV_OUT),
};

#define CMD_FLAG_NAME(name) [__REQ_##name] = #name
static const char *const cmd_flag_name[] = {
    CMD_FLAG_NAME(FAILFAST_DEV),
    CMD_FLAG_NAME(FAILFAST_TRANSPORT),
    CMD_FLAG_NAME(FAILFAST_DRIVER),
    CMD_FLAG_NAME(SYNC),
    CMD_FLAG_NAME(META),
    CMD_FLAG_NAME(PRIO),
    CMD_FLAG_NAME(NOMERGE),
    CMD_FLAG_NAME(IDLE),
    CMD_FLAG_NAME(INTEGRITY),
    CMD_FLAG_NAME(FUA),
    CMD_FLAG_NAME(PREFLUSH),
    CMD_FLAG_NAME(RAHEAD),
    CMD_FLAG_NAME(BACKGROUND),
    CMD_FLAG_NAME(NOUNMAP),
    CMD_FLAG_NAME(NOWAIT),
};

#define RQF_NAME(name) [ilog2((__force u32)RQF_##name)] = #name
static const char *const rqf_name[] = {
    RQF_NAME(SORTED),
    RQF_NAME(STARTED),
    RQF_NAME(QUEUED),
    RQF_NAME(SOFTBARRIER),
    RQF_NAME(FLUSH_SEQ),
    RQF_NAME(MIXED_MERGE),
    RQF_NAME(MQ_INFLIGHT),
    RQF_NAME(DONTPREP),
    RQF_NAME(PREEMPT),
    RQF_NAME(COPY_USER),
    RQF_NAME(FAILED),
    RQF_NAME(QUIET),
    RQF_NAME(ELVPRIV),
    RQF_NAME(IO_STAT),
    RQF_NAME(ALLOCED),
    RQF_NAME(PM),
    RQF_NAME(HASHED),
    RQF_NAME(STATS),
    RQF_NAME(SPECIAL_PAYLOAD),
    RQF_NAME(ZONE_WRITE_LOCKED),
    RQF_NAME(MQ_POLL_SLEPT),
};

static const char *const blk_mq_rq_state_name_array[] = {
    [MQ_RQ_IDLE]        = "idle",
    [MQ_RQ_IN_FLIGHT]   = "in_flight",
    [MQ_RQ_COMPLETE]    = "complete",
};

static const char *blk_mq_rq_state_name(enum mq_rq_state rq_state)
{
    if (WARN_ON_ONCE((unsigned int)rq_state >=
             ARRAY_SIZE(blk_mq_rq_state_name_array)))
        return "(?)";
    return blk_mq_rq_state_name_array[rq_state];
}

static inline enum mq_rq_state blk_mq_rq_state(struct request *rq)
{
    return READ_ONCE(rq->state);
}

static int blk_flags_show(struct seq_file *m, const unsigned long flags,
              const char *const *flag_name, int flag_name_count)
{
    bool sep = false;
    int i;

    for (i = 0; i < sizeof(flags) * BITS_PER_BYTE; i++) {
        if (!(flags & BIT(i)))
            continue;
        if (sep)
            seq_puts(m, "|");
        sep = true;
        if (i < flag_name_count && flag_name[i])
            seq_puts(m, flag_name[i]);
        else
            seq_printf(m, "%d", i);
    }
    return 0;
}

static void blk_mq_debugfs_rq_hang_show(struct seq_file *m, struct request *rq)
{
    const struct blk_mq_ops *const mq_ops = rq->q->mq_ops;
    const unsigned int op = rq->cmd_flags & REQ_OP_MASK;
    struct bio *bio;
    struct bio_vec *bvec;
    int i;

    seq_printf(m, "%px {.op=", rq);
    if (op < ARRAY_SIZE(op_name) && op_name[op])
        seq_printf(m, "%s", op_name[op]);
    else
        seq_printf(m, "%d", op);
    seq_puts(m, ", .cmd_flags=");
    blk_flags_show(m, rq->cmd_flags & ~REQ_OP_MASK, cmd_flag_name,
               ARRAY_SIZE(cmd_flag_name));
    seq_puts(m, ", .rq_flags=");
    blk_flags_show(m, (__force unsigned int)rq->rq_flags, rqf_name,
               ARRAY_SIZE(rqf_name));
    seq_printf(m, ", .state=%s", blk_mq_rq_state_name(blk_mq_rq_state(rq)));
    seq_printf(m, ", .tag=%d, .internal_tag=%d", rq->tag,
           rq->internal_tag);
    seq_printf(m, ", .start_time_ns=%llu", rq->start_time_ns);
    seq_printf(m, ", .io_start_time_ns=%llu", rq->io_start_time_ns);
    seq_printf(m, ", .current_time=%llu", ktime_get_ns());

    bio = rq->bio;
    while (bio) {
        seq_printf(m, ", .bio = %px", bio);
        seq_printf(m, ", .sector = %lu, .len=%u",
                bio->bi_iter.bi_sector, bio->bi_iter.bi_size);
        seq_puts(m, ", .bio_pages = { ");
        bio_for_each_segment_all(bvec, bio, i) {
            struct page *page = bvec->bv_page;

            if (!page)
                continue;
            seq_printf(m, "%px ", page);
        }
        seq_puts(m, "}");
        bio = bio->bi_next;
    }
    //if (mq_ops->show_rq)
    //    mq_ops->show_rq(m, rq);
    seq_puts(m, "}\n");
    return;
}

static void blk_mq_check_rq_hang(struct blk_mq_hw_ctx *hctx,
        struct request *rq, void *priv, bool reserved)
{
    struct seq_file *m = priv;
    u64 now = ktime_get_ns();
    u64 duration;

    duration = div_u64(now - rq->start_time_ns, NSEC_PER_MSEC);
    if (duration < rq->q->rq_hang_threshold)
        return;

    /* See comments in blk_mq_check_expired() */
    if (!refcount_inc_not_zero(&rq->ref))
        return;

    duration = div_u64(now - rq->start_time_ns, NSEC_PER_MSEC);
    if (duration >= rq->q->rq_hang_threshold)
        blk_mq_debugfs_rq_hang_show(m, rq);

    //if (is_flush_rq(rq, hctx))
    //   rq->end_io(rq, 0);
    // else if (refcount_dec_and_test(&rq->ref))
    //    __blk_mq_free_request(rq);
}

void rq_hang_check(struct seq_file *m, void *data)
{
	struct request_queue *q = data;
	orig_blk_mq_queue_tag_busy_iter(q, blk_mq_check_rq_hang, m);
}

static int show_reqinfo(struct seq_file *m, void *v)
{
	struct super_block *sb;
	struct block_device *s_bdev;
	struct gendisk *gendisk;

	spin_lock(orig_sb_lock);
	list_for_each_entry(sb, orig_super_blocks, s_list) {
		struct blk_mq_tags *tags;
		struct request_queue *q;
		struct blk_mq_hw_ctx *hctx;
		int i,j;
		struct request *rq;
		//seq_printf(m, "sb:%lx \n", (unsigned long)sb);
		s_bdev = sb->s_bdev;
		if (!s_bdev)
			continue;

		printk("zz %s bd_queue:%lx  %s\n",__func__, (unsigned long)s_bdev->bd_queue, sb->s_id);
		rq_hang_check(m, s_bdev->bd_queue);
		q = s_bdev->bd_queue;
		printk("zz %s q:%lx nr_hw_queues:%lx \n",__func__, (unsigned long)q, (unsigned long)q->nr_hw_queues);
		printk("zz %s queue_hw_ctx:%lx \n",__func__, (unsigned long)q->queue_hw_ctx);
		for (i = 0; i < q->nr_hw_queues; ++i) {
			hctx = q->queue_hw_ctx[i];
			if (hctx->nr_ctx && hctx->tags) {
				printk("zz %s hctx:%lx \n",__func__, (unsigned long)hctx);
				tags = hctx->tags;
				printk("zz %s nr_tags:%lx nr_reserved_tags:%lx \n",__func__, (unsigned long)tags->nr_tags, (unsigned long)tags->nr_reserved_tags);
				for (j = 0;  j< tags->nr_tags; ++j) {
					struct request *rq;
					rq = tags->rqs[j];
					if (rq) {
						printk("zz %s rqs rq:%lx tag:%lx \n",__func__, (unsigned long)rq, (unsigned long)rq->tag);
					}
					rq = tags->static_rqs[j];
					if (rq) {
						printk("zz %s static rq:%lx tag:%lx \n",__func__, (unsigned long)rq, (unsigned long)rq->tag);
					}
				}
			}
		}
		//gendisk = s_bdev->bd_disk;
	}
	spin_unlock(orig_sb_lock);

   seq_printf(m, ", .bio = %lx\n", 0x24);
   return 0;

}


static int proc_reqinfo_init(void)
{
	proc_create_single("reqinfo", 0, NULL, show_reqinfo);
    return 0;
}

static int __init percpu_hrtimer_init(void)
{

    if (get_kallsyms_lookup_name())
    	return -EINVAL;

    if (sym_init())
    	return -EINVAL;

	proc_reqinfo_init();
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
  remove_proc_entry("reqinfo", NULL);
  //hrtimer_pr_exit();	
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
