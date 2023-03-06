#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/memcontrol.h>
#include <linux/kernfs.h>
#include <ksioctl/kmem_ioctl.h>
#include <linux/vmalloc.h>

#include "internal.h"
//#include "pub/trace_buffer.h"

#define RUNLOG_BUF_SIZE (10 * 1024 * 1024)

struct rinlog_buffer {
	struct ksys_trace_buffer buffer;
} runlog_buf;


void rlog_printf(const char *f, ...)
{
	 va_list args;
	 va_start(args, f);
	 ksys_trace_buffer_printk(&runlog_buf.buffer, f, args);
	 va_end(args);
}

int runlog_init(void)
{
	int ret;

	ret = init_ksys_trace_buffer(&runlog_buf.buffer, RUNLOG_BUF_SIZE);
	if (ret)  {
		memset(&runlog_buf.buffer, 0 ,sizeof(struct ksys_trace_buffer));
		return -EINVAL;
	}
	printk("zz %s buffer->product.data:%lx \n",__func__, (unsigned long)runlog_buf.buffer.product.data);
	rlog_printf("zz %s \n", __func__);
	return 0;
}

void runlog_exit(void)
{
	destroy_ksys_trace_buffer(&runlog_buf.buffer);
}

