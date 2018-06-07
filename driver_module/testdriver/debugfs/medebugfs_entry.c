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
#include <linux/debugfs.h>

static struct dentry *medebug_entry;
static int __init debugfsdriver_init(void)
{
	printk("zz %s \n", __func__);
	medebug_entry = debugfs_create_dir("medebug", NULL);
	//entry = debugfs_create_file("filter", 0644, dir->entry, dir,
	//			    &ftrace_subsystem_filter_fops);
	return 0;
}

static void __exit debugfsdriver_exit(void)
{
	printk("zz %s \n", __func__);
	debugfs_remove(medebug_entry);
	//remove_proc_entry("driver/debugfs", NULL);
}

module_init(debugfsdriver_init);
module_exit(debugfsdriver_exit);

//module_param(dump, bool, S_IRUSR | S_IWUSR);
//MODULE_PARM_DESC(dump, "Enable sdio read/write dumps.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zoucao <zoucaox@outlook.com>");

