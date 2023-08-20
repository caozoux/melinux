#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/sched/debug.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <trace/events/block.h>

#define MAX_TRACE_ENTRIES       (SZ_4K / sizeof(unsigned long))
#define PER_TRACE_ENTRIES_AVERAGE   (8 + 8)

#define MAX_STACE_TRACE_ENTRIES  (MAX_TRACE_ENTRIES / PER_TRACE_ENTRIES_AVERAGE)
#define MAX_LATENCY_RECORD      10

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

void trace_tracepoint_sched_wakeup(void *ignore, struct task_struct *p);
void trace_tracepoint_sched_switch(void *ignore, bool preempt,struct task_struct *prev,struct task_struct *next,unsigned int prev_state);

unsigned long (*cust_kallsyms_lookup_name)(const char *name);

static u64 (*orig_running_clock)(void);

//static DEFINE_PER_CPU(struct list_head, per_task_list);
static struct per_cpu_stack_trace __percpu *cpu_stack_trace;
static DEFINE_PER_CPU(struct task_struct *, per_task_pointer);
struct tracepoint *orig___tracepoint_sched_wakeup;
struct tracepoint *orig___tracepoint_sched_switch;
static u64 trace_irqoff_latency = 1 * 1000 * 1000UL;
static bool trace_enable;

#if 0
struct hrtimer hrtimer_pr;

static enum hrtimer_restart hrtimer_pr_fun(struct hrtimer *hrtimer)
{
  	trace_printk("zz malloc_lantcy_cnt:%d \n", 1);
	//malloc_lantcy_cnt = 0;
	hrtimer_forward_now(&hrtimer_pr, ns_to_ktime(1000000000));
	return HRTIMER_RESTART;
}
#endif

struct irqoff_trace {
	unsigned int nr_entries;
	unsigned long *entries;
};

struct stack_trace_metadata {
    u64 last_timestamp;
    unsigned long nr_irqoff_trace;
    struct irqoff_trace trace[MAX_STACE_TRACE_ENTRIES];
    unsigned long nr_entries;
    unsigned long entries[MAX_TRACE_ENTRIES];
    unsigned long latency_count[MAX_LATENCY_RECORD];
	char comms[MAX_STACE_TRACE_ENTRIES][TASK_COMM_LEN];
	pid_t pids[MAX_STACE_TRACE_ENTRIES];
	u64 nsecs[MAX_STACE_TRACE_ENTRIES];

};

struct per_cpu_stack_trace {
    struct stack_trace_metadata sched_trace;
};

static void smp_clear_stack_trace(void *info)
{          
    int i;
    struct per_cpu_stack_trace *stack_trace = info;
    
    stack_trace->sched_trace.nr_entries = 0;
    stack_trace->sched_trace.nr_irqoff_trace = 0;

    for (i = 0; i < MAX_LATENCY_RECORD; i++)
        stack_trace->sched_trace.latency_count[i] = 0;
}

static void seq_print_stack_trace(struct seq_file *m, struct irqoff_trace *trace)
{
    int i;

    if (WARN_ON(!trace->entries))
        return;
    
    for (i = 0; i < trace->nr_entries; i++)
        seq_printf(m, "%*c%pS\n", 5, ' ', (void *)trace->entries[i]);
}   

static void trace_latency_show_one(struct seq_file *m, void *v)
{
    int cpu;

    for_each_online_cpu(cpu) {
        int i;
        unsigned long nr_irqoff_trace;
        struct stack_trace_metadata *stack_trace;

		stack_trace = this_cpu_ptr(&cpu_stack_trace->sched_trace);

        /**
         * Paired with smp_store_release() in the save_trace().
         */
        nr_irqoff_trace = smp_load_acquire(&stack_trace->nr_irqoff_trace);

        if (!nr_irqoff_trace)
            continue;

        seq_printf(m, " cpu: %d\n", cpu);

        for (i = 0; i < nr_irqoff_trace; i++) {
            struct irqoff_trace *trace = stack_trace->trace + i;

            seq_printf(m, "%*cCOMMAND: %s PID: %d LATENCY: %lld ms\n",
                   5, ' ', stack_trace->comms[i],
                   stack_trace->pids[i],
                   stack_trace->nsecs[i] / (1000 * 1000UL));
            seq_print_stack_trace(m, trace);
            seq_putc(m, '\n');

            cond_resched();
        }
    }
}

static ssize_t trace_latency_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *ppos)
{
    unsigned long latency;

    if (kstrtoul_from_user(buf, count, 0, &latency))
        return -EINVAL;

    if (latency == 0) {
        int cpu;

        for_each_online_cpu(cpu)
            smp_call_function_single(cpu, smp_clear_stack_trace,
                         per_cpu_ptr(cpu_stack_trace, cpu),
                         true);
        return count;
    } else if (latency < 10)
        return -EINVAL;

    trace_irqoff_latency = latency * 1000 * 1000UL;

    return count;
}
 
static int trace_latency_show(struct seq_file *m, void *v)
{       
    seq_printf(m, "trace_irqoff_latency: %llums\n\n",
            trace_irqoff_latency/(1000 * 1000UL));
        
    seq_puts(m, " wakeup sched:\n");
    trace_latency_show_one(m, v);

    seq_putc(m, '\n');
            
    return 0;  
}              

static int trace_latency_open(struct inode *inode, struct file *file)
{
    return single_open(file, trace_latency_show, inode->i_private);
}   

static const struct file_operations trace_latency_fops = {
    .owner      = THIS_MODULE,
    .open       = trace_latency_open,
    .read       = seq_read,
    .write      = trace_latency_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};  

static int enable_show(struct seq_file *m, void *ptr)
{
    seq_printf(m, "%s\n", trace_enable ? "enabled" : "disabled");
    return 0;
}

static int enable_open(struct inode *inode, struct file *file)
{   
    return single_open(file, enable_show, inode->i_private);
}   
    

static void trace_sched_wakeup_enable(bool en)
{
	if (en && !trace_enable) {
		printk("zz %s %d \n", __func__, __LINE__);
		tracepoint_probe_register(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
		tracepoint_probe_register(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
	} else {
		tracepoint_probe_unregister(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
		tracepoint_probe_unregister(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
	}
}

static ssize_t enable_write(struct file *file, const char __user *buf,
                size_t count, loff_t *ppos)
{   
    bool enable;

    if (kstrtobool_from_user(buf, count, &enable))
        return -EINVAL;
    
    if (!!enable == !!trace_enable)
        return count;
    
    if (enable)
        trace_sched_wakeup_enable(true);
    else
        trace_sched_wakeup_enable(false);

    trace_enable = enable;
    
    return count;
}   

static const struct file_operations enable_fops = {
    .open       = enable_open,
    .read       = seq_read,
    .write      = enable_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};  

static unsigned long get_timestamp(void)
{
    return orig_running_clock();  /* 2^30 ~= 10^9 */
}

//void free_task(struct task_struct *tsk;
void trace_tracepoint_sched_wakeup(void *ignore, struct task_struct *p)
{
	p->mce_addr = get_timestamp();
	per_cpu(per_task_pointer,  smp_processor_id()) = p;
}

static inline void store_stack_trace(struct task_struct *task, struct irqoff_trace *trace, unsigned long *entries, unsigned int max_entries, int skip)
{
	struct stack_trace stack_trace;

	stack_trace.nr_entries = 0;
	stack_trace.max_entries = max_entries;
	stack_trace.entries = entries;
	stack_trace.skip = 0;

	save_stack_trace_tsk(task, &stack_trace);

	trace->entries = entries;
	trace->nr_entries = stack_trace.nr_entries;

}
	
void trace_tracepoint_sched_switch(void *ignore, bool preempt,struct task_struct *prev,struct task_struct *next,unsigned int prev_state)
{
	struct task_struct *p = next;
	struct stack_trace_metadata *stack_trace;
	u64 now,delta;

	if (p->mce_addr) {
		now = get_timestamp();
		delta = now - p->mce_addr;
		if (delta > trace_irqoff_latency) {
			unsigned long nr_entries, nr_irqoff_trace;
			struct irqoff_trace *trace;

			trace_printk("zz %s %d \n", __func__, __LINE__);
			stack_trace = this_cpu_ptr(&cpu_stack_trace->sched_trace);
			nr_irqoff_trace = stack_trace->nr_irqoff_trace;
			if (unlikely(nr_irqoff_trace >= MAX_STACE_TRACE_ENTRIES))
				return;
	
			nr_entries = stack_trace->nr_entries;
			if (unlikely(nr_entries >= MAX_TRACE_ENTRIES - 1))
				return;
	
			strlcpy(stack_trace->comms[nr_irqoff_trace], current->comm,	 TASK_COMM_LEN);
			stack_trace->pids[nr_irqoff_trace] = current->pid;
			stack_trace->nsecs[nr_irqoff_trace] = delta; 
			trace = stack_trace->trace + nr_irqoff_trace;

			trace_printk("zz %s stack_trace:%lx nr_entries:%lx nr_irqoff_trace:%lx trace:%lx %lld\n",__func__, (unsigned long)stack_trace->entries, (unsigned long)nr_entries, (unsigned long)nr_irqoff_trace, (unsigned long)trace, delta);

			store_stack_trace(p, trace, stack_trace->entries + nr_entries,MAX_TRACE_ENTRIES - nr_entries, 0);
			stack_trace->nr_entries += trace->nr_entries;

			/**
			 * Ensure that the initialisation of @trace is complete before we
			 * update the @nr_irqoff_trace.
			 */
			smp_store_release(&stack_trace->nr_irqoff_trace, nr_irqoff_trace + 1);

			if (unlikely(stack_trace->nr_entries >= MAX_TRACE_ENTRIES - 1)) {
				pr_info("BUG: MAX_TRACE_ENTRIES too low!");
			}

			//trace_printk("zz timeout %lld\n", delta);
			//sched_show_task(p);
		}
		p->mce_addr = 0;
	}
}

static int hrtimer_pr_init(void)
{
#if 0
	hrtimer_init(&hrtimer_pr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_pr.function = hrtimer_pr_fun;
	hrtimer_start(&hrtimer_pr, ns_to_ktime(1000000000),
			HRTIMER_MODE_REL_PINNED);
#endif
	return 0;
}

static void hrtimer_pr_exit(void)
{
#if 0
	hrtimer_cancel(&hrtimer_pr);
#endif
}

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) {
	return 0;
}

static int sym_init(void)
{
	LOOKUP_SYMS(__tracepoint_sched_wakeup);
	LOOKUP_SYMS(__tracepoint_sched_switch);
	LOOKUP_SYMS(running_clock);
	return 0;
}

static int sysm_lookup_init(void)
{
	int ret;
	struct kprobe kp;

	memset(&kp, 0, sizeof(struct kprobe));

	ret = -1;
	kp.symbol_name = "kallsyms_lookup_name";
	kp.pre_handler = noop_pre_handler;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		printk("Err: find kallsyms_lookup_name failed \n");
		return -EINVAL;
	}

	cust_kallsyms_lookup_name = (void*)kp.addr;
	unregister_kprobe(&kp);

	return 0;
}

static int __init percpu_hrtimer_init(void)
{
	int cpu;
	struct proc_dir_entry *parent_dir;

	cpu_stack_trace = alloc_percpu(struct per_cpu_stack_trace);
	if (!cpu_stack_trace)
		return -ENOMEM;

	parent_dir = proc_mkdir("trace_irqoff", NULL);
	if (!parent_dir)
		goto free_percpu;

	if (!proc_create("trace_latency", S_IRUSR | S_IWUSR, parent_dir,
				 &trace_latency_fops))
		goto remove_proc;

	if (!proc_create("enable", S_IRUSR | S_IWUSR, parent_dir, &enable_fops)) 
		goto remove_proc;

	
	if (sysm_lookup_init())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	for_each_online_cpu(cpu)
		per_cpu(per_task_pointer, cpu) = NULL;

#if 0
	ret = tracepoint_probe_register(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
	if (ret)
		return -EINVAL;

	ret = tracepoint_probe_register(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
	if (ret)
		return -EINVAL;
#endif

	if (hrtimer_pr_init())
		goto out;

	return 0;
#if 0
	tracepoint_probe_unregister(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
	tracepoint_probe_unregister(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
#endif
out:
remove_proc:
	remove_proc_subtree("trace_irqoff", NULL);
free_percpu:
	free_percpu(cpu_stack_trace);
	return -EINVAL;
}

static void __exit percpu_hrtimer_exit(void)
{
	if (trace_enable) {
		tracepoint_probe_unregister(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
		tracepoint_probe_unregister(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
	}
	hrtimer_pr_exit();
	remove_proc_subtree("trace_irqoff", NULL);
	free_percpu(cpu_stack_trace);
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");

