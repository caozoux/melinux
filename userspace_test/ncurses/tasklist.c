#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <proc/readproc.h>
#include "template_iocmd.h"
#include "mmtool.h"

#define DEV_NAME "/dev/misc_template"

#define PROC_ONLY_FLAGS (PROC_FILLENV | PROC_FILLARG | PROC_FILLCOM | PROC_FILLMEM | PROC_FILLCGROUP | \
		PROC_EDITCMDLCVT | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM)

static int  cow_max=64;
static proc_t buf;
unsigned long u64_pidlist[4096];
static void pages_buffer_order(unsigned long *buf, unsigned long size)
{
	int i , j;
	unsigned long val;

	for (i = 1; i < size; ++i) {
		for (j = i; j >= 1; j--) {
			if (buf[j] < buf[j-1]) {
				val = buf[j-1];
				buf[j-1] = buf[j];
				buf[j] = val;
			} else {
				break;
			}
		}
	}
}

static void dump_pages_buffers(int show_x, struct mem_size_stats *mss, unsigned long *order, int show)
{
	int i,j, offset = 0;
	unsigned long val;
	unsigned long last_addr;
	unsigned long block_page = 1;
	unsigned long order0 = mss->page_index;
	char buf[1024], *line;
	
	pages_buffer_order(mss->page_buffer, mss->page_index);

	for (i = 0; i < mss->page_index; i += cow_max) {
 
		last_addr = mss->page_buffer[i];
		line = buf;

		for (j = 0; j < cow_max; ++j) {
			if (j == 0) {
				offset = sprintf(line, "%lx:-", mss->page_buffer[i]);
				line += offset;
			} else {
				if (mss->page_buffer[i+j] == (last_addr + 0x1000)) {
					block_page++;
					offset = sprintf(line, "-");
					line = line + offset;
				} else {
					unsigned long index = 0, val = block_page;
					while (val = val/2)
						index++;
					//printf("%d:%d", (int)block_page,index);
					order[index]++;
					block_page = 1;
					offset = sprintf(line, "*");
					line += offset;
				}
			}
			last_addr = mss->page_buffer[j+i];
		}

		if (show)
    		LOCPRINT(show_x++, 0, "%s\n", buf);

	}
}

static void dump_mss_info(struct mem_size_stats *mss)
{
	int i;

#define PAGE_SIZE (4096)
	printf("resident:%ld \n",(unsigned long)mss->resident/PAGE_SIZE*4);
	printf("shared_clean:%ld \n",(unsigned long)mss->shared_clean/PAGE_SIZE*4);
	printf("shared_dirty:%ld \n",(unsigned long)mss->shared_dirty/PAGE_SIZE*4);
	printf("private_clean:%ld \n",(unsigned long)mss->private_clean/PAGE_SIZE*4);
	printf("private_dirty:%ld \n",(unsigned long)mss->private_dirty/PAGE_SIZE*4);
	printf("referenced:%ld \n",(unsigned long)mss->referenced/PAGE_SIZE*4);
	printf("anonymous:%ld \n",(unsigned long)mss->anonymous/PAGE_SIZE*4);
	printf("lazyfree:%ld \n",(unsigned long)mss->lazyfree/PAGE_SIZE*4);
	printf("anonymous_thp:%ld \n",(unsigned long)mss->anonymous_thp/PAGE_SIZE*4);
	printf("shmem_thp:%ld \n",(unsigned long)mss->shmem_thp/PAGE_SIZE*4);
	printf("swap:%ld \n",(unsigned long)mss->swap/PAGE_SIZE*4);
	printf("shared_hugetlb:%ld \n",(unsigned long)mss->shared_hugetlb/PAGE_SIZE*4);
	printf("private_hugetlb:%ld \n",(unsigned long)mss->private_hugetlb/PAGE_SIZE*4);
	printf("pss:%ld \n",(unsigned long)mss->pss/PAGE_SIZE*4);
	printf("pss_locked:%ld \n",(unsigned long)mss->pss_locked/PAGE_SIZE*4);
	printf("swap_pss:%ld \n",(unsigned long)mss->swap_pss/PAGE_SIZE*4);
	for (i = 0; i < 11; ++i) {
		printf("order %d page count:%d\n", i, mss->page_order[i]);
	}
}

static int get_task_page_list(struct ioctl_data *data, unsigned long pid, unsigned long *page_order)
{
	int ret,i;
	int misc_fd;

	data->type = IOCTL_VMA_SCAN;
	data->pid = pid;
	misc_fd = open(DEV_NAME, O_RDWR);
	ret = ioctl(misc_fd, sizeof(struct ioctl_data), data);
	if (ret)
		return -1;

	memcpy(page_order, data->kmem_data.mss.page_order, sizeof(unsigned long) * 11);

	return 0;
}

unsigned long scan_proc_list(struct proc_task **proc_task_list, int x_off, int offset)
{
	struct ioctl_data data;
	struct mem_size_stats *mss;
	unsigned long page_order[11];
	PROCTAB *ptp;
	pid_t* pidlist;
	int flags, i;
	int show = 0;
	int x = x_off;
	int proc_index = 0;

	pidlist = NULL;
	flags = PROC_ONLY_FLAGS;
	ptp = openproc(flags, pidlist);

	if (!ptp) {
		printf("proc open failed\n");
		return -1;		
	}

#if 0
    LOCPRINT(x++, 0, "%-08s  %-08s %-020s %-020s %-012s %-012s\n", "USER", "PID", "COMMON", "VIRT", "RSS", "order0/1/2/3/4/5/7/8/9/10/11");
#endif

	memset(&data, 0, sizeof(struct ioctl_data));
	mss =  &data.kmem_data.mss;

	while (readproc(ptp, &buf)) {
		struct proc_task *proc_task;

		if (buf.rss && buf.cmdline && x > offset) {

			if (!proc_task_list[proc_index]) {
				proc_task_list[proc_index] = malloc(sizeof(struct proc_task));
				proc_task_init(&buf, proc_task_list[proc_index]);
			}

			proc_task = proc_task_list[proc_index++];
			mss->page_buffer = proc_task->buf;

			get_task_page_list(&data, buf.tid, page_order);
			pages_buffer_order(mss->page_buffer, mss->page_index);
			dump_pages_buffers(x+1, mss, page_order, show);
			memcpy(proc_task->order, page_order, sizeof(unsigned long)*11);
			proc_task->page_index = mss->page_index;

#if 0
    		LOCPRINT(x - offset, 0, "%-08s  %-08d %-020s %-020d %-012d%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/\n", proc_task->ruser, proc_task->tid, proc_task->cmd, proc_task->vsize, proc_task->rss
				, proc_task->order[0]
				, proc_task->order[1]
				, proc_task->order[2]
				, proc_task->order[3]
				, proc_task->order[4]
				, proc_task->order[5]
				, proc_task->order[6]
				, proc_task->order[7]
				, proc_task->order[8]
				, proc_task->order[9]
				, proc_task->order[10]
				, proc_task->order[11]
				);
#endif
			x++;
		}
	}

	return proc_index;
}


