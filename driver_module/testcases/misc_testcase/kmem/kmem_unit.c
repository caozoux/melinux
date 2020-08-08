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

#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"

static mem_proc_show(void)
{
	struct sysinfo i;
	si_meminfo(&i);
	si_swapinfo(&i);

	printk("MemTotal:       %ld", i.totalram);
	printk("MemFree:        %ld", i.freeram);
	printk("Buffers:        %ld", i.bufferram);
	printk("SwapTotal:      %ld", i.totalswap);
	printk("SwapFree:       %ld", i.freeswap);
	printk("Shmem:          %ld", i.sharedram);

}

int kmem_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_data *data)
{
	int ret = -1;
	switch (data->cmdcode) {
		case  IOCTL_USEMEM_SHOW:
			DEBUG("mem_readlock_test_start\n")
			break;
		case  IOCTL_USERCU_READTEST_END:
			DEBUG("mem_readlock_test_stop\n")
			//mem_readlock_test_stop();
			break;
		default:
			goto OUT;
	}
OUT:
	return ret;
}

int kmemt_unit_init(void)
{
	return 0;
}

int kmem_unit_exit(void)
{

}
