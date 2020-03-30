#include<unistd.h>
#include<stdio.h>
#include<getopt.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>


static int file_test(char *filename)
{
    int fd,size;
	int ret;
    fd=open(filename,O_WRONLY|O_CREAT);
	if (!fd) {
    	printf("open %s failed\n",filename);
		ret= -1;
		goto out;
	}

	sleep(5);
#if 0
	while(1)
		sleep(1);
#endif

#if 1
    size=write(fd,"upper",5);
#endif
	sleep(5);
    close(fd);
    printf("%d\n",size);
	ret = 0;

out:
	return ret;
}

int main(int argc, char *argv[])
{

	int choice;
	char *filename;

	while (1)
	{
		static struct option long_options[] =
		{
			/* Use flags like so:
			{"verbose",	no_argument,	&verbose_flag, 'V'}*/
			/* Argument styles: no_argument, required_argument, optional_argument */
			{"version", no_argument,	0,	'v'},
			{"help",	no_argument,	0,	'h'},
			{"file",	required_argument,	0,	'f'},
			
			{0,0,0,0}
		};
	
		int option_index = 0;
	
		/* Argument parameters:
			no_argument: " "
			required_argument: ":"
			optional_argument: "::" */
	
		choice = getopt_long( argc, argv, "vhf:",
					long_options, &option_index);
	
		if (choice == -1)
			break;
	
		switch( choice )
		{
			case 'v':
				
				break;
	
			case 'h':
				
				break;
	
			case '?':
				/* getopt_long will have already printed an error */
				break;
	
			case 'f':
				filename = optarg;
				file_test(filename);
				//printf("zz\n");
				break;

			default:
				/* Not sure how to get here... */
				return 0;
		}
	}
	
}
