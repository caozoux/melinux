#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>
#include <hotfix_util.h>
#include "local.h"

//#include "ktrace.h"

extern struct kd_percpu_data *kd_percpu_data[512];

static struct tracepoint *tp_ret;

typedef struct tracepoint* tracepoint_ptr;
#define tracepoint_ptr_deref(iter) iter

struct mutex *orig_tracepoint_module_list_mutex;
struct list_head *orig_tracepoint_module_list;
tracepoint_ptr orig___start___tracepoints_ptrs;
tracepoint_ptr orig___stop___tracepoints_ptrs;

static void __maybe_unused probe_tracepoint(struct tracepoint *tp, void *priv)
{
    char *n = priv;

    if (strcmp(tp->name, n) == 0)
        tp_ret = tp;
}

#if 0
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
#endif

static struct tracepoint *find_tracepoint(const char *name)
{
	tracepoint_ptr iter;
	for (iter = orig___start___tracepoints_ptrs; iter < orig___stop___tracepoints_ptrs; iter++) {
#if 0
		if (strcmp(tracepoint_ptr_deref(iter)->name, name) == 0)
			return  tracepoint_ptr_deref(iter);
#endif
	}


    return NULL;
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

int base_trace_init(void)
{
	LOOKUP_SYMS(tracepoint_module_list_mutex);
	LOOKUP_SYMS(tracepoint_module_list);
	LOOKUP_SYMS(__start___tracepoints_ptrs);
	LOOKUP_SYMS(__stop___tracepoints_ptrs);
	return 0;
}

