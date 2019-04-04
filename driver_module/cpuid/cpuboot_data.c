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
#include <asm/processor.h>

#define SMT_LEVEL   0
#define SMT_TYPE    1
#define LEAFB_SUBTYPE(ecx)      (((ecx) >> 8) & 0xff)
#define INVALID_TYPE    0
#define CORE_TYPE   2

#define LEVEL_MAX_SIBLINGS(ebx)     ((ebx) & 0xffff)

int cpuboot_data(void)
{
    unsigned int eax, ebx, ecx, edx, sub_index;
    unsigned int core_level_siblings, smp_num_siblings, x86_max_cores;

    ecx = cpuid_ecx(0x80000008);
    x86_max_cores = (ecx & 0xff) + 1;
    printk("x86_max_cores:%d\n", x86_max_cores);
    cpuid(1, &eax, &ebx, &ecx, &edx);

    smp_num_siblings = (ebx & 0xff0000) >> 16;
    printk("smp_num_siblings:%d\n", smp_num_siblings);

    cpuid_count(0xb, SMT_LEVEL, &eax, &ebx, &ecx, &edx);
    if (ebx == 0 || (LEAFB_SUBTYPE(ecx) != SMT_TYPE)) {
        printk(" cpuid find failed\n");
        return 0;
    }

    core_level_siblings = LEVEL_MAX_SIBLINGS(ebx);
    printk("core_level_siblings %d\n", core_level_siblings);
    sub_index = 1;
    do {
        cpuid_count(0xb, sub_index, &eax, &ebx, &ecx, &edx);
        printk("ebx:%x\n", ebx);
        if (LEAFB_SUBTYPE(ecx) == CORE_TYPE) {
            printk("core_level_siblings %d\n", core_level_siblings);
            core_level_siblings = LEVEL_MAX_SIBLINGS(ebx);
        }
    } while (LEAFB_SUBTYPE(ecx) != INVALID_TYPE);
    printk("gpiodriver load \n");
    return 0;
}


