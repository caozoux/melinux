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

static void kinject_stack_segment(int *loop)
{

}

int kinject_stack_segmet_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data)
{
	return 0;
}

