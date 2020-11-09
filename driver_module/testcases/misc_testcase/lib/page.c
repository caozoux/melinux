#ifdef CONFIG_MODULES
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#define LOC_PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define LOC_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

#define NSTABLE (_UL(1)<<63)
#define APTABLE (_UL(3)<<61)
#define UXNTABLE (_UL(1)<<60)
#define PXNTABLE (_UL(1)<<59)
#define UXN (_UL(1)<<54)
#define PXN (_UL(1)<<53)
#define CT (_UL(1)<<52)
#define DMB (_UL(1)<<51)
#define GP (_UL(1)<<50)
#define NT (_UL(1)<<16)
#define NG (_UL(1)<<11)
#define AF (_UL(1)<<10)
#define SH (_UL(3)<<8)
#define AP (_UL(3)<<6)
#define NS (_UL(1)<<5)
#define ATTR (_UL(7)<<2)

#define PTE_BIT_ENABLE(pte,val)  ((int)((pte&val) ? 1 : 0))
#define PTE_MBIT_ENABLE(pte, val, offset)  ((int)((pte & val)>>offset))

void dump_pte_info(unsigned long pte)
{
	LOC_PRINT("pte %lx NS:%d APT:%d UXNT:%d PXNT:%d PXNT%d UXN:%d PXN:%d CT:%d DMB:%d GP:%d NT:%d NG:%d AF:%d SH:%d AP:%d NS:%d ATTR:%d\n",
			pte,
			PTE_BIT_ENABLE(pte, NSTABLE),
			PTE_MBIT_ENABLE(pte, APTABLE,61),
			PTE_BIT_ENABLE(pte, UXNTABLE),
			PTE_BIT_ENABLE(pte, PXNTABLE),
			PTE_BIT_ENABLE(pte, UXN),
			PTE_BIT_ENABLE(pte, PXN),
			PTE_BIT_ENABLE(pte, CT),
			PTE_BIT_ENABLE(pte, DMB),
			PTE_BIT_ENABLE(pte, GP),
			PTE_BIT_ENABLE(pte, NT),
			PTE_BIT_ENABLE(pte, NG),
			PTE_BIT_ENABLE(pte, AF),
			PTE_MBIT_ENABLE(pte, SH, 8),
			PTE_MBIT_ENABLE(pte, AP, 6),
			PTE_BIT_ENABLE(pte, AP),
			PTE_BIT_ENABLE(pte, NS),
			PTE_MBIT_ENABLE(pte, ATTR, 2)
	);
}

