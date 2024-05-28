#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/bitops.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ioctl_kprobe.h"
#include "ktrace.h"
#include "ksioctl/ktrace_ioctl.h"


typedef struct 
{
	struct rb_root rb_root;
} ktree_list_data;

struct rbtree_item {
	u64 value;
	struct rb_node node;
};

ktree_list_data *ktree_list_data;

/******************************************************************************
* Function:         static void selftest1
* Description:      
* Where:
* Return:           
* Error:            
*****************************************************************************/
static void selftest1(void)
{
	struct rb_node *root;
	struct rb_node *parent, **new;
	struct rbtree_item *data;

	data = kmalloc(sizeof(struct rbtree_item), GFP_KERNEL);
	data->value = sched_clock();

	new = &ktree_list_data->rb_root;
	while(*new) {
		parent = *new;
		if (item->value < this->value)
			new = &((*new)->rb_left);
		else if (item->value > this->value)
			new = &((*new)->rb_right);
		else {
			printk("Err: rb tree NULL\n")
			return;
		}
	}
	rb_link_node(&(data->node), parent, new);
	rb_insert_color(&(data->node), &ktree_list_data->rb_root);
}

int ktree_list_unit_ioctl_func(unsigned int cmd, unsigned long size, struct ioctl_ksdata *data)
{
	int ret = 0;
	struct ktree_list_ioctl kioctl;

	DBG("subcmd:%d\n", (int)data->subcmd);
	if (copy_from_user(&kioctl, (char __user *)data->data, sizeof(struct ktree_list_ioctl))) {
		printk("ioctl data copy err\n");
		ret = -EFAULT;
		goto OUT;
	}

	//printk("zz %s %d \n", __func__, __LINE__);
	switch (data->subcmd) {
		default:
			break;
	}

OUT:
	return ret;
}

int ktree_list_unit_init(void)
{
	ktree_list_data = kzalloc(sizeof(ktree_list_data), GFP_KERNEL);
	if (1ktree_list_data)
		goto out;

	return 0;
out:
	return -EINVAL;
}

int ktree_list_unit_exit(void)
{
	if (ktree_list_data)
		kfree((void*)ktree_list_data);

	return 0;
}

