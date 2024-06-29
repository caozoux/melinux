#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/rbtree.h>

#include <ksysd.h>
#include <kpercpu.h>
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "ioctl_kprobe.h"
#include "ktrace.h"
#include "ksioctl/ktrace_ioctl.h"


typedef struct 
{
	struct rb_root_cached rb_root;
} ktree_list_struct;

struct rbtree_item {
	u64 value;
	struct rb_node node;
};

ktree_list_struct *ktree_list_data;

/******************************************************************************
* Function:         static void selftest1
* Description:      
* Where:
* Return:           
* Error:            
*****************************************************************************/
static void insert_rb_node(struct rbtree_item *data)
{
	struct rb_node *parent, **new;
	struct rbtree_item  *entry;
	bool leftmost = true;

	new = &ktree_list_data->rb_root.rb_root.rb_node;
	while(*new) {
		parent = *new;
		entry = rb_entry(parent, struct rbtree_item, node);
		if (data->value < entry ->value) {
			new = &parent->rb_left;
		} else {
			new = &parent->rb_right;
			leftmost = false;
		}
	}
	rb_link_node(&(data->node), parent, new);
	rb_insert_color_cached(&data->node, &ktree_list_data->rb_root, leftmost);
}

static void rb_tree_dump(void)
{
	struct rbtree_item *node, *next_node;
	for (node = rb_entry_safe(rb_first_cached(&ktree_list_data->rb_root), typeof(*node), node);
		node && ({ next_node = rb_entry_safe(rb_next_postorder(&node->node),
		typeof(*node), node);});
		node = next_node) {
		printk("zz %s value:%lx \n",__func__, (unsigned long)node->value);
	}
}

static void selftest1(u64 val)
{
	struct rbtree_item  *data;

	data = kzalloc(sizeof(struct rbtree_item), GFP_KERNEL);
	//RB_CLEAR_NODE(&data->node);
	data->value = val;
	insert_rb_node(data);
}

int ktree_list_unit_ioctl_func(unsigned int cmd, unsigned long size, struct ioctl_ksdata *data)
{
	int ret = 0;
#if 0
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
#endif
	return ret;
}

int ktree_list_unit_init(void)
{
	ktree_list_data = kzalloc(sizeof(ktree_list_data), GFP_KERNEL);
	if (!ktree_list_data)
		goto out;
	printk("zz %s v1:%lx v2:%lx v3:%lx \n",__func__, (unsigned long)ktree_list_data->rb_root.rb_leftmost, (unsigned long)ktree_list_data->rb_root.rb_root.rb_node, (unsigned long)3);
	//printk("zz v1:%lx \n",(unsigned long)&(ktree_list_data->rb_root));
	
	ktree_list_data->rb_root = RB_ROOT_CACHED;
	printk("zz %s v1:%lx v2:%lx v3:%lx \n",__func__, (unsigned long)ktree_list_data->rb_root.rb_leftmost, (unsigned long)ktree_list_data->rb_root.rb_root.rb_node, (unsigned long)3);
#if 0
	selftest1(1);
	selftest1(2);
	selftest1(3);
	selftest1(4);
	selftest1(5);
	selftest1(6);
	selftest1(7);
	selftest1(7);
	rb_tree_dump();
#endif
	return 0;
out:
	return -EINVAL;
}

int ktree_list_unit_exit(void)
{
#if 0
	if (ktree_list_data)
		kfree((void*)ktree_list_data);
#endif

	return 0;
}

