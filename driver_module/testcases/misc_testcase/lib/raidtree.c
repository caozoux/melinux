#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <asm/stacktrace.h>
#include <melib.h>


static inline struct radix_tree_node *entry_to_node(void *ptr)
{
	return (void *)((unsigned long)ptr & ~RADIX_TREE_INTERNAL_NODE);
}

static inline void *node_to_entry(void *ptr)
{
	return (void *)((unsigned long)ptr | RADIX_TREE_INTERNAL_NODE);
}

#define RADIX_TREE_RETRY    node_to_entry(NULL)

bool is_sibling_entry(const struct radix_tree_node *parent, void *node)
{
	void __rcu **ptr = node;
	return (parent->slots <= ptr) &&
		(ptr < parent->slots + RADIX_TREE_MAP_SIZE);
}

static inline unsigned long shift_maxindex(unsigned int shift)
{
	return (RADIX_TREE_MAP_SIZE << shift) - 1;
}

static inline unsigned long node_maxindex(const struct radix_tree_node *node)
{
	return shift_maxindex(node->shift);	
}

void radixtree_page_scan(struct radix_tree_node *node, unsigned long index)
{
	unsigned long i;
	node = entry_to_node(node);

	printk("radix node: %p offset %d indices %lu-%lu parent %p tags %lx %lx %lx shift %d count %d exceptional %d\n",
			node, node->offset, index, index | node_maxindex(node),
			node->parent,
			node->tags[0][0], node->tags[1][0], node->tags[2][0],
			node->shift, node->count, node->exceptional);

	for (i = 0; i < RADIX_TREE_MAP_SIZE; i++) {
		unsigned long first = index | (i << node->shift);
		unsigned long last = first | ((1UL << node->shift) - 1);
		void *entry = node->slots[i];
		 
		if (!entry)
			continue;

		if (entry == RADIX_TREE_RETRY) {
			printk("radix retry offset %ld indices %lu-%lu parent %p\n",i, first, last, node);
		} else if (!radix_tree_is_internal_node(entry)) {
			printk("radix entry offset %ld indices %lu-%lu parent %p %llx\n",i, first, last, node, entry);
		} else if (is_sibling_entry(node, entry)) {
			printk("radix sblng %p offset %ld indices %lu-%lu parent %p val %p\n", entry, i, first, last, node, *(void **)entry_to_node(entry));
		} else  {
			radixtree_page_scan(entry_to_node(entry), first);
		}
	}
}

