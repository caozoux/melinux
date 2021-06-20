#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

static int pci_res_fd0 = -1; 
static int pci_res_fd1 = -1; 
static void  *pci_res_ptr0;
static void  *pci_res_ptr1;
#define PCI_RESOURCE0 "/sys/devices/pci0000:00/0000:00:04.0/resource0"
#define PCI_RESOURCE1 "/sys/devices/pci0000:00/0000:00:04.0/resource4"

int main()
{
	int uiofd;
	int configfd;
	int err;
	int i;
	unsigned icount;
	unsigned char command_high;

	char *ptr;

	pci_res_fd0 = open(PCI_RESOURCE0,  O_RDWR);
	if (pci_res_fd0 < 0) {
		perror("uio open:");
		return errno;
	}
	pci_res_ptr0 = mmap(0, 0x1000 , PROT_READ | PROT_WRITE, MAP_SHARED, pci_res_fd0, 0);

	if (pci_res_ptr0 == MAP_FAILED) {
		printf("map %s failed\n", PCI_RESOURCE0);
		goto out_pci0;
	}

	pci_res_fd1 = open(PCI_RESOURCE1,  O_RDWR);
	if (pci_res_ptr1 == MAP_FAILED) {
		printf("map %s failed\n", PCI_RESOURCE1);
		goto out_pci1;
	}

	uiofd = open("/dev/uio0", O_RDWR);
	if (uiofd < 0) {
		perror("uio open:");
		return errno;
	}

	configfd = open("/sys/class/uio/uio0/device/config", O_RDWR);
	if (uiofd < 0) {
		perror("config open:");
		return errno;
	}


	/* Read and cache command value */
	err = pread(configfd, &command_high, 1, 5);
	if (err != 1) {
		perror("command config read:");
		return errno;
	}
	command_high &= ~0x4;


	for(i = 0;; ++i) {
		/* Print out a message, for debugging. */
		if (i == 0)
			fprintf(stderr, "Started uio test driver.\n");
		else
			fprintf(stderr, "Interrupts: %d\n", icount);

		/****************************************/
		/* Here we got an interrupt from the
		   device. Do something to it. */
		/****************************************/

		/* Re-enable interrupts. */
		err = pwrite(configfd, &command_high, 1, 5);
		if (err != 1) {
			perror("config write:");
			break;
		}

		/* Wait for next interrupt. */
		err = read(uiofd, &icount, 4);
		if (err != 4) {
			perror("uio read:");
			break;
		}

	}
out_pci1:
	munmap(pci_res_ptr0, 0x1000);
out_pci0:
	return errno;
}
