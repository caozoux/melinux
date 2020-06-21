#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"


static unsigned long cnt_test;
static void *rcu_pointer;

struct task_struct *rcutest_thread_read;
// assigned rcu thread
struct task_struct *rcutest_thread_assigned;

#if 0
	synchronize_rcu();
	rcu_read_lock();
	rcu_read_unlock();
#endif


static int rcuread_thread(void *arg)
{
	u64 * addr;
	while (!kthread_should_stop()) {
		//rcu_read_lock();
		addr = rcu_dereference(rcu_pointer);
		if (!addr) {
			//schedule();
			//udelay(100);
			addr = rcu_dereference(rcu_pointer);
			if (!addr) {
				if (printk_ratelimit()) {
					printk("rcu test error\n");	
				}
			}
		}
		//rcu_read_unlock();
	}
	return 0;
}

static int rcuassigned_thread(void *arg)
{
	u64 *addr;
	int cnt = 0;
	addr = rcu_dereference(rcu_pointer);
	while (!kthread_should_stop()) {
		synchronize_rcu();
		rcu_assign_pointer(rcu_pointer, NULL);
		if (printk_ratelimit()) {
			printk("rcu sync:%d\n", cnt++);	
		}
#if 0
		addr = NULL;
#endif
	}
	return 0;
}

static void rcu_readlock_test_stop(void)
{
	kthread_stop(rcutest_thread_read);
	kthread_stop(rcutest_thread_assigned);
}

static void rcu_readlock_test_start(void)
{
	// read rcu thread

	void *addr;	
	addr = kmalloc(4096, GFP_KERNEL);
	printk("zz %s addr:%p \n",__func__, addr);
	rcu_assign_pointer(rcu_pointer, addr);

	//rcutest_thread_read = kthread_create(rcuread_thread, (void *)NULL, 1, "rcuread_thread");
	//rcutest_thread_assigned = kthread_create(rcuassigned_thread, (void *)NULL,2, "rcuassigned_thread");
	//wake_up_process(rcutest_thread_read);
	//wake_up_process(rcutest_thread_assigned);
	return;

}

int rcu_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USERCU_READTEST_START:
			DEBUG("rcu_readlock_test_start\n")
			rcu_readlock_test_start();
			break;
		case  IOCTL_USERCU_READTEST_END:
			DEBUG("rcu_readlock_test_stop\n")
			//rcu_readlock_test_stop();
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;

#if 0
	data->args[0] = cnt_test  ;

	if (copy_to_user((char __user *) addr, data, sizeof(struct ioctl_data))) {
		printk("copy to user failed\n");
		return -1;
	}
#endif

	return 0;
}

int rcutest_init(void)
{
	RCU_INIT_POINTER(rcu_pointer, NULL);
	return 0;
}

