#include <linux/init.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include <linux/ftrace.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/rbtree.h>
#include <melib.h>

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

struct krbtree_data {
	struct rb_root	root_table;
};

struct mytype {
	struct rb_node node;
	int val;
};

int krbtree_insert(struct rb_root *root, struct mytype *data)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct mytype *this = container_of(*new, struct mytype, node);
		int result; 

		if (data->val < this->val)
			result = -1;
		else if (data->val > this->val)
			result = 1;
		else
			result = 0;

		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);

	printk("zz %s %d \n", __func__, __LINE__);
	return true;
}

static void krbtree_scan(struct rb_root *root)
{
	struct rb_node *node;
	for (node = rb_first(root); node; node = rb_next(node))  {
		printk("key=%d\n", rb_entry(node, struct mytype, node)->val);
	}
}

#if 0
rb_insert_color(&group->node, &ctx->mcg_table);
rb_erase(&group->node, &ctx->mcg_table);
ctx->mcg_table = RB_ROOT;
for (p = rb_first(&ctx->mcg_table); p; p = rb_next(p))
#endif

static void selftest(void)
{
	struct krbtree_data *data;
	struct mytype *node;
	data = kmalloc(sizeof(struct krbtree_data), GFP_KERNEL);

	data->root_table = RB_ROOT;
	node = kmalloc(sizeof(struct mytype), GFP_KERNEL);
	node->val = 1;

	krbtree_insert(&data->root_table, node);

	node = kmalloc(sizeof(struct mytype), GFP_KERNEL);
	node->val = 2;
	krbtree_insert(&data->root_table, node);

	node = kmalloc(sizeof(struct mytype), GFP_KERNEL);
	node->val = 3;
	krbtree_insert(&data->root_table, node);

	node = kmalloc(sizeof(struct mytype), GFP_KERNEL);
	node->val = 4;
	krbtree_insert(&data->root_table, node);

	krbtree_scan(&data->root_table);
}

int krbtree_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	return 0;
}

int krbtree_unit_init(void)
{

	//selftest();
	return 0;
}

int krbtree_unit_exit(void)
{
	return 0;
}

