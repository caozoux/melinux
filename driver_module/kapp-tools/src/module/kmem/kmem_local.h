#ifndef __KMEM_LOCAL_H__
#define __KMEM_LOCAL_H__

int kmem_cgroup_init(void);
void kmem_cgroup_exit(void);
int kmem_dump_func(struct kmem_ioctl *data);
#endif

