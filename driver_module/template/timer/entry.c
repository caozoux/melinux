#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

struct timer_list timer;
struct delayed_work patch_work;
struct workqueue_struct *wq;

static void poll_sync_reset(struct timer_list *t)
{
	trace_printk("zz %s %d \n", __func__, __LINE__);
	mod_timer(t, round_jiffies(jiffies + 2));
}

static void patch_event(struct work_struct *work)
{
	trace_printk("zz %s %d \n", __func__, __LINE__);
	del_timer(&timer);
	queue_delayed_work(wq, &patch_work, HZ);
	timer_setup(&timer, poll_sync_reset, 0);
	timer.expires = round_jiffies(jiffies + 5);
	add_timer(&timer);
}

static int hrtimer_pr_init(void)
{
	timer_setup(&timer, poll_sync_reset, 0);
	timer.expires = round_jiffies(jiffies + 5);
	add_timer(&timer);
	wq = create_singlethread_workqueue("timer_test");
	INIT_DELAYED_WORK(&patch_work, patch_event);
    queue_delayed_work(wq, &patch_work, 0);

	return 0;
}

static void hrtimer_pr_exit(void)
{
	del_timer_sync(&timer);
	cancel_delayed_work_sync(&patch_work);
	destroy_workqueue(wq);
}

static int __init percpu_hrtimer_init(void)
{
  hrtimer_pr_init();	
  return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
  hrtimer_pr_exit();	
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
