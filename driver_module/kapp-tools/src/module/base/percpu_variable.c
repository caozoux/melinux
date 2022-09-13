#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>
#include <hotfix_util.h>
#include <kpercpu.h>

#define KSYSD_MAX_CPU (64)
static struct percpu_data *pcpu_data;
struct percpu_data *get_ksys_percpu(void)
{
	int cpu = smp_processor_id();
	return &pcpu_data[cpu];
}

int percpu_variable_init(void)
{
	pcpu_data = kzalloc(sizeof(struct percpu_data) * KSYSD_MAX_CPU, GFP_KERNEL);
	if (!pcpu_data)
		return -ENOMEM;
	return 0;
}

int percpu_variable_exit(void)
{
	if (pcpu_data)
		kfree(pcpu_data);

	return 0;
}

