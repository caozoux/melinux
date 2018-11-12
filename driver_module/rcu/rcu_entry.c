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
#include <linux/syscore_ops.h>
#include <linux/syscalls.h>
#include  "rcu_module.h"

static void *orig_rcu_state_p;
static atomic_t * rcu_state_counter;
#define BARRIER_CPU_COUNT 75680
#define COMPETED  83536
#define GPNUM    83528

static int symbol_init(void)
{
	orig_rcu_state_p = (void*)kallsyms_lookup_name("rcu_sched_state");
	if (!orig_rcu_state_p) {
		trace_printk("find sysmbol rcu_state_p failed\n");
		return 1;
	}
	return 0;
}

static void dump_rcu_state(void)
{
	unsigned long *complete, *gpnum;
	atomic_t *barrier_cpu_count;
	complete=(unsigned long)orig_rcu_state_p+COMPETED;
	gpnum=(unsigned long)orig_rcu_state_p+GPNUM;
	barrier_cpu_count = orig_rcu_state_p+75680;
	printk("zz %s orig_rcu_state_p:%lx \n",__func__, (unsigned long)orig_rcu_state_p);
	printk("zz %s orig_rcu_state_p:%lx \n",__func__, (unsigned long)orig_rcu_state_p+75680);
	//atomic_inc(barrier_cpu_count);
	printk("zz %s barrier_cpu_count:%lx %d\n",__func__, (unsigned long)barrier_cpu_count, atomic_read(barrier_cpu_count));
	//*gpnum = *gpnum+1;
	printk("zz %s complete:%lx gpnum:%lx \n",__func__, (unsigned long)*complete, (unsigned long)*gpnum);
	//rsp->barrier_completion

}

static void set_rcu_stall() {

}

//void call_rcu_sched(struct rcu_head *head, void (*func)struct rcu_head *rcu)
static int __init rcudriver_init(void)
{
	if (symbol_init()) {
		return 1;	
	}

	dump_rcu_state();
	
	rcu_hook_int();
	kmemcache_flag_rcu_init();
	trace_printk("rcudriver load \n");
	//wait_rcu_gp(cj)
	return 0;
}

static void __exit rcudriver_exit(void)
{
	rcu_hook_exit();
	kmemcache_flag_rcu_exit();
	trace_printk("rcudriver unload \n");
}

module_init(rcudriver_init);
module_exit(rcudriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
