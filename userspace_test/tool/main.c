#include<stdio.h>
#include<stdlib.h>

#define PGDIR_SHIFT  0x27
#define PTRS_PER_PGD 0x200
#define PUD_SHIFT    0x1e
#define PTRS_PER_PUD 0x200
#define PMD_SHIFT    0x15
#define PTRS_PER_PMD 0x200
#define PAGE_SHIFT   0xc
#define PTRS_PER_PTE 0x200

int main(int argc, char *argv[])
{
	unsigned long addr;
	char *ptr;
	if (argc == 1)
		printf("please input addr\n");

	addr=strtol(argv[1], &ptr,16);

	addr =0xffff882938E00000;
	printf("zz %s addr:%lx \n",__func__, (unsigned long)addr);
	printf("zz %s PGD:%08x PUD:%08x PMD:%08x PTE:%08x \n",__func__, 
			(int)addr>>PGDIR_SHIFT&(PTRS_PER_PGD-1)
			, (int)addr>>PUD_SHIFT&(PTRS_PER_PGD-1)
			, (int)addr>>PMD_SHIFT&(PTRS_PER_PGD-1)
			, (int)addr>>PAGE_SHIFT&PTRS_PER_PGD-1);
	return 0;
}
