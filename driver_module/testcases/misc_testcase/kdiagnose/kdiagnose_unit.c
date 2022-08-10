#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"
#include "kdiagnose.h"
#include "ktrace.h"

struct kd_percpu_data *kd_percpu_data[32];

extern struct softirq_action *orig_softirq_vec;
extern struct mutex *orig_tracepoint_module_list_mutex;
extern struct list_head *orig_tracepoint_module_list;

int kdiagnose_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = 0;

	MEDEBUG("cmdcode:%d\n", data->cmdcode);

	switch (data->cmdcode) {
		case  IOCTL_TRACE_IRQ_ALL:
			MEDEBUG("IRQALL\n");
			if (data->trace_data.enable) {
				if (!irq_trace_install()) {
					ret = -EINVAL;	
					goto out;
				}
			} else {
				if (!irq_trace_remove()) {
					ret = -EINVAL;	
					goto out;
				}
			}
			break;
		default:
			break;
	}

out:
	return ret;
}

int kdiagnose_unit_init(void)
{
	int i;
	struct kd_percpu_data *kd_list;

	LOOKUP_SYMS(tracepoint_module_list_mutex);
	LOOKUP_SYMS(tracepoint_module_list);
	LOOKUP_SYMS(softirq_vec);

	memset(kd_percpu_data, 0, sizeof(struct kd_percpu_data*) * 32);

	kd_list = kzalloc((sizeof(struct kd_percpu_data)) * 32, GFP_KERNEL);
	if (!kd_list)
		return -ENOMEM;

	for (i = 0; i < 32; ++i) {
		kd_percpu_data[i] = &kd_list[i];
	}

	return 0;
}

int kdiagnose_unit_exit(void)
{
	if (kd_percpu_data[0])
		kfree(kd_percpu_data[0]);

	return 0;
}

