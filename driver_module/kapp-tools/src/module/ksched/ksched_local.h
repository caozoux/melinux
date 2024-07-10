#ifndef __KSCHED_LOCAL_H__
#define __KSCHED_LOCAL_H__

#include "kernel/sched/sched.h"

extern struct rq __percpu * orig_runqueues;
#define this_rq_cp() this_cpu_ptr(orig_runqueues)
#define cpu_rq_cp(cpu)  (&per_cpu(*orig_runqueues, (cpu)))

#endif
