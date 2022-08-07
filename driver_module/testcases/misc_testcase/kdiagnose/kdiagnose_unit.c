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
#include "kmemlocal.h"

struct kd_percpu_data *kd_percpu_data[NR_CPUS];
int kdiagnose_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{

	return 0;
}

int kdiagnose_unit_init(void)
{
	int i;

	memset(kd_percpu_data, 0, sizeof(struct kd_percpu_data*));

	for (i = 0; i < NR_CPUS; ++i) {
		kd_percpu_data[i] = kzalloc(sizeof(struct kd_percpu_data), GFP_KERNEL);
		if (!kd_percpu_data[i])
			goto data_fail;
	}		

	return 0;

data_fail:
	for (i = 0; i < NR_CPUS; ++i) {
		if (kd_percpu_data[i])
			kfree(kd_percpu_data[i]);
		else
			break;
	}
	return -ENOMEM;

}

int kdiagnose_unit_exit(void)
{
	for (i = 0; i < NR_CPUS; ++i) {
		if (kd_percpu_data[i])
			kfree(kd_percpu_data[i]);
	}
	return 0;
}

