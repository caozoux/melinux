#ifndef __MISC_IOCTL_H__
#define __MISC_IOCTL_H__
#include "template_iocmd.h"

int page_ioctl_func(unsigned int  cmd, unsigned long arg);

int mem_ioctl_func(unsigned int  cmd, struct ioctl_data *data);
int rcu_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int kprobe_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int rcutest_init(void);
int kprobe_init(void);
void misc_show_stack(const char *log_buf, struct task_struct *task, unsigned long *sp);
int showstack_init(void);

int workqueue_test_init(void);
void workqueue_test_exit(void);
int workqueue_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);

int locktest_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data);
int locktest_init(void);

int msr_init(void);
void msr_exit(void);

int raidtree_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int raidtree_init(void);
int raidtree_exit(void);

int ext2test_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data);
int ext2test_init(void);
void ext2test_exit(void);

int atomic_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data);
int atomic_init(void);
int atomic_exit(void);

int kmemt_unit_init(void);
int kmem_unit_exit(void);
int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);


int devbusdrvtest_exit(void);
int devbusdrvtest_init(void);
int devbusdrv_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
#endif
