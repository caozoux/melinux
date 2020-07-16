#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../template_iocmd.h"


#define DEV_NAME "/dev/misc_template"

void usage_help()
{
	printf("help: \n");
	printf("e   enum all supper_block list\n");
}

int main(int argc, char *argv[])
{
	int fd;	
	struct ioctl_data data;
	int cmd_pass = 1;
	char ch;

	fd = open(DEV_NAME, O_RDWR);
	if (fd <= 0) {
		printf("%s open failed", DEV_NAME);
		return 0;
	}

	data.type = IOCTL_USEREXT2;
    while((ch=getopt(argc,argv,"heb:r"))!=-1)
  	{
		switch (ch) {
			case 'h':
				usage_help();
				break;
			case 'e':
				data.cmdcode = IOCTL_USEEXT2_ENUM_SUPBLOCK;
				cmd_pass = 1;
				break;
			case 'b':
				strcpy(data.ext2_data.blk_name,optarg);
				cmd_pass = 1;
				break;
			case 'r':
				data.cmdcode = IOCTL_USEEXT2_GET_BLOCK;
				cmd_pass = 1;
				break;
			default:
				usage_help();
				return 0;
		}
	}

	if (cmd_pass)
		ioctl(fd, sizeof(struct ioctl_data), &data);

out:
	close(fd);

	return 0;

}
