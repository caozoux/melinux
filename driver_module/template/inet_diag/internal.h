#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#define MAX_HWIRQ   (1024)
#include<linux/interrupt.h>

typedef struct {
	u64 start;
	u64 end;
}time_record ;

typedef struct {
	u64 g_cnt;
	u64 min;
	u64 max;
	u64 avg;
}time_avg;

#define AVG_INIT(avg)  \
	do {   \
		avg->g_cnt = 0;	 \
		avg->min = ~0;	 \
		avg->max = 0;	\
	} while(0)

enum TRACE_IRQ_TYPE {
	TRACE_IRQ_TYPE_NONE = -1,
	// harware irq
	TRACE_IRQ_TYPE_HWIRQ,
	// softirq
	TRACE_IRQ_TYPE_SOFTIRQ,
};

enum TRACE_TIMER_TYPE {
	TRACE_TIMER_TYPE_NONE = -1,
	// hrtimer
	TRACE_TIMER_TYPE_HR,
	// soft timer
	TRACE_TIMER_TYPE_SOFT,
};

struct trace_irq {
	enum TRACE_IRQ_TYPE irqtype;
	time_record  t_record;
	time_avg     t_avg;
};

struct trace_timer {
	enum TRACE_TIMER_TYPE timertype;
	time_record  t_record;
	time_avg     t_avg;
};

struct kd_percpu_data {
	struct trace_irq  trace_hwirq[MAX_HWIRQ];
	struct trace_irq  trace_softirq[NR_SOFTIRQS];
	struct trace_timer trace_hrtimer;
	struct trace_timer trace_softtimer;
	u64    monnitor_hwirq;
	u64    monnitor_softirq;
	u64    monnitor_hrtimer;
	u64    monnitor_softtimer;
};

int register_tracepoint(const char *name, void *probe, void *data);
int unregister_tracepoint(const char *name, void *probe, void *data);
struct kd_percpu_data *get_kd_percpu_data(void);
int base_func_init(void);

#endif /* ifndef __INTERNAL_H__ */

