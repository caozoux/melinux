// SPDX-License-Identifier: GPL-2.0
/*
 * ARM system register access device
 *
 * This device is accessed by lseek() to the appropriate register number
 * and then read/write in chunks of 8 bytes.  A larger size means multiple
 * reads or writes of the same register.
 *
 * This driver uses /dev/cpu/%d/msr where %d is the minor number, and on
 * an SMP box will direct the access to CPU %d.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/smp.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/cpufeature.h>
#include <asm/cpu.h>
#include <asm/sysreg.h>
#include <asm/insn.h>
#include <asm/traps.h>
#include <asm/mmu.h>
#include <asm/barrier.h>
#include <asm-generic/fixmap.h>
#include <linux/printk.h>
#include <linux/kallsyms.h>
#include "msr_arm.h"
#include "misc_ioctl.h"

#define AARCH64_INSN_SF_BIT	BIT(31)
#define AARCH64_INSN_N_BIT	BIT(22)
#define AARCH64_INSN_LSL_12	BIT(22)

static DEFINE_RAW_SPINLOCK(msr_lock);
/* record the address of reading or writing */
static u32 *rd_tp;
static u32 *wr_tp;

struct msr_info_completion {
	struct msr_info		msr;
	struct completion	done;
};

atomic_t msr_flags = ATOMIC_INIT(0);
EXPORT_SYMBOL(msr_flags);
atomic_t mrs_flags = ATOMIC_INIT(0);
EXPORT_SYMBOL(mrs_flags);

static struct class *msr_class;
static enum cpuhp_state cpuhp_msr_state;

/* kernel function which not export */
int aarch64_insn_patch_text_smc(void *addrs[], u32 insns[], int cnt)
{
	/* kernel function which not export */
	int (*_aarch64_insn_patch_text)(void *addrs[], u32 insns[], int cnt);
	_aarch64_insn_patch_text =
		(void *)get_func_from_kallsyms("aarch64_insn_patch_text");
	_aarch64_insn_patch_text(addrs, insns, cnt);
}

/*
 * Self-modify code for label of read address.
 */
int aarch64_modify_read_text(u32 opcode)
{
	void *addrs[1];

	addrs[0] = rd_tp;
	/*
	 * call aarch64_insn_patch_text to modify
	 * the opcode
	 */
	return aarch64_insn_patch_text_smc(addrs, &opcode, 1);
}
EXPORT_SYMBOL(aarch64_modify_read_text);

/*
 * Self-modify code for label of write address.
 */
int aarch64_modify_write_text(u32 opcode)
{
	void *addrs[1];

	addrs[0] = wr_tp;
	/*
	 * call aarch64_insn_patch_text to modify
	 * the opcode
	 */
	return aarch64_insn_patch_text_smc(addrs, &opcode, 1);
}
EXPORT_SYMBOL(aarch64_modify_write_text);

/*
 * return a address of read or write label
 */
u32 *get_read_insn_addr()
{
	/* TODO: rd_tp on each cpu */
	return rd_tp;
}
EXPORT_SYMBOL(get_read_insn_addr);

u32 *get_write_insn_addr()
{
	return wr_tp;
}
EXPORT_SYMBOL(get_write_insn_addr);

/*
 * read data from register
 * the opcode in rd_tp will be modified in ioctl.
 */
static noinline int rdmsr_safe_aarch64(u32 opt, u32 *data0, u32 *data1)
{
	/* reg is encoded by op0,op1,cn... */
	u32 err = 0;
	unsigned long __val = 0;
	unsigned long __pc_addr = 0;

	/* essential? */
	barrier();
	/*
	 * the "MIDR_EL:x
	 *  will be modified by aarch64_insn_read_addr
	 */
	raw_spin_lock(&msr_lock);
	atomic_add(1, &mrs_flags);
	asm volatile("mov %3, 0\n\t"
			"cmp %4, 0\n\t"
			"b.eq 1f\n\t"
			"mrs %0, MIDR_EL1\n\t"
			"b 1f\n\t"
			"mov %3, 1\n\t"
			"1:adr %1, .\n\t"
			"sub %1, %1, 12\n\t"
			"mov %2, %1\n\t"
			: "=r"(__val), "=r"(__pc_addr), "=r"(rd_tp), "=&r"(err)
			: "r"(opt));

	atomic_sub(1, &mrs_flags);
	raw_spin_unlock(&msr_lock);

	if ((data0 == NULL) && (data1 == NULL)) {
		/* init read or write address in somewhere */
		return 0;
	}

	*data0 = __val;
	*data1 = __val >> 32;
	if (err == 1) {
		/* undef instruction occurred */
		goto rd_error;
	}
	/* be successful */
	return 0;
rd_error:
	return -1;
}

/*
 * write data into register
 * the opcode below the wr_label label will be modified.
 */
static noinline int wrmsr_safe_aarch64(u32 opt, u32 data0, u32 data1)
{
	unsigned long __val = 0;
	unsigned long __pc_addr = 0;
	u64 data = 0;
	int err = 0;

	data = data1;
	data = (data << 32) | (data0);
	__val = data;
	barrier();
	raw_spin_lock(&msr_lock);
	atomic_add(1, &msr_flags);
	/* exec msr */
	asm volatile("mov %2, 0\n\t"
			"cmp %4, 0\n\t"
			"b.eq 1f\n\t"
			"msr TCR_EL1, %3\n\t"
			"b 1f\n\t"
			"mov %2, 1\n\t" /* exec when exception occurred */
			"1:adr %0, .\n\t"
			"sub %0, %0, 12\n\t"
			"mov %1, %0\n\t"
			: "=r"(__pc_addr), "=r"(wr_tp), "=&r"(err)
			: "rZ"(__val), "r"(opt));
	atomic_sub(1, &msr_flags);
	raw_spin_unlock(&msr_lock);
	if (err == 1) {
		/* undef instruction occurred */
		goto wr_error;
	}
	/* be successful */
	return 0;
wr_error:
	return -1;
}

/*
 * These "safe" variants are slower and should be used when the target MSR
 * may not actually exist.
 */
static void __rdmsr_safe_on_cpu_aarch64(void *info)
{
	struct msr_info_completion *rv = info;

	rv->msr.err = rdmsr_safe_aarch64(rv->msr.opt, &rv->msr.reg.l,
			&rv->msr.reg.h);
	complete(&rv->done);
}

static void __wrmsr_safe_on_cpu_aarch64(void *info)
{
	struct msr_info *rv = info;

	rv->err = wrmsr_safe_aarch64(rv->opt, rv->reg.l, rv->reg.h);
}

int rdmsr_safe_on_cpu_aarch64(unsigned int cpu, u32 opt, u32 *l, u32 *h)
{
	struct msr_info_completion rv;
	call_single_data_t csd = {
		.func	= __rdmsr_safe_on_cpu_aarch64,
		.info	= &rv,
	};
	int err;

	memset(&rv, 0, sizeof(rv));
	init_completion(&rv.done);
	rv.msr.opt = opt;

	err = smp_call_function_single_async(cpu, &csd);
	if (!err) {
		wait_for_completion(&rv.done);
		err = rv.msr.err;
	}

	if ((l != NULL) && (h != NULL)) {
		*l = rv.msr.reg.l;
		*h = rv.msr.reg.h;
	}

	return err;
}
EXPORT_SYMBOL(rdmsr_safe_on_cpu_aarch64);

int wrmsr_safe_on_cpu_aarch64(unsigned int cpu, u32 opt, u32 l, u32 h)
{
	int err;
	struct msr_info rv;

	memset(&rv, 0, sizeof(rv));

	rv.opt = opt;
	rv.reg.l = l;
	rv.reg.h = h;
	err = smp_call_function_single(cpu,
			__wrmsr_safe_on_cpu_aarch64, &rv, 1);

	return err ? err : rv.err;
}
EXPORT_SYMBOL(wrmsr_safe_on_cpu_aarch64);

/*
 * register a hook function for msr
 */
void register_undef_hook_el1(struct undef_hook *hook)
{
	void (*_register_undef_hook)(struct undef_hook *hook);
	_register_undef_hook =
		(void *)get_func_from_kallsyms("register_undef_hook");
	_register_undef_hook(hook);
}
EXPORT_SYMBOL(register_undef_hook_el1);

/*
 * unregister the hook function for msr
 */
void unregister_undef_hook_el1(struct undef_hook *hook)
{
	void (*_unregister_undef_hook)(struct undef_hook *hook);
	_unregister_undef_hook =
		(void *)get_func_from_kallsyms("unregister_undef_hook");
	_unregister_undef_hook(hook);
}
EXPORT_SYMBOL(unregister_undef_hook_el1);
/*
 *  * get the function from kallsyms
 *   */
unsigned long get_func_from_kallsyms(char *func)
{
	/* mainly for finding patch_text (/proc/kallsyms) */
	unsigned long addr;
	addr = kallsyms_lookup_name(func);
	return addr;
}

/*
 * ARMv8 ARM reserves the following encoding for system registers:
 * (Ref: ARMv8 ARM, Section: "System instruction class encoding overview",
 *  C5.2, version:ARM DDI 0487A.f)
 *	[20-19] : Op0
 *	[18-16] : Op1
 *	[15-12] : CRn
 *	[11-8]  : CRm
 *	[7-5]   : Op2
 *
 * make MSR/MRS instruction
 */

u32 aarch64_insn_mrs_gen(u32 op0, u32 op1, u32 crn, u32 crm, u32 op2, u32 rt)
{
	return (AARCH64_MRS_INSN | sys_reg(op0, op1, crn, crm, op2) | rt);
}
EXPORT_SYMBOL(aarch64_insn_mrs_gen);

u32 aarch64_insn_msr_gen(u32 op0, u32 op1, u32 crn, u32 crm, u32 op2, u32 rt)
{
	return (AARCH64_MSR_INSN | sys_reg(op0, op1, crn, crm, op2) | rt);
}
EXPORT_SYMBOL(aarch64_insn_msr_gen);

/*
 * make SYS instruction
 */
u32 aarch64_insn_other_gen(u32 op0, u32 op1, u32 crn, u32 crm, u32 op2, u32 rt)
{
	return (AARCH64_SYS_INSN | sys_reg(1, op1, crn, crm, op2) | rt);
}
EXPORT_SYMBOL(aarch64_insn_other_gen);

/*
 * check the regcode legal
 */
int aarch64_register_check(u32 reg)
{
	unsigned int op0 = 0, op1 = 0, cn = 0, cm = 0, op2 = 0;
	int ret;

	op0 = sys_reg_Op0(reg);
	op1 = sys_reg_Op1(reg);
	cn  = sys_reg_CRn(reg);
	cm  = sys_reg_CRm(reg);
	op2 = sys_reg_Op2(reg);

	if ((op0 <= MAX_OP0) && (op1 <= MAX_OP1)
		&& (op2 <= MAX_OP2) && (cn <= MAX_CN)
			&& (cm <= MAX_CM)) {
		/* legal regcode */
		ret = 0;
	} else {
		/* illegal regcode */
		ret = -EFAULT;
	}
	return ret;
}
EXPORT_SYMBOL(aarch64_register_check);
static int hookers_mrs(struct pt_regs *regs, u32 insn)
{
	/* judge whether to jump */
	if (atomic_read(&mrs_flags) &&
		(regs->pc == (u64)get_read_insn_addr())) {
		/* skip undef instruction and jump */
		regs->pc += 2*AARCH64_INSN_SIZE;
		pr_warn("MSR: undef exception!\n");

		return 0;
	} else {
		/* must be return 1 */
		return 1;
	}
}

static int hookers_msr(struct pt_regs *regs, u32 insn)
{
	/* judge whether to jump */
	if (atomic_read(&msr_flags) &&
		(regs->pc == (u64)get_write_insn_addr())) {
		/* skip undef instruction and jump */
		regs->pc += 2*AARCH64_INSN_SIZE;
		pr_warn("MSR: undef exception!\n");

		return 0;
	} else {
		/* must be return 1 */
		return 1;
	}
}

static struct undef_hook mrs_hook = {
	.instr_mask = 0xfff00000,
	.instr_val  = 0xd5300000,
	.pstate_mask = PSR_MODE_MASK_ALL,
	.pstate_val = PSR_MODE_ALL_EL,
	.fn = hookers_mrs,
};

static struct undef_hook msr_hook = {
	.instr_mask = 0xfff00000,
	.instr_val  = 0xd5100000,
	.pstate_mask = PSR_MODE_MASK_ALL,
	.pstate_val = PSR_MODE_ALL_EL,
	.fn = hookers_msr,
};

/*
 * In ARMv8-A, A64 instructions have a fixed length of 32 bits
 * and are always little-endian.
 */
int aarch64_insn_read_smc(void *addr, u32 *insnp)
{
	int ret;
	__le32 val;

	ret = probe_kernel_read(&val, addr, AARCH64_INSN_SIZE);
	if (!ret)
		*insnp = le32_to_cpu(val);

	return ret;
}


static ssize_t msr_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	u32 __user *tmp = (u32 __user *) buf;
	u32 data[2];
	u32 reg = *ppos;
	int cpu = iminor(file_inode(file));
	int err = 0;
	ssize_t bytes = 0;

	err = aarch64_register_check(reg);
	if (err != 0) {
		/* illegal register */
		return err;
	}

	if (count % 8)
		return -EINVAL;	/* Invalid chunk size */
	for (; count; count -= 8) {
		err = rdmsr_safe_on_cpu_aarch64(cpu, 1, &data[0], &data[1]);
		if (err)
			break;
		if (copy_to_user(tmp, &data, 8)) {
			printk(KERN_INFO"copy error\n");
			err = -EFAULT;
			break;
		}
		tmp += 2;
		bytes += 8;
	}

	return bytes ? bytes : err;
}

static ssize_t msr_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	const u32 __user *tmp = (const u32 __user *)buf;
	u32 data[2];
	u32 reg = *ppos;
	int cpu = iminor(file_inode(file));
	int err = 0;
	ssize_t bytes = 0;

	err = aarch64_register_check(reg);
	if (err != 0) {
		/* illegal register */
		return err;
	}

	if (count % 8)
		return -EINVAL;	/* Invalid chunk size */

	if (copy_from_user(&data, tmp, 8)) {
		err = -EFAULT;
		return err;
	}
	err = wrmsr_safe_on_cpu_aarch64(cpu, 1, data[0], data[1]);
	if (err)
		return err;
	bytes += 8;

	return bytes ? bytes : err;
}

/*
 * Before reading and writing register, modify the instruction on
 * corresponding address
 */
static long msr_ioctl(struct file *file, unsigned int ioc,
	unsigned long arg)
{
	u32 insnp = 0, insn = 0;
	u32 reg = arg;
	int err = 0, ret = 0;
	unsigned int op0, op1, cn, cm, op2;

	op0 = sys_reg_Op0(reg);
	op1 = sys_reg_Op1(reg);
	cn  = sys_reg_CRn(reg);
	cm  = sys_reg_CRm(reg);
	op2 = sys_reg_Op2(reg);
	err = aarch64_register_check(reg);
	if (err != 0) {
		/* illegal register */
		return err;
	}
	switch (ioc) {
	case 0x00:
		ret = aarch64_insn_read_smc((void *)get_read_insn_addr(),
				&insnp);
		insn = aarch64_insn_mrs_gen(op0, op1, cn, cm, op2,
				insnp & INSN_REG_MASK);
		err = aarch64_modify_read_text(insn);
		break;
	case 0x01:
		ret = aarch64_insn_read_smc((void *)get_write_insn_addr(),
				&insnp);
		insn = aarch64_insn_msr_gen(op0, op1, cn, cm, op2,
				insnp & INSN_REG_MASK);
		err = aarch64_modify_write_text(insn);
		break;
	default:
		err = -ENOTTY;
		break;
	}

	return err;
}

static int msr_open(struct inode *inode, struct file *file)
{
	unsigned int cpu = iminor(file_inode(file));
	/* TODO */
	struct cpuinfo_arm64 *c;

	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;

	if (cpu >= nr_cpu_ids || !cpu_online(cpu))
		return -ENXIO;	/* No such CPU */

	c = &per_cpu(cpu_data, cpu);
	return 0;
}

/*
 * File operations we support
 */
static const struct file_operations msr_fops = {
	.owner = THIS_MODULE,
	.llseek = no_seek_end_llseek,
	.read = msr_read,
	.write = msr_write,
	.open = msr_open,
	.unlocked_ioctl = msr_ioctl,
	.compat_ioctl = msr_ioctl,
};

static int msr_device_create(unsigned int cpu)
{
	struct device *dev;

	dev = device_create(msr_class, NULL, MKDEV(MSR_MAJOR, cpu), NULL,
			    "msr%d", cpu);
	return PTR_ERR_OR_ZERO(dev);
}

static int msr_device_destroy(unsigned int cpu)
{
	device_destroy(msr_class, MKDEV(MSR_MAJOR, cpu));
	return 0;
}

static char *msr_devnode(struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "cpu/%u/msr", MINOR(dev->devt));
}

int msr_init(void)
{
	int err;

	if (__register_chrdev(MSR_MAJOR, 0, NR_CPUS, "cpu/msr", &msr_fops)) {
		pr_err("unable to get major %d for msr\n", MSR_MAJOR);
		return -EBUSY;
	}
	msr_class = class_create(THIS_MODULE, "msr");
	if (IS_ERR(msr_class)) {
		err = PTR_ERR(msr_class);
		goto out_chrdev;
	}
	msr_class->devnode = msr_devnode;

	/* TODO: set callback function for hotplug */
	err  = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "arm/msr:online",
				 msr_device_create, msr_device_destroy);
	if (err < 0)
		goto out_class;
	cpuhp_msr_state = err;

	/*
	 * register two hooks to block undef instruction exception
	 * in EL1
	 */
	register_undef_hook_el1(&mrs_hook);
	register_undef_hook_el1(&msr_hook);

	err = rdmsr_safe_on_cpu_aarch64(0, 0, NULL, NULL);
	err = wrmsr_safe_on_cpu_aarch64(0, 0, 0, 0);
	return 0;

out_class:
	class_destroy(msr_class);
out_chrdev:
	__unregister_chrdev(MSR_MAJOR, 0, NR_CPUS, "cpu/msr");
	return err;
}

void msr_exit(void)
{
	unregister_undef_hook_el1(&mrs_hook);
	unregister_undef_hook_el1(&msr_hook);
	cpuhp_remove_state(cpuhp_msr_state);
	class_destroy(msr_class);
	__unregister_chrdev(MSR_MAJOR, 0, NR_CPUS, "cpu/msr");
}
