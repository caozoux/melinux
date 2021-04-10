#ifdef CONFIG_MODULES
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#define LOC_PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define LOC_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

void crc32_test(void *buf, unsigned long size)
{
	unsigned long ret = crc32_le(0,buf, size);
}
