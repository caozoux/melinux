#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/errno.h>
#include <linux/string.h>

unsigned long symbol_addr = 0;

static int symbol_walk_callback(void *data, const char *name, struct module *mod, unsigned long addr){
    if (mod != 0) {
        return 0;
    }

    if (strcmp(name, "calc_load_tasks") == 0) {
        if (symbol_addr != 0) {
            printk(KERN_INFO "Found two symbols in the kernel, unable to continue\n");
            return -EFAULT;
        }
        symbol_addr = addr;
    }

    return 0;
}

static int __init load5s_init(void){
    int ret = kallsyms_on_each_symbol(symbol_walk_callback, NULL);
    if (ret){
        return ret;
    }
    printk(KERN_INFO "start..................... \n");
    printk("<<<%p>>>\n", (atomic_long_t *)symbol_addr);

    if (symbol_addr == 0) {
        printk(KERN_INFO "Unable to find symbol addr.\n");
        return -EFAULT;
    }

    return 0;
}
 
static void __exit load5s_exit(void){
    printk(KERN_INFO "exit...................... \n");
    symbol_addr = 0;
}

module_init(load5s_init);
module_exit(load5s_exit);
MODULE_LICENSE("GPL");
