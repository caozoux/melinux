#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/timekeeper_internal.h>
#include <kernel/sched/sched.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"

/*
 * Ease the printing of nsec fields:
 */
static long long nsec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}

static unsigned long nsec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}
#define SPLIT_NS(x) nsec_high(x), nsec_low(x)

#define P(F)		printk("  .%-30s: %lld\n",	#F, (long long)F)
#define P_SCHEDSTAT(F)	printk("  .%-30s: %lld\n",	#F, (long long)schedstat_val(F))
#define PN(F)		printk("  .%-30s: %lld.%06ld\n", #F, SPLIT_NS((long long)F))
#define PN_SCHEDSTAT(F)	printk("  .%-30s: %lld.%06ld\n", #F, SPLIT_NS((long long)schedstat_val(F)))

static void print_cfs_group_stats(int cpu, struct task_group *tg)
{
	struct sched_entity *se = tg->se[cpu];


	if (!se)
		return;

	PN(se->exec_start);
	PN(se->vruntime);
	PN(se->sum_exec_runtime);

	if (schedstat_enabled()) {
		PN_SCHEDSTAT(se->statistics.wait_start);
		PN_SCHEDSTAT(se->statistics.sleep_start);
		PN_SCHEDSTAT(se->statistics.block_start);
		PN_SCHEDSTAT(se->statistics.sleep_max);
		PN_SCHEDSTAT(se->statistics.block_max);
		PN_SCHEDSTAT(se->statistics.exec_max);
		PN_SCHEDSTAT(se->statistics.slice_max);
		PN_SCHEDSTAT(se->statistics.wait_max);
		PN_SCHEDSTAT(se->statistics.wait_sum);
		P_SCHEDSTAT(se->statistics.wait_count);
	}

	P(se->load.weight);
	P(se->runnable_weight);
	P(se->avg.load_avg);
	P(se->avg.util_avg);
	P(se->avg.runnable_load_avg);
	P(se->load.weight);
	P(se->runnable_weight);
	P(se->avg.load_avg);
	P(se->avg.util_avg);
	P(se->avg.runnable_load_avg);
}

static void
print_task(struct rq *rq, struct task_struct *p)
{
	if (rq->curr == p)
		printk(">R");
	else
		printk(" %c", task_state_to_char(p));

	printk("%15s %5d %9Ld.%06ld %9Ld %5d ",
		p->comm, task_pid_nr(p),
		SPLIT_NS(p->se.vruntime),
		(long long)(p->nvcsw + p->nivcsw),
		p->prio);

	printk("%9Ld.%06ld %9Ld.%06ld %9Ld.%06ld",
		SPLIT_NS(schedstat_val_or_zero(p->se.statistics.wait_sum)),
		SPLIT_NS(p->se.sum_exec_runtime),
		SPLIT_NS(schedstat_val_or_zero(p->se.statistics.sum_sleep_runtime)));

	printk(" %d %d", task_node(p), task_numa_group_id(p));
	printk(" %s", task_group_path(task_group(p)));
	printk("\n");
}

static void print_rq(struct seq_file *m, struct rq *rq, int rq_cpu)
{
	struct task_struct *g, *p;

	printk("\n");
	printk("runnable tasks:\n");
	printk(" S           task   PID         tree-key  switches  prio"
		   "     wait-time             sum-exec        sum-sleep\n");
	printk("-------------------------------------------------------"
		   "----------------------------------------------------\n");

	rcu_read_lock();
	for_each_process_thread(g, p) {
		if (task_cpu(p) != rq_cpu)
			continue;

		print_task(m, rq, p);
	}
	rcu_read_unlock();
}

void print_cfs_rq(struct seq_file *m, int cpu, struct cfs_rq *cfs_rq)
{
	s64 MIN_vruntime = -1, min_vruntime, max_vruntime = -1,
		spread, rq0_min_vruntime, spread0;
	struct rq *rq = cpu_rq(cpu);
	struct sched_entity *last;
	unsigned long flags;

	printk("\n");
	printk("cfs_rq[%d]:%s\n", cpu, task_group_path(cfs_rq->tg));
	printk("  .%-30s: %Ld.%06ld\n", "exec_clock",
			SPLIT_NS(cfs_rq->exec_clock));

	raw_spin_lock_irqsave(&rq->lock, flags);
	if (rb_first_cached(&cfs_rq->tasks_timeline))
		MIN_vruntime = (__pick_first_entity(cfs_rq))->vruntime;
	last = __pick_last_entity(cfs_rq);
	if (last)
		max_vruntime = last->vruntime;
	min_vruntime = cfs_rq->min_vruntime;
	rq0_min_vruntime = cpu_rq(0)->cfs.min_vruntime;
	raw_spin_unlock_irqrestore(&rq->lock, flags);
	printk("  .%-30s: %Ld.%06ld\n", "MIN_vruntime",
			SPLIT_NS(MIN_vruntime));
	printk("  .%-30s: %Ld.%06ld\n", "min_vruntime",
			SPLIT_NS(min_vruntime));
	printk("  .%-30s: %Ld.%06ld\n", "max_vruntime",
			SPLIT_NS(max_vruntime));
	spread = max_vruntime - MIN_vruntime;
	printk("  .%-30s: %Ld.%06ld\n", "spread",
			SPLIT_NS(spread));
	spread0 = min_vruntime - rq0_min_vruntime;
	printk("  .%-30s: %Ld.%06ld\n", "spread0",
			SPLIT_NS(spread0));
	printk("  .%-30s: %d\n", "nr_spread_over",
			cfs_rq->nr_spread_over);
	printk("  .%-30s: %d\n", "nr_running", cfs_rq->nr_running);
	printk("  .%-30s: %ld\n", "load", cfs_rq->load.weight);
	printk("  .%-30s: %ld\n", "runnable_weight", cfs_rq->runnable_weight);
	printk("  .%-30s: %lu\n", "load_avg",
			cfs_rq->avg.load_avg);
	printk("  .%-30s: %lu\n", "runnable_load_avg",
			cfs_rq->avg.runnable_load_avg);
	printk("  .%-30s: %lu\n", "util_avg",
			cfs_rq->avg.util_avg);
	printk("  .%-30s: %u\n", "util_est_enqueued",
			cfs_rq->avg.util_est.enqueued);
	printk("  .%-30s: %ld\n", "removed.load_avg",
			cfs_rq->removed.load_avg);
	printk("  .%-30s: %ld\n", "removed.util_avg",
			cfs_rq->removed.util_avg);
	printk("  .%-30s: %ld\n", "removed.runnable_sum",
			cfs_rq->removed.runnable_sum);
	printk("  .%-30s: %lu\n", "tg_load_avg_contrib",
			cfs_rq->tg_load_avg_contrib);
	printk("  .%-30s: %ld\n", "tg_load_avg",
			atomic_long_read(&cfs_rq->tg->load_avg));
	printk("  .%-30s: %d\n", "throttled",
			cfs_rq->throttled);
	printk("  .%-30s: %d\n", "throttle_count",
			cfs_rq->throttle_count);

	print_cfs_group_stats(m, cpu, cfs_rq->tg);
}

void print_rt_rq(struct seq_file *m, int cpu, struct rt_rq *rt_rq)
{
	printk("\n");
	printk("rt_rq[%d]:%s\n", cpu, task_group_path(rt_rq->tg));

#define P(x) \
	printk("  .%-30s: %Ld\n", #x, (long long)(rt_rq->x))
#define PU(x) \
	printk("  .%-30s: %lu\n", #x, (unsigned long)(rt_rq->x))
#define PN(x) \
	printk("  .%-30s: %Ld.%06ld\n", #x, SPLIT_NS(rt_rq->x))

	PU(rt_nr_running);
	PU(rt_nr_migratory);
	P(rt_throttled);
	PN(rt_time);
	PN(rt_runtime);

}

void print_dl_rq(struct seq_file *m, int cpu, struct dl_rq *dl_rq)
{
	struct dl_bw *dl_bw;

	printk("\n");
	printk("dl_rq[%d]:\n", cpu);

#define PU(x) \
	printk("  .%-30s: %lu\n", #x, (unsigned long)(dl_rq->x))

	PU(dl_nr_running);
	PU(dl_nr_migratory);
	dl_bw = &cpu_rq(cpu)->rd->dl_bw;
	printk("  .%-30s: %lld\n", "dl_bw->bw", dl_bw->bw);
	printk("  .%-30s: %lld\n", "dl_bw->total_bw", dl_bw->total_bw);

}

static void print_cpu(struct seq_file *m, int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long flags;

	{
		unsigned int freq = cpu_khz ? : 1;

		printk("cpu#%d, %u.%03u MHz\n",
			   cpu, freq / 1000, (freq % 1000));
	}

#define P(x)								\
do {									\
	if (sizeof(rq->x) == 4)						\
		printk("  .%-30s: %ld\n", #x, (long)(rq->x));	\
	else								\
		printk("  .%-30s: %Ld\n", #x, (long long)(rq->x));\
} while (0)

	printk("  .%-30s: %Ld.%06ld\n", #x, SPLIT_NS(rq->x))

	P(nr_running);
	P(nr_switches);
	P(nr_load_updates);
	P(nr_uninterruptible);
	PN(next_balance);
	printk("  .%-30s: %ld\n", "curr->pid", (long)(task_pid_nr(rq->curr)));
	PN(clock);
	PN(clock_task);

#define P64(n) printk("  .%-30s: %Ld\n", #n, rq->n);
	P64(avg_idle);
	P64(max_idle_balance_cost);

#define P(n) printk("  .%-30s: %d\n", #n, schedstat_val(rq->n));
	if (schedstat_enabled()) {
		P(yld_count);
		P(sched_count);
		P(sched_goidle);
		P(ttwu_count);
		P(ttwu_local);
	}

	spin_lock_irqsave(&sched_debug_lock, flags);
	print_cfs_stats(m, cpu);
	print_rt_stats(m, cpu);
	print_dl_stats(m, cpu);

	print_rq(m, rq, cpu);
	spin_unlock_irqrestore(&sched_debug_lock, flags);
	printk("\n");
}
