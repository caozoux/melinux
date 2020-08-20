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
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/irq.h>
#include <linux/list.h>

#ifdef CONFIG_ARM64
#include <linux/irqchip/arm-gic-common.h>
#include <linux/irqchip/arm-gic-v3.h>
#endif
#include "../template_iocmd.h"
#include "../misc_ioctl.h"
#include "../debug_ctrl.h"
#include "gic_local_data.h"
#include "mekernel.h"

static int gic_its_interrupt_mis_irq_dump(int irq)
{
	struct irq_desc *desc;
	struct irq_data *data;
	struct irq_chip *chip;
	int err = -ENODEV;

	desc = irq_to_desc(irq);
	if (!desc)
		return err;

	data = irq_desc_get_irq_data(desc);

	do {
		chip = irq_data_get_irq_chip(data);
		if (!chip)
			return -ENODEV;

		if (chip->irq_set_irqchip_state)
			break;

#ifdef CONFIG_IRQ_DOMAIN_HIERARCHY
		data = data->parent_data;
#else
		data = NULL;
#endif
	} while (data);

	if (data)
		err = chip->irq_set_irqchip_state(data, IRQCHIP_STATE_PENDING, 1);

	return err;
}
