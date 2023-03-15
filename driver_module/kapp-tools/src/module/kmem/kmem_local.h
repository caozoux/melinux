#ifndef __KMEM_LOCAL_H__
#define __KMEM_LOCAL_H__

#include "internal.h"
int kmem_cgroup_init(void);
void kmem_cgroup_exit(void);

int kmem_dump_func(struct kmem_ioctl *data);
void dump_cgroup_kmem_info(pid_t pid);
#endif

