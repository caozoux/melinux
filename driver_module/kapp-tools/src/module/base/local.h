#ifndef __H_BASE_LOCAL__
#define __H_BASE_LOCAL__

#include <linux/slab.h>
int base_trace_init(void);
int base_slab_init(void);
void base_slab_exit(void);

extern void (*orig_get_slabinfo)(struct kmem_cache *s, struct slabinfo *sinfo);
#endif

