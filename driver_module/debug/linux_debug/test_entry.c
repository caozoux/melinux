#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

int debug=0;
char help_str_err[]="not support";
char help_str[]="h help \n"
		"s write slab info \n"
		"D force curretn process into D state \n"
		"s force curretn process into spiblock \n"
		;

#define ARRT_MARCO(name) DEVICE_ATTR(name, S_IWUSR | S_IRUGO, read_##name, write_##name);

#define ARRT_MARCO_READ(name)  \
	static ssize_t read_##name(struct device *dev,struct device_attribute *attr, char *buf) \
	{ \
		ssize_t size;\
		size = sprintf(buf, "%d\n", name);\
		return size;\
	}

#define ARRT_MARCO_WRITE(name)  \
	static ssize_t write_##name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)\
	{\
		unsigned long val;\
		if (!kstrtoul(buf, 0, &val)) {\
			name = val;\
		}\
		return count; \
	}

#define MISC_NAME "misc_blocktest"

static int enable;

struct misc_private_data {
	int flag;
};

struct ping_data {
	short unsigned int  s_send;
	short unsigned int  s_recv;
};

static int misc_template_open(struct inode *inode, struct file * file)
{

	struct misc_private_data *data;
	data = kzalloc(sizeof(struct misc_private_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}
	file->private_data = (void *) data;
	return 0;
}

static ssize_t misc_template_read(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t misc_template_write(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	char data[128];
	void *slab_buf;
	if (copy_from_user(data, (char __user *) buf, 128)) {
		goto failed1;
	}

	printk("zz size:%d %c\n", size, data[0]);
	switch (data[0]) {
		case 'h':
			printk("%s\n", help_str);
			break;
		case 'D':
			slab_buf=kmalloc(4096, GFP_KERNEL);
			if (debug) {
				printk("malloc slab 4k\n");
			}
			break;

		case 'd':
			if (debug)
				debug=0;
			else
				debug=1;
			break;

		default:
			if (copy_to_user((char __user *) buf, help_str_err, sizeof(help_str_err))) {
				goto failed1;
			}
			break;

	}
	return size;

failed1:
	return -ENOMEM;
}

static int misc_template_release (struct inode *inode, struct file *file)
{
	return 0;
}

static long misc_template_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{

	struct ping_data data;
	int ret = 0;

	data.s_send = 3;
	data.s_recv= 4;
	if (copy_to_user((char __user *) arg, &data, sizeof(struct ping_data))) {
		ret = -EFAULT;
		goto OUT;
	}

OUT:
	return ret;
}

struct file_operations misc_temp_ops = {
	.open = misc_template_open,
	.read = misc_template_read,
	.write = misc_template_write,
	.unlocked_ioctl =misc_template_unlocked_ioctl,
	.release = misc_template_release
};

struct miscdevice  misc_dev = {
	.name = MISC_NAME,
	.fops = &misc_temp_ops,
};

ARRT_MARCO_READ(enable);
ARRT_MARCO_WRITE(enable);
ARRT_MARCO(enable);

static int __init miscdriver_init(void)
{
	int ret;
	if (misc_register(&misc_dev)) {
		pr_err(" misc register err\n");
		return 1;
	}

	device_create_file(misc_dev.this_device, &dev_attr_enable);
	printk("miscdriver load \n");
	return 0;
}

static void __exit miscdriver_exit(void)
{
	device_remove_file(misc_dev.this_device, &dev_attr_enable);
	misc_deregister(&misc_dev);
	printk("miscdriver unload \n");
}

module_init(miscdriver_init);
module_exit(miscdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
