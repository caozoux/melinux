#ifndef __KPERCPU_H__
#define __KPERCPU_H__

typedef struct {
	u64 prev_time;
	struct task_struct *prev_task;
} percpu_runtime_stat ;

struct percpu_data {
	int cpu;
	struct percpu_runtime_stat
};

struct percpu_data *get_ksys_percpu(void);
int percpu_variable_init(void);
int percpu_variable_exit(void);

#endif

