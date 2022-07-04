#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/crc32.h>
#include <linux/module.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

int kvm_unit_ioctl_func(unsigned int cmd, unsigned long addr, struct ioctl_data *data)
{
	switch (data->cmdcode) {
		default:
			break;
	}
	return 0;
}

int kvm_unit_init(void)
{
	return 0;
}

int kvm_unit_exit(void)
{
	return 0;
}

