#ifndef __ME_KTRACE_H__
#define __ME_KTRACE_H__

#define MAX_HWIRQ   (1024)

typedef struct time_record {
	u64 start;
	u64 end;
};

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
	TRACE_IRQ_TYPE irqtype;
	time_record  t_record;
};

struct trace_timer {
	TRACE_TIMER_TYPE timertype;
	time_record  t_record;
};

struct kd_percpu_data {
	struct trace_irq  trace_hwirq[MAX_HWIRQ];
	struct trace_irq  trace_softirq[NR_SOFTIRQS];
	struct trace_timer trace_hrtimer;
	struct trace_timer trace_softtimer;
};

int register_tracepoint(const char *name, void *probe, void *data);
int unregister_tracepoint(const char *name, void *probe, void *data);
#endif

