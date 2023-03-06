#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/uaccess.h>
#include <ksioctl/kmem_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"

#if 0
void (*orig_get_slabinfo)(struct kmem_cache *s, struct slabinfo *sinfo);

void (*orig_memcg_accumulate_slabinfo)(struct kmem_cache *s, struct slabinfo *info);
void slab_dump(void)
{
	struct slabinfo sinfo;
	memset(&sinfo, 0, sizeof(sinfo));

	orig_get_slabinfo(s, &sinfo);
	memcg_accumulate_slabinfo(s, &sinfo);
	seq_printf(m, "%-17s %6lu %6lu %6u %4u %4d",
			cache_name(s), sinfo.active_objs, sinfo.num_objs, s->size,
			sinfo.objects_per_slab, (1 << sinfo.cache_order));

	seq_printf(m, " : tunables %4u %4u %4u",
			sinfo.limit, sinfo.batchcount, sinfo.shared);
	seq_printf(m, " : slabdata %6lu %6lu %6lu",
			sinfo.active_slabs, sinfo.num_slabs, sinfo.shared_avail);
	//slabinfo_show_stats(m, s);
	//seq_putc(m, '\n');
}

int kmem_slab_syms_init(void)
{
	LOOKUP_SYMS(cgroup_roots);
	LOOKUP_SYMS(mem_cgroup_iter);
	return 0;
}

int kmem_slab_init(void)
{
	int ret;

	if (kmem_slab_syms_init())
		return -EINVAL;
	//kmem_cgroup_scan_memcg(NULL);

	return 0;
}

void kmem_slab_exit(void)
{
	unregister_kprobes(kps_kmem,3);
}

#endif
