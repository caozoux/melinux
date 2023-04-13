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
#include <linux/slab.h>
#include <linux/slub_def.h>
#include <linux/blk-cgroup.h>
#include <ksioctl/kmem_ioctl.h>

#include "hotfix_util.h"
#include "ksysdata.h"
#include "ksysd_ioctl.h"
#include "../kmem_local.h"

//#define ENABLE_KMEM_CGROUP_KPROBE


#define for_each_mem_cgroup_tree(iter, root) \
	for (iter = orig_mem_cgroup_iter(root, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(root, iter, NULL))

#define for_each_mem_cgroup(iter) \
	for (iter = orig_mem_cgroup_iter(NULL, NULL, NULL);  \
			iter != NULL;  \
			iter = orig_mem_cgroup_iter(NULL, iter, NULL))

struct cgroup_subsys **orig_cgroup_subsys;
#define for_each_subsys(ss, ssid) \
	 for ((ssid) = 0; (ssid) < CGROUP_SUBSYS_COUNT &&        \
			 (((ss) = orig_cgroup_subsys[ssid]) || true); (ssid)++)

#define for_each_root(root) \
	list_for_each_entry((root), &cgroup_roots, root_list)

struct cgroup_subsys_state * (*orig_css_next_descendant_post)(struct cgroup_subsys_state *pos, struct cgroup_subsys_state *root);
#define mcss_for_each_descendant_post(pos, css)              \
	for ((pos) = orig_css_next_descendant_post(NULL, (css)); (pos);  \
			(pos) = orig_css_next_descendant_post((pos), (css)))
	
struct list_head *orig_cgroup_roots;
struct mem_cgroup *orig_root_mem_cgroup;
spinlock_t *orig_css_set_lock;

unsigned long (*orig_node_page_state)(struct pglist_data *pgdat,
                          enum node_stat_item item);

struct mem_cgroup *(*orig_mem_cgroup_iter)(struct mem_cgroup *root, struct mem_cgroup *prev, struct mem_cgroup_reclaim_cookie *reclaim);
//unsigned long (*orig_try_to_free_mem_cgroup_pages)(struct mem_cgroup *memcg, unsigned long nr_pages,gfp_t gfp_mask, bool may_swap);

void transfter_css_str_status(struct cgroup_subsys_state *css)
{
	CSS_NO_REF
	
}

int kmem_cgroup_scan_blkcg(struct kmem_ioctl *kmem_data)
{
	struct cgroup *root = blkcg_root.css.cgroup;
	struct cgroup_subsys_state  *css;
	int count = 0;

	mcss_for_each_descendant_post(css, &blkcg_root.css) {
	//printk("zz css:%lx refcnt:%ld\n", (unsigned long)css, refcount_read(&css->refcnt));
	printk("zz css:%lx refcnt:%lx %d %d\n", (unsigned long)css, atomic_long_read(&css->refcnt.count), percpu_ref_is_zero(&css->refcnt), atomic_read(&css->online_cnt));
		count++;
	}
	printk("zz %s count:%lx \n",__func__, (unsigned long)count);
#if 0
	struct cgroup_subsys_state *pos_css;
	struct blkcg_gq *blkg;
	blkg_for_each_descendant_post(blkg, pos_css, blkcg_root.blkg_hint) {

	}
#endif
	return 0;
}

int kmem_cgroup_scan_subsys(struct kmem_ioctl *kmem_data)
{
	struct cgroup_subsys *ss;
	int i;

	for_each_subsys(ss, i) {
		printk("%s\t%d\t%d\t%d\n",
			ss->legacy_name, ss->root->hierarchy_id,
			atomic_read(&ss->root->nr_cgrps),
			i);
	}

	return 0;
}

int kmem_cgroup_scan_memcg(struct kmem_ioctl *kmem_data)
{
	struct mem_cgroup *memcg;

	for_each_mem_cgroup(memcg) {
		//struct cgroup *cgroup;
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
	LOOKUP_SYMS(css_next_descendant_post);
	return 0;
}


void dump_cgroup_kmem_info(pid_t pid)
{
	struct task_struct *task = find_process_by_pid(pid);
	struct mem_cgroup *memcg;
	struct slabinfo sinfo;
	struct kmem_cache *s;
	
	if (!task) {
		DBG("pid:%d not find task\n", pid);
		return;
	}

	memcg = task->active_memcg;
	if (!memcg)
		memcg = get_mem_cgroup_from_mm(task->mm);

	if (!memcg) {
		DBG("pid:%d not find memcg\n", pid);
		return;
	}

	s = list_entry(&memcg->kmem_caches, struct kmem_cache, memcg_params.kmem_caches_node);
	//s = list_entry(p, struct kmem_cache, list);
	//s = list_first_entry(&memcg->kmem_caches, struct kmem_cache, list);		

	//
	//memset(&sinfo, 0, sizeof(sinfo));
	//get_slabinfo(s, &sinfo);
	//mutex_lock(&slab_mutex);

	//&memcg->kmem_cachesk
	
}

#ifdef ENABLE_KMEM_CGROUP_KPROBE
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

static int kmem_krpobe_init(void)
{
	return register_kprobes(kps_kmem,3);
}

static void kmem_krpobe_exit(void)
{
	unregister_kprobes(kps_kmem,3);
}
#else
static int kmem_krpobe_init(void)
{
	return 0;
}

static void kmem_krpobe_exit(void)
{
}
#endif

int kmem_cgroup_init(void)
{
	int ret;

	if (kmem_cgroup_syms_init())
		return -EINVAL;
	//kmem_cgroup_scan_memcg(NULL);
	kmem_cgroup_scan_subsys(NULL);
	kmem_cgroup_scan_blkcg(NULL);

	ret = kmem_krpobe_init();
	if (ret)
		return ret;

	return 0;
}

void kmem_cgroup_exit(void)
{
	kmem_krpobe_exit();
}

