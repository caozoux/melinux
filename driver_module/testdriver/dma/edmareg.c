#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/freezer.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/edma.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/platform_data/edma.h>

/* Offsets matching "struct edmacc_param" */
#define PARM_OPT		0x00
#define PARM_SRC		0x04
#define PARM_A_B_CNT		0x08
#define PARM_DST		0x0c
#define PARM_SRC_DST_BIDX	0x10
#define PARM_LINK_BCNTRLD	0x14
#define PARM_SRC_DST_CIDX	0x18
#define PARM_CCNT		0x1c

#define PARM_SIZE		0x20

/* Offsets for EDMA CC global channel registers and their shadows */
#define SH_ER		0x00	/* 64 bits */
#define SH_ECR		0x08	/* 64 bits */
#define SH_ESR		0x10	/* 64 bits */
#define SH_CER		0x18	/* 64 bits */
#define SH_EER		0x20	/* 64 bits */
#define SH_EECR		0x28	/* 64 bits */
#define SH_EESR		0x30	/* 64 bits */
#define SH_SER		0x38	/* 64 bits */
#define SH_SECR		0x40	/* 64 bits */
#define SH_IER		0x50	/* 64 bits */
#define SH_IECR		0x58	/* 64 bits */
#define SH_IESR		0x60	/* 64 bits */
#define SH_IPR		0x68	/* 64 bits */
#define SH_ICR		0x70	/* 64 bits */
#define SH_IEVAL	0x78
#define SH_QER		0x80
#define SH_QEER		0x84
#define SH_QEECR	0x88
#define SH_QEESR	0x8c
#define SH_QSER		0x90
#define SH_QSECR	0x94
#define SH_SIZE		0x200

/* Offsets for EDMA CC global registers */
#define EDMA_REV	0x0000
#define EDMA_CCCFG	0x0004
#define EDMA_QCHMAP	0x0200	/* 8 registers */
#define EDMA_DMAQNUM	0x0240	/* 8 registers (4 on OMAP-L1xx) */
#define EDMA_QDMAQNUM	0x0260
#define EDMA_QUETCMAP	0x0280
#define EDMA_QUEPRI	0x0284
#define EDMA_EMR	0x0300	/* 64 bits */
#define EDMA_EMCR	0x0308	/* 64 bits */
#define EDMA_QEMR	0x0310
#define EDMA_QEMCR	0x0314
#define EDMA_CCERR	0x0318
#define EDMA_CCERRCLR	0x031c
#define EDMA_EEVAL	0x0320
#define EDMA_DRAE	0x0340	/* 4 x 64 bits*/
#define EDMA_QRAE	0x0380	/* 4 registers */
#define EDMA_QUEEVTENTRY	0x0400	/* 2 x 16 registers */
#define EDMA_QSTAT	0x0600	/* 2 registers */
#define EDMA_QWMTHRA	0x0620
#define EDMA_QWMTHRB	0x0624
#define EDMA_CCSTAT	0x0640

#define EDMA_M		0x1000	/* global channel registers */
#define EDMA_ECR	0x1008
#define EDMA_ECRH	0x100C
#define EDMA_SHADOW0	0x2000	/* 4 regions shadowing global channels */
#define EDMA_PARM	0x4000	/* 128 param entries */

#define PARM_OFFSET(param_no)	(EDMA_PARM + ((param_no) << 5))

#define EDMA_DCHMAP	0x0100  /* 64 registers */

/* CCCFG register */
#define GET_NUM_DMACH(x)	(x & 0x7) /* bits 0-2 */
#define GET_NUM_PAENTRY(x)	((x & 0x7000) >> 12) /* bits 12-14 */
#define GET_NUM_EVQUE(x)	((x & 0x70000) >> 16) /* bits 16-18 */
#define GET_NUM_REGN(x)		((x & 0x300000) >> 20) /* bits 20-21 */
#define CHMAP_EXIST		BIT(24)

#define EDMA_MAX_DMACH           64
#define EDMA_MAX_PARAMENTRY     512

/*****************************************************************************/

static void __iomem *edmacc_regs_base[EDMA_MAX_CC];

static inline unsigned int edma_read(unsigned ctlr, int offset)
{
	return (unsigned int)__raw_readl(edmacc_regs_base[ctlr] + offset);
}

static inline void edma_write(unsigned ctlr, int offset, int val)
{
	__raw_writel(val, edmacc_regs_base[ctlr] + offset);
}
static inline void edma_modify(unsigned ctlr, int offset, unsigned and,
		unsigned or)
{
	unsigned val = edma_read(ctlr, offset);
	val &= and;
	val |= or;
	edma_write(ctlr, offset, val);
}
static inline void edma_and(unsigned ctlr, int offset, unsigned and)
{
	unsigned val = edma_read(ctlr, offset);
	val &= and;
	edma_write(ctlr, offset, val);
}
static inline void edma_or(unsigned ctlr, int offset, unsigned or)
{
	unsigned val = edma_read(ctlr, offset);
	val |= or;
	edma_write(ctlr, offset, val);
}
static inline unsigned int edma_read_array(unsigned ctlr, int offset, int i)
{
	return edma_read(ctlr, offset + (i << 2));
}
static inline void edma_write_array(unsigned ctlr, int offset, int i,
		unsigned val)
{
	edma_write(ctlr, offset + (i << 2), val);
}
static inline void edma_modify_array(unsigned ctlr, int offset, int i,
		unsigned and, unsigned or)
{
	edma_modify(ctlr, offset + (i << 2), and, or);
}
static inline void edma_or_array(unsigned ctlr, int offset, int i, unsigned or)
{
	edma_or(ctlr, offset + (i << 2), or);
}
static inline void edma_or_array2(unsigned ctlr, int offset, int i, int j,
		unsigned or)
{
	edma_or(ctlr, offset + ((i*2 + j) << 2), or);
}
static inline void edma_write_array2(unsigned ctlr, int offset, int i, int j,
		unsigned val)
{
	edma_write(ctlr, offset + ((i*2 + j) << 2), val);
}
static inline unsigned int edma_shadow0_read(unsigned ctlr, int offset)
{
	return edma_read(ctlr, EDMA_SHADOW0 + offset);
}
static inline unsigned int edma_shadow0_read_array(unsigned ctlr, int offset,
		int i)
{
	return edma_read(ctlr, EDMA_SHADOW0 + offset + (i << 2));
}
static inline void edma_shadow0_write(unsigned ctlr, int offset, unsigned val)
{
	edma_write(ctlr, EDMA_SHADOW0 + offset, val);
}
static inline void edma_shadow0_write_array(unsigned ctlr, int offset, int i,
		unsigned val)
{
	edma_write(ctlr, EDMA_SHADOW0 + offset + (i << 2), val);
}
static inline unsigned int edma_parm_read(unsigned ctlr, int offset,
		int param_no)
{
	return edma_read(ctlr, EDMA_PARM + offset + (param_no << 5));
}
static inline void edma_parm_write(unsigned ctlr, int offset, int param_no,
		unsigned val)
{
	edma_write(ctlr, EDMA_PARM + offset + (param_no << 5), val);
}
static inline void edma_parm_modify(unsigned ctlr, int offset, int param_no,
		unsigned and, unsigned or)
{
	edma_modify(ctlr, EDMA_PARM + offset + (param_no << 5), and, or);
}
static inline void edma_parm_and(unsigned ctlr, int offset, int param_no,
		unsigned and)
{
	edma_and(ctlr, EDMA_PARM + offset + (param_no << 5), and);
}
static inline void edma_parm_or(unsigned ctlr, int offset, int param_no,
		unsigned or)
{
	edma_or(ctlr, EDMA_PARM + offset + (param_no << 5), or);
}

static inline void set_bits(int offset, int len, unsigned long *p)
{
	for (; len > 0; len--)
		set_bit(offset + (len - 1), p);
}

static inline void clear_bits(int offset, int len, unsigned long *p)
{
	for (; len > 0; len--)
		clear_bit(offset + (len - 1), p);
}

void edmreg_debug(struct edm_debug_data *data)
{
	
	printk("zz %s\n", __func__);
}

static struct edm_debug_fun_d edmreg_data = {
	.edm_debug = edmreg_debug,
};

static int __init edmareg_init(void)
{

	/* 
	ctlr = EDMA_CTLR(channel);
	channel = EDMA_CHAN_SLOT(channel);
	pr_info("EDMA: ER%d %08x\n", j,
		edma_shadow0_read_array(ctlr, SH_ER, j));
	
	// Clear any pending event or error 
	edma_write_array(ctlr, EDMA_ECR, j, mask);
	edma_write_array(ctlr, EDMA_EMCR, j, mask);
	// Clear any SER 
	edma_shadow0_write_array(ctlr, SH_SECR, j, mask);
	edma_shadow0_write_array(ctlr, SH_EESR, j, mask);
	pr_info("EDMA: EER%d %08x\n", j,
		edma_shadow0_read_array(ctlr, SH_EER, j));
	*/
	edma_set_debug(&edmreg_data);

	return 0;
}

/* when compiled-in wait for drivers to load first */
late_initcall(edmareg_init);

static void __exit edmareg_exit(void)
{
	edma_free_debug();
}

module_exit(edmareg_exit);
