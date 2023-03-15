#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>
#include <hotfix_util.h>

#include "internal.h"
#include "local.h"

void (*orig_get_slabinfo)(struct kmem_cache *s, struct slabinfo *sinfo);

void get_slabinfo(struct kmem_cache *s, struct slabinfo *sinfo)
{
	return orig_get_slabinfo(s, sinfo);
}

int base_slab_init(void)
{
	LOOKUP_SYMS(get_slabinfo);
	return 0;
}

void base_slab_exit(void)
{
	
}

