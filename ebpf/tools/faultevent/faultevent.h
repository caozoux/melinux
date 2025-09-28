#ifndef __TEMPLATE_H
#define __TEMPLATE_H

enum FAULT_EVENT {
       FE_SOFTLOCKUP,
       FE_RCUSTALL,
       FE_HUNGTASK,
       FE_OOM_GLOBAL,
       FE_OOM_CGROUP,
       FE_ALLOCFAIL,
       FE_LIST_CORRUPT,
       FE_MM_STATE,
       FE_IO_ERR,
       FE_EXT4_ERR,
       FE_MCE,
       FE_SIGNAL,
       FE_WARN,
       FE_PANIC,
       FE_MAX
};

struct ksym {
	long addr;
	char *name;
};

struct arg_info
{

};

typedef struct {
	enum FAULT_EVENT type;
	//rwsem start to get lock
} report;
#endif /* ifndef __TEMPLATE_H */

