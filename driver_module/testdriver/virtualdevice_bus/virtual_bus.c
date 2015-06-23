#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/irqdomain.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/msi.h>
//#include "arch/arm/plat-armada/linux_oss/mvOs.h"
//#include "mvOs.h"
#include "armadaxp.h"
#include "virtual_platform.h"
#include "virtual_bus.h"

#define MV_MBUS_REGS_OFFSET         (0x20000) 
#define MV_CPUIF_SHARED_REGS_BASE       (MV_MBUS_REGS_OFFSET)
#define CPU_INT_SOURCE_CONTROL_REG(i)       (MV_CPUIF_SHARED_REGS_BASE + 0xB00 + (i * 0x4))
int virme_bus_match(struct device *dev, struct device_driver *drv)
{
	return 0;
}

int virme_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return 0;
}

int virme_bus_probe(struct device *dev)
{
	return 0;
}
int virme_bus_remove(struct device *dev)
{
	return 0;
}

void virme_bus_shutdown(struct device *dev)
{
}

int virme_bus_online(struct device *dev)
{
	return 0;
}

int virme_bus_offline(struct device *dev)
{
	return 0;
}

int virme_bus_suspend(struct device *dev, pm_message_t state)
{
	return 0;
}

int virme_bus_resume(struct device *dev)
{
	return 0;
}

struct bus_type virme_bus_type = {
	.name    = "virme_bus",
	.match   = virme_bus_match,
	.uevent = virme_bus_uevent,
	//.probe   = virme_bus_probe,
	//.remove  = virme_bus_remove,
	//.shutdown = virme_bus_shutdown,
	.suspend = virme_bus_suspend,
	.resume  = virme_bus_resume,
	//.dev_attrs = virme_dev_attrs,
};

#define SHARE_REG_DOORBELL1 (AXP_BRIDGE_VIRT_BASE | 0x400)
#define SHARE_REG_DOORBELL2 (AXP_BRIDGE_VIRT_BASE | 0x404)
#define SHARE_REG_DOORBELL3 (AXP_BRIDGE_VIRT_BASE | 0x408)
irqreturn_t test_irq_handler(int irq, void *data)
{
	printk("zz %s S_DB_1:%08x\n", __func__, self_read(SHARE_REG_DOORBELL1));
	printk("zz %s S_DB_2:%08x\n", __func__, self_read(SHARE_REG_DOORBELL2));
	printk("zz %s S_DB_3:%08x\n", __func__, self_read(SHARE_REG_DOORBELL3));
}

static int irq_r[16];
#define IRQ_MAX 3
#define SHARE_IRQ_DOORBELL1 96
#define SHARE_IRQ_DOORBELL2 97
#define SHARE_IRQ_DOORBELL3 98
void marvell_test_entry(void)
{
	int irq;
	int ret;
	int i =0;
	printk("zz %s +inb doorbell:%08x \n", __func__, self_read(0xf1020a04));
	printk("zz %s S_DB_1:%08x\n", __func__, self_read(SHARE_REG_DOORBELL1));
	printk("zz %s S_DB_2:%08x\n", __func__, self_read(SHARE_REG_DOORBELL2));
	printk("zz %s S_DB_3:%08x\n", __func__, self_read(SHARE_REG_DOORBELL3));
#if 0
	for (i=0; i < IRQ_MAX;i++)
	{
		irq = self_arch_setup_irq();
		printk("zz %s irq:%d\n", __func__, irq);
		ret = request_irq(irq, test_irq_handler, NULL, "test_dev", NULL);
		if (ret) {
			printk("request irq failed\n");
		} else {
			irq_r[i] = irq;
		}
	}
#endif

	/*
	 request_irq(SHARE_IRQ_DOORBELL1, test_irq_handler, NULL, "test_dev", NULL);
	 request_irq(SHARE_IRQ_DOORBELL2, test_irq_handler, NULL, "test_dev", NULL);
	 request_irq(SHARE_IRQ_DOORBELL3, test_irq_handler, NULL, "test_dev", NULL);
	 */
}

void marvell_test_exit(void)
{
	int irq;
	int ret;
	int i =0;
	/*
	for (i=0; i < IRQ_MAX;i++)
	{
		 free_irq(irq_r[i], NULL);
		 self_free_irq(irq_r[i]);
	}*/

	/*
	 free_irq(SHARE_IRQ_DOORBELL1, NULL);
	 free_irq(SHARE_IRQ_DOORBELL2, NULL);
	 free_irq(SHARE_IRQ_DOORBELL3, NULL);
	 */

}

static ssize_t test_dev_write(struct device *dev,
									  struct device_attribute *attr,
									  const char *buf, size_t count)
{
	//int data = 0xf1f ;
	int data = 0x118 ;
	int doorbell = 1;
	data |= doorbell << 5;
	data = 0x0101;
	printk("zz %s +inb doorbell:%08x data:%08x\n", __func__, self_read(0xf1020a04), data);
	self_write(0xf1020a04, data );
	printk("zz %s -inb doorbell:%08x\n", __func__, self_read(0xf1020a04));
	return count; 
} 

static ssize_t test_dev_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	printk("zz %s\n", __func__);
	return 0;
}

static DEVICE_ATTR(dev_test, S_IWUSR | S_IRUGO, test_dev_read, test_dev_write);
struct virme_bus_data virme_bus_d;
static struct platform_device *test_dev;
int test_dev_init(void) 
{
	int res;

	test_dev = platform_device_alloc("test_dev",0);
	res = platform_device_add(test_dev);
	if (res) {
		printk("register fail \n");
		return 0;
	}

	res = device_create_file(&test_dev->dev, &dev_attr_dev_test);
	return 0;
}

int test_dev_exit(void)
{
	if (test_dev) {
		device_remove_file(&test_dev->dev, &dev_attr_dev_test);
		platform_device_del(test_dev);
		platform_device_put(test_dev);
	}
	return 0;
}

static int virme_bus_driver_init(void)
{
	int res;
    res = bus_register(&virme_bus_type);

	if (res) {
		printk("bus register failed\n");
		return -1;
	}
	printk("register bus %s \n", virme_bus_type.name);
	return 0;
}

static int __init vir_init(void)
{
	printk("zz %s \n", __func__);
	virme_bus_driver_init();
	test_dev_init();
	marvell_test_entry();
	return 0;
}

static void __exit vir_exit(void)
{
	printk("zz %s \n", __func__);
	test_dev_exit();
    bus_unregister(&virme_bus_type);
 	marvell_test_exit();
}

module_init(vir_init);
module_exit(vir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
