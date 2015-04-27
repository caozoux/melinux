#include <linux/kernel.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/printk.h>

struct mwlcore_data {
	int flags;
};

struct mwlcore_data *mwclore_prv;
static irqreturn_t wlcore_irq(int irq, void *cookie)
{
	return IRQ_HANDLED;
}
static irqreturn_t wlcore_hardirq(int irq, void *cookie)
{
    return IRQ_WAKE_THREAD;
}

static int mwlcore_irq = 257;
int test_wlcore_thread_irq(void)
{
	int ret;
	unsigned long irqflags;

	mwclore_prv = kmalloc(sizeof(mwclore_prv), GFP_KERNEL);	
	if (!mwclore_prv) {
		ret= -ENOMEM;
		goto out_ret_1;
	}
	irqflags = IRQF_TRIGGER_HIGH | IRQF_SHARED;
    ret = request_threaded_irq(mwlcore_irq, wlcore_hardirq, wlcore_irq,
                   irqflags, "test_wlcore", mwclore_prv);
	if (ret < 0) {
		printk("zz %s register thread irq failed ret:%d\n",__func__, ret);
		goto out_ret;
	} else {
		printk(" thread irq %d\n",mwlcore_irq);
	}
	return 0;

out_ret:
	kfree(mwclore_prv);
out_ret_1:
	return ret;
}

void test_wlcore_exit(void)
{
	if(mwclore_prv)
		free_irq(mwlcore_irq, mwclore_prv);
}

static int __init testdriver_init(void)
{	
	test_wlcore_thread_irq();
	if (!mwclore_prv)
		kfree(mwclore_prv);
	return 0;
}

static void __exit testdriver_exit(void)
{
	test_wlcore_exit();
}

module_init(testdriver_init);
module_exit(testdriver_exit);

//module_param(dump, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(dump, "Enable sdio read/write dumps.");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
