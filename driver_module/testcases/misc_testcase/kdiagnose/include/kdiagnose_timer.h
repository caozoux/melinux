#ifndef __KDIAGNOSE_TIMER__
#define __KDIAGNOSE_TIMER__

typedef void (*monitor_handle)(void *data);
struct dg_mt_timer {
	monitor_handle func;
	struct list_head list;
	void *data;
};

int register_hrtime_moninter(struct dg_mt_timer *timer);
int remove_hrtime_moninter(struct dg_mt_timer *timer);
int diagnose_hrtime_init(void);

#endif /* ifndef __KDIAGNOSE_TIMER__ */

