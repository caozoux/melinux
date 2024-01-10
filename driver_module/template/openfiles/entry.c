#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/file.h>

static ssize_t trace_latency_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *ppos)
{
	struct file *open_file;
	char filename[128];

	memset(filename, 0, 128);

    if (copy_from_user(filename, buf, count < 127 ? count: 127)) {
        return -EINVAL;
	}

	filename[count-1] = 0;
	if (!*filename)
        return -EINVAL;

	open_file = filp_open(filename, O_RDONLY, 0);
	 if (IS_ERR(open_file))
		 return PTR_ERR(open_file);

	printk("zz %s f_path:%lx \n",__func__, (unsigned long)&open_file->f_path);

    return count;
}
 
static int trace_latency_show(struct seq_file *m, void *v)
{       
    //seq_printf(m, "trace_irqoff_latency: %llums\n\n",
    //        trace_irqoff_latency/(1000 * 1000UL));
        
    //seq_puts(m, " wakeup sched:\n");
    //trace_latency_show_one(m, v);

    seq_putc(m, '\n');
            
    return 0;  
}              

static int trace_latency_open(struct inode *inode, struct file *file)
{
    return single_open(file, trace_latency_show, inode->i_private);
}

static const struct file_operations file_fops = {
    .owner      = THIS_MODULE,
    .open       = trace_latency_open,
    .read       = seq_read,
    .write      = trace_latency_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};  

static int __init gpiodriver_init(void)
{
	struct proc_dir_entry *parent_dir;
	parent_dir = proc_mkdir("modulefiles", NULL);
	if (!parent_dir)
		return -EINVAL;

	if (!proc_create("filename", S_IRUSR | S_IWUSR, parent_dir,
				 &file_fops))
		goto remove_proc;

	return 0;

remove_proc:
	remove_proc_subtree("trace_irqoff", NULL);
	return -EINVAL;
}

static void __exit gpiodriver_exit(void)
{
	remove_proc_subtree("modulefiles", NULL);
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

