#ifndef __MEKERNEL_H__
#define __MEKERNEL_H__

#include <linux/blkdev.h>
#include <melib.h>
#include "hotfix_util.h"

#define TRACK_ADDRS_COUNT 16
struct track {
	unsigned long addr;
	unsigned long addrs[TRACK_ADDRS_COUNT];
	int cpu;
	int pid;
	unsigned long when;
};

extern int init_dump_info;

extern struct mutex *orig_text_mutex;

extern void * (*orig_text_poke_bp)(void *addr, \
						const void *opcode, size_t len, void *handler);


void task_file_enum(struct task_struct *task);
void medentry_dump_full_patch(struct dentry *dentry);
void merequest_dump_full_patch(struct request *req);
void pci_bus_scan(void);

void merequest_list_dump(struct request *req);
void scan_block_dev_disk(void);

void dump_inode_data(struct inode *inode, int leve);

int virtual_test_bus_init(void);
int virtual_test_bus_exit(void);
struct device * virtual_test_get_device(void);
struct device *virtual_get_new_device(void);
void virtual_put_new_device(struct device *dev);

void bio_dump_data(struct bio *bio);
struct task_struct *get_taskstruct_by_pid(int pid);

//kmem
pte_t *get_pte(unsigned long addr, struct mm_struct *mm);
void vma_pte_dump(struct vm_area_struct *vma, u64 start_addr, u64 nr_page);
void dump_page_info(struct page *page);

//get the dump addr
int get_current_track(struct track *track, unsigned long addr);
void print_track(const char *s, struct track *t, unsigned long pr_time);
#endif

