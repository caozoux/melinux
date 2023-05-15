#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>
#include <hotfix_util.h>

#include "local.h"
#include "internal.h"

//#include "ktrace.h"

extern struct kd_percpu_data *kd_percpu_data[512];
struct perf_callchain_entry *
(*orig_get_perf_callchain)(struct pt_regs *regs, u32 init_nr, bool kernel, bool user,
		  u32 max_stack, bool crosstask, bool add_mark);
int (*orig_get_callchain_buffers)(int event_max_stack);
struct softirq_action *orig_softirq_vec;
struct task_struct *(*orig_find_task_by_vpid)(pid_t vnr);

struct kd_percpu_data *get_kd_percpu_data(void)
{
	int cpu = smp_processor_id();
	return kd_percpu_data[cpu];
}

int base_func_init(void)
{
	LOOKUP_SYMS(softirq_vec);
	LOOKUP_SYMS(find_task_by_vpid);
	LOOKUP_SYMS(get_perf_callchain);
	LOOKUP_SYMS(get_callchain_buffers);
	base_trace_init();
	return 0;
}

struct task_struct *find_process_by_pid(pid_t vnr)
{
	return orig_find_task_by_vpid(vnr);
}


