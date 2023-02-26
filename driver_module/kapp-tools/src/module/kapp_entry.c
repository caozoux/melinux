#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/kprobes.h>
#include <linux/module.h>

#include<linux/miscdevice.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ktrace.h"

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

#define MISC_NAME "ksysd"

struct ksysd_private_data {
	int flag;		
	struct miscdevice *dev;
};

struct kd_percpu_data kd_percpu_data[512];
unsigned long (*cust_kallsyms_lookup_name)(const char *name);
struct mutex *orig_text_mutex;
TEXT_DECLARE()
static struct ksysd_data *ksysd_data;
static int enable;

struct ksysd_uint_item unit_list[] =
{
	KSYSD_UNIT(kprobe, IOCTL_USEKPROBE),
	KSYSD_UNIT(ktrace, IOCTL_KTRACE),
	KSYSD_UNIT(kinject, IOCTL_INJECT),
	KSYSD_UNIT(kmem, IOCTL_KMEM),
	KSYSD_UNIT(kdevice, IOCTL_KDEVICE),
	KSYSD_UNIT(kblock, IOCTL_KBLOCK),
};

static int ksysd_template_open(struct inode *inode, struct file * file)
{
	struct ksysd_private_data *data;
	data = kzalloc(sizeof(struct ksysd_private_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	file->private_data = (void *) data;

	return 0;
}

static ssize_t ksysd_template_read(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t ksysd_template_write(struct file *file, const char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static int ksysd_template_release(struct inode *inode, struct file *file)
{
	static struct ksysd_private_data *data;

	data = (struct ksysd_private_data *)file->private_data;
	if (data)
		kfree(data);

	return 0;
}

int golable_sysm_init(void)
{
	int ret = 0;

	TEXT_SYMS()

	if (ret) {
		ERR("base init failed\n");
		goto OUT;
	}


OUT:
	return ret;
}

static long ksysd_template_unlocked_ioctl(struct file *file, unsigned int size, unsigned long data)
{
	int ret = 0;
	struct ioctl_ksdata ctl_data; 
	struct ksysd_private_data  *dev_data;
	int i;

	dev_data = (struct ksysd_private_data *) file->private_data;

	if (copy_from_user(&ctl_data, (char __user *) data, sizeof(struct ioctl_ksdata))) {
		dev_err(ksysd_data->dev, "ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	DBG("cmd:%d\n", (int)ctl_data.cmd);

	for(i=0; unit_list[i].type; i++) {
		if (unit_list[i].type == ctl_data.cmd)
			ret = unit_list[i].ioctl(ctl_data.cmd, sizeof(struct ioctl_ksdata), &ctl_data);
	}

OUT:
	return ret;
}

struct file_operations ksysd_temp_ops = {
	.open = ksysd_template_open,
	.read = ksysd_template_read,
	.write = ksysd_template_write,
	.unlocked_ioctl =ksysd_template_unlocked_ioctl,
	.release = ksysd_template_release
};

struct miscdevice  ksysd_dev = {
	.name = MISC_NAME,
	.fops = &ksysd_temp_ops,
};

ARRT_MARCO_READ(enable);
ARRT_MARCO_WRITE(enable);
ARRT_MARCO(enable);

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

static int __init ksys_tool_init(void)
{
	int ret;
	int i;

	if (get_kallsyms_lookup_name()) {
		ERR("get_kallsyms_lookup_name failed\n");
		return -EINVAL;
	}

	ksysd_data = kzalloc(sizeof(struct ksysd_data), GFP_KERNEL);
	if (!ksysd_data) {
		return -ENOMEM;
	}

	ret = misc_register(&ksysd_dev);
	if (ret) {
		ERR(" ksysd register err\n");
		goto out1;
	}

	ret = device_create_file(ksysd_dev.this_device, &dev_attr_enable);
	if (ret)
		goto out2;

	ksysd_data->dev = ksysd_dev.this_device;
	if (percpu_variable_init()) {
		ERR("percpu variable init failed\n");
		goto out3;
	}

	ret = base_func_init();
	if (ret)
		goto out3;

	for(i=0; unit_list[i].type; i++) {
		if (unit_list[i].init()) {
			ERR("%s init failed\n", unit_list[i].u_name);
			goto out4;
		}
	}
	INFO("ksysddriver load \n");

	return 0;
out4:
	for(; i>=0; i--) {
		unit_list[i].exit();
	}
out3:
	device_remove_file(ksysd_dev.this_device, &dev_attr_enable);
out2:
	misc_deregister(&ksysd_dev);
out1:
	kfree(ksysd_data);
	return ret;
}

static void __exit ksys_tool_exit(void)
{
	int i;

	for(i=0; unit_list[i].type; i++)
		unit_list[i].exit();

	percpu_variable_exit();

	device_remove_file(ksysd_dev.this_device, &dev_attr_enable);
	misc_deregister(&ksysd_dev);
	kfree(ksysd_data);
	INFO("ksysddriver unload \n");
}

module_init(ksys_tool_init);
module_exit(ksys_tool_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

