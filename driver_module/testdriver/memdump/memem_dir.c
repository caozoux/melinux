#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/irqdomain.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/miscdevice.h>

static u32 tag_address;
struct miscdevice  mem_dir_dev = {
	.name = "mem_dir",
};

static u32 *testaddress;
static ssize_t mem_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	if (strict_strtoul(buf, 0, &val)) 
		goto out;

	__raw_writel(val, tag_address);
out:
	return count;
}

static ssize_t mem_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int size;
	size = sprintf(buf, "0x%08x\n", __raw_readl(tag_address));
	return size;
}

static ssize_t tag_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long address;
	if (!strict_strtoul(buf, 0, &address)) 
		tag_address = address;
	return count; 
}

static ssize_t tag_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int size;
	size = sprintf(buf, "0x%08x\n", tag_address);
	return size;
}

static DEVICE_ATTR(mem_op, S_IWUSR | S_IRUGO, mem_read, mem_write);
static DEVICE_ATTR(mem_tag, S_IWUSR | S_IRUGO, tag_read, tag_write);
static int __init mem_dir_init(void)
{
	int ret;
	if (misc_register(&mem_dir_dev)) {
		pr_err(" misc register err\n");
		return 1;
	}

	ret = device_create_file(mem_dir_dev.this_device, &dev_attr_mem_op);
	if (ret) {
		pr_err("sys file create failed \n");
		misc_deregister(&mem_dir_dev);
		return 1;
	}

	ret = device_create_file(mem_dir_dev.this_device, &dev_attr_mem_tag);
	if (ret) {
		pr_err("sys file create failed \n");
		misc_deregister(&mem_dir_dev);
		return 1;
	}

	testaddress = kmalloc(1024, GFP_KERNEL);

	printk("zz %s %08x\n", __func__, testaddress);
	return 0;
}

static void __exit mem_dir_exit(void)
{
	device_remove_file(mem_dir_dev.this_device, &dev_attr_mem_op);
	device_remove_file(mem_dir_dev.this_device, &dev_attr_mem_tag);
	misc_deregister(&mem_dir_dev);
	kfree(testaddress);
}

module_init(mem_dir_init);
module_exit(mem_dir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
