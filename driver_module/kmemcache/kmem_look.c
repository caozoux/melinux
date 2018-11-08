#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include  "kmemcache.h"

struct pglist_data *(*orig_first_online_pgdat)(void);
void kmem_dump_zone(void)
{
	orig_first_online_pgdat=kallsyms_lookup_name("first_online_pgdat");
	if(!orig_first_online_pgdat)
		printk("orig_first_online_pgdat:%ld\n", (unsigned long)orig_first_online_pgdat);
 	//for (zone = (orig_first_online_pgdat())->node_zones; \
 	//	zone; zone = orig_next_zone(zone))
}
