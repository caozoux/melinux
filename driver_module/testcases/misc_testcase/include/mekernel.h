#ifndef __MEKERNEL_H__
#define __MEKERNEL_H__

void task_file_enum(struct task_struct *task);
void medentry_dump_full_patch(struct dentry *dentry);
void merequest_dump_full_patch(struct request *req);
void pci_bus_scan(void);


int virtual_test_bus_init(void);
int virtual_test_bus_exit(void);
struct device * virtual_test_get_device(void);
#endif
