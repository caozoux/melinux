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
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/irq_work.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "mekernel.h"

#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
#include "kernel/rcu/tree.h"
#include "kernel/rcu/rcu.h"
#endif


//static unsigned long cnt_test;
static void *rcu_pointer;

static struct list_head *orig_rcu_struct_flavors;
static int *orig_rcu_num_lvls;
static int *orig_rcu_num_nodes;

#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
#define orig_for_each_rcu_flavor(rsp) \
	list_for_each_entry((rsp), orig_rcu_struct_flavors, flavors)

#define orig_rcu_first_leaf_node(rsp) ((rsp)->level[*orig_rcu_num_lvls - 1])
#define orig_rcu_for_each_leaf_node(rsp, rnp) \
	for ((rnp) = orig_rcu_first_leaf_node(rsp); \
			(rnp) < &(rsp)->node[*orig_rcu_num_nodes]; (rnp)++)
#endif
		

struct task_struct *rcutest_thread_read1;
struct task_struct *rcutest_thread_read2;
// assigned rcu thread
struct task_struct *rcutest_thread_assigned;
struct task_struct *rcutest_thread_detect;

#if 0
	synchronize_rcu();
	rcu_read_lock();
	rcu_read_unlock();
#endif


static void dump_rcu_rsp(void)
{
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	struct rcu_state *rsp;
	struct rcu_node *rnp;
	int cpu;

	orig_for_each_rcu_flavor(rsp) {
#if 0
		for_each_online_cpu(cpu) {
			struct rcu_data *rdp = per_cpu_ptr(rsp->rda, cpu);
			struct rcu_node *rnp = rdp->mynode;
			printk("zz %s cpu:%d rdp:%lx rnp:%lx \n",__func__, cpu, (unsigned long)rdp, (unsigned long)rnp);
		}
#else
		orig_rcu_for_each_leaf_node(rsp, rnp) {
			printk("zz cpu%d rnp:%lx qsmask:%lx jiffies_force_qs:%lx gp_seq:%lx jiffies_stall:%lx\n",
					smp_processor_id(),(unsigned long)rnp, (unsigned long)rnp->qsmask
					, rsp->jiffies_force_qs
					, rsp->gp_seq
					, rsp->jiffies_stall
					);
#if 0
			for_each_leaf_node_possible_cpu(rnp, cpu) {
				unsigned long bit = leaf_node_cpu_bit(rnp, cpu);
				printk("zz %s cpu:%lx bit:%lx \n",__func__, (unsigned long)cpu, (unsigned long)bit);
			}
#endif
		}
#endif

	}
#endif
}

static int mercu_read_thread(void *arg)
{
	u64 * addr;
	while (!kthread_should_stop()) {
#if 0
		int udelay_cnt = 20, i;
		trace_printk("addr:%lx +\n",(unsigned long)addr);
		for (i = 0; i < udelay_cnt; ++i) {
			udelay(100);
		}
		trace_printk("addr:%lx -\n",(unsigned long)addr);
		schedule_timeout(msecs_to_jiffies(1));
#endif
		rcu_read_lock();
		trace_printk("zzrr ++\n");
		addr = rcu_dereference(rcu_pointer);
		udelay(1);
		trace_printk("zzrr:%llx \n", (u64)addr);
		//udelay(100);
		trace_printk("zzrr --\n");
		rcu_read_unlock();
		//schedule_timeout(msecs_to_jiffies(1000));
		//schedule_timeout_uninterruptible(1);
		//cond_resched();
		schedule_timeout(msecs_to_jiffies(1));
	}
	return 0;
}

static int mercu_write_thread(void *arg)
{
	u64 *new, *old;
	while (!kthread_should_stop()) {
		old = rcu_dereference(rcu_pointer);
		new = kmalloc(PAGE_SIZE, GFP_KERNEL);
		rcu_assign_pointer(rcu_pointer, new);
		trace_printk("zzww %llx -\n", (u64)new);
		synchronize_rcu();
		trace_printk("zzww +\n");
		kfree(old);
		udelay(100);
		//if (printk_ratelimit())
	}
	return 0;
}

static void rcu_readlock_test_stop(void)
{
	if (rcutest_thread_read1)
		kthread_stop(rcutest_thread_read1);

	if (rcutest_thread_read2)
		kthread_stop(rcutest_thread_read2);

	if (rcutest_thread_assigned)
		kthread_stop(rcutest_thread_assigned);
}

static int mercu_detech_thread(void *arg)
{
	while (!kthread_should_stop()) {
#if 1
		schedule_timeout_uninterruptible(HZ*5);
#else
		int udelay_cnt = 40000, i;
		for (i = 0; i < udelay_cnt; ++i)
			udelay(100)	;
#endif
		dump_rcu_rsp();
	}
	return 0;
}

static void  __maybe_unused  rcu_detect_thread_init(void)
{
	rcutest_thread_detect = kthread_create(mercu_detech_thread, (void *)NULL, "mercu_detech_thread");
	wake_up_process(rcutest_thread_detect);
}

static void __maybe_unused rcu_detect_thread_exit(void)
{
	if (rcutest_thread_detect)
		kthread_stop(rcutest_thread_detect);
}

static void rcu_readlock_test_start(void)
{
	// read rcu thread
	u64 *addr;
	//addr = kmalloc(PAGE_SIZE * 16, GFP_KERNEL);
	addr = NULL;
	rcu_assign_pointer(rcu_pointer, addr);

	rcutest_thread_read1 = kthread_create(mercu_read_thread, (void *)NULL, "mercu_read_thread");
	rcutest_thread_read2 = kthread_create(mercu_read_thread, (void *)NULL, "mercu_read_thread");
	rcutest_thread_assigned = kthread_create(mercu_write_thread, (void *)NULL, "mercu_write_thread");
	wake_up_process(rcutest_thread_read1);
	wake_up_process(rcutest_thread_read2);
	wake_up_process(rcutest_thread_assigned);
	return;

}

int rcutest_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USERCU_READTEST_START:
			MEDEBUG("rcu_readlock_test_start\n");
			rcu_readlock_test_start();
			break;
		case  IOCTL_USERCU_READTEST_END:
			MEDEBUG("rcu_readlock_test_stop\n");
			rcu_readlock_test_stop();
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

int rcutest_unit_init(void)
{
#if 0
	LOOKUP_SYMS(rcu_struct_flavors);
	LOOKUP_SYMS(rcu_num_lvls);
	LOOKUP_SYMS(rcu_num_nodes);
	RCU_INIT_POINTER(rcu_pointer, NULL);
	//dump_rcu_rsp();
	//rcu_detect_thread_init();
	
	//rcu_readlock_test_start();
#endif
	return 0;
}

int rcutest_unit_exit(void)
{
	//rcu_detect_thread_exit();
	
	//rcu_readlock_test_stop();
	return 0;
}

