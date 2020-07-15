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

#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

struct rxtree_misc_data {
	struct radix_tree_root root;
} *rxtree_misc_data;

RADIX_TREE(roottest, GFP_ATOMIC);
static int rxtree_misc_dump(struct radix_tree_root *root)
{
	struct radix_tree_node *node = root->rnode, *node_iter, **slot;
	struct radix_tree_node *child;
	unsigned long maxindex;
	void *p, *n;
	int i;

#if 1
	printk("zz %s shift:%lx offset:%lx count:%lx exceptional:%lx parent:%lx root:%lx slots:%lx tags:%lx rcu:%lx\n",
			__func__, (unsigned long)node->shift, (unsigned long)node->offset, (unsigned long)node->count,
			(unsigned long)node->exceptional, (unsigned long)node->parent, (unsigned long)node->root,
			(unsigned long)node->slots, (unsigned long)node->tags, (unsigned long)&node->rcu_head);
#endif
			
#if 0
	struct radix_tree_node *node, *parent;
	void __rcu **slot;
	parent = NULL;
	slot = (void __rcu **)&root->rnode;
	adix_tree_load_root(root, &node, &maxindex);
	list_for_each_entry_safe(p, n, root, list) {
	}
#else
	for (i = 0; i < RADIX_TREE_MAP_SIZE; i++) {
			if (node->slots[i]) {
			slot = node->slots[i];
			printk("zz %s slot:%lx lx\n",__func__, (unsigned long)slot, (unsigned long)*slot);
			//node_iter = rcu_dereference_raw(node->slots[i]);
#if 0
			printk("zz %s shift:%lx offset:%lx count:%lx exceptional:%lx"
					" parent:%lx root:%lx slots:%lx tags:%lx rcu:%lx\n",
					__func__, (unsigned long)node_iter->shift, (unsigned long)node_iter->offset, (unsigned long)node_iter->count,
					(unsigned long)node_iter->exceptional, (unsigned long)node_iter->parent, (unsigned long)node_iter->root,
					(unsigned long)node_iter->slots, (unsigned long)node_iter->tags, (unsigned long)&node_iter->rcu_head);
#endif
			}
				//printk("zz %s node->slots:%lx \n",__func__, (unsigned long)node->slots[i]);
	}
#endif
	return 0;
}


int raidtree_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = 0;
	void *p;
	struct radix_tree_node *rx_node;

	rx_node = rxtree_misc_data->root.rnode;

	printk("zz %s %d \n", __func__, __LINE__);
	switch (data->cmdcode) {
		case  IOCTL_USERAIDIXTREE_ADD:
			p = kzalloc(PAGE_SIZE, GFP_KERNEL);
			//printk("zz %s p:%lx \n",__func__, (unsigned long)p);
			//DEBUG("rx inert page:%d \n", data->rx_data.index);
			if (copy_from_user(p, (char __user *) data->rx_data.buf, data->rx_data.buf_len))
				return -EINVAL;

			//ret = radix_tree_insert(&roottest, data->rx_data.index, p);
			ret = radix_tree_insert(&rxtree_misc_data->root, data->rx_data.index, p);
			if (ret)
					printk("rxtree insert failed \n");

			break;

		case  IOCTL_USERAIDIXTREE_DEL:
			p = radix_tree_delete(&rxtree_misc_data->root, data->rx_data.index);
			if (!p) {
				ERR("rx tree delete %d failed\n", data->rx_data.index);
				ret = -EINVAL;
				break;
			}
			kfree(p);
			break;

		case  IOCTL_USERAIDIXTREE_GET:
			p = radix_tree_lookup(&rxtree_misc_data->root, data->rx_data.index);
			if (!p) {
				ERR("rx tree get %d failed\n", data->rx_data.index);
				ret = -EINVAL;
				break;
			}

			copy_to_user((char __user *) data->rx_data.buf, p, data->rx_data.buf_len);
			break;

		case  IOCTL_USERAIDIXTREE_DUMP:
			rxtree_misc_dump(&rxtree_misc_data->root);
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}


int raidtree_init(void)
{
	rxtree_misc_data = kzalloc(sizeof(struct rxtree_misc_data), GFP_KERNEL);
	INIT_RADIX_TREE(&rxtree_misc_data->root, GFP_KERNEL);
#if 0
	    for(i = 0; i < num; i++) {
        radix_tree_insert(&root, i, test[i]);
    }

		radix_tree_delete(&root, 13);
		radix_tree_lookup
#endif
	return 0;

}

int raidtree_exit(void)
{
	kfree(rxtree_misc_data);
	return 0;
}
