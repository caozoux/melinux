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

void kinject_stack_segment(char *overwrite_buf)
{
	memset(overwrite_buf, 1, 512);
	//memset(overwrite_buf, 1, 528);
	printk("zz %s overwrite_buf:%lx \n",__func__, (unsigned long)overwrite_buf[32]);
	printk("zz %s overwrite_buf:%lx \n",__func__, (unsigned long)overwrite_buf[64]);
}

int kinject_stack_segmet_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	char overwrite_buf[512]; 
	kinject_stack_segment(overwrite_buf);
	return 0;
}

