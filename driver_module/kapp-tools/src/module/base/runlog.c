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

#include "ksioctl/krunlog_ioctl.h"

#include "internal.h"
//#include "pub/trace_buffer.h"

#define RUNLOG_BUF_SIZE (10 * 1024 * 1024)


struct rinlog_buffer {
	struct ksys_trace_buffer buffer;
} runlog_buf;


struct ksys_trace_buffer *getlog_buffer(void)
{
	return &runlog_buf.buffer;
}

int krunlog_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_ksdata *ksdata)
{
	int ret;
	int buf_size = 0;
	struct krunlog_ioctl kioctl;
	if (copy_from_user(&kioctl, (char __user *)ksdata->data, sizeof(struct krunlog_ioctl))) {
		pr_err("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	switch (ksdata->subcmd) {
		case IOCTL_RUNLOG_DUMP:
			backup_ksys_trace_buffer(&runlog_buf.buffer);
			buf_size = (kioctl.len  < RUNLOG_BUF_SIZE) ? runlog_buf.buffer.product.len : RUNLOG_BUF_SIZE;
			if (ret = copy_to_user((char __user *)kioctl.buf, runlog_buf.buffer.product.data, buf_size))
				goto OUT;
			break;

		case IOCTL_RUNLOG_CLEAN:
			discard_ksys_trace_buffer(&runlog_buf.buffer);
			break;

		default:
			break;
	}

	return 0;
OUT:
	return ret;
}

int runlog_init(void)
{
	int ret;

	ret = init_ksys_trace_buffer(&runlog_buf.buffer, RUNLOG_BUF_SIZE);
	if (ret)  {
		memset(&runlog_buf.buffer, 0 ,sizeof(struct ksys_trace_buffer));
		return -EINVAL;
	}

	return 0;
}

void runlog_exit(void)
{
	destroy_ksys_trace_buffer(&runlog_buf.buffer);
}

int krunlog_unit_init(void)
{
	return 0;
}

int krunlog_unit_exit(void)
{
	return 0;
}

