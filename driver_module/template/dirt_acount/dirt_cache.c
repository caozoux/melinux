 #include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/blkdev.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include "internal.h"
#include "hotfix_util.h"

static DEFINE_SPINLOCK(kinject_lock);
static inline bool blk_do_io_stat(struct request *rq)
{
    return rq->rq_disk &&
           (rq->rq_flags & RQF_IO_STAT) &&
        !blk_rq_is_passthrough(rq);
}

struct mutex *orig_text_mutex;

//struct cpumask watchdog_allowed_mask __read_mostly;
unsigned long (*cust_kallsyms_lookup_name)(const char *name);
void (*orig_arch_trigger_cpumask_backtrace)(const cpumask_t *mask, bool exclude_self);
void *(*orig_text_poke_bp)(void *addr, const void *opcode, size_t len, void *handler);
struct cred *(*orig_get_task_cred)(struct task_struct *task);

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

DEFINE_ORIG_FUNC(void, account_page_dirtied, 2, struct page  *, page, struct address_space *, mapping);

long sda2_dirt_count=0;
long old_sda2_dirt_count=0;
long account_dirt=0;


struct pid_dirty_data {
	u64 tgid;
	u64 dirt_size;
};

#define MAX_RECORD 1000
struct pid_dirty_data *pid_dirt_array;
int offset;

struct hrtimer hrtimer_pr;

static enum hrtimer_restart hrtimer_pr_fun(struct hrtimer *hrtimer)
{
	long diff = (sda2_dirt_count > old_sda2_dirt_count ? sda2_dirt_count - old_sda2_dirt_count : old_sda2_dirt_count - sda2_dirt_count)/1024;
	int i;
	if (diff > 1000)
		trace_printk("zz Dirt:%ld Diff:%ld\n", sda2_dirt_count, diff);

	old_sda2_dirt_count = sda2_dirt_count;
	sda2_dirt_count = 0;

	spin_lock_irq(&kinject_lock);
	for (i = 0; i < offset; ++i) {
		struct pid_dirty_data *dirty_data = &pid_dirt_array[i];
		trace_printk("tgid:%lld write:%lld\n", dirty_data->tgid, dirty_data->dirt_size);
	}
	spin_unlock_irq(&kinject_lock);
	offset = 0;
	account_dirt = 0;
	memset(pid_dirt_array, 0 , sizeof(struct pid_dirty_data) * MAX_RECORD);

	hrtimer_forward_now(&hrtimer_pr, ns_to_ktime(1000000000));
	return HRTIMER_RESTART;
}

static int hrtimer_pr_init(void)
{
	hrtimer_init(&hrtimer_pr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_pr.function = hrtimer_pr_fun;
	hrtimer_start(&hrtimer_pr, ns_to_ktime(1000000000),
			HRTIMER_MODE_REL_PINNED);

	return 0;
}

static void hrtimer_pr_exit(void)
{
	hrtimer_cancel(&hrtimer_pr);
}

void new_account_page_dirtied(struct page *page, struct address_space *mapping)
{
	struct inode *inode = mapping->host;
	struct pid *pid = task_tgid(current);
	struct super_block *i_sb;
	struct dentry *dentry;
	const char *name = "?";
	const struct cred *cred;
	int i;
	u64 tgid;

	if (!inode || !inode->i_ino)
		goto out;

	if (!inode->i_sb)
		goto out;

	i_sb = inode->i_sb;

	//printk("zz %s s_id:%s \n",__func__, i_sb->s_id);
	//if (strncmp(i_sb->s_id, "sda2", 4))
	//	goto out;

	cred = orig_get_task_cred(current);
	if (!cred)
		goto out;

	sda2_dirt_count += PAGE_SIZE;
	account_dirt += 1;
	tgid = current->tgid;
	spin_lock_irq(&kinject_lock);
	if (offset < MAX_RECORD) {
		for (i = 0; i < MAX_RECORD; ++i) {
			struct pid_dirty_data *dirty_data = &pid_dirt_array[i];
			if ( dirty_data->tgid == tgid) {
				dirty_data->dirt_size += PAGE_SIZE;
				break;
			}

			if ( dirty_data->tgid == 0) {
				dirty_data->tgid = tgid;
				dirty_data->dirt_size += PAGE_SIZE;
				offset++;
				break;
			}
		}
	}
	spin_unlock_irq(&kinject_lock);

	sda2_dirt_count += PAGE_SIZE;

	goto out;

	dentry = d_find_alias(inode);
	if (dentry) {
		spin_lock(&dentry->d_lock);
		name = (const char *) dentry->d_name.name;
	} else
		goto out;

	trace_printk("%s \n",name);
	if (dentry) {
		spin_unlock(&dentry->d_lock);
		dput(dentry);
	}

	//mm = get_task_mm(tsk);

out:
    return old_account_page_dirtied(page, mapping);
}

DEFINE_ORIG_FUNC(void, blk_account_io_completion, 2, struct request *, req, unsigned int , bytes);
void new_blk_account_io_completion(struct request *req, unsigned int bytes)
{
	 if (blk_do_io_stat(req)) {			
		 struct hd_struct *part;
		 part_stat_lock();
		 part = req->part;
		 if (!strncmp(dev_name(&part->__dev), "sda2", 4)) {
		 	//sda2_dirt_count -= bytes;
		 	//trace_printk("%s %lx %lx dirty:%ld\n", dev_name(&part->__dev), bytes>>9, bytes, sda2_dirt_count);
		 }
		 part_stat_unlock();
	 }
	return  old_blk_account_io_completion(req, bytes);
}

void account_page_dirtied_trace(void *data, u64 *var, unsigned int var_ref_idx)
{
	struct test_data *trace_data;
	trace_data = (struct test_data *)var;
	printk("zz %s var:%lx var_ref_idx:%lx \n",__func__, (unsigned long)var, (unsigned long)var_ref_idx);

	//printk("zz %s var:%lx \n",__func__, (unsigned long)var[var_ref_idx]);
}

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
	LOOKUP_SYMS(text_mutex);
	LOOKUP_SYMS(text_poke_bp);
	LOOKUP_SYMS(account_page_dirtied);
	LOOKUP_SYMS(blk_account_io_completion);
	LOOKUP_SYMS(get_task_cred);

	return 0;
}

static int func_replace_init(void)
{
	JUMP_INIT(account_page_dirtied);
	JUMP_INIT(blk_account_io_completion);

	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_INSTALLWITHOLD(account_page_dirtied);
	JUMP_INSTALLWITHOLD(blk_account_io_completion);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	return 0;
}

static int func_replace_exit(void)
{
	get_online_cpus();
	mutex_lock(orig_text_mutex);
	JUMP_REMOVE(account_page_dirtied);
	JUMP_REMOVE(blk_account_io_completion);
	mutex_unlock(orig_text_mutex);
	put_online_cpus();
	return 0;
}

static int __init percpu_hrtimer_init(void)
{
	int ret;

	pid_dirt_array = kvmalloc(sizeof(struct pid_dirty_data) * MAX_RECORD, GFP_KERNEL);
	if (!pid_dirt_array)
		return -EINVAL;
	memset(pid_dirt_array, 0 , sizeof(struct pid_dirty_data) * MAX_RECORD);

	if (get_kallsyms_lookup_name())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	if (func_replace_init())
		return -EINVAL;
	hrtimer_pr_init();

	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
	func_replace_exit();
	printk("zz %s %d \n", __func__, __LINE__);
	hrtimer_pr_exit();
	kvfree(pid_dirt_array);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

