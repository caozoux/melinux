#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/fdtable.h>

#include <asm/stacktrace.h>
#include "template_iocmd.h"
#include "misc_ioctl.h"
#include "debug_ctrl.h"
#include "medelay.h"
#include "mekernel.h"

//d_inode(dentry) file_dentry
void task_file_enum(struct task_struct *task)
{
#if 0
	//struct files_struct *files_st;
	//struct file __rcu * fd_array[NR_OPEN_DEFAULT];
	struct file *file;
	struct dentry *dentry;
	int i;

	for (i; i < NR_OPEN_DEFAULT; i++) {
		file=task->files->fd_array[i];
		printk("zz %s file:%lx \n",__func__, (unsigned long)file);
	}
#else
	unsigned int fd;
	struct files_struct *files;
	struct dentry *dentry;
#if LINUX_VERSION_CODE <  KERNEL_VERSION(5,0,0)
	files = get_files_struct(current);
#else
	files = task->files;;
#endif

	if (!files)
		return;
	for(fd = 0; fd <files_fdtable(files)->max_fds; fd++) {
		struct file *f;
		char name[10+1];
		//f = fcheck_files(files, fd);
		f = files_lookup_fd_rcu(files,fd);
		if (!f)
			continue;
		printk("zz %s fd:%08x \n",__func__, (int)fd);
		dentry = file_dentry(f);
		if (dentry) {
			printk("fd:%d %s\n", fd, dentry->d_iname);
		}
		medentry_dump_full_patch(dentry);
	}

	 //put_files_struct(files);
#endif

}

struct task_struct *get_taskstruct_by_pid(int pid)
{
	return pid_task(find_pid_ns(pid, task_active_pid_ns(current)), PIDTYPE_PID);
}
