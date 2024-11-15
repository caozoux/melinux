#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/jump_label.h>
#include <ksioctl/kinject_ioctl.h>

#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "kinject_local.h"

static DEFINE_STATIC_KEY_FALSE(kinject_enable);

noinline static void jump_label_test(void)
{
	if (static_key_enabled(&kinject_enable))
		printk("enable\n");
	else
		printk("disable\n");
}

int kinject_jumptable_init(void)
{
	static_branch_enable(&kinject_enable);
	jump_label_test();
	return 0;
}

void kinject_jumptable_remove(void)
{

}

