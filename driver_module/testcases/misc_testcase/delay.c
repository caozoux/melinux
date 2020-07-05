#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"

#define CPU_FREQ (2600000000)

void me_udelay(unsigned long var)
{
        var = var*((CPU_FREQ)/1000000);
        var = var*((CPU_FREQ)/2000);
#ifdef CONFIG_ARM64
        asm (   "mov x1, %0\n"
                "loop2: sub x1, x1, #1\n"
                "mov   x2, x1\n"
                "cmp   x1, #0x0\n"
                "b.ne  loop2\n"
                : "=r" (var)
        );
#else
	asm volatile(
		"	test %0,%0	\n"
		"	jz 3f		\n"
		"	jmp 1f		\n"

		".align 16		\n"
		"1:	jmp 2f		\n"

		".align 16		\n"
		"2:	dec %0		\n"
		"	jnz 2b		\n"
		"3:	dec %0		\n"

		: /* we don't need output */
		:"a" (var)
	);
#endif
}

void me_mdelay(unsigned long var)
{
        var = var*((CPU_FREQ)/1000);
#ifdef CONFIG_ARM64
        asm (   "mov x1, %0\n"
                "loop1: sub x1, x1, #1\n"
                "mov   x2, x1\n"
                "cmp   x1, #0x0\n"
                "b.ne  loop1\n"
                : "=r" (var)
        );
#else
	asm volatile(
		"	test %0,%0	\n"
		"	jz 3f		\n"
		"	jmp 1f		\n"

		".align 16		\n"
		"1:	jmp 2f		\n"

		".align 16		\n"
		"2:	dec %0		\n"
		"	jnz 2b		\n"
		"3:	dec %0		\n"

		: /* we don't need output */
		:"a" (var)
	);
#endif
}

