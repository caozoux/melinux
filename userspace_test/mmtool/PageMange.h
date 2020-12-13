#ifndef __PAGEMANAGE_H__
#define __PAGEMANAGE_H__

#include <list>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <proc/readproc.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <linux/kernel-page-flags.h>


struct pageitem {
	unsigned long flag;
	struct pageinfo *info;
	void *priv;
};

#define HUGE_PAGE_NR            512
#define KPF_BYTES                8
#define PAGEITEMSIZE 			(sizeof(pageitem))

//映射机器所的内存结构体的管理
//
class MachinePageManage {

	int mKerFlagFd;
	int mNrSlab;
    int mNrBuddy;
	int mNrMovable;

public:
	static void 	*mMachineMap;
	static uint64_t *mPageFlagsBuf;
	static uint64_t  mPageFlagsSize;

public:
	MachinePageManage()
	{
		if (mMachineMap == NULL) {
			mMachineMap = malloc(1UL<<30);
			if (!mMachineMap) {
				printf("malloc kernel page flags malloc failed\n");
				return;
			}

			mPageFlagsBuf = (uint64_t *) malloc(1UL<<30);
			if (!mPageFlagsBuf) {
				free(mMachineMap);
				printf("malloc kernel page flags malloc failed\n");
			}
			printf("zz %s mPageFlagsBuf:%p \n",__func__, mPageFlagsBuf);
		}
	}

	~MachinePageManage()
	{

	}

	void MachineKerPageFlags(void)
	{
		printf("slab:%d buddy:%d movable:%d\n", mNrSlab, mNrBuddy, mNrMovable);
	}

	int KerPageScan(void)
	{
		unsigned long pfn = 0;
		uint64_t pageflagsbuf[HUGE_PAGE_NR], pageflags;
		long read_bytes;
		uint64_t *pagebuf;
		uint64_t idx;

		mKerFlagFd= open("/proc/kpageflags", O_RDONLY);
		if (mKerFlagFd < 0) {
			printf( "open %s failed %d\n","/proc/kpageflags", -errno);
			return -1;
		}

		pagebuf = (uint64_t *) MachinePageManage::mPageFlagsBuf;

		while (1) {

			read_bytes = pread(mKerFlagFd, pagebuf, HUGE_PAGE_NR * KPF_BYTES, pfn * KPF_BYTES);
			if (read_bytes != HUGE_PAGE_NR * KPF_BYTES)
				break;

				for (idx = 0; idx < HUGE_PAGE_NR; idx++) {

					pageflags = pagebuf[idx];

					if (pageflags & (1 << KPF_SLAB))
						mNrSlab++;
					else if (pageflags & (1 << KPF_BUDDY))
						mNrBuddy++;
					else if ((pageflags & (1 << KPF_LRU))
						&& !(pageflags & ((1 << KPF_COMPOUND_HEAD)
										| (1 << KPF_COMPOUND_TAIL)
										| (1 << KPF_WRITEBACK)
										| (1 << KPF_LOCKED)
										| (1 << KPF_THP))))
						mNrMovable++;
				}

				pfn += HUGE_PAGE_NR;
				pagebuf += HUGE_PAGE_NR;
				mPageFlagsSize += HUGE_PAGE_NR;
		}

		printf("scan %dG memory \n", (mPageFlagsSize*4096)/(1024*1024*1024));

		return 0;
	}
};

#endif
