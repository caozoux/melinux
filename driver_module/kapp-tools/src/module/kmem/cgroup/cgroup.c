#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/nodemask.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/memcontrol.h>
#include <linux/kernfs.h>
#include <ksioctl/kmem_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "../kmem_local.h"

#define for_each_mem_cgroup_tree(iter, root) \
	for (iter = orig_mem_cgroup_iter(root, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(root, iter, NULL))

#define for_each_mem_cgroup(iter) \
	for (iter = orig_mem_cgroup_iter(NULL, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(NULL, iter, NULL))



struct list_head *orig_cgroup_roots;
struct cgroup_subsys **orig_cgroup_subsys;
struct mem_cgroup *orig_root_mem_cgroup;
spinlock_t *orig_css_set_lock;

unsigned long (*orig_node_page_state)(struct pglist_data *pgdat,
                          enum node_stat_item item);

struct mem_cgroup *(*orig_mem_cgroup_iter)(struct mem_cgroup *root, struct mem_cgroup *prev, struct mem_cgroup_reclaim_cookie *reclaim);
//unsigned long (*orig_try_to_free_mem_cgroup_pages)(struct mem_cgroup *memcg, unsigned long nr_pages,gfp_t gfp_mask, bool may_swap);

static int kmem_charge_kprobe(struct kprobe *p, struct pt_regs *regs)
{
	struct page *page = regs->di;
	int order = regs->dx;
	struct mem_cgroup *memcg = NULL;
	struct cgroup *cgroup = NULL;
	struct kernfs_node *kn;
	//struct cgroup_subsys *ss = memcg->css.ss;

	if (unlikely(current->active_memcg)) {
		if (css_tryget_online(&current->active_memcg->css)) {
			memcg = current->active_memcg;
		}
	} else {
		memcg = get_mem_cgroup_from_mm(current->mm);
	}
	cgroup = memcg->css.cgroup;
	kn = cgroup->kn;
	if (strstr(kn->name, "test"))
		printk("zz %s cgroup:%s order:%d\n",__func__, (unsigned long)kn->name ? kn->name : "NULL", order);


	//printk("zz %s page:%lx \n",__func__, (unsigned long)page);
	//struct cgroup *cgroup = memcg->css.cgroup;
	//struct kernfs_node *kn = cgroup->kn;
	//struct cgroup_subsys *ss = memcg->css.ss;
	//printk("zz %s memcg:%lx \n",__func__, (unsigned long)memcg);


	//if (kn)
	//	trace_printk("zz %s order:%lx %s\n",__func__, (unsigned long)order, kn->name);
    return 0;
}

static int kmem_uncharge_memcg_kprobe(struct kprobe *p, struct pt_regs *regs)
{
	//struct page *page = regs->di;
	int order = regs->si;
	struct mem_cgroup *memcg = regs->di;
	struct cgroup *cgroup = NULL;
	struct kernfs_node *kn;

	cgroup = memcg->css.cgroup;
	kn = cgroup->kn;
	if (strstr(kn->name, "test"))
		printk("zz %s name:%s \n",__func__, (unsigned long)kn->name ? kn->name : "NULL");

	printk("zz %s name:%s \n",__func__, (unsigned long)current->comm);

    return 0;
}

static int kmem_uncharge_kprobe(struct kprobe *p, struct pt_regs *regs)
{
	struct page *page = regs->di;
	int order = regs->si;

	printk("zz %s name:%s order:%d\n",__func__, (unsigned long)current->comm, order);

	return 0;
}

int kmem_cgroup_scan_memcg(struct kmem_ioctl *kmem_data)
{
	struct mem_cgroup *memcg;

	for_each_mem_cgroup(memcg) {
		struct cgroup *cgroup;
		printk("zz %s memcg:%lx \n",__func__, (unsigned long)memcg);
	}
	return 0;
}

int kmem_cgroup_syms_init(void)
{
	LOOKUP_SYMS(cgroup_roots);
	LOOKUP_SYMS(mem_cgroup_iter);
	LOOKUP_SYMS(css_set_lock);
	LOOKUP_SYMS(node_page_state);
	LOOKUP_SYMS(cgroup_roots);
	LOOKUP_SYMS(cgroup_subsys);
	return 0;
}

static struct kprobe kmemkps_uncharge= {
	.symbol_name = "__memcg_kmem_uncharge_memcg",
	.pre_handler = kmem_uncharge_memcg_kprobe,
};

static struct kprobe kmemkps_uncharge_1= {
	.symbol_name = "__memcg_kmem_uncharge",
	.pre_handler = kmem_uncharge_kprobe,
};

static struct kprobe kmemkps_charge = {
	.symbol_name = "__memcg_kmem_charge",
   	.pre_handler = kmem_charge_kprobe,
};
struct kprobe *kps_kmem[3] = {&kmemkps_charge, &kmemkps_uncharge, &kmemkps_uncharge_1};

int kmem_cgroup_init(void)
{
	int ret;

	if (kmem_cgroup_syms_init())
		return -EINVAL;
	//kmem_cgroup_scan_memcg(NULL);

	ret = register_kprobes(kps_kmem,3);
	if (ret)
		return ret;

	return 0;
}

void kmem_cgroup_exit(void)
{
	unregister_kprobes(kps_kmem,3);
}

