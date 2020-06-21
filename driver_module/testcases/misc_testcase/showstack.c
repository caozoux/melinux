#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/hardirq.h>
#include <linux/kdebug.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/ftrace.h>
#include <linux/kexec.h>
#include <linux/bug.h>
#include <linux/nmi.h>
#include <linux/sysfs.h>
#include <linux/kasan.h>

#include <asm/cpu_entry_area.h>
#include <asm/stacktrace.h>
#include <asm/unwind.h> 
#include "template_iocmd.h"

#define  LOOKUP_SYMBOLEX(name, sym) do {\
		orig_##name = (void*)kallsyms_lookup_name(sym); \
		if (!orig_##name) {\
			pr_err("find %s failed\n", #name); \
			return -EINVAL; \
		} 									\
	} while(0)

#define  LOOKUP_SYMBOL(name) LOOKUP_SYMBOLEX(name,#name)

static unsigned long *(*orig_unwind_get_return_address_ptr)(struct unwind_state *state);
static int (*orig___kernel_text_address)(unsigned long addr);
static int (*orig_get_stack_info)(unsigned long *stack, struct task_struct *task, 
		struct stack_info *info, unsigned long *visit_mask);
static const char *(*orig_stack_type_name)(enum stack_type type);
static void (*orig_show_regs_if_on_stack)(struct stack_info *info, struct pt_regs *regs,
			bool partial);
static  unsigned long (*orig_ftrace_graph_ret_addr)(struct task_struct *task, int *idx,
 			unsigned long ret, unsigned long *retp);


static void show_trace_log(char *log_buf, struct task_struct *task, struct pt_regs *regs,
            unsigned long *stack, char *log_lvl)
{
    struct unwind_state state;
    struct stack_info stack_info = {0};
    unsigned long visit_mask = 0;
    int graph_idx = 0;
    bool partial = false;

    //printk("%sCall Trace:\n", log_lvl);

    unwind_start(&state, task, regs, stack);
    stack = stack ? : get_stack_pointer(task, regs);
    regs = unwind_get_entry_regs(&state, &partial);

    /*
     * Iterate through the stacks, starting with the current stack pointer.
     * Each stack has a pointer to the next one.
     *
     * x86-64 can have several stacks:
     * - task stack
     * - interrupt stack
     * - HW exception stacks (double fault, nmi, debug, mce)
     * - entry stack
     *
     * x86-32 can have up to four stacks:
     * - task stack
     * - softirq stack
     * - hardirq stack
     * - entry stack
     */
    for ( ; stack; stack = PTR_ALIGN(stack_info.next_sp, sizeof(long))) {
        const char *stack_name;

        if (orig_get_stack_info(stack, task, &stack_info, &visit_mask)) {
            /*
             * We weren't on a valid stack.  It's possible that
             * we overflowed a valid stack into a guard page.
             * See if the next page up is valid so that we can
             * generate some kind of backtrace if this happens.
             */
            stack = (unsigned long *)PAGE_ALIGN((unsigned long)stack);
            if (orig_get_stack_info(stack, task, &stack_info, &visit_mask))
                break;
        }

        stack_name = orig_stack_type_name(stack_info.type);
        //if (stack_name)
        //   printk("%s <%s>\n", log_lvl, stack_name);

        //if (regs)
        //   orig_show_regs_if_on_stack(&stack_info, regs, partial);

        /*
         * Scan the stack, printing any text addresses we find.  At the
         * same time, follow proper stack frames with the unwinder.
         *
         * Addresses found during the scan which are not reported by
         * the unwinder are considered to be additional clues which are
         * sometimes useful for debugging and are prefixed with '?'.
         * This also serves as a failsafe option in case the unwinder
         * goes off in the weeds.
         */
        for (; stack < stack_info.end; stack++) {
            unsigned long real_addr;
            int reliable = 0;
            unsigned long addr = READ_ONCE_NOCHECK(*stack);
            unsigned long *ret_addr_p =
                orig_unwind_get_return_address_ptr(&state);

            if (!orig___kernel_text_address(addr))
                continue;

            /*
             * Don't print regs->ip again if it was already printed
             * by show_regs_if_on_stack().
             */
            if (regs && stack == &regs->ip)
                goto next;

            if (stack == ret_addr_p)
                reliable = 1;

            /*
             * When function graph tracing is enabled for a
             * function, its return address on the stack is
             * replaced with the address of an ftrace handler
             * (return_to_handler).  In that case, before printing
             * the "real" address, we want to print the handler
             * address as an "unreliable" hint that function graph
             * tracing was involved.
             */
            real_addr = orig_ftrace_graph_ret_addr(task, &graph_idx,
                             addr, stack);
#if 0
            if (real_addr != addr)
				 printk("%s %s%pB\n", log_lvl,  "", (void *)addr);
			printk("%s %s%pB\n", log_lvl, reliable ? "" : "? ", (void *)addr);
            //   printk_stack_address(addr, 0, log_lvl);
            //printk_stack_address(real_addr, reliable, log_lvl);
#else
            if (real_addr != addr)
				 log_buf += sprintf(log_buf, "%pB\n", (void *)addr);
			log_buf += sprintf(log_buf, "%pB\n", (void *)addr);
            //   printk_stack_address(addr, 0, log_lvl);
#endif

            if (!reliable)
                continue;

next:
            /*
             * Get the next frame from the unwinder.  No need to
             * check for an error: if anything goes wrong, the rest
             * of the addresses will just be printed as unreliable.
             */
            unwind_next_frame(&state);

            /* if the frame has entry regs, print them */
            regs = unwind_get_entry_regs(&state, &partial);
            //if (regs)
            //    orig_show_regs_if_on_stack(&stack_info, regs, partial);
        }

        //if (stack_name)
        //   printk("%s </%s>\n", log_lvl, stack_name);
    }
}

void misc_show_stack(char *log_buf, struct task_struct *task, unsigned long *sp)
{
	task = task ? : current; 
	if (!sp && task == current)
		sp = get_stack_pointer(current, NULL);
	show_trace_log(log_buf, task, NULL, sp, KERN_DEFAULT);
}

int showstack_init(void)
{
	LOOKUP_SYMBOL(unwind_get_return_address_ptr);
	LOOKUP_SYMBOL(__kernel_text_address);
	LOOKUP_SYMBOL(get_stack_info);
	LOOKUP_SYMBOL(stack_type_name);
	LOOKUP_SYMBOL(show_regs_if_on_stack);
	LOOKUP_SYMBOL(ftrace_graph_ret_addr);
	return 0;
}
