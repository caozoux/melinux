#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>

#define MEWAITMISC 191 
#define WAITQUEUE_MISC_END 1024
DECLARE_WAIT_QUEUE_HEAD(waitqueue_head);

static loff_t waitqueue_llseek(struct file *file, loff_t offset, int origin)
{
	switch (origin) {
	case 0:
		/* nothing to do */
		break;
	case 1:
		offset += file->f_pos;
		break;
	case 2:
		offset += WAITQUEUE_MISC_END;
		break;
	default:
		return -EINVAL;
	}

	return (offset >= 0) ? (file->f_pos = offset) : -EINVAL;
}

static ssize_t waitqueue_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	char  contents[64];

	if (copy_from_user(contents, buf, count))
		return -EFAULT;
	return count;
}

static ssize_t waitqueue_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	char  contents[64];
	if (copy_to_user(buf, contents, count))
		return -EFAULT;
	return 64;
}

static int waitqueue_open(struct inode *inode, struct file *file)
{
	return 0;
}

static long waitqueue_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	return 0;
}

static int waitqueue_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations waitqueue_fops = {
	.owner		= THIS_MODULE,
	.llseek		= waitqueue_llseek,
	.read		= waitqueue_read,
	.write		= waitqueue_write,
	.unlocked_ioctl	= waitqueue_ioctl,
	.open		= waitqueue_open,
	.release	= waitqueue_release,
};

static struct miscdevice waitqueue_dev = {
	MEWAITMISC,
	"mewaitqueue",
	&waitqueue_fops
};

static int waitqueue_proc_read(struct seq_file *seq, void *offset)
{
	//mach_proc_infos(contents, seq, offset);

	seq_printf(seq, "Checksum status: %svalid\n", "test");
	return 0;
}

static int waitqueue_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, waitqueue_proc_read, NULL);
}

static const struct file_operations waitqueue_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= waitqueue_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int waitqueue_add_proc_fs(void)
{
#if 0
	if (!proc_create("driver/waitqueue", 0, NULL, &waitqueue_proc_fops))
		return -ENOMEM;
#endif
	return 0;
}

static int waitqueue_mis_init(void)
{
	int ret;
	ret = misc_register(&waitqueue_dev);
	if (ret) {
		pr_err(" misc register error\n");
		goto out;
	}
	ret = waitqueue_add_proc_fs();
out:
	return ret;
}

static int __init waitqueuedriver_init(void)
{
	printk("zz %s \n", __func__);
	waitqueue_mis_init();
	return 0;
}

static void __exit waitqueuedriver_exit(void)
{
	printk("zz %s \n", __func__);
	//remove_proc_entry("driver/waitqueue", NULL);
	misc_deregister(&waitqueue_dev);
}

module_init(waitqueuedriver_init);
module_exit(waitqueuedriver_exit);

//module_param(dump, bool, S_IRUSR | S_IWUSR);
//MODULE_PARM_DESC(dump, "Enable sdio read/write dumps.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zoucao <zoucaox@outlook.com>");

