#ifndef __KSCHEDLOCAL_H__
#define __KSCHEDLOCAL_H__

typedef struct task_group *rt_rq_iter_t;

static inline struct task_group *next_task_group(struct task_group *tg)
{
    do {
        tg = list_entry_rcu(tg->list.next,
            typeof(struct task_group), list);
    } while (&tg->list != &task_groups && task_group_is_autogroup(tg));

    if (&tg->list == &task_groups)
        tg = NULL;

    return tg;
}


#define for_each_rt_rq(rt_rq, iter, rq)                 \
		for (iter = container_of(&task_groups, typeof(*iter), list);    \
			(iter = next_task_group(iter)) &&           \
			(rt_rq = iter->rt_rq[cpu_of(rq)]);)

extern struct rq *orig_runqueues;
#define cpu_rq(cpu)     (&per_cpu(*orig_runqueues, (cpu)))
#define for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos)          \
	list_for_each_entry_safe(cfs_rq, pos, &rq->leaf_cfs_rq_list,    \
			leaf_cfs_rq_list)

/* Iterate through all leaf cfs_rq's on a runqueue: */
#define for_each_leaf_cfs_rq(rq, cfs_rq) \
    list_for_each_entry_rcu(cfs_rq, &rq->leaf_cfs_rq_list, leaf_cfs_rq_list)

static inline struct task_struct *task_of(struct sched_entity *se)
{
	SCHED_WARN_ON(!entity_is_task(se));
	return container_of(se, struct task_struct, se);
}

void ksched_print_cpu(int cpu);
void print_rq(struct rq *rq, int rq_cpu);
void print_task(struct rq *rq, struct task_struct *p);

#endif

