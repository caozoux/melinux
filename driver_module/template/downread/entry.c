#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>
#include <trace/events/block.h>

#include <kernel/sched/sched.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

unsigned long (*cust_kallsyms_lookup_name)(const char *name);

typedef struct {
	u32 pid;
	struct task_struct *task;
	u64 start_ts;
	u64 get_ts;
	u64 end_ts;
	u64 wakeup_ts;
	u64 count;
	bool is_throttle;
	u64 throttle_time;
} task_lock_data;


atomic_t alloc_cnt;
struct radix_tree_root pid_radix_root;
static struct check_regs __percpu *per_regs;
static bool en_trace=false;

//u64 hook_addr = 0xffff946c4018a360;
u64 hook_addr = 0xffff9423c018a360;

static DEFINE_SPINLOCK(data_lock);
//static struct per_cpu_stack_trace __percpu *cpu_stack_trace;
static u64 trace_latency = 1 * 1000 * 1000UL;
static u64 (*orig_running_clock)(void);
struct kmem_cache *pidtrace_kmem_cache;
static task_lock_data *get_pid_data(u32 pid);
//#define HOOK_FUN "show_cpuinfo"

struct tracepoint *orig___tracepoint_sched_wakeup;
struct tracepoint *orig___tracepoint_sched_wakeup_new;
struct tracepoint *orig___tracepoint_sched_switch;
void trace_tracepoint_sched_wakeup_new(void *ignore, struct task_struct *p);
void trace_tracepoint_sched_wakeup(void *ignore, struct task_struct *p);
void trace_tracepoint_sched_switch(void *ignore, bool preempt,struct task_struct *prev,struct task_struct *next,unsigned int prev_state);

static bool debug_enable;
module_param(debug_enable, bool, 0644);
static unsigned long get_timestamp(void)
{
    return orig_running_clock();
}

//void free_task(struct task_struct *tsk;
void trace_tracepoint_sched_wakeup(void *ignore, struct task_struct *p)
{
	u32 pid;
	task_lock_data *lock_data;

	if (!en_trace)
		return;

	//trace_printk("wakeup %d\n", p->pid);
	if (p->mce_addr != 1)
		return;

	pid = p->pid;

	lock_data = get_pid_data(pid);
	if (lock_data) {
		if (lock_data->wakeup_ts == 0 && lock_data->get_ts == 0) {
			lock_data->wakeup_ts = get_timestamp();
			if (p->se.cfs_rq->throttle_count) {
				//lock_data->throttle_time =  p->se.cfs_rq->throttled_clock_task_time;
				lock_data->is_throttle = true;
				lock_data->throttle_time =  p->se.cfs_rq->throttled_clock_task;
			}
			//lock_data->throttle_count_old =  p->se.cfs_rq->throttle_count;
		}
	}
}

//void free_task(struct task_struct *tsk;
void trace_tracepoint_sched_wakeup_new(void *ignore, struct task_struct *p)
{
	u32 pid;
	task_lock_data *lock_data;

	if (!en_trace)
		return;

	//trace_printk("wakeup %d\n", p->pid);
	if (p->mce_addr != 1)
		return;

	pid = p->pid;
	lock_data = get_pid_data(pid);
	if (lock_data) {
		if (lock_data->wakeup_ts == 0 && lock_data->get_ts == 0) {
			lock_data->wakeup_ts = get_timestamp();
			//lock_data->throttle_time =  p->se.cfs_rq->throttled_clock_task_time;
			//lock_data->throttle_count_old =  p->se.cfs_rq->throttle_count;
			if (p->se.cfs_rq->throttle_count) {
				lock_data->is_throttle = true;
				//lock_data->throttle_time =  p->se.cfs_rq->throttled_clock_task_time;
				lock_data->throttle_time =  p->se.cfs_rq->throttled_clock_task;
			}
		}
	}
}

void trace_tracepoint_sched_switch(void *ignore, bool preempt,struct task_struct *prev,struct task_struct *next,unsigned int prev_state)
{

}

static task_lock_data * attach_pid_data(struct task_struct *task)
{
	u32 pid = task->pid;
	task_lock_data *lock_data;

	get_task_struct(task);
	lock_data = kmem_cache_alloc(pidtrace_kmem_cache, GFP_ATOMIC);
	if (!lock_data) {
		put_task_struct(task);
		return NULL;
	}
	memset(lock_data, 0 ,sizeof(task_lock_data));

	spin_lock(&data_lock);
	radix_tree_insert(&pid_radix_root, pid, lock_data);
	spin_unlock(&data_lock);
	lock_data->pid =task->pid;
	lock_data->task = task;
	atomic_inc(&alloc_cnt);
	//trace_printk("zz %s attach lock_data:%lx \n",__func__, (unsigned long)lock_data);
	return lock_data;
}

static task_lock_data *get_pid_data(u32 pid)
{
	task_lock_data *lock_data;

	rcu_read_lock();
	lock_data = radix_tree_lookup(&pid_radix_root, pid);
	rcu_read_unlock();
	
	//trace_printk("zz %s get lock_data:%lx \n",__func__, (unsigned long)lock_data);
	return lock_data;
}

static int free_pid_data(task_lock_data *lock_data)
{
	struct task_struct *p = lock_data->task;
	//trace_printk("zz %s free lock_data:%lx \n",__func__, (unsigned long)lock_data);
	spin_lock(&data_lock);
	radix_tree_delete(&pid_radix_root, lock_data->pid);
	spin_unlock(&data_lock);
	kfree(lock_data);
	atomic_dec(&alloc_cnt);
	
	put_task_struct(p);
	return 0;
}


static int kprobe_down_read(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;
	task_lock_data *lock_data;

	if (!en_trace)
		return 0;

	if ((u64)sem != hook_addr) 
		return 0;

	//trace_printk("get read %d\n", current->pid);
	lock_data = get_pid_data(current->pid);
	if (!lock_data)
		lock_data = attach_pid_data(current);

	if (lock_data) {
		//memset(lock_data, 0, sizeof(task_lock_data));
		lock_data->pid = current->pid;
		lock_data->start_ts = get_timestamp();
		lock_data->get_ts = 0;
		lock_data->end_ts = 0;
		lock_data->wakeup_ts = 0;
		lock_data->count= atomic_long_read(&sem->count)>>8;
		current->mce_addr = 1;
	}

	return 0;
}

static int kretprobe_down_read(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	task_lock_data *lock_data;
	if (!en_trace)
		return 0;

	if (current->mce_addr != 1)
		return 0;

	lock_data = get_pid_data(current->pid);
	if (lock_data && !lock_data->get_ts)
		lock_data->get_ts = get_timestamp();

	return 0;
}

static int kprobe_up_read(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;
	u32 pid;
	u64 delta;
	task_lock_data *lock_data;

	if (!en_trace)
		return 0;

	if ((u64)sem != hook_addr) 
		return 0;

	if (current->mce_addr != 1)
		return 0;

	pid = current->pid;
	lock_data = get_pid_data(pid);
	if (lock_data) {
		lock_data->end_ts = get_timestamp();
		delta = lock_data->end_ts - lock_data->start_ts;
		if (unlikely(debug_enable)) {
			trace_printk("%d read  %lld %lld %lld %lld %lld \n",  lock_data->pid
					, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
					, lock_data->wakeup_ts, lock_data->count);
		} else {
			if (delta > trace_latency) {
				u64 delta = 0;

				if (lock_data->is_throttle) 
					delta = current->se.cfs_rq->throttled_clock_task - lock_data->throttle_time;
				trace_printk("%d R %lld %lld %lld %lld %lld  %lld\n",  lock_data->pid
						, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
						, lock_data->wakeup_ts, lock_data->count, delta);
			}
		}
					
		free_pid_data(lock_data);
	}

	current->mce_addr = 0;
	return 0;
}

static int kprobe_down_write(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;
	task_lock_data *lock_data;

	if (!en_trace)
		return 0;

	if ((u64)sem != hook_addr) 
		return 0;

	lock_data = get_pid_data(current->pid);
	if (!lock_data)
		lock_data = attach_pid_data(current);

	if (lock_data) {
		//memset(lock_data, 0, sizeof(task_lock_data));
		lock_data->pid = current->pid;
		lock_data->start_ts = get_timestamp();
		lock_data->get_ts = 0;
		lock_data->end_ts = 0;
		lock_data->wakeup_ts = 0;
		lock_data->count= atomic_long_read(&sem->count)>>8;
		current->mce_addr = 1;
	}

	return 0;
}

static int kretprobe_down_write(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	task_lock_data *lock_data;
	if (!en_trace)
		return 0;

	if (current->mce_addr != 1)
		return 0;

	lock_data = get_pid_data(current->pid);
	if (lock_data && !lock_data->get_ts)
		lock_data->get_ts = get_timestamp();

	return 0;
}

static int kprobe_up_write(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;
	task_lock_data *lock_data;
	u32 pid;
	u64 delta;

	if (!en_trace)
		return 0;

	if ((u64)sem != hook_addr) 
		return 0;

	if (current->mce_addr != 1)
		return 0;

	pid = current->pid;
	lock_data = get_pid_data(pid);
	if (lock_data) {
		lock_data->end_ts = get_timestamp();
		delta = lock_data->end_ts - lock_data->start_ts;
		if (unlikely(debug_enable)) {
			trace_printk("%d write %lld %lld %lld %lld %lld \n",  lock_data->pid
					, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
					, lock_data->wakeup_ts, lock_data->count);
		} else {
			if (delta > trace_latency)
				trace_printk("%d W %lld %lld %lld %lld %lld  %d \n",  lock_data->pid
						, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
						, lock_data->wakeup_ts, lock_data->count, current->se.cfs_rq->throttle_count);
		}
					

		free_pid_data(lock_data);
	}

	current->mce_addr = 0;

	return 0;
}

struct kprobe kplist[] = {
	{
        .symbol_name = "down_read",
        .pre_handler = kprobe_down_read,
	},
	{
        .symbol_name = "up_read",
        .pre_handler = kprobe_up_read,
	},
	{
        .symbol_name = "down_write",
        .pre_handler = kprobe_down_write,
	},
	{
        .symbol_name = "up_write",
        .pre_handler = kprobe_up_write,
	},
};

struct kretprobe kretlist[]  = {
	{
		.kp.symbol_name = "down_read",
		.handler= kretprobe_down_read,
	},
	{
		.kp.symbol_name = "down_write",
		.handler= kretprobe_down_write,
	},
};

static ssize_t trace_lockaddr_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *ppos)
{
    unsigned long addr;
    if (kstrtoul_from_user(buf, count, 0, &addr))
        return -EINVAL;

    hook_addr = addr;

	return count;
}

static int trace_lockaddr_show(struct seq_file *m, void *v)
{

    seq_printf(m, "lock addr: %llu\n", hook_addr);
            	
    seq_putc(m, '\n');

    return 0;
}

static int trace_lockaddr_open(struct inode *inode, struct file *file)
{
    return single_open(file, trace_lockaddr_show, inode->i_private);
}

static const struct proc_ops trace_lockaddr_fops = {
    .proc_open       = trace_lockaddr_open,
    .proc_read       = seq_read,
    .proc_write      = trace_lockaddr_write,
    .proc_lseek     = seq_lseek,
    .proc_release    = single_release,
};


static ssize_t trace_latency_write(struct file *file, const char __user *buf,
                   size_t count, loff_t *ppos)
{
    unsigned long latency;
    if (kstrtoul_from_user(buf, count, 0, &latency))
        return -EINVAL;

    trace_latency = latency * 1000UL;

	return count;
}

static int trace_latency_show(struct seq_file *m, void *v)
{

    seq_printf(m, "trace_latency: %llu us\n\n",
            trace_latency/( 1000UL));
    seq_putc(m, '\n');

    return 0;
}

static int trace_latency_open(struct inode *inode, struct file *file)
{
    return single_open(file, trace_latency_show, inode->i_private);
}

static const struct proc_ops trace_latency_fops = {
    .proc_open       = trace_latency_open,
    .proc_read       = seq_read,
    .proc_write      = trace_latency_write,
    .proc_lseek     = seq_lseek,
    .proc_release    = single_release,
};

static int enable_show(struct seq_file *m, void *ptr)
{
    seq_printf(m, "%s\n", en_trace? "enabled" : "disabled");
    return 0;
}

static int enable_open(struct inode *inode, struct file *file)
{
    return single_open(file, enable_show, inode->i_private);
}

static void trace_sched_wakeup_enable(bool en)
{
#if 0
	int kpnum, kpretnum, i;
#else
	int i;
#endif
	int ret;
	struct kprobe *kpo_array[sizeof(kplist)/sizeof(struct kprobe)];
	struct kretprobe *kretpo_array[sizeof(kplist)/sizeof(struct kprobe)];

	if (en && !en_trace) {
		en_trace = en;
		printk("enable trace \n");
#if 0
		for (kpnum = 0; kpnum < sizeof(kplist)/sizeof(struct kprobe); kpnum++) {
			if (ret = register_kprobe(&kplist[kpnum])) {
				printk("zz %s kprobe ret:%ld \n", kplist[kpnum].symbol_name, (unsigned long)ret);
				goto out1;
			}
		}

		for (kpretnum = 0; kpretnum < sizeof(kretlist)/sizeof(struct kretprobe); kpretnum++)  {
			if (register_kretprobe(&kretlist[kpretnum])) {
				printk("zz %s kretprobe ret:%ld \n", kretlist[kpnum].kp.symbol_name, (unsigned long)ret);
				goto out2;
			}
		}
#else
		for (i = 0; i < sizeof(kretlist)/sizeof(struct kretprobe); i++) {
			kretlist[i].kp.addr = NULL;
			kretlist[i].kp.flags = 0;
			kretpo_array[i] = &kretlist[i];
		}

		for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); i++) {
			kplist[i].addr = NULL;
			kplist[i].flags = 0;
			kpo_array[i] = &kplist[i];
		}

		if (register_kretprobes(kretpo_array, sizeof(kretlist)/sizeof(struct kretprobe)))  {
			printk("register kretprobe failed\n");
			goto out;
		}

		if (register_kprobes(kpo_array, sizeof(kplist)/sizeof(struct kprobe))) {
			printk("register kprobe failed\n");
			goto out1;
		}
#endif

		tracepoint_probe_register(orig___tracepoint_sched_wakeup_new, trace_tracepoint_sched_wakeup_new, NULL);
		tracepoint_probe_register(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
		//tracepoint_probe_register(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
	} else {
		if (en_trace) {

			for (i = 0; i < sizeof(kretlist)/sizeof(struct kretprobe); i++)
				kretpo_array[i] = &kretlist[i];

			for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); i++)
				kpo_array[i] = &kplist[i];

			printk("disable trace\n");
			en_trace = false;
			tracepoint_probe_unregister(orig___tracepoint_sched_wakeup_new, trace_tracepoint_sched_wakeup_new, NULL);
			tracepoint_probe_unregister(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
			//tracepoint_probe_unregister(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
#if 0			
			for (i = 0; i < sizeof(kretlist)/sizeof(struct kretprobe); i++)
				unregister_kretprobe(&kretlist[i]);
			for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); i++)
				unregister_kprobe(&kplist[i]);
#endif
			unregister_kretprobes(kretpo_array, sizeof(kretlist)/sizeof(struct kretprobe)); 

			unregister_kprobes(kpo_array, sizeof(kplist)/sizeof(struct kprobe));

		}

	}

	return;
#if 0
out2:
	while (kpretnum>0) {
		kpretnum--;
		unregister_kretprobe(&kretlist[kpretnum]);
	}
out1:
	while (kpnum>0) {
		kpnum--;
		unregister_kprobe(&kplist[kpnum]);
	}
#else
out1:
	unregister_kretprobes(kretpo_array, sizeof(kretlist)/sizeof(struct kretprobe)); 
out:
	en_trace = false;
#endif
}

static ssize_t enable_write(struct file *file, const char __user *buf,
                size_t count, loff_t *ppos)
{
    bool enable;
    if (kstrtobool_from_user(buf, count, &enable))
        return -EINVAL;

    if (!!enable == !!en_trace)
        return count;

    if (enable) {
        trace_sched_wakeup_enable(true);
	} else {
        trace_sched_wakeup_enable(false);
	}
    


    return count;
}

static const struct proc_ops enable_fops = {
    .proc_open = enable_open,
    .proc_read       = seq_read,
    .proc_write      = enable_write,
    .proc_lseek     = seq_lseek,
    .proc_release    = single_release,
};

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) {
	return 0;
}

static int sym_init(void)
{
	LOOKUP_SYMS(running_clock);
	LOOKUP_SYMS(__tracepoint_sched_wakeup_new);
	LOOKUP_SYMS(__tracepoint_sched_wakeup);
	LOOKUP_SYMS(__tracepoint_sched_switch);
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

static int __init kprobe_driver_init(void)
{
	struct proc_dir_entry *parent_dir;

	if (sysm_lookup_init())
		return -EINVAL;

	if (sym_init())
		return -EINVAL;

	atomic_set(&alloc_cnt, 0);
	pidtrace_kmem_cache = kmem_cache_create("locktrace_data", sizeof(task_lock_data), 0, SLAB_HWCACHE_ALIGN, NULL);
	if (!pidtrace_kmem_cache) {
		printk("alloc kmemcache locktrace failed\n");
		goto free_percpu;
	}

	parent_dir = proc_mkdir("locktrace", NULL);
	if (!parent_dir)
		goto free_percpu;

	if (!proc_create("trace_latency", S_IRUSR | S_IWUSR, parent_dir,
				 &trace_latency_fops))
		goto remove_proc;

	if (!proc_create("enable", S_IRUSR | S_IWUSR, parent_dir, &enable_fops))
		goto remove_proc;

	if (!proc_create("lock", S_IRUSR | S_IWUSR, parent_dir,
				 &trace_lockaddr_fops))
		goto remove_proc;

	INIT_RADIX_TREE(&pid_radix_root, GFP_ATOMIC);
	spin_lock_init(&data_lock);
	//en_trace = true;
	return 0;

remove_proc:
	remove_proc_subtree("trace_irqoff", NULL);
free_percpu:
	return -EINVAL;
}

static void free_pid_lock_data(void)
{
	void __rcu **slot;
	struct radix_tree_iter iter;

	 radix_tree_for_each_slot(slot, &pid_radix_root, &iter, 0) {
		 task_lock_data *lock_data;
		 lock_data = rcu_dereference(*slot);
		 if (lock_data) {
			//trace_printk("zz %s pid:%lx lock_data:%lx \n",__func__, (unsigned long)lock_data->pid, (unsigned long)lock_data);
			put_task_struct(lock_data->task);
			kfree(lock_data);
		 }
		 radix_tree_delete(&pid_radix_root, iter.index);
	 }
}

static void __exit kprobe_driver_exit(void)
{

	trace_sched_wakeup_enable(false);

	remove_proc_subtree("locktrace", NULL);
	free_pid_lock_data();
	kmem_cache_destroy(pidtrace_kmem_cache);
	printk("zz %s cnt:%lx \n",__func__, (unsigned long)atomic_read(&alloc_cnt));
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
