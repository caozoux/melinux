#ifndef __BLOCK_LOCAL_H__
#define __BLOCK_LOCAL_H__

void file_readwrite_test(void);
extern int (*orig_generic_fadvise)(struct file *file, loff_t offset, loff_t len, int advice);
void drop_inode_page_cache(char *filename);
struct inode *get_inode_with_filename(char *filename);
void inode_mapping_dump(struct inode *inode);
void scan_inode_radix_pagecache(struct inode *inode);

#endif /* ifndef __BLOCK_LOCAL_H__ */
