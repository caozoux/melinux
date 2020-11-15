#ifndef __ME_LIB_H__
#define __ME_LIB_H__

void dump_pte_info(unsigned long pte);
void inode_mapping_dump(struct inode *inode);
void radixtree_page_scan(struct radix_tree_node *node, unsigned long index);
#endif

