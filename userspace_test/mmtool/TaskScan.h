#ifndef __TAKSSCAN_H__
#define __TAKSSCAN_H__
#include <list>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/kernel-page-flags.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mmpagecomman.h"
#include "PageMange.h"

#include <proc/readproc.h>

#define DEBUG

/*
 * Number of elements in procmap struct.
 */
#define PROCMAP_SZ 8
#define PAGEMAP_MASK 			0x007FFFFFFFFFFFFFLL    /* bits 54:0 */

#define PROT_SZ 5
#define BUFSIZE                 8192
#define PROC_ONLY_FLAGS (PROC_FILLENV | PROC_FILLARG | PROC_FILLCOM | PROC_FILLMEM | PROC_FILLCGROUP | \
		PROC_EDITCMDLCVT | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM)

#define PAGE_SHIT               (12)
#define PAGE_SIZE               (1UL<<PAGE_SHIT)
#define min(v1, v2) 			(((v1) < (v2)) ? (v1) : (v2))

/*
#define KPF_LOCKED      0
#define KPF_ERROR       1
#define KPF_REFERENCED  2
#define KPF_UPTODATE    3
#define KPF_DIRTY       4
#define KPF_LRU         5
#define KPF_ACTIVE      6
#define KPF_SLAB        7
#define KPF_WRITEBACK   8
#define KPF_RECLAIM     9
#define KPF_BUDDY       10

//11-20: new additions in 2.6.31 
#define KPF_MMAP        11
#define KPF_ANON        12
#define KPF_SWAPCACHE   13
#define KPF_SWAPBACKED  14
#define KPF_COMPOUND_HEAD   15
#define KPF_COMPOUND_TAIL   16
#define KPF_HUGE        17
#define KPF_UNEVICTABLE 18
#define KPF_HWPOISON    19
#define KPF_NOPAGE      20

#define KPF_KSM         21
#define KPF_THP         22
#define KPF_BALLOON     23
#define KPF_ZERO_PAGE   24
#define KPF_IDLE        25
#define KPF_PGTABLE     26
*/

/*
 * Structure containing information gathered from maps file.
 */
struct procmap {
    uint64_t va_start;      /* Start virtual address of this region. */
    uint64_t va_end;        /* End virtual address of this region. */
    uint64_t pgoff;         /* Not used. */
    uint32_t maj;           /* Not used. */
    uint32_t min;           /* Not used. */
    uint32_t ino;           /* Not used. */
    char prot[PROT_SZ];     /* Not used. */
    char fname[PATH_MAX];   /* File name. */
};

class TaskItem {

	proc_t mProc;
	char ruser[128];
	char cmd[1024];
	unsigned long vsize;
	unsigned long rss;
	int mPageMapeFd;

public:
	int tid;

public:
	void dump(int SkipKer)
	{
		if (this->rss == 0 && SkipKer)
			return;

		printf("%-012s  %-08d %-020s %-020ld %-012ld\n", this->ruser, this->tid, this->cmd, this->vsize, this->rss);
	}

	TaskItem(proc_t *proc) {
		memcpy(&mProc, proc, sizeof(proc_t));
		strcpy(this->ruser, proc->ruser);
		this->tid = proc->tid;
		strcpy(this->cmd, proc->cmd);
		this->vsize= proc->vsize;
		this->rss = proc->rss;
	}

	~TaskItem()
	{

	}

	int get_memory_map(pid_t pid, struct procmap **p_procmap)
	{
		int procmap_max_num = 128, procmap_num = 0;
		struct procmap *procmap = NULL;
		int i;
		char mapfile[PATH_MAX];
		char *path = NULL;
		FILE *fmap = NULL;
		char line[BUFSIZE];
		char dlm[] = "-   :   ";
		char *str, *sp, *in[PROCMAP_SZ];
		char *end = NULL;

		snprintf(mapfile, PATH_MAX, "/proc/%u/maps", pid);

		fmap = fopen(mapfile, "r");
		if (fmap == NULL) {
			printf("Failed to open maps file %s: %d\n", path, errno);
			return -1;
		}

		procmap = (struct procmap*) calloc(procmap_max_num, sizeof(*procmap));
		if (procmap == NULL) {
			printf("Failed to alloc procmaps: %d\n", errno);
			goto failed;
		}

		/* Read through maps file until we find out base_address. */
		while (fgets(line, BUFSIZE, fmap) != 0) {
			str = line;
			errno = 0;
			/* Split line into fields. */
			for (i = 0; i < PROCMAP_SZ; i++) {
				in[i] = strtok_r(str, &dlm[i], &sp);
				if ((in[i] == NULL) || (errno != 0))
					goto failed;
				str = NULL;
			}

			/* Convert/Copy each field as needed. */
			procmap[procmap_num].va_start = strtoull(in[0], &end, 16);
			if ((in[0] == '\0') || (end == NULL) || (*end != '\0') ||
				(errno != 0))
				goto failed;

			procmap[procmap_num].va_end = strtoull(in[1], &end, 16);
			if ((in[1] == '\0') || (end == NULL) || (*end != '\0') ||
				(errno != 0))
				goto failed;

			procmap[procmap_num].pgoff = strtoull(in[3], &end, 16);
			if ((in[3] == '\0') || (end == NULL) || (*end != '\0') ||
				(errno != 0))
				goto failed;

			procmap[procmap_num].maj = strtoul(in[4], &end, 16);
			if ((in[4] == '\0') || (end == NULL) || (*end != '\0') ||
				(errno != 0))
				goto failed;

			procmap[procmap_num].min = strtoul(in[5], &end, 16);
			if ((in[5] == '\0') || (end == NULL) || (*end != '\0') ||
				(errno != 0))
				goto failed;

			procmap[procmap_num].ino = strtoul(in[6], &end, 16);
			if ((in[6] == '\0') || (end == NULL) || (*end != '\0') ||
				(errno != 0))
				goto failed;

			memcpy(&procmap[procmap_num].prot, in[2], PROT_SZ);
			memcpy(&procmap[procmap_num].fname, in[7], PATH_MAX);

			if (++procmap_num == procmap_max_num) {
				procmap_max_num *= 2;
				procmap = (struct procmap*)realloc(procmap, procmap_max_num * sizeof(*procmap));
				if (procmap == NULL) {
					printf("realloc procmap %d failed: %d\n",
							  procmap_max_num, errno);
					goto failed;
				}
			}
		}

		if (fmap)
			fclose(fmap);

		*p_procmap = procmap;

		return procmap_num;

	failed:
		if (fmap)
			fclose(fmap);
		if (procmap)
			free(procmap);
		return -1;
	}

#define PFN_NUM         (PAGE_SIZE / sizeof(uint64_t))

	int TaskPageScan(void) 
	{
		char path[PATH_MAX];
		uint64_t pagePfn[512];
		int readSize, procmap_idx;
		int procmap_max_num = 128, procmap_num = 0;
		struct procmap *procmap = NULL;

		snprintf(path, sizeof(path), "/proc/%u/pagemap", this->tid);

		//snprintf(path, sizeof(path), "/proc/%u/pagemap",  44101);
        mPageMapeFd = open(path, O_RDONLY);
        if (mPageMapeFd <= 0) {
            printf("process %u open %s failed: %d\n", this->tid, path, errno);
			return -1;
        }

		procmap_num = get_memory_map(this->tid, &procmap);

		for (procmap_idx = 0; procmap_idx < procmap_num; procmap_idx++) {
			int i;
			uint64_t offset, va_start, va_end;
			uint64_t total_pfn_num, target_pfn_num, scan_pfn_num;

			va_start = procmap[procmap_idx].va_start;
			va_end = procmap[procmap_idx].va_end;
			offset = (va_start >> PAGE_SHIT) * sizeof(uint64_t);
			total_pfn_num = (va_end - va_start) / PAGE_SIZE;

			if (lseek64(mPageMapeFd, offset, SEEK_SET) < 0) {
				printf("seek failed\n");
				return 0;
			}

			//printf("va:%-20lx -  %-20lx\n",va_start, va_end);
#if 1
			while ((target_pfn_num = min(total_pfn_num, PFN_NUM)) != 0)
			{
				int readpages;

				readSize = read(mPageMapeFd, pagePfn, min(target_pfn_num, 512)*8);
				if (readSize <= 0)
					break;

				readpages = readSize/8;
				//printf("zz %s readSize:%08x \n",__func__, (int)readSize);
				for (i = 0; i < readpages; i += 16) {
					uint64_t pageindex;
					int j;
#if 0
					if (pagePfn[i] != 0) {
						pageindex = pagePfn[i] & PAGEMAP_MASK;
						//printf("va_statr:%lx page index:%d -> %lx kflag:%lx\n", va_start, i, pagePfn[i] & PAGEMAP_MASK, MachinePageManage::mPageFlagsSize);
						printf("va_statr:%lx page index:%d -> %lx kflag:%lx\n", va_start, i, pagePfn[i] & PAGEMAP_MASK, MachinePageManage::mPageFlagsBuf[pageindex]);
					}
#else
					//pageindex = pagePfn[i] & PAGEMAP_MASK;
					printf("%lx:", va_start+PAGE_SIZE*i);
					for (j = 0; j < 16; ++j) {
						if ((i+j) >= readpages)
							break;
						pageindex = pagePfn[i+j] & PAGEMAP_MASK;
						printf(" %x", MachinePageManage::mPageFlagsBuf[pageindex+j]);
					}
					printf("\n");
#endif
				}
				total_pfn_num -= readSize;
				break;
			}
#endif
		}
	}
};

class TaskManage {

	unsigned long mPidList[4096];
	std::list<TaskItem> mTaskList;
	static proc_t buf;

public:
	TaskManage()
	{

	}

	~TaskManage()
	{

	}

public:

	void DumpTaskList(int SkipKer)
	{
		std::list<TaskItem>::iterator itor;
		itor = this->mTaskList.begin();
		while(itor != this->mTaskList.end())
		{
			itor->dump(SkipKer);
			itor++;
		}
	}

	TaskItem *GetTaskItem(int pid)
	{
		std::list<TaskItem>::iterator itor;
		itor = this->mTaskList.begin();
		while(itor != this->mTaskList.end())
		{
			if (itor->tid == pid)
				return &(*itor);
			itor++;
		}

		return NULL;
	}

	int ScanTask(void)
	{
		int	flags = PROC_ONLY_FLAGS;
		PROCTAB *ptp;
		pid_t pidlis;

		ptp = openproc(flags, &pidlis);

		if (!ptp) {
			printf("proc open failed\n");
			return -1;
		}

		while (readproc(ptp,&buf)) {
			if (buf.tid != getpid())
				this->mTaskList.push_back(TaskItem(&buf));
		}
	}
};


#endif

