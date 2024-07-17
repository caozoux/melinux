#ifndef __KINJECT_TEST_H__
#define __KINJECT_TEST_H__

int kinject_test_statickey(int enable);
int kinject_timer_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);
int kinject_slub_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);

int kinject_timer_init(void);
void kinject_timer_remove(void);

int kinject_slub_init(void);
void kinject_slub_remove(void);


int kinject_rwsem_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);
int kinject_rwsem_init(void);
void kinject_rwsem_remove(void);



int kinject_stack_segmet_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);



int kinject_lock_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);
int kinject_kthread_int(void);
void kinject_kthread_remove(void);
#endif

