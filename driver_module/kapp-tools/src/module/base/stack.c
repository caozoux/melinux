#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/tracepoint.h>
#include <hotfix_util.h>
#include <linux/perf_event.h>
#include <asm/unwind.h>

#include <ksioctl/kstack_ioctl.h>
#include "internal.h"
#include "local.h"

struct ksys_callchain_data {
	ksys_callchain *array;
	atomic_t curcnt;
	spinlock_t lock;
};

struct ksys_callchain_data *ksys_callchain_head;

extern struct perf_callchain_entry *
(*orig_get_perf_callchain)(struct pt_regs *regs, u32 init_nr, bool kernel, bool user,
		  u32 max_stack, bool crosstask, bool add_mark);
extern int (*orig_get_callchain_buffers)(int event_max_stack);
/*
 * Determine whether the regs were taken from an irq/exception handler rather
 * than from perf_arch_fetch_caller_regs().
 */
static bool perf_hw_regs(struct pt_regs *regs)
{
    return regs->flags & X86_EFLAGS_FIXED;
}

void
ksys_callchain_kernel(struct perf_callchain_entry_ctx *entry, struct pt_regs *regs)
{
#if 0
    struct unwind_state state;
    unsigned long addr;

    if (perf_callchain_store(entry, regs->ip))
        return;

    if (perf_hw_regs(regs))
        unwind_start(&state, current, regs, NULL);
    else
        unwind_start(&state, current, NULL, (void *)regs->sp);

    for (; !unwind_done(&state); unwind_next_frame(&state)) {
        addr = unwind_get_return_address(&state);
        if (!addr || perf_callchain_store(entry, addr))
            return;
    }
#endif
}

int ksys_callchain_save(ksys_callchain *entry)
{
	int cnt, i; 
	ksys_callchain *record;

	spin_lock(&ksys_callchain_head->lock);
	cnt = atomic_read(&ksys_callchain_head->curcnt);

	if (cnt >= MAX_SAVE)
		goto failed;

	for (i = 0; i < cnt; ++i) {
		record = &ksys_callchain_head->array[i];
		if (record->key == entry->key) {
			record->count++;
			goto find;
		}
	}
	if (cnt >= MAX_SAVE)
		goto failed;

	record = &ksys_callchain_head->array[cnt];
	*record = *entry;
	record->count++;
	atomic_inc_return(&ksys_callchain_head->curcnt);

	spin_unlock(&ksys_callchain_head->lock);
	return 0;
find:
	spin_unlock(&ksys_callchain_head->lock);
	return 0;
failed:
	spin_unlock(&ksys_callchain_head->lock);
	return -ENOMEM;
}

int ksys_stack_dump(struct pt_regs *regs)
{
	unsigned long addr;
	unsigned long *sp;
	struct unwind_state state;
	unsigned long key = 0;
	ksys_callchain entry = {0};

	if (!regs)
		sp = get_stack_pointer(current, NULL);
	else
		sp = (void*)regs->sp;

	//unwind_start(&state, current, NULL, (void *)regs->sp);
	unwind_start(&state, current, NULL, (void *)sp);
	for (; !unwind_done(&state); unwind_next_frame(&state)) {
		addr = unwind_get_return_address(&state);

		if (!addr)
			goto save;

		key += addr;
		entry.address[entry.offset++] = addr;
		if (entry.offset >= MAX_DEPTACH)
			goto save;

	}

save:
	entry.key = key;
	ksys_callchain_save(&entry);
	/*
	int err;
	u32 init_nr = 127 - 0;
	struct perf_callchain_entry *trace;

	
	printk("zz %s trace:%lx \n",__func__, (unsigned long)trace);
	err = orig_get_callchain_buffers(127);
	trace = orig_get_perf_callchain(NULL, init_nr, true, false,
  			127, false, false);
	printk("zz %s err:%lx \n",__func__, (unsigned long)err);
	*/
	return 0;
}

int user_get_ksys_callchain_buffers(void __user *buf, size_t size)
{
	int cnt, chain_size; 
	int ret;
	ksys_callchain *entry;

	spin_lock(&ksys_callchain_head->lock);
	cnt = atomic_read(&ksys_callchain_head->curcnt);
	chain_size = (sizeof(ksys_callchain) * cnt);
	size = size > chain_size ? chain_size : size/(sizeof(ksys_callchain)) * sizeof(ksys_callchain);

	ret = copy_to_user(buf, ksys_callchain_head->array, size);
	if (!ret)
		ret = size;

	spin_unlock(&ksys_callchain_head->lock);
	entry = &ksys_callchain_head->array[0];
	return ret;
}

int clear_ksys_callchain_buffers(void)
{
	spin_lock(&ksys_callchain_head->lock);
	atomic_set(&ksys_callchain_head->curcnt, 0);
	spin_unlock(&ksys_callchain_head->lock);
	return 0;
}

int stack_init(void)
{
	ksys_callchain_head = kzalloc(sizeof(struct ksys_callchain_data), GFP_KERNEL);
	if (!ksys_callchain_head)
		return -ENOMEM;

	ksys_callchain_head->array = kmalloc(sizeof(ksys_callchain) * MAX_SAVE, GFP_KERNEL);
	if (!ksys_callchain_head->array)
		goto out;

	spin_lock_init(&ksys_callchain_head->lock);

	return 0;
out:
	kfree(ksys_callchain_head);
	return -ENOMEM;
}

void stack_exit(void)
{
	kfree(ksys_callchain_head->array);
	kfree(ksys_callchain_head);	
}

