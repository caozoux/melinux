#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <ksioctl/ksched_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ksched_local.h"

#define for_each_domain(cpu, __sd) \
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

void sched_sched_group_dump(struct sched_domain *sd)
{
	struct sched_group * group = sd->groups;	
	int i;

	do {
		unsigned long load, avg_load, runnable_load;
		for_each_cpu(i, sched_group_span(group)) {
			load = cfs_rq_runnable_load_avg(&cpu_rq_cp(i)->cfs);
			avg_load = cfs_rq_load_avg(&cpu_rq_cp(i)->cfs);
		}
	} while(group =group->next, group != sd->groups);
}

void sched_sched_domain_dump(struct sched_domain *sd_ptr)
{
	struct sched_domain * sd;	

	while(sd) {


		sd = sd->child;
	}
}

void sched_sched_domain(void)
{
	int cpu;
	struct sched_domain *tmp;

	for_each_domain(cpu, tmp) {		
			
	}
}

