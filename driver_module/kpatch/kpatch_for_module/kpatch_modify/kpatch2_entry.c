#include <linux/init.h>

#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

extern int hello_export(void);
static int usehello_init(void)
{
       printk(KERN_INFO "Hello usehello");
       hello_export();
       return 0;
}

static void usehello_exit(void)
{
       printk(KERN_INFO "Goodbye usehello");
}
module_init(usehello_init);
module_exit(usehello_exit);
