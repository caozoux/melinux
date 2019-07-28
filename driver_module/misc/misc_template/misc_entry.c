#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include "template_iocmd.h"

#define ARRT_MARCO(name) DEVICE_ATTR(name, S_IWUSR | S_IRUGO, read_##name, write_##name);

#define ARRT_MARCO_READ(name)  \
	static ssize_t read_##name(struct device *dev,struct device_attribute *attr, char *buf) \
	{ \
		ssize_t size;\
		size = sprintf(buf, "%d\n", name);\
		return size;\
	}

#define ARRT_MARCO_WRITE(name)  \
	static ssize_t write_##name(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)\
	{\
		unsigned long val;\
		if (!kstrtoul(buf, 0, &val)) {\
			name = val;\
		}\
		return count; \
	}

#define MISC_NAME "misc_template"

static int enable;

struct misc_private_data {
	int flag;		
};

struct ping_data {
	short unsigned int  s_send;
	short unsigned int  s_recv;
};

static int misc_template_open(struct inode *inode, struct file * file)
{

	struct misc_private_data *data;
	data = kzalloc(sizeof(struct misc_private_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}
	file->private_data = (void *) data;
	printk("zz %s inode:%lx file:%lx \n",__func__, (unsigned long)inode, (unsigned long)file);
	return 0;
}

static ssize_t misc_template_read(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t misc_template_write(struct file *file, char __user * buf, size_t size, loff_t *ppos)
{
	struct misc_private_data *data;
	data = (struct misc_private_data *)file->private_data;
	kfree(data);
	return 0;
}

static int misc_template_release (struct inode *inode, struct file *file)
{
	printk("zz %s inode:%lx file:%lx \n",__func__, (unsigned long)inode, (unsigned long)file);
	return 0;
}

static pte_t *pgd_pud_pnd_pte_dump(unsigned long addr)
{
	struct mm_struct *mm = current->mm;
	unsigned long *p_buf;

	struct page *page;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;

	printk("zz %s mm->pgd:%lx \n",__func__, (unsigned long)mm->pgd);
	printk("zz %s addr:%lx \n",__func__, (unsigned long)addr);
	page=virt_to_page(addr);
	pgd = pgd_offset(mm, addr);
	if (pgd_present(*pgd)) {
		pud = pud_offset(pgd, addr);
		if (pud_present(*pud)) {
			if (pud_large(*pud))
				return (pte_t *)pud;
			pmd = pmd_offset(pud, addr);

			if (pmd_present(*pmd)) {
				pte = pte_offset_kernel(pmd, addr);
				if (pte_present(*pte)) {
					printk("zz %s page:%lx \n",__func__, (unsigned long)page);
					printk("zz %s pgd:%lx pud:%lx pmd:%lx pte:%lx \n",__func__, (unsigned long)pgd, (unsigned long)pud, (unsigned long)pmd, (unsigned long)pte);
					printk("zz %s pgd:%lx pud:%lx pmd:%lx pte:%lx \n",__func__, (unsigned long)pgd_index(addr), (unsigned long)pud_index(addr), (unsigned long)pmd_index(addr), (unsigned long)pte_index(addr));
					p_buf= (unsigned long*)pgd;
					printk("zz %s pgd:%lx \n",__func__, (unsigned long)*p_buf);
					p_buf= (unsigned long*)pud;
					printk("zz %s pud:%lx \n",__func__, (unsigned long)*p_buf);
					p_buf= (unsigned long*)pmd;
					printk("zz %s pmd:%lx \n",__func__, (unsigned long)*p_buf);

					printk("zz %s pte:%lx val:%lx phy:%lx addrphy:%lx\n",__func__, (unsigned long)pte, (unsigned long)*((unsigned long *)pte),(unsigned long)virt_to_phys(pte), (unsigned long)virt_to_phys(addr));
					return pte;
				}
			}
		}
	}
	return pte;
}
static void test_user_page(u64 addr)
{
	struct page **pages;
	struct page *page;
	void *data;
	u64 *p;

	p = (u64 *)addr;
	
	data =kmalloc(4096, GFP_KERNEL);
	
	pgd_pud_pnd_pte_dump(data);
	printk("zz %s data:%lx page:%lx\n",__func__, (unsigned long)data, (unsigned long)virt_to_page(data));
	//alloc_page();
	printk("zz %s addr:%lx \n",__func__, (unsigned long)addr);
	pages = kcalloc(1, sizeof(struct page *), GFP_KERNEL);
	get_user_pages_fast(addr, 1, 1, &pages[0]);
	pgd_pud_pnd_pte_dump((u64)page_to_virt(pages[0]));
	printk("zz %s val:%lx \n",__func__, (unsigned long)p[0]);

}

static long misc_template_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{

	struct ping_data data;
	int ret = 0;

#if 0
	data.s_send = 3;
	data.s_recv= 4;
	if (copy_to_user((char __user *) arg, &data, sizeof(struct ping_data))) {
		printk("zz %s %d copy err\n", __func__, __LINE__);
		ret = -EFAULT;	
		goto OUT;
	}
	printk("zz %s cmd:%lx arg:%lx \n",__func__, (unsigned long)cmd, (unsigned long)arg);
#else
	printk("cmd:%d arg:%lx\n", cmd, arg);
	switch (cmd) {
		case IOCMD_USERMAP:
			pgd_pud_pnd_pte_dump(arg);
			test_user_page((u64)arg);
			break;
		default:
			goto OUT;
	}
#endif
OUT:
	return ret;
}

struct file_operations misc_temp_ops = {
	.open = misc_template_open,
	.read = misc_template_read,
	.write = misc_template_write,
	.unlocked_ioctl =misc_template_unlocked_ioctl,
	.release = misc_template_release
};

struct miscdevice  misc_dev = {
	.name = MISC_NAME,
	.fops = &misc_temp_ops,
};

ARRT_MARCO_READ(enable);
ARRT_MARCO_WRITE(enable);
ARRT_MARCO(enable);

static int __init miscdriver_init(void)
{
	int ret;
	if (misc_register(&misc_dev)) {
		pr_err(" misc register err\n");
		return 1;
	}

	device_create_file(misc_dev.this_device, &dev_attr_enable);
	printk("miscdriver load \n");
	return 0;
}

static void __exit miscdriver_exit(void)
{
	device_remove_file(misc_dev.this_device, &dev_attr_enable);
	misc_deregister(&misc_dev);
	printk("miscdriver unload \n");
}

module_init(miscdriver_init);
module_exit(miscdriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zou Cao<zoucaox@outlook.com>");
