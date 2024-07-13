#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ioctl_kprobe.h"
#include "ksioctl/ksched_ioctl.h"
#include "ksched_local.h"

#define cpu_rq_cp(cpu) (&per_cpu(*orig_runqueues, (cpu)))
#define for_each_domain_cp(cpu, __sd) \
	for (__sd = rcu_dereference_check_sched_domain(cpu_rq_cp(cpu)->sd); \
		__sd; __sd = __sd->parent)

static inline unsigned long cfs_rq_runnable_load_avg(struct cfs_rq *cfs_rq)
{
	return cfs_rq->avg.runnable_load_avg;
}

static inline unsigned long cfs_rq_load_avg(struct cfs_rq *cfs_rq)
{
	return cfs_rq->avg.load_avg;
}

unsigned long weighted_cpuload(struct rq *rq)
{
	return cfs_rq_runnable_load_avg(&rq->cfs);
}

void dump_sched_groups(struct sched_domain *sd)
{
	struct sched_group *group  = sd->groups;

	do {
		group = group->next;
	} while (group != sd->groups);
}

void dump_sched_domain(struct seq_file *seq)
{
	int cpu;
	struct sched_domain *sd;
	struct sched_group* sg;
	unsigned long load, avg_load, runnable_load;

#if 0
	struct rq *rq;
	for_each_online_cpu(cpu) {
		rq = &per_cpu(*orig_runqueues, cpu);
		sd = rq->sd;
		printk("zz %s sd:%lx \n",__func__, (unsigned long)sd);
		sg = sd->groups;
	}
#else

	for_each_online_cpu(cpu) {
		avg_load = cfs_rq_load_avg(&cpu_rq_cp(cpu)->cfs);
		load = weighted_cpuload(cpu_rq_cp(cpu));
		seq_printf(seq, "cpu%d: %d %d\n", cpu, avg_load, load);

		for_each_domain_cp(cpu, sd) 
			dump_sched_groups(sd);
	}

#endif
}

static int load_show(struct seq_file *seq, void *offset)
{
	dump_sched_domain(seq);
	return 0;
}

int ksched_domain_init(void)
{
	proc_create_single_data("show_load", S_IRUGO, ksched_proc,
  			load_show, NULL);
	return 0;
}

int ksched_domain_exit(void)
{
	return 0;
}

