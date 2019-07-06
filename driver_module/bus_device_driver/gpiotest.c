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
#include <linux/kallsyms.h>
#include "base.h"

#define to_device_private_bus(obj)  \
	container_of(obj, struct device_private, knode_bus)

static char * bus_name;
struct bus_type *orig_bus_type;

static char * device_name_load;
static char * driver_name_load;
//module_param(bus_name, charp, 0444);
module_param(bus_name, charp, S_IRUGO);
MODULE_PARM_DESC(bus_name, "specif load the diver for specify driver_name_load");
module_param(device_name_load, charp, S_IRUGO);
MODULE_PARM_DESC(driver_name_load, "force to load driver");

static struct device_driver *next_driver(struct klist_iter *i)
{
    struct klist_node *n = klist_next(i);
    struct driver_private *drv_priv;

    if (n) {
        drv_priv = container_of(n, struct driver_private, knode_bus);
        return drv_priv->driver;
    }    
    return NULL;
}

static struct device *next_device(struct klist_iter *i)
{
	struct klist_node *n = klist_next(i);
	struct device *dev = NULL;
	struct device_private *dev_prv;
	if (n) {
		 dev_prv = to_device_private_bus(n);
		 dev = dev_prv->device;
	}
	return dev;
}

static int scan_bus_for_each_drv(struct bus_type *bus, struct device *dev, void *data)
{
	struct klist_iter i;
	struct device_driver *drv;
	int error = 0;

	if (!bus)
		return -EINVAL;

	klist_iter_init_node(&bus->p->klist_drivers, &i, NULL);
	while ((drv = next_driver(&i)) && !error) {
		
	   if (!strcmp(driver_name_load, drv->name? drv->name :"NULL")) { 
			driver_probe_device(drv, dev);
			break;
		}
	}

	klist_iter_exit(&i);
	return error;
}

static int bus_scan_fn(struct device *dev, void *data)
{
   printk("bus scan device:%s\n", dev_name(dev));
   if (driver_name_load && device_name_load) {
	   if (!strcmp(device_name_load, dev_name(dev))) { 
		   scan_bus_for_each_drv(dev->bus,  dev, NULL);
	   }
   }
   return 0;
}
static void bus_scan_device(struct bus_type *bus)
{

    struct klist_iter i;
	struct device *dev;
	int error = 0;

	if (!bus || !bus->p) {
		printk("bus dat is invalid \n");
		return -EINVAL;
	}

	klist_iter_init_node(&bus->p->klist_devices, &i,NULL);
	
	while ((dev = next_device(&i)) && !error)
		error = bus_scan_fn(dev, NULL); 
	klist_iter_exit(&i); 
	return error;
}


static int symbol_int()
{
	if (!bus_name) {
		printk("bus_name param is NULL\n" );
		goto err1;	
	}
	orig_bus_type = kallsyms_lookup_name(bus_name);
	if (!orig_bus_type) {
		printk("find %s failed\n", bus_name);
		goto err1;	
	}
	
	return 0;

err1:
    return 1;
}

static int __init gpiodriver_init(void)
{
	printk("gpiodriver load \n");
	if (symbol_int()) {
		return -EINVAL;
	}

	if (orig_bus_type)
		bus_scan_device(orig_bus_type);
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

