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

static char blk_name[128];
void (*orig_blk_mq_queue_tag_busy_iter)(struct request_queue *q, busy_iter_fn *fn,  void *priv);
struct list_head *orig_super_blocks;
spinlock_t *orig_sb_lock;
struct proc_dir_entry *blk_req_proc;

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

static void dump_tags_request(struct seq_file *m, struct blk_mq_tags *tags)
{
	int j;
	struct request *rq;

	if (!tags)
		return ;

	seq_printf(m, "nr_tags:%ld nr_reserved_tags:%ld \n", (unsigned long)tags->nr_tags, (unsigned long)tags->nr_reserved_tags);
	for (j = 0;  j< tags->nr_tags; ++j) {
		rq = tags->rqs[j];
		if (rq)
			seq_printf(m, "rqs rq:%lx tag:%ld internal_tag:%ld flags:%lx cmd_flags:%lx stat:%lx ref:%ld sec:%ld len:%ld\n", (unsigned long)rq, (unsigned long)rq->tag,
							(unsigned long)rq->internal_tag, (unsigned long)rq->rq_flags, (unsigned long)rq->cmd_flags,
							(unsigned long)rq->state, (unsigned long)refcount_read(&rq->ref), (unsigned long)rq->__sector, (unsigned long)rq->__data_len);
		rq = tags->static_rqs[j];
		if (rq)
			seq_printf(m, "static rq:%lx tag:%ld internal_tag:%ld flags:%lx cmd_flags:%lx stat:%lx ref:%ld sec:%ld len:%ld\n", (unsigned long)rq, (unsigned long)rq->tag,
							(unsigned long)rq->internal_tag, (unsigned long)rq->rq_flags, (unsigned long)rq->cmd_flags,
							(unsigned long)rq->state, (unsigned long)refcount_read(&rq->ref), (unsigned long)rq->__sector, (unsigned long)rq->__data_len);
	}
}

static void blk_mq_check_rq_hang(struct blk_mq_hw_ctx *hctx,
        struct request *rq, void *priv, bool reserved)
{
    u64 now = ktime_get_ns();
    u64 duration;
	struct seq_file *m = (struct seq_file *)priv;

    duration = div_u64(now - rq->start_time_ns, NSEC_PER_MSEC);
    seq_printf(m, "duration:%lx \n", (unsigned long)duration);
	seq_printf(m, "%s rqs rq:%lx tag:%ld internal_tag:%ld flags:%lx cmd_flags:%lx stat:%lx ref:%ld sec:%ld len:%ld\n",__func__, (unsigned long)rq, (unsigned long)rq->tag,
					(unsigned long)rq->internal_tag, (unsigned long)rq->rq_flags, (unsigned long)rq->cmd_flags,
					(unsigned long)rq->state, (unsigned long)refcount_read(&rq->ref), (unsigned long)rq->__sector, (unsigned long)rq->__data_len);
    //if (duration >= rq->q->rq_hang_threshold)
    //   blk_mq_debugfs_rq_hang_show(m, rq);

    //if (is_flush_rq(rq, hctx))
    //   rq->end_io(rq, 0);
    // else if (refcount_dec_and_test(&rq->ref))
    //    __blk_mq_free_request(rq);
}

void rq_hang_check(struct seq_file *m, void *data)
{
	struct request_queue *q = data;
	orig_blk_mq_queue_tag_busy_iter(q, (busy_iter_fn*)blk_mq_check_rq_hang, m);
}

static int show_reqinfo(struct seq_file *m, void *v)
{
	struct super_block *sb;
	struct block_device *s_bdev;

	spin_lock(orig_sb_lock);
	list_for_each_entry(sb, orig_super_blocks, s_list) {
		struct blk_mq_tags *tags;
		struct request_queue *q;
		struct blk_mq_hw_ctx *hctx;
		int i;

		s_bdev = sb->s_bdev;
		if (!s_bdev)
			continue;

		if (sb->s_id && strstr(sb->s_id, blk_name)) {
			printk("zz %s %d \n", __func__, __LINE__);
			q = s_bdev->bd_queue;
			seq_printf(m, "%s q:%lx nr_hw_queues:%lx elevator:%lx\n",sb->s_id, (unsigned long)q, (unsigned long)q->nr_hw_queues, (unsigned long)q->elevator);
			rq_hang_check(m, s_bdev->bd_queue);
			for (i = 0; i < q->nr_hw_queues; ++i) {
				hctx = q->queue_hw_ctx[i];
				if (hctx->nr_ctx && hctx->tags) {
					seq_printf(m, "hctx:%lx \n", (unsigned long)hctx);
					tags = hctx->tags;
					seq_printf(m, "tags:\n");
					dump_tags_request(m, tags);
					seq_printf(m, "sched tags:\n");
					dump_tags_request(m, hctx->sched_tags);
				}
			}
		}
	}
	spin_unlock(orig_sb_lock);

	seq_printf(m, " \n");
	return 0;
}

static int blk_req_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, show_reqinfo, NULL);
}

static ssize_t blk_req_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	int off;
	off = count >128 ? 128 : count;
	if (copy_from_user(blk_name, buf, off))
		return -EINVAL;

	blk_name[off-1] = 0;
	return count;
}

const struct file_operations blk_req_fops = {
	.open = blk_req_open,
	.read = seq_read,
	.write = blk_req_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init block_dump_init(void)
{

    if (get_kallsyms_lookup_name())
    	return -EINVAL;

    if (sym_init()) {
    	return -EINVAL;
	}

	blk_req_proc = proc_create("blk_req", S_IRUSR | S_IWUSR, NULL, &blk_req_fops);

	return 0;
}

static void __exit block_dump_exit(void)
{
  proc_remove(blk_req_proc);
  //hrtimer_pr_exit();
}

module_init(block_dump_init);
module_exit(block_dump_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
