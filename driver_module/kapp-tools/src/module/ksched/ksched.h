#ifndef __KSCHED_H__
#define __KSCHED_H__

#include <linux/sched/topology.h>
#include "kernel/sched/sched.h"

extern struct proc_dir_entry *ksched_proc;
extern struct rq __percpu *orig_runqueues;
void dump_sched_domain(struct seq_file *seq);
int ksched_domain_init(void);
int ksched_domain_exit(void);

#endif
