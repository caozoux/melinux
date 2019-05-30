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

raw_spinlock_t      lock;

static void dead_irq_lock()
{
	unsigned long flags;
	raw_spin_lock_irqsave(&lock, flags);
	raw_spin_lock_irqsave(&lock, flags);
	//local_irq_disable();
	raw_spin_unlock_irqrestore(&lock, flags);
}

static void dead_lock()
{
	raw_spin_lock(&lock);
	raw_spin_lock(&lock);
	//local_irq_disable();
	raw_spin_unlock(&lock);

}

static int __init gpiodriver_init(void)
{
	printk("gpiodriver load \n");
	raw_spin_lock_init(&lock);
	dead_lock();
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
