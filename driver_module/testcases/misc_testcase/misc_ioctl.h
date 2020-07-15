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

int raidtree_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int raidtree_init(void);
int raidtree_exit(void);
#endif
