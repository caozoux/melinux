#include <linux/init.h>
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
#include <linux/crc32.h>
#include <linux/module.h>
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

//#define MISC_UNIT(name)  struct misc_unit_item_##name = { 
#define MISC_UNIT(name, utype)  { \
		.u_name =	#name, \
		.type =	utype, \
		.ioctl = name##_unit_ioctl_func,	 \
		.init =	 name##_unit_init, \
		.exit =	 name##_unit_exit, \
	}

#define MISC_NAME "misc_template"

static int enable;

struct misc_private_data {
	int flag;		
	const struct device *dev;
	struct workqueue_struct  *thread_wq;
};

struct misc_uint_item unit_list[] =
{
	MISC_UNIT(locktest, IOCTL_LOCK),
	MISC_UNIT(kmem, IOCTL_USEKMEM),
	MISC_UNIT(atomic, IOCTL_USEATOMIC),
	MISC_UNIT(devbusdrvtest, IOCTL_USEDEVBUSDRV),
	MISC_UNIT(ext2test, IOCTL_USEEXT2),
	MISC_UNIT(msr, IOCTL_USEMSR),
	MISC_UNIT(rcutest, IOCTL_USERCU),
	MISC_UNIT(kprobe, IOCTL_USEKPROBE),
	MISC_UNIT(workqueue, IOCTL_USEWORKQUEUE),
	MISC_UNIT(hwpci, IOCTL_USEHWPCI),
	MISC_UNIT(statickey, IOCTL_USEHWPCI),
	MISC_UNIT(sched, IOCTL_USEHWPCI),
	MISC_UNIT(cpu, IOCTL_USEHWPCI),
	MISC_UNIT(block, IOCTL_USEBLOCK),
	MISC_UNIT(cpu, IOCTL_USEBLOCK),
	MISC_UNIT(ktime, IOCTL_USEKTIME),
	MISC_UNIT(page, IOCTL_USEMEM),
	MISC_UNIT(radixtree, IOCTL_USERAIDIXTREE),
	MISC_UNIT(krbtree, IOCTL_USEKRBTREE),
	MISC_UNIT(pci, IOCTL_PCI),
	MISC_UNIT(efi, IOCTL_PCI),
#ifdef CONFIG_ARM64
	//MISC_UNIT(arm64gic, IOCTL_USEBLOCK),
#endif
	{}
};

static struct misc_private_data *misc_data;

void crc32_test(void *buf, unsigned long size)
{
	crc32_le(0, buf, size);
}

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
	int i;

	dev_data = (struct misc_private_data *) file->private_data;

	if (copy_from_user(&data, (char __user *) arg, sizeof(struct ioctl_data))) {
		dev_err(dev_data->dev, "cmd %d copy err\n", cmd);
		ret = -EFAULT;	
		goto OUT;
	}

	DEBUG("ioctl cmd:%d cmdcode:%x\n", data.type, data.cmdcode);

	for(i=0; unit_list[i].type; i++) {
		if (unit_list[i].type == data.type) {
			unit_list[i].ioctl(cmd, arg, &data);
			if (copy_to_user(arg, &data,  sizeof(struct ioctl_data)))
				dev_err(dev_data->dev, "cmd %d copy err\n", cmd);

			break;
		}
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
	int i=0;
	int init_offset =0;

#if 0
	if (kmem_unit_init()) {
		pr_err("atomic init failed\n");
		goto out0;
	}

	if (atomic_unit_init()) {
		pr_err("atomic init failed\n");
		goto out0;
	}

	if (devbusdrvtest_unit_init()) {
		pr_err("devbusdrvtest_init failed\n");
		goto out0;
	}

	if (ext2test_unit_init()) {
		pr_err("ext2test_unit_init failed\n");
		goto out0;
	}

	if(msr_unit_init()) {
		pr_err("msr_unit_init failed\n");
		goto out0;
	}

	if(rcutest_unit_init()) {
		pr_err("rcutest_unit_init failed\n");
		goto out0;
	}

	if(kprobe_unit_init()) {
		pr_err("kprobe_unit_init failed\n");
		goto out0;
	}

	if(showstack_unit_init()) {
		pr_err("showstack_unit_init failed\n");
		goto out0;
	}

	if(workqueue_unit_init()) {
		pr_err("showstack_unit_init failed\n");
		goto out0;
	}

	if(locktest_init()) {
		pr_err("locktest_init failed\n");
		goto out0;
	}

	if(raidtree_unit_init()) {
		pr_err("locktest_init failed\n");
		goto out0;
	}
#else
	for(i=0; unit_list[i].type; i++) {
		if (unit_list[i].init()) {
			printk("%s init failed\n", unit_list[i].u_name);
			ret = -EINVAL;
			goto out0;
		}
		init_offset++;
	}
#endif

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
	for(i=0; i < init_offset; i++)
		unit_list[i].exit();

	ret = -EINVAL;
	return ret;
}

static void __exit miscdriver_exit(void)
{
	int i;
#if 1
	for(i=0; unit_list[i].type; i++)
		unit_list[i].exit();

#else

	kmem_unit_exit();
	atomic_unit_exit();
	devbusdrvtest_unit_exit();
	raidtree_unit_exit();
	ext2test_unit_exit();
	workqueue_unit_exit();
	msr_unit_exit();
#endif
	//misctest_workquere_exit();
	device_remove_file(misc_dev.this_device, &dev_attr_enable);
	misc_deregister(&misc_dev);
	kfree(misc_data);
	printk("miscdriver unload \n");
}

module_init(miscdriver_init);
module_exit(miscdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

