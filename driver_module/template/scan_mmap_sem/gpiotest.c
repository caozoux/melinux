#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/kthread.h>
#include <linux/rwsem.h>
#include <linux/pid_namespace.h>

enum rwsem_waiter_type {
    RWSEM_WAITING_FOR_WRITE,
    RWSEM_WAITING_FOR_READ
};

struct rwsem_waiter {
    struct list_head list;
    struct task_struct *task;
    enum rwsem_waiter_type type;
};

static int checkpointer(void *addr)
{
	int ret;
	ret = pfn_valid(page_to_pfn(virt_to_page(addr)));
	printk("zz %s ret:%lx page:%lx \n",__func__, (unsigned long)ret, page_count(virt_to_page(addr)));
	printk("zz %s ret:%lx pfn:%lx \n",__func__, (unsigned long)ret, page_to_pfn(virt_to_page(addr)));
	return ret;
}

static int __init gpiodriver_init(void)
{
	struct task_struct * task;
	int pid = 2435;
	struct mm_struct *mm;
	struct rwsem_waiter *waiter, *tmp;
	struct list_head wlist;
	struct rw_semaphore *mmap_sem;
	void *test;

	test =kmalloc(4096, GFP_KERNEL);
	printk("zz %s test:%lx \n",__func__, (unsigned long)test);
	checkpointer(test);


	task = pid_task(find_pid_ns(pid, task_active_pid_ns(current)), PIDTYPE_PID);
	if (task) 
		printk("zz %s task:%lx \n",__func__, (unsigned long)task);
	else
		return -1;

	mm = task->mm;
	if (mm)
		mmap_sem = &mm->mmap_sem;
	else
		return -1;

	list_for_each_entry_safe(waiter, tmp, &mmap_sem->wait_list, list) {
		//if (!checkpointer(waiter))
		//	goto out;
			
	//list_for_each_entry(waiter, &mmap_sem->wait_list, list) {
	//list_for_each_entry(waiter, &mmap_sem->wait_list, list)
		printk("waiter:%lx\n", waiter);			
	}

	printk("zz %s task:%lx \n",__func__, (unsigned long)task);

	return 0;
out:
	return -1;
}

static void __exit gpiodriver_exit(void)
{
	printk("gpiodriver unload \n");
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
