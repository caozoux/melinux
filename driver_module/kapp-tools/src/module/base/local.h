#ifndef __H_BASE_LOCAL__
#define __H_BASE_LOCAL__

#include <linux/slab.h>

int base_trace_init(void);
int base_slab_init(void);
void base_slab_exit(void);
int ksys_stack_dump(struct pt_regs *regs);

#endif

