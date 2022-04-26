#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"


#define LOOP_CNT 		(10000000)
#define THREAD_CNT  	(12)

static atomic_t atomic_test_val1 __attribute__((aligned (128)));
static __maybe_unused struct task_struct *thread_task[THREAD_CNT];

struct atomic_test_data {
	struct workqueue_struct *sigtestworkqueue;
	struct workqueue_struct *workqueue;
	struct work_struct wq_sigtestwq;
	struct work_struct per_cpu_atomic_wq;
	struct delayed_work wkq_delay_test;;
	unsigned long time[NR_CPUS];
	int mode;
} *atm_dt;

static DEFINE_SPINLOCK(locktest_lock);
static void atm_sig_test(struct work_struct *work)
{
	unsigned long k;
	unsigned long count, flags;
	unsigned long long time_cnt_old = 0, time_cnt_new = 0;

	time_cnt_old = get_time_tick();

	printk("singal thread atomic set:");
	spin_lock_irqsave(&locktest_lock, flags);

	for(k = 0; k < LOOP_CNT; k++)
	{
		count= (unsigned long)atomic_test_val1.counter;
		count++;
		if (atm_dt->mode == 1) {
			atomic_set(&atomic_test_val1, count);
		}
  	}
	spin_unlock_irqrestore(&locktest_lock, flags);

  	time_cnt_new = get_time_tick() - time_cnt_old;
	printk("time_cnt_new:%lx \n",(unsigned long)time_cnt_new);
}

//mutilpe thread test
static __maybe_unused unsigned long long atm_per_cpu_test(struct work_struct *work)
{
	unsigned long k;
	unsigned long count, flags;
	unsigned long long time_cnt_old = 0, time_cnt_new = 0;
	int cpu = smp_processor_id();
	spinlock_t locktest_lock;

	time_cnt_old = get_time_tick();

	printk("singal thread atomic set:");

	spin_lock_irqsave(&locktest_lock, flags);
	for(k = 0; k < LOOP_CNT; k++)
	{
		count= (unsigned long)atomic_test_val1.counter;
		count++;
		if (atm_dt->mode == 1) {
			atomic_set(&atomic_test_val1, count);
		}
  	}
	spin_unlock_irqrestore(&locktest_lock, flags);

	time_cnt_new = get_time_tick() - time_cnt_old;
	atm_dt->time[cpu] = time_cnt_new;
	printk("time_cnt_new:%lx \n", (unsigned long)time_cnt_new);
	return time_cnt_new;
}

int atomic_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{

	switch (data->cmdcode) {
		case  IOCTL_USEATOMIC_PERFORMANCE:
			atm_dt->mode = 1;
			queue_work(atm_dt->sigtestworkqueue, &atm_dt->wq_sigtestwq);
			flush_work(&atm_dt->wq_sigtestwq);
#if 0
			for (i=0; i<THREAD_CNT;i++) {
				if (i == cpu)
					queue_work_on(THREAD_CNT+1, atm_dt->sigtestworkqueue, &atm_dt->wq_sigtestwq);
				else
					queue_work_on(i, atm_dt->sigtestworkqueue, &atm_dt->wq_sigtestwq);
			}
			queue_work_on(i,atm_dt->sigtestworkqueue, &atm_dt->wq_sigtestwq);
#endif
			MEDEBUG("atomic test\n");
			break;
		default:
			goto OUT;
	}

OUT:
	return 0;
}

int atomic_unit_init(void)
{
	atm_dt = kzalloc(sizeof(struct atomic_test_data), GFP_KERNEL);

	if (!atm_dt)
		return -EINVAL;

	atm_dt->sigtestworkqueue = create_singlethread_workqueue("s_atomic_work");
	if (!atm_dt->sigtestworkqueue) {
		kfree(atm_dt);
		return -EINVAL;
	}

	atm_dt->workqueue = create_workqueue("atomic_work");
	if (!atm_dt->workqueue) {
		goto OUT;
	}

	INIT_WORK(&atm_dt->wq_sigtestwq, atm_sig_test);
	INIT_WORK(&atm_dt->per_cpu_atomic_wq, atm_sig_test);
#if 0
	for(i=0; i < THREAD_CNT, i++) {
		kthread_create(speakup_thread, NULL, "atomic_test");
	}
#endif

	return 0;

OUT:
	destroy_workqueue(atm_dt->sigtestworkqueue);
	kfree(atm_dt);
	return -EINVAL;

}

int atomic_unit_exit(void)
{
	destroy_workqueue(atm_dt->sigtestworkqueue);
	destroy_workqueue(atm_dt->workqueue);
	kfree(atm_dt);
	return 0;
}

