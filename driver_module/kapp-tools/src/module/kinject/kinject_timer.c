#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

struct list_head inject_hrtimer;

struct inject_timer_data {
	struct hrtimer inject_timer;
	unsigned int delay_us;
	int timeout;
} inject_tm_data;

static enum hrtimer_restart inject_hrtime_func(struct hrtimer *timer)
{
	struct inject_timer_data *data = container_of(timer, struct inject_timer_data, inject_timer);
	ktime_t now;

	if (data->delay_us)
		udelay(data->delay_us);

 	now = ktime_get();

	hrtimer_forward(timer, now, ns_to_ktime(data->timeout*1000));
	return HRTIMER_RESTART;
	//return HRTIMER_NORESTART;
}

static void inject_hrtime_start(int timeout)
{
	ktime_t kt;
	inject_tm_data.delay_us = 0;
	inject_tm_data.timeout = timeout;
	inject_tm_data.inject_timer.function = inject_hrtime_func;
	kt = ktime_add_us(ktime_get(), timeout);

	hrtimer_set_expires(&inject_tm_data.inject_timer, kt);
	hrtimer_start_expires(&inject_tm_data.inject_timer, HRTIMER_MODE_ABS_PINNED);
}

static void inject_hrtime_stop(void)
{
	hrtimer_cancel(&inject_tm_data.inject_timer);
}

int kinject_timer_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	//list_add_tail(&item->list, &kprobe_unit_data->head);
	if (data->enable) {
		if (!hrtimer_active(&inject_tm_data.inject_timer)) {
			inject_hrtime_start(1000);
			DBG("inject hrtimer start\n");
		}
	} else {
		if (hrtimer_active(&inject_tm_data.inject_timer)) {
			inject_hrtime_stop();
			DBG("inject hrtimer stop\n");
		}
	}
	return 0;
}

int kinject_timer_init(void)
{
	hrtimer_init(&inject_tm_data.inject_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	return 0;
}

void kinject_timer_remove(void)
{
	if (hrtimer_active(&inject_tm_data.inject_timer))
		inject_hrtime_stop();
}
