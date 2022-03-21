#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/mman.h>
#include "template_iocmd.h"
#include "common_head.h"

extern int misc_fd;

static void help(void)
{
	printf("kmem --dump  get the vm_state of zone/none/numa\n");
	printf("kmem --vma_scan --pid scan the vma of current task\n");
	printf("kmem --pte addr  get the addr pte\n");
	printf("kmem --page_attr $pfn --extern size  dump page flags\n");
	printf("kmem --buddypagetest   dump page flags\n");
	printf("kmem --full_page_scan  full page scan\n");
	printf("kmem --slub_op $name   kmemcache operation\n");
	printf("kmem --slub_op $name   --op_name \"create/remove/add/dec\"	--extern ${size}  slub operation\n");
	printf("kmem --resource_scan   scan all resource\n");
	printf("kmem --testmmap  test mmap page flag\n");
	printf("kmem --hugetlb_alloc_nofault  alloc hugetlb but not fault\n");
	printf("kmem --hugetlb_alloc  alloc hugetlb\n");
}

static pages_buffer_order(unsigned long *buf, unsigned long size)
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

static void dump_pages_buffer(struct mem_size_stats *mss)
{
	int i;
#if 1
	pages_buffer_order(mss->page_buffer, mss->page_index);
	for (i = 0; i < mss->page_index; ++i) {
		printf("%d:%lx\n",i, mss->page_buffer[i]);
	}
#else
	printf("zz %s %08x \n",__func__, mss->page_buffer);
#endif
}

static void dump_mss_info(struct mem_size_stats *mss)
{

	int i;

#define PAGE_SIZE (4096)

	printf("resident     :%-18ld",(unsigned long)mss->resident/PAGE_SIZE*4);
	printf("shared_clean :%-18ld",(unsigned long)mss->shared_clean/PAGE_SIZE*4);
	printf("shared_dirty :%-18ld",(unsigned long)mss->shared_dirty/PAGE_SIZE*4);
	printf("private_clean:%-18ld",(unsigned long)mss->private_clean/PAGE_SIZE*4);
	printf("private_dirty:%-18ld\n",(unsigned long)mss->private_dirty/PAGE_SIZE*4);
	printf("referenced   :%-18ld",(unsigned long)mss->referenced/PAGE_SIZE*4);
	printf("anonymous    :%-18ld",(unsigned long)mss->anonymous/PAGE_SIZE*4);
	printf("lazyfree     :%-18ld",(unsigned long)mss->lazyfree/PAGE_SIZE*4);
	printf("anonymous_thp:%-18ld",(unsigned long)mss->anonymous_thp/PAGE_SIZE*4);
	printf("shmem_thp    :%-18ld\n",(unsigned long)mss->shmem_thp/PAGE_SIZE*4);
	printf("swap           :%-18ld",(unsigned long)mss->swap/PAGE_SIZE*4);
	printf("shared_hugetlb :%-18ld",(unsigned long)mss->shared_hugetlb/PAGE_SIZE*4);
	printf("private_hugetlb:%-17ld",(unsigned long)mss->private_hugetlb/PAGE_SIZE*4);
	printf("pss            :%-18ld",(unsigned long)mss->pss/PAGE_SIZE*4);
	printf("pss_locked     :%-18ld",(unsigned long)mss->pss_locked/PAGE_SIZE*4);
	printf("swap_pss       :%ld\n",(unsigned long)mss->swap_pss/PAGE_SIZE*4);

	printf("order0/1/2/3/4/5/6/7/8/9/10/11 ");

	for (i = 0; i < 11; ++i) {
		printf("%d/", mss->page_order[i]);
	}

	printf("\n");
	dump_pages_buffer(mss);
}

static const struct option long_options[] = {
	{"help",     no_argument, 0,  0 },
	{"dump",     required_argument, 0,  0 },
	{"output",     required_argument, 0,  0 },
	{"vma_scan",   no_argument, 0,  0 },
	{"pid",   required_argument, 0,  0 },
	{"pte",   no_argument, 0,  0 },
	{"extern",   required_argument, 0,  0 },
	{"page_attr",   required_argument, 0,  0 },
	{"buddypagetest",   no_argument, 0,  0 },
	{"full_page_scan",   no_argument, 0,  0 },
	{"slub_op",   required_argument, 0,  0 },
	{"op_name",   required_argument, 0,  0 },
	{"resource_scan",   no_argument, 0,  0 },
	{"testmmap",   no_argument, 0,  0 },
	{"hugetlb_alloc_nofault",   required_argument, 0,  0 },
	{"hugetlb_alloc",   required_argument, 0,  0 },
	{"sleep",   required_argument, 0,  0 },
	{0,0,0,0}
};

int kmem_usage(int argc, char **argv)
{
	struct ioctl_data data;
	int  __attribute__ ((unused)) ret = 0;
	int c;
	char pidstr[128];
	int  pid = -1; 
	unsigned long zone_vm_stat[64];
	unsigned long numa_vm_stat[64];
	unsigned long node_vm_stat[64];
	unsigned long extern_arg;
	void *buf;
	char op_name[128];
	int mapfd;
	unsigned char *p_map;
	unsigned int size, sleep_time;
	int loopnum;

	if (argc <= 1) { 
		help();
		return 0;
	}

	memset(&data.kmem_data.mss, 0, sizeof(struct mem_size_stats));
	ioctl_data_init(&data);

	data.type = IOCTL_USEKMEM;
	data.kmem_data.zone_vm_state = zone_vm_stat;
	data.kmem_data.zone_len = 4*64;
	data.kmem_data.node_vm_state = node_vm_stat;
	data.kmem_data.node_len = 4*64;
	data.kmem_data.numa_vm_state = numa_vm_stat;
	data.kmem_data.numa_len = 4*64;
	data.kmem_data.mss.page_buffer = malloc(4096*4096);


	while (1) {
		int option_index = -1;

		c = getopt_long_only(argc, argv, "", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (option_index) {
			case 0:
				help();
				return 0;
			case 1:
				data.cmdcode = IOCTL_USEKMEM_GET;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;
			case 2:
				data.cmdcode = IOCTL_USEKMEM_GET;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;
			case 3:
				data.cmdcode = IOCTL_USEKMEM_VMA_SCAN;
				//return ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;
			case 4:
				strncpy(pidstr, optarg, 128);
				pid = atoi(pidstr);
				break;
			case 5:
				data.cmdcode = IOCTL_USEKMEM_GET_PTE;
				data.kmem_data.addr=atoi(pidstr);;
				ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				break;

			case 6:
				extern_arg = strtoul(optarg, 0, 0);
				break;

			case 7:
				data.cmdcode = IOCTL_USEKMEM_PAGE_ATTR;
				data.kmem_data.pageattr_data.start_pfn = atoi(optarg);
				data.kmem_data.pageattr_data.size = 1;
				break;

			case 8:
				data.cmdcode = IOCTL_USEKMEM_TESTBUDDY;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);

			case 9:
				data.cmdcode = IOCTL_USEKMEM_FULL_PAGE_SCAN;
				return ioctl(misc_fd, sizeof(struct ioctl_data), &data);

			case 10:
				data.cmdcode = IOCTL_USEKMEM_SLUB_OP;
				//strncpy(data.kmem_data.name, 128, optarg);
				strcpy(data.kmem_data.name, optarg);
				break;

			case 11:
				strcpy(op_name, optarg);
				break;

			case 12:
				data.cmdcode =IOCTL_USEKMEM_RESOURCE_SCAN;
				break;

			case 13:
				mapfd = open("/tmp/testmap", O_RDWR | O_CREAT);
				if (mapfd < 0) {
					printf("create /tmp/testmap failed\n");
					return -1;
				}
				//p_map = (unsigned char *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,mapfd, 0);
				p_map = mmap(NULL, 2 * 0x200000, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | 0x40000 /*MAP_HUGETLB*/, -1, 0);

			    if(p_map == MAP_FAILED) {
					printf("create /tmp/testmap failed\n");
					return -1;
				}

				//p_map = (unsigned char *)malloc(4096);

				data.kmem_data.addr=p_map;
				data.cmdcode=IOCTL_USEKMEM_TESTMMAP;

				*p_map = 1;
				printf("zz %s p_map:%lx \n",__func__, (unsigned long)*p_map);
				ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				//*p_map = 3;
				printf("zz %s p_map:%lx \n",__func__, (unsigned long)*p_map);
				data.kmem_data.addr=p_map;
				data.cmdcode=IOCTL_USEKMEM_TESTMMAP;
				ioctl(misc_fd, sizeof(struct ioctl_data), &data);
				munmap(p_map, PAGE_SIZE);
				break;
			case 14:
				size = atoi(optarg);
				p_map = mmap(NULL, size * 0x200000, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | 0x40000 /*MAP_HUGETLB*/, -1, 0);
			case 15:
				size = atoi(optarg);
				p_map = mmap(NULL, size * 0x200000, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | 0x40000 /*MAP_HUGETLB*/, -1, 0);
				memset(p_map, 0, size * 0x200000);
			case 16:
				sleep_time = atoi(optarg);

			default:
				break;
		}
	}

	if (data.cmdcode == IOCTL_USEKMEM_VMA_SCAN) {
		data.pid = pid;
		printf("zz %s page_buffer:%lx cmdcode:%lx\n",__func__, data.kmem_data.mss.page_buffer, data.cmdcode);
		ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
		dump_mss_info(&data.kmem_data.mss);
		//printf("zz %s data.log_buf:%s \n",__func__, data.log_buf);
	} else if (data.cmdcode == IOCTL_USEKMEM_GET_PTE) { 
		if (extern_arg == NULL)
			buf = malloc(4096);
		else
			buf = extern_arg;

		//memset(buf, 0, 4096);
		data.cmdcode = IOCTL_USEKMEM_GET_PTE;
		data.kmem_data.addr =  buf;
		ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
		if (!ret)
			printf("%lx pte %lx\n", buf, data.kmem_data.val);
		if (extern_arg == NULL)
			free(buf);
	} else if (data.cmdcode == IOCTL_USEKMEM_PAGE_ATTR) { 
		data.kmem_data.pageattr_data.size = extern_arg;
		ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
	} else if (data.cmdcode == IOCTL_USEKMEM_SLUB_OP) { 
		if (!strcmp(op_name, "create")) {
			data.kmem_data.slub_ctrl.slub_size = extern_arg;
			data.kmem_data.slub_ctrl.op = SLUB_OP_CREATE;
		} else if (!strcmp(op_name, "remove")) {
			data.kmem_data.slub_ctrl.op = SLUB_OP_REMOVE;
		} else if (!strcmp(op_name, "add")) {
			data.kmem_data.slub_ctrl.op = SLUB_OP_ADD;
			data.kmem_data.slub_ctrl.count = extern_arg;
		} else if (!strcmp(op_name, "dec")) {
			data.kmem_data.slub_ctrl.op = SLUB_OP_DEC;
			data.kmem_data.slub_ctrl.count = extern_arg;
		} else {
			return -1;
		}
		ret = ioctl(misc_fd, sizeof(struct ioctl_data), &data);
	}
	
	if (sleep_time)
		sleep(sleep_time);
	data.kmem_data.slub_ctrl.op = extern_arg;
	free(data.kmem_data.mss.page_buffer);

	return ret;
}

