#ifndef __MISC_IOCTL_H__
#define __MISC_IOCTL_H__
#include "template_iocmd.h"
#include <linux/kallsyms.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

#define FUNC_UNIT(name) \
	int name##_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data); \
	int name##_unit_init(void); \
	int name##_unit_exit(void); 

typedef int (*misc_unit_fn)(void);
typedef int (*misc_unit_ioctl_fn)(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
struct misc_uint_item {
	const char *u_name;
	enum ioctl_cmdtype type;
	misc_unit_ioctl_fn ioctl;
	misc_unit_fn init;
	misc_unit_fn exit;
};


int page_unit_ioctl_func(unsigned int  cmd, unsigned long arg);


int rcutest_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int rcutest_unit_init(void);
int rcutest_unit_exit(void);

int kprobe_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int kprobe_unit_init(void);
int kprobe_unit_exit(void);

void misc_show_stack(const char *log_buf, struct task_struct *task, unsigned long *sp);
int showstack_unit_init(void);

int workqueue_unit_init(void);
int workqueue_unit_exit(void);
int workqueue_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);

int locktest_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data);
int locktest_unit_init(void);

int msr_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int msr_unit_init(void);
int msr_unit_exit(void);

int raidtree_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);
int raidtree_unit_init(void);
int raidtree_unit_exit(void);

int ext2test_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data);
int ext2test_unit_init(void);
int ext2test_unit_exit(void);

int atomic_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data);
int atomic_unit_init(void);
int atomic_unit_exit(void);

int kmem_unit_init(void);
int kmem_unit_exit(void);
int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);


int devbusdrvtest_unit_exit(void);
int devbusdrvtest_unit_init(void);
int devbusdrvtest_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data);

FUNC_UNIT(hwpci);
FUNC_UNIT(statickey);
#endif
