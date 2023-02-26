#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slub_def.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

#define MAX_WORKQUEUE (512)

struct kinject_wkq_data {
	struct workqueue_struct *kinject_workqueue_thread;
	struct workqueue_struct *workqueue[512];

	//struct work_struct wq_sigtestwq;

	//struct delayed_work wkq_delay_test;

	//wait_queue_head_t wait;
	//u8 wq_complete;
	//unsigned long performance_delay;
	//unsigned long performance_exp;

	//int runtime;
} *kj_wkq_dt;

wkq_dt->sigtestworkqueue = create_singlethread_workqueue("sigworkqt");
wkq_dt = kzalloc(sizeof(struct wkq_data), GFP_KERNEL);
INIT_WORK(&wkq_dt->wq_sigtestwq, wkq_sig_test);
INIT_DELAYED_WORK(&wkq_dt->wkq_delay_test, wkq_delay_test);

int kinject_workqueue_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	switch (cmd) {
		default:
			break;
	}

	return 0;
}


int kinject_workqueue_int(void)
{
	kj_wkq_dt = kzalloc(sizeof(struct kinject_wkq_data), GFP_KERNEL);
	if (!kj_wkq_dt)
		return -EINVAL;
}

void kinject_workqueue_int(void)
{
	if (kj_wkq_dt)
		kfree(kj_wkq_dt);

}
