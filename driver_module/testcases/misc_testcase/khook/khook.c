#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/page_idle.h>
#include <linux/version.h>
#include <linux/mmu_notifier.h>
//#include <asm/tlb.h>

#include "../template_iocmd.h"
#include "../block/blocklocal.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "mekernel.h"


