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

//#include "ktrace.h"

extern struct kd_percpu_data *kd_percpu_data[512];

static struct tracepoint *tp_ret;
struct mutex *orig_tracepoint_module_list_mutex;
struct list_head *orig_tracepoint_module_list;
struct softirq_action *orig_softirq_vec;

static void probe_tracepoint(struct tracepoint *tp, void *priv)
{
    char *n = priv;

    if (strcmp(tp->name, n) == 0)
        tp_ret = tp;
}

static void orig_for_each_tracepoint_range(
                struct tracepoint *begin, struct tracepoint *end,
                void (*fct)(struct tracepoint *tp, void *priv),
                void *priv)
{
        struct tracepoint *iter;

        if (!begin)
                return;
        //for (iter = begin; iter < end; iter++)
        //       fct(tracepoint_ptr_deref(iter), priv);
}

static void for_each_moudule_trace_point(
        void (*fct)(struct tracepoint *tp, void *priv),
        void *priv)
{
    struct tp_module *tp_mod;
    struct module *mod;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 15, 0)
    mutex_lock(orig_tracepoints_mutex);
#else
    mutex_lock(orig_tracepoint_module_list_mutex);
#endif

    list_for_each_entry(tp_mod, orig_tracepoint_module_list, list) {
            mod = tp_mod->mod;
            if (mod->num_tracepoints) {
               // orig_for_each_tracepoint_range((mod->tracepoints_ptrs),
            //mod->tracepoints_ptrs + mod->num_tracepoints, fct, priv);
          }
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 15, 0)
    mutex_unlock(orig_tracepoints_mutex);
#else
    mutex_unlock(orig_tracepoint_module_list_mutex);
#endif
}


static struct tracepoint *find_tracepoint(const char *name)
{
    tp_ret = NULL;
    for_each_kernel_tracepoint(probe_tracepoint, (void *)name);
    for_each_moudule_trace_point(probe_tracepoint, (void *)name);

    return tp_ret;
}

int register_tracepoint(const char *name, void *probe, void *data)
{
    struct tracepoint *tr_point;

    tr_point = find_tracepoint(name);
    if (!tr_point)
        return 0;

    return tracepoint_probe_register(tr_point, probe, data);
}

int unregister_tracepoint(const char *name, void *probe, void *data)
{
    struct tracepoint *tr_point;
    int ret = 0;

    tr_point = find_tracepoint(name);
    if (!tr_point)
        return 0;

    do {
        ret = tracepoint_probe_unregister(tr_point, probe, data);
    } while (ret == -ENOMEM);

    return ret;
}

struct kd_percpu_data *get_kd_percpu_data(void)
{
	int cpu = smp_processor_id();
	return kd_percpu_data[cpu];
}

int base_func_init(void)
{
	LOOKUP_SYMS(tracepoint_module_list_mutex);
	LOOKUP_SYMS(tracepoint_module_list);
	LOOKUP_SYMS(softirq_vec);
	return 0;
}

