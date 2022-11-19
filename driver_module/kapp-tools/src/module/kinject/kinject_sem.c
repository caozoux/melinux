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

DECLARE_RWSEM(kinject_sem);
int kinject_rwsem_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	switch (cmd) {
		case IOCTL_INJECT_RWSEM_WRITEDOWN:
			printk("zz %s %d \n", __func__, __LINE__);
			down_write(&kinject_sem);
			break;
		case IOCTL_INJECT_RWSEM_WRITEUP:
			printk("zz %s %d \n", __func__, __LINE__);
			up_write(&kinject_sem);
			break;
		case IOCTL_INJECT_RWSEM_READDOWN:
			printk("zz %s %d \n", __func__, __LINE__);
			down_read(&kinject_sem);
			break;
		case IOCTL_INJECT_RWSEM_READUP:
			printk("zz %s %d \n", __func__, __LINE__);
			up_read(&kinject_sem);
			break;
		case IOCTL_INJECT_MMAP_SEM_WRITEDWON:
			down_write(&current->mm->mmap_sem);
			break;
		case IOCTL_INJECT_MMAP_SEM_WRITEUP:
			up_write(&current->mm->mmap_sem);
			break;
		default:
			break;
	}
	return 0;
}

int kinject_rwsem_init(void)
{
	init_rwsem(&kinject_sem);
	return 0;
}

void kinject_rwsem_remove(void)
{

}
