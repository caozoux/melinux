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
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#define PROC_PARENT "ctrlproc"

static int sampling_period_show(struct seq_file *m, void *ptr)
{
    seq_printf(m, "%llums\n", 10);

    return 0;
}

static int sampling_open(struct inode *inode, struct file *file)                                                                                                                                                                                 
{
    return single_open(file, sampling_period_show, inode->i_private);
}

static ssize_t ctrl_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *ppos)
{
    unsigned long val;

	if (kstrtoul_from_user(buf, count, 0, &val))
        return -EINVAL;

	return count;
}

static const struct file_operations sampling_period_fops = {
    .open       = sampling_open,
    .read       = seq_read,
    .write      = ctrl_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static int proc_dir_init(void)
{
	struct proc_dir_entry *parent_dir;
	parent_dir = proc_mkdir(PROC_PARENT, NULL);
	if (!parent_dir) 
		return -ENOMEM;

	if (!proc_create(PROC_PARENT, S_IRUSR | S_IWUSR, parent_dir,
             &sampling_period_fops))                                                                                                                                                                                                                      
        goto remove_proc;

	return 0;

remove_proc:
    remove_proc_subtree(PROC_PARENT, NULL);

    return -ENOMEM;
}

static void proc_dir_exit(void)
{
    remove_proc_subtree(PROC_PARENT, NULL);
}

static int __init entry_init(void)
{
	if (proc_dir_init())
		return -EINVAL;

	return 0;
}

static void __exit entry_exit(void)
{
	proc_dir_exit();
}

module_init(entry_init);
module_exit(entry_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
