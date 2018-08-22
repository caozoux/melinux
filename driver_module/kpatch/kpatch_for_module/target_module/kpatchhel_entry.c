#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");
static int hello_init(void)
{
    printk(KERN_INFO "Hello,world");
    return 0;
}

static int hello_export(void)
{
	printk(KERN_INFO "Hello from another module");
   	return 0;
}

static void hello_exit(void)
{
	printk(KERN_INFO "Goodbye!");
}

EXPORT_SYMBOL(hello_export);
module_init(hello_init);
module_exit(hello_exit);
