#ifndef __KSCHED_H__
#define  __KSCHED_H__

enum IOCTL_KRPOBE_SUB {
	IOCTL_KSCHED_NONE = 0,
	//dumpstack of function
	IOCTL_KSCHED_DUMP,
	IOCTL_KSCHED_MONITOR_PID,
};

struct sched_entity_patial {
    //struct load_avg
    unsigned long           weight;
    unsigned long           load_sum;
    unsigned long           runnable_sum;
    unsigned int            util_sum;
    unsigned int            period_contrib;
    unsigned long           load_avg;
    unsigned long           runnable_avg;
    unsigned long           util_avg;
};

struct cfs_rq_patial {
	unsigned long           weight;
	unsigned int        nr_running;
	unsigned int        h_nr_running; 

    //struct load_avg
    unsigned long           last_update_time;
    unsigned long           load_sum;
    unsigned long           runnable_sum;
    unsigned int            util_sum;
    unsigned int            period_contrib;
    unsigned long           load_avg;
    unsigned long           runnable_avg;
    unsigned long           util_avg;
};

struct rq_patial{
    unsigned int        nr_running;
    unsigned int        nr_numa_running;
    unsigned int        nr_preferred_running;

};

struct ksched_ioctl {
	int enable;
	union {
		struct {
			int pid;
			struct sched_entity_patial se;
			struct cfs_rq_patial cfs_rq;
			struct rq_patial rq;
		};
	};
};

#endif
