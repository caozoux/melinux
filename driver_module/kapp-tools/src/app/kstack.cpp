#include <iostream>
#include <ostream>
#include <string>
#include <stdio.h>
#include <string.h>

#include <ksioctl/kstack_ioctl.h>
#include "kapp_unit.h"
#include "lib/symbole.h"

#define BUF_SIZE (MAX_SAVE * sizeof(ksys_callchain))
int unit_kstack(int argc, char** argv)
{
	int ret;
	struct ioctl_ksdata data;
	struct kstack_ioctl kstack_data;
	char dump_buf[BUF_SIZE] = {0};

	data.data = &kstack_data;
	data.len = sizeof(struct kstack_ioctl);
	kstack_data.buf = dump_buf;
	kstack_data.size = BUF_SIZE;

	if (argc != 2)
		return -1;

	if (strstr(argv[1], "help"))  {
		printf("     -d dump log\n");
		printf("     -c clean log\n");
		return 0;
	}

	if (!strcmp(argv[1], "-d")) {
		int cnt, i, j;
		ret = ktools_ioctl::kioctl(IOCTL_KSTACK, (int)IOCTL_KSTACK_DUMP, &data, sizeof(struct ioctl_ksdata));
		if (!ret)
			return 0;
		if (ret%sizeof(ksys_callchain)) {
			printf("Warning: stack cnt is uncorrept\n");
			return 0;
		}
		cnt = ret/sizeof(ksys_callchain);
		for (i = 0; i < cnt; ++i) {
			struct fsymbol sym;
			ksys_callchain *entry = (ksys_callchain*)dump_buf;
			for (j = 0; j < entry->offset; j++) {
				sym.reset(entry->address[j]);
				if (g_elf_sym->find_kernel_symbol(sym)) {

					printf("zz %s %d \n", __func__, __LINE__);
					std::cout<<sym.name<<std::endl;
				}
			}
			
		}

	} else if (!strcmp(argv[1], "-c")) {
		ktools_ioctl::kioctl(IOCTL_KSTACK, (int)IOCTL_KSTACK_CLEAN, &data, sizeof(struct ioctl_ksdata));
	}

	return 0;
}

