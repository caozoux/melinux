#ifndef __KSYS_INTERNAL_H__
#define __KSYS_INTERNAL_H__

#include "hotfix_util.h"
#include "ksysdata.h"
#include "krunlog.h"
#include "trace_buffer.h"

struct ksys_trace_buffer *getlog_buffer(void);

#define rlog_printk(fmt,args...) ksys_trace_buffer_printk(getlog_buffer(), fmt, ## args)

#endif

