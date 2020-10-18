#ifndef __MEKERNEL_H__
#define __MEKERNEL_H__

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
#endif
