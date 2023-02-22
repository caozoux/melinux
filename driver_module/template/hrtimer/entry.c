#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>

#define LOOKUP_SYMS(name) do {							\
		orig_##name = (void *)cust_kallsyms_lookup_name(#name);		\
		if (!orig_##name) {						\
			pr_err("kallsyms_lookup_name: %s\n", #name);		\
			return -EINVAL;						\
		}								\
	} while (0)

unsigned long (*cust_kallsyms_lookup_name)(const char *name);
struct hrtimer hrtimer_pr;
int *orig_css_set_count;

struct list_head *origme__cgrp_cpuctx_list;
static DEFINE_PER_CPU(struct list_head, me_cgrp_cpuctx_list);
static enum hrtimer_restart hrtimer_pr_fun(struct hrtimer *hrtimer)
{
  	trace_printk("zz css_set_count:%d \n", *orig_css_set_count);
	hrtimer_forward_now(&hrtimer_pr, ns_to_ktime(1000000000));
	return HRTIMER_RESTART;
}

static int (*ksys_kallsyms_on_each_symbol)(int (*fn)(void *, const char *,
        struct module *, unsigned long),void *data);

static int symbol_walk_callback(void *data, const char *name,
        struct module *mod, unsigned long addr)
{
    if (strcmp(name, "kallsyms_lookup_name") == 0) {
        cust_kallsyms_lookup_name = (void *)addr;
        return addr;
    }

    return 0;
}

static int get_kallsyms_lookup_name(void)
{
    int ret;
    ksys_kallsyms_on_each_symbol = &kallsyms_on_each_symbol;
    ret = ksys_kallsyms_on_each_symbol(symbol_walk_callback, NULL);
    if (!ret || !cust_kallsyms_lookup_name)
        return -EINVAL;

    return 0;
}

int sym_init(void)
{
	if (LOOKUP_SYMS("css_set_count")) 
		return -EINVAL;

	if (LOOKUP_SYMS("cgrp_cpuctx_list")) 
		return -EINVAL;

  return 0;
}

static int hrtimer_pr_init(void)
{
	hrtimer_init(&hrtimer_pr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_pr.function = hrtimer_pr_fun;
	hrtimer_start(&hrtimer_pr, ns_to_ktime(1000000000),
			HRTIMER_MODE_REL_PINNED);

	return 0;
}

static void hrtimer_pr_exit(void)
{
	hrtimer_cancel(&hrtimer_pr);
}

static int __init percpu_hrtimer_init(void)
{

  if (get_kallsyms_lookup_name())
    return -EINVAL;

  if (sym_init())
    return -EINVAL;

  printk("zz css_set_count:%d \n", *orig_css_set_count);
  printk("zz %s me_cgrp_cpuctx_list:%lx \n",__func__, (unsigned long)&me_cgrp_cpuctx_list);
  printk("zz %s origme__cgrp_cpuctx_list:%lx %lx\n",__func__, (unsigned long)origme__cgrp_cpuctx_list, this_cpu_ptr(origme__cgrp_cpuctx_list));
  hrtimer_pr_init();	
  return 0;
}

static void __exit percpu_hrtimer_exit(void)
{
  hrtimer_pr_exit();	
}

module_init(percpu_hrtimer_init);
module_exit(percpu_hrtimer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
