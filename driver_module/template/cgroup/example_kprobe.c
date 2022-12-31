#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/cgroup.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/netdevice.h>
#include <linux/syscore_ops.h>
#include <linux/cgroup-defs.h>
#include <linux/hashtable.h>

struct cgrp_cset_link {
    /* the cgroup and css_set this link associates */
    struct cgroup       *cgrp;
    struct css_set      *cset;

    /* list of cgrp_cset_links anchored at cgrp->cset_links */
    struct list_head    cset_link;

    /* list of cgrp_cset_links anchored at css_set->cgrp_links */
    struct list_head    cgrp_link;
};

#define LOOKUP_SYMS(name) do {                          \
       orig_##name = (void *)kallsyms_lookup_name(#name);      \
       if (!orig_##name) {                     \
           pr_err("kallsyms_lookup_name: %s\n", #name);        \
           return -EINVAL;                     \
       }                               \
   } while (0)

bool (*orig_cgroup_on_dfl)(const struct cgroup *cgrp);
spinlock_t *orig_css_set_lock;
struct cgroup_subsys **orig_cgroup_subsys;
struct mutex *orig_cgroup_mutex;
static u16 *orig_cgrp_dfl_threaded_ss_mask;
struct hlist_head *orig_css_set_table;
//DEFINE_SPINLOCK(css_set_lock);

#define for_each_subsys(ss, ssid)                   \
			for ((ssid) = 0; (ssid) < CGROUP_SUBSYS_COUNT &&        \
				(((ss) = orig_cgroup_subsys[ssid]) || true); (ssid)++)

bool cgroup_is_threaded(struct cgroup *cgrp)
{
    return cgrp->dom_cgrp != cgrp;
}

/* subsystems enabled on a cgroup */
static u16 cgroup_ss_mask(struct cgroup *cgrp)
{
    struct cgroup *parent = cgroup_parent(cgrp);

    if (parent) {
        u16 ss_mask = parent->subtree_ss_mask;

        /* threaded cgroups can only have threaded controllers */
        if (cgroup_is_threaded(cgrp))
            ss_mask &= *orig_cgrp_dfl_threaded_ss_mask;
        return ss_mask;
    }

    return cgrp->root->subsys_mask;
}

/**
 * cgroup_css - obtain a cgroup's css for the specified subsystem
 * @cgrp: the cgroup of interest
 * @ss: the subsystem of interest (%NULL returns @cgrp->self)
 *
 * Return @cgrp's css (cgroup_subsys_state) associated with @ss.  This
 * function must be called either under cgroup_mutex or rcu_read_lock() and
 * the caller is responsible for pinning the returned css if it wants to
 * keep accessing it outside the said locks.  This function may return
 * %NULL if @cgrp doesn't have @subsys_id enabled.
 */
static struct cgroup_subsys_state *cgroup_css(struct cgroup *cgrp,
                          struct cgroup_subsys *ss)
{
    if (ss)
        return rcu_dereference_check(cgrp->subsys[ss->id],
                    lockdep_is_held(&cgroup_mutex));
    else
        return &cgrp->self;
}

/**
 * cgroup_e_css_by_mask - obtain a cgroup's effective css for the specified ss
 * @cgrp: the cgroup of interest
 * @ss: the subsystem of interest (%NULL returns @cgrp->self)
 *
 * Similar to cgroup_css() but returns the effective css, which is defined
 * as the matching css of the nearest ancestor including self which has @ss
 * enabled.  If @ss is associated with the hierarchy @cgrp is on, this
 * function is guaranteed to return non-NULL css.
 */
static struct cgroup_subsys_state *cgroup_e_css_by_mask(struct cgroup *cgrp,
                            struct cgroup_subsys *ss)
{
    lockdep_assert_held(&cgroup_mutex);

    if (!ss)
        return &cgrp->self;

    /*
     * This function is used while updating css associations and thus
     * can't test the csses directly.  Test ss_mask.
     */
    while (!(cgroup_ss_mask(cgrp) & (1 << ss->id))) {
        cgrp = cgroup_parent(cgrp);
        if (!cgrp)
            return NULL;
    }

    return cgroup_css(cgrp, ss);
}

static bool compare_css_sets(struct css_set *cset,
                 struct css_set *old_cset,
                 struct cgroup *new_cgrp,
                 struct cgroup_subsys_state *template[])
{
    struct cgroup *new_dfl_cgrp;
    struct list_head *l1, *l2;

    /*
     * On the default hierarchy, there can be csets which are
     * associated with the same set of cgroups but different csses.
     * Let's first ensure that csses match.
     */
    if (memcmp(template, cset->subsys, sizeof(cset->subsys)))
        return false;


    /* @cset's domain should match the default cgroup's */
    if (orig_cgroup_on_dfl(new_cgrp))
        new_dfl_cgrp = new_cgrp;
    else
        new_dfl_cgrp = old_cset->dfl_cgrp;

    if (new_dfl_cgrp->dom_cgrp != cset->dom_cset->dfl_cgrp)
        return false;
    /*
     * Compare cgroup pointers in order to distinguish between
     * different cgroups in hierarchies.  As different cgroups may
     * share the same effective css, this comparison is always
     * necessary.
     */
    l1 = &cset->cgrp_links;
    l2 = &old_cset->cgrp_links;
    while (1) {
        struct cgrp_cset_link *link1, *link2;
        struct cgroup *cgrp1, *cgrp2;

        l1 = l1->next;
        l2 = l2->next;
        /* See if we reached the end - both lists are equal length. */
        if (l1 == &cset->cgrp_links) {
            BUG_ON(l2 != &old_cset->cgrp_links);
            break;
        } else {
            BUG_ON(l2 == &old_cset->cgrp_links);
        }
        /* Locate the cgroups associated with these links. */
        link1 = list_entry(l1, struct cgrp_cset_link, cgrp_link);
        link2 = list_entry(l2, struct cgrp_cset_link, cgrp_link);
        cgrp1 = link1->cgrp;
        cgrp2 = link2->cgrp;
        /* Hierarchies should be linked in the same order. */
        BUG_ON(cgrp1->root != cgrp2->root);

        /*
         * If this hierarchy is the hierarchy of the cgroup
         * that's changing, then we need to check that this
         * css_set points to the new cgroup; if it's any other
         * hierarchy, then this css_set should point to the
         * same cgroup as the old css_set.
         */
        if (cgrp1->root == new_cgrp->root) {
            if (cgrp1 != new_cgrp)
                return false;
        } else {
            if (cgrp1 != cgrp2)
                return false;
        }
    }

    return true;
}

static unsigned long css_set_hash(struct cgroup_subsys_state *css[])
{
    unsigned long key = 0UL;
    struct cgroup_subsys *ss;
    int i;

    for_each_subsys(ss, i)
        key += (unsigned long)css[i];
    key = (key >> 16) ^ key;

    return key;
}

/**
 * find_existing_css_set - init css array and find the matching css_set
 * @old_cset: the css_set that we're using before the cgroup transition
 * @cgrp: the cgroup that we're moving into
 * @template: out param for the new set of csses, should be clear on entry
 */
static struct css_set *find_existing_css_set(struct css_set *old_cset,
                    struct cgroup *cgrp,
                    struct cgroup_subsys_state *template[])
{
    struct cgroup_root *root = cgrp->root;
    struct cgroup_subsys *ss;
    struct css_set *cset;
    unsigned long key;
    int i;

    /*
     * Build the set of subsystem state objects that we want to see in the
     * new css_set. while subsystems can change globally, the entries here
     * won't change, so no need for locking.
     */
    for_each_subsys(ss, i) {
        if (root->subsys_mask & (1UL << i)) {
            /*
             * @ss is in this hierarchy, so we want the
             * effective css from @cgrp.
             */
            template[i] = cgroup_e_css_by_mask(cgrp, ss);
			printk("zz %s %d subsys activate:%d\n", __func__, __LINE__, i);
        } else {
			printk("zz %s %d notactivate:%d\n", __func__, __LINE__, i);
            /*
             * @ss is not in this hierarchy, so we don't want
             * to change the css.
             */
            template[i] = old_cset->subsys[i];
        }
    }

    key = css_set_hash(template);
	#define CSS_SET_HASH_BITS   7
    //hash_for_each_possible(orig_css_set_table, cset, hlist, key) {
	
	//hlist_for_each_entry(cset, &orig_css_set_table[hash_min(key, ilog2 HASH_BITS(name))], hlist) {
	hlist_for_each_entry(cset, &orig_css_set_table[hash_min(key, CSS_SET_HASH_BITS)], hlist) {
        if (!compare_css_sets(cset, old_cset, cgrp, template))
            continue;
        /* This css_set matches what we need */
        return cset;
    }

    /* No existing cgroup group matched */
    return NULL;
}

static int kprobe_find_css_set(struct kprobe *kp, struct pt_regs *regs)
{
	struct cgroup_subsys_state *template[CGROUP_SUBSYS_COUNT] = { };
	struct css_set *cset;
	struct list_head tmp_links;
	struct cgrp_cset_link *link;
	struct cgroup_subsys *ss;
	unsigned long key;
	int ssid;
	struct timespec64 ts_s, ts_e;
	unsigned long diff = 0;


	struct css_set *old_cset = (struct css_set *) regs->di;
	struct cgroup *cgrp = (struct cgroup *) regs->si;

	if (cgrp && cgrp->kn) {
		if (cgrp->kn->name)
			printk("zz %s %d %s\n", __func__, __LINE__, cgrp->kn->name);
		if (cgrp->kn->parent)
			printk("zz %s %d parent:%s\n", __func__, __LINE__, cgrp->kn->parent->name);
		if (cgrp->kn->parent && cgrp->kn->parent->parent && cgrp->kn->parent->parent->name)
			printk("zz %s %d parent-p:%s\n", __func__, __LINE__, cgrp->kn->parent->parent->name);

	}
	ktime_get_real_ts64(&ts_s);

	/* First see if we already have a cgroup group that matches
	* the desired set */
	spin_lock_irq(orig_css_set_lock);
	cset = find_existing_css_set(old_cset, cgrp, template);
	//if (cset)
	//	get_css_set(cset);
	spin_unlock_irq(orig_css_set_lock);
	ktime_get_real_ts64(&ts_e);

	diff = (ts_e.tv_sec - ts_s.tv_sec) * NSEC_PER_SEC + \ 
		 (ts_e.tv_nsec - ts_s.tv_nsec);
	printk("zz %s diff:%ld \n",__func__, (unsigned long)diff);


	return 0;
}

struct kprobe kplist[] = {
	{
        .symbol_name = "find_css_set",
        .pre_handler = kprobe_find_css_set,
	},
};

static int symbol_init(void)
{
	orig_css_set_lock = kallsyms_lookup_name("css_set_lock");
	if (!orig_css_set_lock)
		return -EINVAL;

	orig_cgroup_subsys = (struct cgroup_subsys **)kallsyms_lookup_name("cgroup_subsys");
	if (!orig_cgroup_subsys)
		return -EINVAL;

	LOOKUP_SYMS(cgroup_mutex);
	LOOKUP_SYMS(cgrp_dfl_threaded_ss_mask);
	LOOKUP_SYMS(css_set_table);
	LOOKUP_SYMS(cgroup_on_dfl);

	return 0;
}

static int __init kprobe_driver_init(void)
{
	int i;
	if (symbol_init())
		return -EINVAL;

	for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
		if (register_kprobe(&kplist[i])) {
			printk("register kprobe failed:%s \n", kplist[i].symbol_name);
			while (i>0) {
				i--;
				unregister_kprobe(&kplist[i]);
			}
			goto out;
		}
	}
	printk("zz %s %d \n", __func__, __LINE__);
	return 0;
out:
	return -EINVAL;
}

static void __exit kprobe_driver_exit(void)
{
	int i;
	for (i = 0; i < sizeof(kplist)/sizeof(struct kprobe); ++i) {
		unregister_kprobe(&kplist[i]);
	}
	printk("zz %s %d \n", __func__, __LINE__);
}

module_init(kprobe_driver_init);
module_exit(kprobe_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
