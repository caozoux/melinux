#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/aio.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>


static int __init taskenumdriver_init(void)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct kioctx_table *table;
	for_each_process(task) {
		mm = task->mm;
		table = rcu_dereference_raw(mm->ioctx_table);
		printk("zz %s task->comm:%s nr_event"%d"\n",__func__, task->comm, table->nr);
	}
	printk("taskenumdriver load \n");
	return 0;
}

static void __exit taskenumdriver_exit(void)
{
	printk("taskenumdriver unload \n");
}

module_init(taskenumdriver_init);
module_exit(taskenumdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
