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
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

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

#define MISC_NAME "misc_template"

static int enable;

struct misc_private_data {
	int flag;		
	const struct device *dev;
	struct workqueue_struct  *thread_wq;
};

static struct misc_private_data *misc_data;

static int misc_template_open(struct inode *inode, struct file * file)
{

	static struct misc_private_data *data;
	data = kzalloc(sizeof(struct misc_private_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	file->private_data = (void *) data;
	return 0;
}

static ssize_t misc_template_read(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t misc_template_write(struct file *file, const char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static int misc_template_release (struct inode *inode, struct file *file)
{
	static struct misc_private_data *data;

	data = (struct misc_private_data *)file->private_data;
	if (data)
		kfree(data);

	return 0;
}

static long misc_template_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{

	int ret = 0;
	struct ioctl_data data;
	struct misc_private_data  *dev_data;

	dev_data = (struct misc_private_data *) file->private_data;
	if (copy_from_user(&data, (char __user *) arg, sizeof(struct ioctl_data))) {
		dev_err(dev_data->dev, "cmd %d copy err\n", cmd);
		ret = -EFAULT;	
		goto OUT;
	}

	DEBUG("ioctl cmd:%d\n", data.type);

	switch (data.type) {

		case  IOCTL_USERMAP:
			page_ioctl_func(cmd, arg);
			break;

		case  IOCTL_USERCU:
			rcu_ioctl_func(cmd, arg, &data);
			break;

		case  IOCTL_USEKPROBE:
			kprobe_ioctl_func(cmd, arg, &data);
			break;

		case  IOCTL_USEWORKQUEUE:
			workqueue_ioctl_func(cmd, arg, &data);
			break;

		case  IOCTL_HARDLOCK:
			locktest_ioctl_func(cmd, arg, &data);
			break;

		case  IOCTL_USEREXT2:
			ext2test_ioctl_func(cmd, arg, &data);

		case  IOCTL_USEATOMIC:
			atomic_ioctl_func(cmd, arg, &data);

		default:
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

	if (kmemt_unit_init()) {
		pr_err("atomic init failed\n");
		goto out0;
	}

	if (atomic_init()) {
		pr_err("atomic init failed\n");
		goto out0;
	}

	if (devbusdrvtest_init()) {
		pr_err("devbusdrvtest_init failed\n");
		goto out0;
	}

	if (ext2test_init()) {
		pr_err("ext2test_init failed\n");
		goto out0;
	}

	if(msr_init()) {
		pr_err("msr_init failed\n");
		goto out0;
	}

	if(rcutest_init()) {
		pr_err("rcutest_init failed\n");
		goto out0;
	}

	if(kprobe_init()) {
		pr_err("kprobe_init failed\n");
		goto out0;
	}

	if(showstack_init()) {
		pr_err("showstack_init failed\n");
		goto out0;
	}

	if(workqueue_test_init()) {
		pr_err("showstack_init failed\n");
		goto out0;
	}


#if 0
	if(locktest_init()) {
		pr_err("locktest_init failed\n");
		goto out0;
	}
#endif

	if(raidtree_init()) {
		pr_err("locktest_init failed\n");
		goto out0;
	}

	misc_data = kzalloc(sizeof(struct misc_private_data), GFP_KERNEL);
	if (!misc_data) {
		return -ENOMEM;
	}

	ret = misc_register(&misc_dev);
	if (ret) {
		pr_err(" misc register err\n");
		goto out1;
	}

	ret = device_create_file(misc_dev.this_device, &dev_attr_enable);
	if (ret)
		goto out2;

	misc_data->dev = misc_dev.this_device;

	printk("miscdriver load \n");

	return 0;

#if 0
out3:
	device_remove_file(misc_dev.this_device, &dev_attr_enable);
#endif

out2:
	misc_deregister(&misc_dev);
out1:
	kfree(misc_data);
	return ret;
out0:
	ret = -EINVAL;
	return ret;
}

static void __exit miscdriver_exit(void)
{
	kmem_unit_exit();
	atomic_exit();
	devbusdrvtest_exit();
	raidtree_exit();
	ext2test_exit();
	workqueue_test_exit();
	//misctest_workquere_exit();
	device_remove_file(misc_dev.this_device, &dev_attr_enable);
	msr_exit();
	misc_deregister(&misc_dev);
	kfree(misc_data);
	printk("miscdriver unload \n");
}

module_init(miscdriver_init);
module_exit(miscdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

