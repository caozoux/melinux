#ifndef __KMEM_DATA_H__
#define __KMEM_DATA_H__


struct page_info {
	unsigned long page_addr;
	unsigned long page_order;
	int type;
	struct page_info *next;
	struct page_info *prev;
};

#endif

