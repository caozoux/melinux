#ifndef __MISC_IOCTL_H__
#define __MISC_IOCTL_H__
#include "template_iocmd.h"

int page_ioctl_func(unsigned int  cmd, unsigned long arg);

int mem_ioctl_func(unsigned int  cmd, struct ioctl_data *data);
int rcu_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
void rcutest_init(void);
#endif
