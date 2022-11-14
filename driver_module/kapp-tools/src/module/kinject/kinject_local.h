#ifndef __KINJECT_TEST_H__
#define __KINJECT_TEST_H__

int kinject_test_statickey(int enable);
int kinject_timer_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);
int kinject_slub_func(enum IOCTL_INJECT_SUB cmd, struct kinject_ioctl *data);

int kinject_timer_init(void);
void kinject_timer_remove(void);

int kinject_slub_init(void);
void kinject_slub_remove(void);
#endif

