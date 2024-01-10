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

static DEFINE_SPINLOCK(kinject_lock);
static DEFINE_MUTEX(kinject_mutex);
int kinject_lock_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	int cnt = 0;
	int i;
	switch (cmd) {
		case IOCTL_INJECT_SPINLOCK_DEPLOCK:
			cnt = data->lock.lock_ms;
			printk("zz %s cnt:%lx \n",__func__, (unsigned long)cnt);
			spin_lock(&kinject_lock);

			while(cnt--)
				for (i = 0; i < 10; ++i)
					udelay(100);

			spin_unlock(&kinject_lock);
			break;
		case IOCTL_INJECT_IRQSPINLOCK_DEPLOCK:
			cnt = data->lock.lock_ms;
			printk("zz %s cnt:%lx \n",__func__, (unsigned long)cnt);
			spin_lock_irq(&kinject_lock);

			while(cnt--)
				for (i = 0; i < 10; ++i)
					udelay(100);

			spin_unlock_irq(&kinject_lock);
			printk("zz %s cnt:%lx \n",__func__, (unsigned long)cnt);
			break;

		case IOCTL_INJECT_MUTEXT_LOCK:
			mutex_lock(&kinject_mutex);
			break;

		case IOCTL_INJECT_MUTEXT_UNLOCK:
			mutex_unlock(&kinject_mutex);
			break;

		case IOCTL_INJECT_MUTEXT_DEALY:
			cnt = data->lock.lock_ms;
			printk("zz %s %d \n", __func__, __LINE__);
			mutex_lock(&kinject_mutex);
			while(cnt--)
				for (i = 0; i < 10; ++i)
					udelay(100);
			mutex_unlock(&kinject_mutex);
			break;
		default:
			break;
	}

	return 0;
}

