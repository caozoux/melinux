#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

//queue_delayed_work(pollworkqueue, &vub300->pollwork, delay))

struct workqueue_base_data {
	struct workqueue_struct  *wq;
	struct work_struct work;
	wait_queue_head_t wr_wait; 
};

struct workqueue_base_data *wq_data;
static int cond = 0;

static void workquere_base_thread(struct work_struct *work)
{
#if 0
	ktime_t wait;
	wait = ms_to_ktime(10 * 1000);
	printk("zz %s %d \n", __func__, __LINE__);
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_hrtimeout(&wait, HRTIMER_MODE_REL);
#else
	//queue_work(wq_data->wq, &wq_data->work);
	//dump_stack();
	printk("zz %s current->pid:%08x \n",__func__, (int)current->pid);
	//wait_event(wq_data->wr_wait, );
	//set_current_state(TASK_UNINTERRUPTIBLE);
	set_current_state(TASK_INTERRUPTIBLE);
	schedule();
#endif
	printk("zz %s %d \n", __func__, __LINE__);
}

void workquere_base_init(void)
{
	wq_data = kmalloc(sizeof(struct workqueue_base_data), GFP_KERNEL);
	wq_data->wq = create_singlethread_workqueue("workquerebase");
	INIT_WORK(&wq_data->work, workquere_base_thread);
	init_waitqueue_head(&wq_data->wr_wait);
	queue_work(wq_data->wq, &wq_data->work);
}

void workquere_base_exit(void)
{
	flush_workqueue(wq_data->wq);
	destroy_workqueue(wq_data->wq);
	kfree(wq_data);
}
