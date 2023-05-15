#ifndef __KSYS_INTERNAL_H__
#define __KSYS_INTERNAL_H__

#include "hotfix_util.h"
#include "ksysdata.h"
#include "kbase.h"
#include "krunlog.h"
#include "trace_buffer.h"


struct task_struct *find_process_by_pid(pid_t vnr);

struct ksys_trace_buffer *getlog_buffer(void);

int user_get_ksys_callchain_buffers(void __user *buf, size_t size);
int clear_ksys_callchain_buffers(void);
int ksys_stack_dump(struct pt_regs *regs);

#define rlog_printk(fmt,args...) ksys_trace_buffer_printk(getlog_buffer(), fmt, ## args)

#endif

