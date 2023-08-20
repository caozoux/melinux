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


#define PROC_MARCO(__name) \
	static const struct proc_ops __name ## _fops = { \
		.proc_open       = __name ## _open, \
		.proc_read       = seq_read,   \
		.proc_write      = __name ## _write, \
		.proc_lseek     = seq_lseek, \
		.proc_release    = single_release, \
	};

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
static void trace_sched_wakeup_enable(bool en);
static void free_pid_lock_data(void);

#define MAX_TRACE_CNT (16)
#define SCHED_TRACE_MAX (128)
#define MAX_TRACE_ENTRIES       (SZ_16K / sizeof(unsigned long))
#define MAX_SCHED_ENTRIES  (512)

enum TRACE_MODE {
	TRACE_NONE,
	TRACE_MUTEX,
	TRACE_RWSEM,
};

struct sched_trace {
	u64 timestampe;
	bool sched_in;
	unsigned int nr_entries;
	unsigned long *entries;
};

struct task_sched_trace {
	u32 pid;
	char comm[32];
	u64 start_ts;
	u64 get_ts;
	u64 end_ts;
	u64 wakeup_ts;
	u64 last_ts;
	int sched_cnt;
	int nr_trace;
	struct sched_trace stack_trace[MAX_SCHED_ENTRIES];
	unsigned long nr_entries;
	unsigned long entries[MAX_TRACE_ENTRIES];
};

typedef struct {
	u32 pid;
	//struct task_struct *task;
	u64 start_ts;
	u64 get_ts;
	u64 end_ts;
	u64 wakeup_ts;
	u64 count;
	bool is_throttle;
	bool is_record;
	int record_entry;
	int sched_cnt;
	//u64 sched_ts[256];
} task_lock_data;


struct task_sched_trace *sched_trace_head;
static atomic_t sched_trace_cnt;

atomic_t alloc_cnt;
struct radix_tree_root pid_radix_root;
static struct check_regs __percpu *per_regs;
static bool en_trace=false;

static u64 hook_addr = 0;
static enum TRACE_MODE trace_mode;

static DEFINE_SPINLOCK(data_lock);
//static struct per_cpu_stack_trace __percpu *cpu_stack_trace;
static u64 trace_latency = 100 * 1000 * 1000UL;
struct kmem_cache *pidtrace_kmem_cache;
static task_lock_data *get_pid_data(u32 pid);
//#define HOOK_FUN "show_cpuinfo"

static u64 (*orig_running_clock)(void);
static unsigned int (*orig_stack_trace_save_tsk)(struct task_struct *task,
		unsigned long *store, unsigned int size,
		unsigned int skipnr);
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static inline void store_stack_trace(struct pt_regs *regs,
                     struct sched_trace *trace,
                     unsigned long *entries,
                     unsigned int max_entries, int skip)
{
    struct stack_trace stack_trace;

    stack_trace.nr_entries = 0;
    stack_trace.max_entries = max_entries;
    stack_trace.entries = entries;
    stack_trace.skip = skip;

    save_stack_trace(&stack_trace);

    trace->entries = entries;
    trace->nr_entries = stack_trace.nr_entries;

    /*
     * Some daft arches put -1 at the end to indicate its a full trace.
     *
     * <rant> this is buggy anyway, since it takes a whole extra entry so a
     * complete trace that maxes out the entries provided will be reported
     * as incomplete, friggin useless </rant>.
     */
    if (trace->nr_entries != 0 &&
        trace->entries[trace->nr_entries - 1] == ULONG_MAX)
        trace->nr_entries--;
}

#else

static inline int store_stack_trace(struct task_struct *task, struct sched_trace *trace, unsigned long *entries, unsigned int max_entries, int skip)
{
	unsigned long nr_entries;

	nr_entries = orig_stack_trace_save_tsk(task, entries, max_entries,0);

	trace->entries = entries;
	trace->nr_entries = nr_entries;
	return 0;

}
#endif

//void free_task(struct task_struct *tsk;
void trace_tracepoint_sched_wakeup(void *ignore, struct task_struct *p)
{
	u32 pid;
	task_lock_data *lock_data;

	if (!en_trace)
		return;

	if (p->mce_addr != 1)
		return;

	pid = p->pid;

	lock_data = get_pid_data(pid);
	if (lock_data) {
		if (lock_data->wakeup_ts == 0 && lock_data->get_ts == 0) {
			lock_data->wakeup_ts = get_timestamp();
			if (p->se.cfs_rq->throttle_count)
				lock_data->is_throttle = true;
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

	if (p->mce_addr != 1)
		return;

	pid = p->pid;
	lock_data = get_pid_data(pid);
	if (lock_data) {
		if (lock_data->wakeup_ts == 0 && lock_data->get_ts == 0) {
			lock_data->wakeup_ts = get_timestamp();
			if (p->se.cfs_rq->throttle_count)
				lock_data->is_throttle = true;
		}
	}
}

void trace_tracepoint_sched_switch(void *ignore, bool preempt,struct task_struct *prev,struct task_struct *next,unsigned int prev_state)
{
	u32 pid;
	task_lock_data *lock_data;
	bool sched_in;
	struct task_struct *task;

	if (!en_trace)
		return;

	if (prev->mce_addr == 2) {
		sched_in = false;
		pid = prev->pid;
		task = prev;
	} else if (next->mce_addr == 2) {
		sched_in = true;
		pid = next->pid;
		task = next;
	} else
		return;

	lock_data = get_pid_data(pid);
	if (lock_data) {
		u64 delta, now;
		now = get_timestamp();
		delta = now - lock_data->get_ts;

		//trace_printk("zz pid:%ld delta0:%lx \n", (unsigned long)pid, (unsigned long)delta);
		if (lock_data->is_record || delta >= trace_latency/3) {
			struct task_sched_trace *trace;
			struct sched_trace *stack_trace;
			unsigned long nr_entries;
			u64 now;
			int cur_cnt;

			now = get_timestamp();
			if (lock_data->is_record) {
				cur_cnt = lock_data->record_entry;
				trace = &sched_trace_head[cur_cnt];
			} else {

				cur_cnt = atomic_read(&sched_trace_cnt);
				if (cur_cnt >= (MAX_TRACE_CNT - 1)) {
					trace_printk("Warning rearch max record count\n");
					goto out;
				}

				cur_cnt = atomic_inc_return(&sched_trace_cnt);

				trace = &sched_trace_head[cur_cnt];
				lock_data->record_entry = cur_cnt;
				lock_data->is_record = true;
				trace->pid = prev->pid;
				trace->start_ts = lock_data->start_ts;
				trace->wakeup_ts = lock_data->wakeup_ts;
				trace->get_ts = lock_data->get_ts;
				strncpy(trace->comm, task->comm, 32);
			}

			nr_entries = trace->nr_entries;

			if (unlikely(trace->nr_entries >= MAX_TRACE_ENTRIES - 1) || trace->nr_trace >= MAX_SCHED_ENTRIES -1)
				goto norecord;

			stack_trace = &trace->stack_trace[trace->nr_trace];
			stack_trace->timestampe = now;
			stack_trace->sched_in = sched_in;

			store_stack_trace(task, stack_trace, trace->entries + nr_entries, MAX_TRACE_ENTRIES - nr_entries, 0);
			trace->nr_entries += stack_trace->nr_entries;
			trace->nr_trace++;
norecord:
			trace->sched_cnt++;
			trace->last_ts = now;
out:
		}
		lock_data->sched_cnt++;
	} else {
		task->mce_addr = 0;
	}
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
	//lock_data->task = task;
	atomic_inc(&alloc_cnt);
	return lock_data;
}

static task_lock_data *get_pid_data(u32 pid)
{
	task_lock_data *lock_data;

	rcu_read_lock();
	lock_data = radix_tree_lookup(&pid_radix_root, pid);
	rcu_read_unlock();

	return lock_data;
}

static int free_pid_data(task_lock_data *lock_data)
{
	//struct task_struct *p = lock_data->task;
	spin_lock(&data_lock);
	radix_tree_delete(&pid_radix_root, lock_data->pid);
	spin_unlock(&data_lock);
	kfree(lock_data);
	atomic_dec(&alloc_cnt);

	//put_task_struct(p);
	return 0;
}

typedef void (*trace_callback)(task_lock_data *data, u64 addr);

static void lock_trace(u64 lock_addr, trace_callback func)
{
	task_lock_data *lock_data;

	if (lock_addr != hook_addr)
		return;

	lock_data = get_pid_data(current->pid);
	if (!lock_data)
		lock_data = attach_pid_data(current);

	if (lock_data) {
		lock_data->pid = current->pid;
		lock_data->start_ts = get_timestamp();
		lock_data->get_ts = 0;
		lock_data->end_ts = 0;
		lock_data->wakeup_ts = 0;
		lock_data->count= 0;
		current->mce_addr = 1;

		if (func)
			func(lock_data, lock_addr);
	}
}

static int unlock_trace(u64 lock_addr, trace_callback func)
{
	u32 pid;
	task_lock_data *lock_data;

	if (lock_addr != hook_addr)
		return 0;

	if (current->mce_addr != 2)
		return 0;

	pid = current->pid;
	lock_data = get_pid_data(pid);
	if (lock_data) {
		lock_data->end_ts = get_timestamp();
#if 0
		delta = lock_data->end_ts - lock_data->start_ts;
		if (unlikely(debug_enable)) {
			trace_printk("%d R %lld %lld %lld %lld %d  %lld\n",  lock_data->pid
					, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
					, lock_data->wakeup_ts, lock_data->sched_cnt, delta);
		} else {
			if (delta > trace_latency) {
				trace_printk("%d R %lld %lld %lld %lld %d  %lld\n",  lock_data->pid
						, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
						, lock_data->wakeup_ts, lock_data->sched_cnt, delta);
			}
		}
#else
		if (func)
			func(lock_data, lock_addr);
#endif

		if (lock_data->is_record) {
			struct task_sched_trace *trace;
			int cur_cnt;
			cur_cnt = lock_data->record_entry;
			trace = &sched_trace_head[cur_cnt];
			trace->end_ts = lock_data->end_ts;
		}

		free_pid_data(lock_data);
		current->mce_addr = 3;
	}

	return 0;
}

static void kprobe_down_read_callback(task_lock_data *lock_data, u64 addr)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) addr;
	lock_data->count= atomic_long_read(&sem->count)>>8;
}

static int kprobe_down_read(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;

	if (!en_trace)
		return 0;

	lock_trace((u64)sem, kprobe_down_read_callback);
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
	if (lock_data && current->mce_addr == 1) {
		lock_data->get_ts = get_timestamp();
		current->mce_addr = 2;
	}

	return 0;
}

static void kprobe_rwsem_callback(task_lock_data *lock_data, u64 addr, char *name)
{
	//struct rw_semaphore *sem = (struct rw_semaphore*) addr;
	u64 delta;

	delta = lock_data->end_ts - lock_data->start_ts;

	if (unlikely(debug_enable)) {
		trace_printk("%d %s %lld %lld %lld %lld %lld \n",  lock_data->pid, name,
				lock_data->start_ts, lock_data->get_ts, lock_data->end_ts,
				lock_data->wakeup_ts, lock_data->count);
	} else {
		if (delta > trace_latency) {
			//if (lock_data->is_throttle)
			//	delta = current->se.cfs_rq->throttled_clock_task - lock_data->throttle_time;
			trace_printk("%d %s %lld %lld %lld %lld %lld  %lld\n",  lock_data->pid, name,
					lock_data->start_ts, lock_data->get_ts, lock_data->end_ts,
					lock_data->wakeup_ts, lock_data->count, delta);
		}
	}
}

static void kprobe_up_read_callback(task_lock_data *lock_data, u64 addr)
{
	kprobe_rwsem_callback(lock_data, addr, "LR");
}

static int kprobe_up_read(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;
	if (!en_trace)
		return 0;

	unlock_trace((u64) sem, kprobe_up_read_callback);
	return 0;
}

static void kprobe_up_write_callback(task_lock_data *lock_data, u64 addr)
{
	kprobe_rwsem_callback(lock_data, addr, "LW");
}

static int kprobe_up_write(struct kprobe *kp, struct pt_regs *regs)
{
	struct rw_semaphore *sem = (struct rw_semaphore*) regs->di;
	if (!en_trace)
		return 0;

	if (current->mce_addr != 2)
		return 0;

	unlock_trace((u64) sem, kprobe_up_read_callback);

	return 0;
}

static int kprobe_mutex_lock(struct kprobe *kp, struct pt_regs *regs)
{
	struct mutex *mutex = (struct mutex*) regs->di;
	//task_lock_data *lock_data;

	if (!en_trace)
		return 0;

	lock_trace((u64)mutex, NULL);

	return 0;
}

static int kretprobe_mutex_lock(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	task_lock_data *lock_data;
	if (!en_trace)
		return 0;

	if (current->mce_addr != 1)
		return 0;

	lock_data = get_pid_data(current->pid);
	if (lock_data && current->mce_addr == 1) {
		lock_data->get_ts = get_timestamp();
		current->mce_addr = 2;
	}

	return 0;
}

static void mutex_unlock_callback(task_lock_data *lock_data, u64 addr)
{
	u64 delta;

	delta = lock_data->end_ts - lock_data->start_ts;
	if (unlikely(debug_enable)) {
		trace_printk("%d LM %lld %lld %lld %lld %d  %lld\n",  lock_data->pid
				, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
				, lock_data->wakeup_ts, lock_data->sched_cnt, delta);
	} else {
		if (delta > trace_latency) {
			trace_printk("%d LM %lld %lld %lld %lld %d  %lld\n",  lock_data->pid
					, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
					, lock_data->wakeup_ts, lock_data->sched_cnt, delta);
		}
	}
}

static int kprobe_mutex_unlock(struct kprobe *kp, struct pt_regs *regs)
{
	struct mutex *mutex = (struct mutex*) regs->di;
	//u32 pid;
	//u64 delta;
	//task_lock_data *lock_data;

	if (!en_trace)
		return 0;

#if 0
	if ((u64)mutex != hook_addr)
		return 0;

	if (current->mce_addr != 2)
		return 0;

	pid = current->pid;
	lock_data = get_pid_data(pid);
	if (lock_data) {
		lock_data->end_ts = get_timestamp();
		delta = lock_data->end_ts - lock_data->start_ts;
		if (unlikely(debug_enable)) {
			trace_printk("%d R %lld %lld %lld %lld %d  %lld\n",  lock_data->pid
					, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
					, lock_data->wakeup_ts, lock_data->sched_cnt, delta);
		} else {
			if (delta > trace_latency) {
				trace_printk("%d R %lld %lld %lld %lld %d  %lld\n",  lock_data->pid
						, lock_data->start_ts, lock_data->get_ts, lock_data->end_ts
						, lock_data->wakeup_ts, lock_data->sched_cnt, delta);
			}
		}

		if (lock_data->is_record) {
			struct task_sched_trace *trace;
			int cur_cnt;
			cur_cnt = lock_data->record_entry;
			trace = &sched_trace_head[cur_cnt];
			trace->end_ts = lock_data->end_ts;
		}

		free_pid_data(lock_data);
		current->mce_addr = 3;
	}
#else
	unlock_trace((u64)mutex, mutex_unlock_callback);
#endif

	return 0;
}

struct kprobe kplist_rwsem[] = {
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
        .pre_handler = kprobe_down_read,
	},
	{
        .symbol_name = "up_write",
        .pre_handler = kprobe_up_write,
	},
};

struct kretprobe kretlist_rwsem[]  = {
	{
		.kp.symbol_name = "down_read",
		.handler= kretprobe_down_read,
	},
	{
		.kp.symbol_name = "down_write",
		.handler= kretprobe_down_read,
	},
};

struct kprobe kplist_mutex[] = {
	{
        .symbol_name = "mutex_lock",
        .pre_handler = kprobe_mutex_lock,
	},
	{
        .symbol_name = "mutex_lock_killable",
        .pre_handler = kprobe_mutex_lock,
	},
	{
        .symbol_name = "mutex_lock_interruptible",
        .pre_handler = kprobe_mutex_lock,
	},
	{
        .symbol_name = "mutex_lock_io",
        .pre_handler = kprobe_mutex_lock,
	},
	{
        .symbol_name = "mutex_trylock",
        .pre_handler = kprobe_mutex_lock,
	},
	{
        .symbol_name = "mutex_unlock",
        .pre_handler = kprobe_mutex_unlock,
	},
};

struct kretprobe kretlist_mutex[]  = {
	{
		.kp.symbol_name = "mutex_lock",
		.handler= kretprobe_mutex_lock,
	},
	{
        .kp.symbol_name = "mutex_lock_killable",
        .handler = kretprobe_mutex_lock,
	},
	{
        .kp.symbol_name = "mutex_lock_interruptible",
        .handler = kretprobe_mutex_lock,
	},
	{
        .kp.symbol_name = "mutex_lock_io",
        .handler = kretprobe_mutex_lock,
	},
	{
        .kp.symbol_name = "mutex_trylock",
        .handler = kretprobe_mutex_lock,
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

    seq_printf(m, "lock addr: %llx\n", hook_addr);

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

	if (latency == 0) {
		if (en_trace) {
#if 0
			trace_sched_wakeup_enable(false);
			free_pid_lock_data();
			memset(sched_trace_head, 0, sizeof(struct task_sched_trace) * MAX_TRACE_CNT);
			atomic_set(&sched_trace_cnt,-1);
			trace_sched_wakeup_enable(true);
#else
			en_trace = false;
			synchronize_rcu();
			memset(sched_trace_head, 0, sizeof(struct task_sched_trace) * MAX_TRACE_CNT);
			atomic_set(&sched_trace_cnt,-1);
			en_trace = true;
#endif
		} else {
			memset(sched_trace_head, 0, sizeof(struct task_sched_trace) * MAX_TRACE_CNT);
			atomic_set(&sched_trace_cnt,-1);
		}
		return count;
	}

	if (latency < 10000)
		return -EINVAL;

	trace_latency = latency * 1000UL;

	return count;
}

static void seq_print_stack_trace(struct seq_file *m, struct sched_trace *trace)
{
	int i;

	if (WARN_ON(!trace->entries))
		return;

	for (i = 0; i < trace->nr_entries; i++)
		seq_printf(m, "%*c%pS\n", 5, ' ', (void *)trace->entries[i]);
}

static int trace_latency_show(struct seq_file *m, void *v)
{
	int i,cnt = atomic_read(&sched_trace_cnt);
	seq_printf(m, "trace_latency: %llu us\n\n",
			trace_latency/(1000UL));
	seq_printf(m, "L: lock start  G: lock get E:unlock T:sched last ts S: sched switch\n");

	for (i = 0; i <= cnt; ++i) {
		struct task_sched_trace *trace;
		int j;
		u64 last_ts, delta = 0;

		trace = &sched_trace_head[i];

		if (trace->end_ts)
			delta = trace->end_ts - trace->get_ts;
		else
			delta = trace->last_ts - trace->get_ts;

		seq_printf(m, "%s %lld ms [L:%lld G:%lld E:%lld T:%lld S:%d]\n",
			trace->comm,  delta,
			trace->start_ts,
			trace->get_ts, trace->end_ts,
			trace->last_ts,
			trace->sched_cnt
			);

		last_ts = trace->get_ts;
		for (j = 0;  j < trace->nr_trace; ++j) {
			struct sched_trace *stack_trace = &trace->stack_trace[j];
			seq_printf(m, "%s: %llu us\n",  stack_trace->sched_in ? "sched-in" : "sched-out",
					    (stack_trace->timestampe - last_ts)/1000);
			seq_print_stack_trace(m, stack_trace);
			seq_putc(m, '\n');
			cond_resched();
			last_ts = stack_trace->timestampe;
		}
	}

	return 0;
}

static int trace_latency_open(struct inode *inode, struct file *file)
{
    return single_open(file, trace_latency_show, inode->i_private);
}

PROC_MARCO(trace_latency);

static int enable_show(struct seq_file *m, void *ptr)
{
	seq_printf(m, "%s\n", en_trace? "enabled" : "disabled");
	return 0;
}

static int enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, enable_show, inode->i_private);
}

static int trace_kprobe_ctrl_mutex(bool en)
{
	int i;
	struct kprobe *kpo_array[sizeof(kplist_mutex)/sizeof(struct kprobe)];
	struct kretprobe *kretpo_array[sizeof(kretlist_mutex)/sizeof(struct kretprobe)];

	if (en) {
		for (i = 0; i < sizeof(kretlist_mutex)/sizeof(struct kretprobe); i++) {
			kretlist_mutex[i].kp.addr = NULL;
			kretlist_mutex[i].kp.flags = 0;
			kretpo_array[i] = &kretlist_mutex[i];
		}

		for (i = 0; i < sizeof(kplist_mutex)/sizeof(struct kprobe); i++) {
			kplist_mutex[i].addr = NULL;
			kplist_mutex[i].flags = 0;
			kpo_array[i] = &kplist_mutex[i];
		}

		if (register_kretprobes(kretpo_array, sizeof(kretlist_mutex)/sizeof(struct kretprobe)))  {
			printk("register kretprobe failed\n");
			return 1;
		}

		if (register_kprobes(kpo_array, sizeof(kplist_mutex)/sizeof(struct kprobe))) {
			printk("register kprobe failed\n");
			goto out1;
		}
	} else  {
		for (i = 0; i < sizeof(kretlist_mutex)/sizeof(struct kretprobe); i++)
			kretpo_array[i] = &kretlist_mutex[i];

		for (i = 0; i < sizeof(kplist_mutex)/sizeof(struct kprobe); i++)
			kpo_array[i] = &kplist_mutex[i];

		synchronize_rcu();
		unregister_kretprobes(kretpo_array, sizeof(kretlist_mutex)/sizeof(struct kretprobe));
		synchronize_rcu();
		unregister_kprobes(kpo_array, sizeof(kplist_mutex)/sizeof(struct kprobe));
	}
	return 0;
out1:
	unregister_kretprobes(kretpo_array, sizeof(kretlist_mutex)/sizeof(struct kretprobe));
	return 0;
}

static int trace_kprobe_ctrl_rwsem(bool en)
{
	int i;
	struct kprobe *kpo_array[sizeof(kplist_rwsem)/sizeof(struct kprobe)];
	struct kretprobe *kretpo_array[sizeof(kretlist_rwsem)/sizeof(struct kretprobe)];

	if (en) {
		for (i = 0; i < sizeof(kretlist_rwsem)/sizeof(struct kretprobe); i++) {
			kretlist_rwsem[i].kp.addr = NULL;
			kretlist_rwsem[i].kp.flags = 0;
			kretpo_array[i] = &kretlist_rwsem[i];
		}

		for (i = 0; i < sizeof(kplist_rwsem)/sizeof(struct kprobe); i++) {
			kplist_rwsem[i].addr = NULL;
			kplist_rwsem[i].flags = 0;
			kpo_array[i] = &kplist_rwsem[i];
		}

		if (register_kretprobes(kretpo_array, sizeof(kretlist_rwsem)/sizeof(struct kretprobe)))  {
			printk("register kretprobe failed\n");
			return 1;
		}

		if (register_kprobes(kpo_array, sizeof(kplist_rwsem)/sizeof(struct kprobe))) {
			printk("register kprobe failed\n");
			goto out1;
		}
	} else  {
		for (i = 0; i < sizeof(kretlist_rwsem)/sizeof(struct kretprobe); i++)
			kretpo_array[i] = &kretlist_rwsem[i];

		for (i = 0; i < sizeof(kplist_rwsem)/sizeof(struct kprobe); i++)
			kpo_array[i] = &kplist_rwsem[i];

		synchronize_rcu();
		unregister_kretprobes(kretpo_array, sizeof(kretlist_rwsem)/sizeof(struct kretprobe));
		synchronize_rcu();
		unregister_kprobes(kpo_array, sizeof(kplist_rwsem)/sizeof(struct kprobe));
	}
	return 0;
out1:
	unregister_kretprobes(kretpo_array, sizeof(kretlist_rwsem)/sizeof(struct kretprobe));
	return 0;
}

static void trace_sched_wakeup_enable(bool en)
{
	int ret;

	if (en && !en_trace) {

		en_trace = en;

		if (trace_mode == TRACE_MUTEX) {
			ret = trace_kprobe_ctrl_mutex(en);
			if (ret)
				goto out;
		} else if (trace_mode == TRACE_RWSEM) {
			ret = trace_kprobe_ctrl_rwsem(en);
			if (ret)
				goto out;
		} else
			goto out;

		tracepoint_probe_register(orig___tracepoint_sched_wakeup_new, trace_tracepoint_sched_wakeup_new, NULL);
		tracepoint_probe_register(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
		tracepoint_probe_register(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
	} else {
		if (en_trace) {

			en_trace = false;

			if (trace_mode == TRACE_MUTEX)
				trace_kprobe_ctrl_mutex(en);
			else if (trace_mode == TRACE_RWSEM)
				trace_kprobe_ctrl_rwsem(en);
			else
				goto out;

			tracepoint_probe_unregister(orig___tracepoint_sched_wakeup_new, trace_tracepoint_sched_wakeup_new, NULL);
			tracepoint_probe_unregister(orig___tracepoint_sched_wakeup, trace_tracepoint_sched_wakeup, NULL);
			tracepoint_probe_unregister(orig___tracepoint_sched_switch, trace_tracepoint_sched_switch, NULL);
		}
	}

	return;
out:
	en_trace = false;
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

PROC_MARCO(enable);

static int noop_pre_handler(struct kprobe *p, struct pt_regs *regs) {
	return 0;
}

static int sym_init(void)
{
	LOOKUP_SYMS(running_clock);
	LOOKUP_SYMS(__tracepoint_sched_wakeup_new);
	LOOKUP_SYMS(__tracepoint_sched_wakeup);
	LOOKUP_SYMS(__tracepoint_sched_switch);
	LOOKUP_SYMS(stack_trace_save_tsk);
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

	sched_trace_head = kvmalloc(sizeof(struct task_sched_trace) * MAX_TRACE_CNT, GFP_KERNEL);
	if (!sched_trace_head)
		return -EINVAL;

	memset(sched_trace_head, 0, sizeof(struct task_sched_trace) * MAX_TRACE_CNT);

	atomic_set(&alloc_cnt, 0);
	pidtrace_kmem_cache = kmem_cache_create("locktrace_data", sizeof(task_lock_data), 0, SLAB_HWCACHE_ALIGN, NULL);
	if (!pidtrace_kmem_cache) {
		printk("alloc kmemcache locktrace failed\n");
		goto free;
	}

	parent_dir = proc_mkdir("locktrace", NULL);
	if (!parent_dir)
		goto remove_kmemcache;

	if (!proc_create("trace_latency", S_IRUSR | S_IWUSR, parent_dir,
				 &trace_latency_fops))
		goto remove_proc;

	if (!proc_create("enable", S_IRUSR | S_IWUSR, parent_dir, &enable_fops))
		goto remove_proc;

	if (!proc_create("lock", S_IRUSR | S_IWUSR, parent_dir,
				 &trace_lockaddr_fops))
		goto remove_proc;

	atomic_set(&sched_trace_cnt,-1);

	INIT_RADIX_TREE(&pid_radix_root, GFP_ATOMIC);
	spin_lock_init(&data_lock);
	trace_mode = TRACE_MUTEX;

	return 0;

remove_proc:
	remove_proc_subtree("trace_irqoff", NULL);
remove_kmemcache:
	kmem_cache_destroy(pidtrace_kmem_cache);
free:
	kvfree(sched_trace_head);
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
			//put_task_struct(lock_data->task);
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
	kvfree(sched_trace_head);
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
