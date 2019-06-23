#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/trace_test.h>
#include <linux/kallsyms.h>
#include <asm/ftrace.h>

u32 nop_val=0xd503201f;
u32 bl_val=0x97ffffff;
int (*orig_aarch64_insn_read) (void *addr, u32 *insnp);
int (*orig_aarch64_insn_patch_text_nosync)(void *addr, u32 insn);

static int sysmbol_init(void)
{

	orig_aarch64_insn_read=kallsyms_lookup_name("aarch64_insn_read");
	if (!orig_aarch64_insn_read) {
		printk("find aarch64_insn_read failed\n");
		return 1;
	}

	orig_aarch64_insn_patch_text_nosync=kallsyms_lookup_name("aarch64_insn_patch_text_nosync");
	if (!orig_aarch64_insn_patch_text_nosync) {
		printk("find aarch64_insn_patch_text_nosync failed\n");
		return 1;
	}
	return 0;
}

static int ftrace_modify_jump(unsigned long func_addr)
{
	unsigned long pc;
	int offset;
	unsigned long mcount_addr=(unsigned long) _mcount;
	u32 replaced, new;
	pc = func_addr+12;
	offset= (int)(mcount_addr - pc)/4;
	/*
	printk("jump:%lx mcount:%lx\n", pc, mcount_addr);
	printk("offset:%x \n", offset);
	printk("trace_test mcount addr:%lx \n", pc);
	*/
	new = bl_val & (u32)offset;
	//printk("new code:%x \n", new);
	if (orig_aarch64_insn_read((void *)pc, &replaced)) {
		printk("read trace_test pc code failed\n");
        return 1;
	}

	if (orig_aarch64_insn_patch_text_nosync((void *)pc, new)) {
		printk("read trace_test pc reset nop failed\n");
        return 1;

	}

   return 0;
}

static int ftrace_modify_nop(unsigned long func_addr)
{
	unsigned long pc;
	u32 replaced;
	pc = func_addr+12;
	printk("trace_test mcount addr:%lx \n", pc);
	if (orig_aarch64_insn_read((void *)pc, &replaced)) {
		printk("read trace_test pc code failed\n");
        return 1;
	}

	if (orig_aarch64_insn_patch_text_nosync((void *)pc, nop_val)) {
		printk("read trace_test pc reset nop failed\n");
        return 1;

	}

	if (orig_aarch64_insn_read((void *)pc, &replaced)) {
		printk("read trace_test pc code failed\n");
        return 1;
	}
   return 0;
}

static int __init gpiodriver_init(void)
{
	u32 replaced;
    unsigned long pc = (unsigned long)trace_test;
	printk("gpiodriver load \n");
	if (sysmbol_init())
		return 0;

	if (ftrace_modify_nop(pc)) {
		return 0;
	}

	if (ftrace_modify_jump(pc)) {
		return 0;
	}

	printk("trace_test pc code:%x \n", replaced);

	trace_test_init();
	trace_test();
	return 0;
}

static void __exit gpiodriver_exit(void)
{
	printk("gpiodriver unload \n");
}

module_init(gpiodriver_init);
module_exit(gpiodriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
MODULE_LICENSE("Dual BSD/GPL");
