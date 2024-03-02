#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/file.h>
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

#define LOOKUP_SYMS(name) do {                          \
       orig_##name = (void *)kallsyms_lookup_name(#name);      \
       if (!orig_##name) {                     \
           pr_err("kallsyms_lookup_name: %s\n", #name);        \
           return -EINVAL;                     \
       }                               \
   } while (0)

static char file_name[256];
void (*orig_blk_mq_queue_tag_busy_iter)(struct request_queue *q, busy_iter_fn *fn,  void *priv);
struct list_head *orig_super_blocks;
spinlock_t *orig_sb_lock;
struct proc_dir_entry *proc_lock;
struct proc_dir_entry *proc_unlock;

int sym_init(void)
{
	return 0;
}

static int show_reqinfo(struct seq_file *m, void *v)
{
	seq_printf(m, " \n");
	return 0;
}

static int file_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, show_reqinfo, NULL);
}

static ssize_t lock_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct file *open_file;
	int off;
	struct inode *d_inode;
	unsigned long i_ino;

	off = count >256 ? 256 : count;
	if (copy_from_user(file_name, buf, off))
		return -EINVAL;

	file_name[off-1] = 0;

	open_file = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(open_file))
		return PTR_ERR(open_file);

	d_inode = open_file->f_path.dentry->d_inode;
	i_ino = d_inode->i_ino;
	inode_lock(d_inode);
	printk("d_inode:%lx lock\n", (unsigned long)d_inode);

	fput(open_file);
	return count;
}

const struct file_operations lock_fops = {
	.open = file_open,
	.read = seq_read,
	.write = lock_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int unlock_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, show_reqinfo, NULL);
}

static ssize_t unlock_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct file *open_file;
	int off;
	struct inode *d_inode;
	unsigned long i_ino;

	off = count >256 ? 256 : count;
	if (copy_from_user(file_name, buf, off))
		return -EINVAL;

	file_name[off-1] = 0;

	open_file = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(open_file))
		return PTR_ERR(open_file);

	d_inode = open_file->f_path.dentry->d_inode;
	i_ino = d_inode->i_ino;
	inode_unlock(d_inode);
	printk("d_inode:%lx unlock\n", (unsigned long)d_inode);

	fput(open_file);
	return count;
}

const struct file_operations unlock_fops = {
	.open = file_open,
	.read = seq_read,
	.write = unlock_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init block_dump_init(void)
{

    if (sym_init()) {
    	return -EINVAL;
	}

	proc_lock = proc_create("file_monitor_lock", S_IRUSR | S_IWUSR, NULL, &lock_fops);
	proc_unlock = proc_create("file_monitor_unlock", S_IRUSR | S_IWUSR, NULL, &unlock_fops);

	return 0;
}

static void __exit block_dump_exit(void)
{
  proc_remove(proc_lock);
  proc_remove(proc_unlock);
  //hrtimer_pr_exit();
}

module_init(block_dump_init);
module_exit(block_dump_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
