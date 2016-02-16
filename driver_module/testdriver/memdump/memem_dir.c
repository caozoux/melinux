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
#include <linux/miscdevice.h>
#include <asm/page.h>
#include <linux/gfp.h>
#include <linux/page-flags.h>
#include <linux/sched.h>//find_task_by_vpid
#include <linux/mm.h>//find_vma
#include <asm/system_info.h>
#include <asm/mach/map.h>

static int pid;
//static unsigned long va;
static u32 tag_address;
static u32 vir_address;
static u32 phy_address;
struct miscdevice  mem_dir_dev = {
	.name = "mem_dir",
};

static u32 *testaddress;

static int find_pgd_init(unsigned long va)
{
    unsigned long pa=0;
    struct task_struct *pcb_tmp=NULL;
    pgd_t *pgd_tmp=NULL;
    pud_t *pud_tmp=NULL;
    pmd_t *pmd_tmp=NULL;
    pte_t *pte_tmp=NULL;

	pid=1;
    rcu_read_lock();
    if( !( pcb_tmp = pid_task(find_vpid(pid), PIDTYPE_PID) ) ) 
    {
        rcu_read_unlock();
        printk(KERN_ALERT "Can't find the task %d.\n",pid);
        return 0;
    }
    rcu_read_unlock();
    
    printk(KERN_ALERT "pgd=0x%p\n",pcb_tmp->mm->pgd);
    if(!find_vma(pcb_tmp->mm,va))
    {
        printk(KERN_ALERT "virt_addr 0x%lx not available.\n",va);
        return 0;
    }
    pgd_tmp=pgd_offset(pcb_tmp->mm,va);
    printk(KERN_ALERT "pgd_tmp=0x%p\n",pgd_tmp);
    printk(KERN_ALERT "pgd_val(*pgd_tmp)=0x%lx\n",pgd_val(*pgd_tmp));
    if(pgd_none(*pgd_tmp))
    {
        printk(KERN_ALERT "Not mapped in pgd.\n");
        return 0;
    }
    pud_tmp=pud_offset(pgd_tmp,va);
    pmd_tmp=pmd_offset(pud_tmp,va);
/*FIXME:
  Do I need to check Large Page ? PSE bit.
if(pmd_large(*pmd_tmp) == 1){
pa = (pmd_val(*pmd_tmp) & PMD_MASK) | (va & ~PMD_MASK);
}
  */
    pte_tmp=pte_offset_kernel(pmd_tmp,va);
    if(pte_none(*pte_tmp))
    {
        printk(KERN_ALERT "Not mapped in pte.\n");
        return 0;
    }
    if(!pte_present(*pte_tmp))
    {
        printk(KERN_ALERT "pte not in RAM,maybe swaped.\n");
        return 0;
    }
    pa=(pte_val(*pte_tmp)&PAGE_MASK)|(va&~PAGE_MASK);
    printk(KERN_ALERT "virt_addr 0x%lx in RAM is 0x%lx.\n",va,pa);
    printk(KERN_ALERT "content in 0x%lx is 0x%lx.\n",pa,*(unsigned long*)((char *)pa+PAGE_OFFSET));
    return 0;
}

static ssize_t mem_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	//if (strict_strtoul(buf, 0, &val)) 
	if (kstrtoul(buf, 0, &val)) 
		goto out;


	__raw_writel(val, tag_address);
out:
	return count;
}

static ssize_t mem_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int size;
	size = sprintf(buf, "0x%08x\n", __raw_readl(tag_address));
	return size;
}

static ssize_t tag_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long address;
	if (!kstrtoul(buf, 0, &address)) 
		tag_address = address;
	return count; 
}

static ssize_t tag_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int size;
	size = sprintf(buf, "0x%08x\n", tag_address);
	return size;
}

static ssize_t phy_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long address;
	if (!kstrtoul(buf, 0, &address)) 
		phy_address = address;
	return count; 
}

static ssize_t phy_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int size;
	size = sprintf(buf, "phy %08x -> pfn:%08x vir 0x%08x\n", vir_address , __pfn_to_phys(phy_address), phys_to_virt((void*)phy_address));

	return size;
}

static ssize_t vir_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long address;
	if (!kstrtoul(buf, 0, &address)) 
		vir_address = address;
	return count; 
}

struct static_vm {
     //struct vm_struct vm;
     struct list_head list;
};

static ssize_t vir_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int size;
	struct static_vm *vm;
	
	size = sprintf(buf, "vir %08x -> pfn:%08x phy 0x%08x\n", vir_address , __pfn_to_phys(vir_address), virt_to_phys((const volatile void *)vir_address));

#if 0
	vm = find_static_vm_vaddr((void*)vir_address);

	if (vm)
		printk("zz %s %08x\n", __func__, &vm->vm);
#endif
	return size;
}

static ssize_t fixed_map_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long address;
	if (!kstrtoul(buf, 0, &address)) {
	}
	return count; 
}

static DEVICE_ATTR(mem_op, S_IWUSR | S_IRUGO, mem_read, mem_write);
static DEVICE_ATTR(mem_tag, S_IWUSR | S_IRUGO, tag_read, tag_write);
static DEVICE_ATTR(mem_phy, S_IWUSR | S_IRUGO, phy_read, phy_write);
static DEVICE_ATTR(mem_vir, S_IWUSR | S_IRUGO, vir_read, vir_write);
//static DEVICE_ATTR(mem_map, S_IWUSR | S_IRUGO, vir_read, vir_write);
//static DEVICE_ATTR(mem_fixed_map, S_IWUSR | S_IRUGO, fixed_map_read, fixed_map_write);


static struct map_desc serria_io_desc[] __initdata = {
		{
		.virtual    = 0xf8022000, 
		.pfn        = __phys_to_pfn(0x48022000), /* run-time */ 
		.length     = SZ_4K,
		.type       = MT_DEVICE,
		}
};
static int __init mem_dir_init(void)
{
	int ret;
	if (misc_register(&mem_dir_dev)) {
		pr_err(" misc register err\n");
		return 1;
	}
	ret = device_create_file(mem_dir_dev.this_device, &dev_attr_mem_op);
	if (ret) {
		pr_err("sys file create failed \n");
		misc_deregister(&mem_dir_dev);
		return 1;
	}

	ret = device_create_file(mem_dir_dev.this_device, &dev_attr_mem_tag);
	if (ret) {
		pr_err("sys file create failed \n");
		misc_deregister(&mem_dir_dev);
		return 1;
	}

	ret = device_create_file(mem_dir_dev.this_device, &dev_attr_mem_phy);
	if (ret) {
		pr_err("sys file create failed \n");
		misc_deregister(&mem_dir_dev);
		return 1;
	}

	ret = device_create_file(mem_dir_dev.this_device, &dev_attr_mem_vir);
	if (ret) {
		pr_err("sys file create failed \n");
		misc_deregister(&mem_dir_dev);
		return 1;
	}
	testaddress = kmalloc(1024, GFP_KERNEL);

	printk("zz %s %08x\n", __func__, testaddress);
	iotable_init(serria_io_desc, ARRAY_SIZE(serria_io_desc));
	return 0;
}

static void __exit mem_dir_exit(void)
{
	device_remove_file(mem_dir_dev.this_device, &dev_attr_mem_op);
	device_remove_file(mem_dir_dev.this_device, &dev_attr_mem_tag);
	device_remove_file(mem_dir_dev.this_device, &dev_attr_mem_phy);
	device_remove_file(mem_dir_dev.this_device, &dev_attr_mem_vir);
	misc_deregister(&mem_dir_dev);
	kfree(testaddress);
}

module_init(mem_dir_init);
module_exit(mem_dir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
