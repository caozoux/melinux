#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <linux/slub_def.h>
#include <linux/slab.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <asm/tlbflush.h>
#include <linux/syscore_ops.h>


#define MODULE_CACHE_NAME "dkmalloc-32"
//#define SLUB_KMALLOC_DEBUG (SLAB_CONSISTENCY_CHECKS | SLAB_RED_ZONE |SLAB_POISON |SLAB_STORE_USER | SLAB_TRACE | SLAB_FAILSLAB)
#define SLUB_KMALLOC_DEBUG (SLAB_CONSISTENCY_CHECKS | SLAB_RED_ZONE |SLAB_POISON |SLAB_STORE_USER | SLAB_FAILSLAB)

#define SLAB_DEBUG_FLAGS (SLAB_RED_ZONE | SLAB_POISON | SLAB_STORE_USER | \
		SLAB_TRACE | SLAB_CONSISTENCY_CHECKS)

void (*orig_flush_tlb_kernel_range)(unsigned long start, unsigned long end);
struct kmem_cache *new_kmalloc_32;
struct kmem_cache *old_kmalloc_32;

struct mm_struct *orig_init_mm;


int pmd_huge(pmd_t pmd)
{
	return !pmd_none(pmd) &&
		(pmd_val(pmd) & (_PAGE_PRESENT|_PAGE_PSE)) != _PAGE_PRESENT;
}

//UAF
void test_code_1(void)
{
	char *data1,*data2;
	data1 = (char *)kmalloc(24, GFP_KERNEL);
	if (!data1)
		return;
	kfree(data1);
	data1[0] = 17;
	data2 = (char *)kmalloc(24, GFP_KERNEL);
	if (!data2) {
		kfree(data2);
		return;
	}
	kfree(data2);
}

//overlay 
void test_code_2(void)
{
	char *data1,*data2;
	data1 = (char *)kmalloc(24, GFP_KERNEL);
	if (!data1)
		return;
	data2 = (char *)kmalloc(24, GFP_KERNEL);
	if (!data2) {
		kfree(data2);
		return;
	}
	data1[32] = 17;
	kfree(data1);
	kfree(data2);
}

static int replace_kmalloc32(unsigned long value)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pmd_t pmd_val;
	unsigned long address;

	address = (unsigned long)kmalloc_caches& ~(PAGE_SIZE-1);

    pgd = pgd_offset(orig_init_mm, (address));
	if (pgd_none(*pgd))
		return -1;

	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d))
		return -1;

	pud = pud_offset(p4d, address);
	if (pud_none(*pud))
		return -1;

	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
		return -1;

	if (pmd_huge(*pmd)) {
		pmd_val = *pmd;
		pmd_val = pmd_mkwrite(pmd_val);
		set_pmd_at(orig_init_mm, address, pmd, pmd_val);
		orig_flush_tlb_kernel_range(address, address + PAGE_SIZE);

		kmalloc_caches[KMALLOC_NORMAL][5] = (void*)value;

		pmd_val = pmd_wrprotect(pmd_val);
		set_pmd_at(orig_init_mm, address, pmd, pmd_val);
		orig_flush_tlb_kernel_range(address, address + PAGE_SIZE);
		return 0;
	}

	return -1;
}

static int __init slubufadriver_init(void)
{

	orig_init_mm = (void*) kallsyms_lookup_name("init_mm");
	if (!orig_init_mm)
		return -EINVAL;

	orig_flush_tlb_kernel_range = (void*) kallsyms_lookup_name("flush_tlb_kernel_range");
	if (!orig_flush_tlb_kernel_range)
		return -EINVAL;

	old_kmalloc_32 = kmalloc_caches[KMALLOC_NORMAL][5];
	if (!old_kmalloc_32)
		return -EINVAL;

#if 0
    new_kmalloc_32 = kmem_cache_create_usercopy(MODULE_CACHE_NAME,
						  32,
						  0,
                          SLAB_HWCACHE_ALIGN | SLAB_DEBUG_FLAGS,
                          0, 32,	 
                          NULL);
#else
	new_kmalloc_32 = kmem_cache_create(MODULE_CACHE_NAME, 32, SLAB_HWCACHE_ALIGN, SLUB_KMALLOC_DEBUG, NULL);
#endif

	if (!new_kmalloc_32)
		return -EINVAL;

	if (new_kmalloc_32->refcount++ != 1) {
		printk("Err: slub alloc debug kmalloc 32 failed\n");
		goto failed;
	}

	if (replace_kmalloc32((unsigned long) new_kmalloc_32))
		goto failed;

	//test red_zone memory overlay 
	test_code_1();
	test_code_2();
	printk("Info: add new kmalloc_32 %lx, it will not be free until reboot\n", (unsigned long)new_kmalloc_32);
	return 0;

failed:
	printk("Err: modify failed kmalloc-32 \n");
	kmem_cache_destroy(new_kmalloc_32);
	return -EINVAL;
}

static void __exit slubufadriver_exit(void)
{
	if (replace_kmalloc32((unsigned long) old_kmalloc_32)) {
		printk("Err: modify failed kmalloc-32 \n");
	}
		
}

module_init(slubufadriver_init);
module_exit(slubufadriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
