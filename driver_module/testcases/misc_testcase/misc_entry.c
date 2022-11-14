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
#include "mekernel.h"
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

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
int init_dump_info = 0;
static int enable;
struct mutex *orig_text_mutex;
TEXT_DECLARE()

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
	MISC_UNIT(panic, IOCTL_PANIC),
	MISC_UNIT(time, IOCTL_PANIC),
	MISC_UNIT(inject, IOCTL_INJECT),
	MISC_UNIT(kdiagnose, IOCTL_TRACE),
#ifdef CONFIG_ARM64
	//MISC_UNIT(arm64gic, IOCTL_USEBLOCK),
#endif
	{}
};

static struct misc_private_data *misc_data;

int golable_sysm_init(void)
{
	TEXT_SYMS()

	return 0;
}

static int extra_module_init(u64 address, int *init_offset)
{
	int i;

	cust_kallsyms_lookup_name = address;
	
	if (golable_sysm_init())
		return -EINVAL;

	for(i=0; unit_list[i].type; i++) {
		if (unit_list[i].init()) {
			printk("%s init failed\n", unit_list[i].u_name);
			return -EINVAL;
		}
		(*init_offset)++;
	}

	MEDEBUG("complete unit init\n");
	return  0;
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
	int i, init_offset = 0;

	dev_data = (struct misc_private_data *) file->private_data;

	if (copy_from_user(&data, (char __user *) arg, sizeof(struct ioctl_data))) {
		dev_err(dev_data->dev, "cmd %d copy err\n", cmd);
		ret = -EFAULT;	
		goto OUT;
	}

	MEDEBUG("ioctl cmd:%d cmdcode:%x\n", data.type, data.cmdcode);

	if (data.type == IOCTL_INIT) {
		if (extra_module_init(data.init_data.kallsyms_func, &init_offset)) {
			// some module init failed
			for(i=0; i < init_offset; i++)
				unit_list[i].exit();
		}
	} else {
		for(i=0; unit_list[i].type; i++) {
			if (unit_list[i].type == data.type) {

				unit_list[i].ioctl(cmd, arg, &data);
				if (copy_to_user((void __user *)arg, &data,  sizeof(struct ioctl_data)))
					dev_err(dev_data->dev, "cmd %d copy err\n", cmd);

				break;
			}
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
	
	cust_kallsyms_lookup_name = NULL;

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
	device_remove_file(misc_dev.this_device, &dev_attr_enable);
#endif

out2:
	misc_deregister(&misc_dev);
out1:
	kfree(misc_data);
	return ret;
}

static void __exit miscdriver_exit(void)
{
	int i;

	//has initalazition
	if (cust_kallsyms_lookup_name)
		for(i=0; unit_list[i].type; i++) {
			unit_list[i].exit();
		}

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

