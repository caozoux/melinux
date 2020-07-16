
/*
 * wsysreg.c
 *
 * Utility to write data to a system register.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include "version.h"
#include "sysreg.h"

struct option long_options[] = {
	{ "help",                0, 0, 'h' },
	{ "version",             0, 0, 'V' },
	{ "hexadecimal",         0, 0, 'x' },
	{ "capital-hexadecimal", 0, 0, 'X' },
	{ "decimal",             0, 0, 'd' },
	{ "signed-decimal",      0, 0, 'd' },
	{ "unsigned-decimal",    0, 0, 'u' },
	{ "octal",               0, 0, 'o' },
	{ "c-language",          0, 0, 'c' }, 
	{ "zero-fill",           0, 0, '0' },
	{ "zero-pad",            0, 0, '0' },
	{ "register",            0, 0, 'r' },
	{ "processor",           1, 0, 'p' },
	{ "cpu",                 1, 0, 'p' },
	{ "bitfield",            1, 0, 'f' },
	{ 0, 0, 0, 0 }
};

/* Number of decimal digits for a certain number of bits */
/* (int) ceil(log(2^n)/log(10)) */
int decdigits[] = {
	1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5,
	5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
	10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15,
	15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 19,
	20
};

const char *program;
static const char *global_register_name;
struct register_t *global_register;
char *TABLEPATH; /* the path for regs.table */
char *TABLENAME = "regsv8_1.table"; /* the path for regs.table */

void usage(void)
{
	fprintf(stderr,
	  "Usage: %s [options] regno\n"
	  "  --help         -h  Print this help\n"
	  "  --version      -V  Print current version\n"
	  "  --hexadecimal  -x  Hexadecimal output (lower case)\n"
	  "  --capital-hex  -X  Hexadecimal output (upper case)\n"
	  "  --decimal      -d  Signed decimal output\n"
	  "  --unsigned     -u  Unsigned decimal output\n"
	  "  --octal        -o  Octal output\n"
	  "  --c-language   -c  Format output as a C language constant\n"
	  "  --zero-pad     -0  Output leading zeroes\n"
	  "  --processor #  -p  Select processor number (default 0)\n"
	  "  --bitfield h:l -f  Output bits [h:l] only\n"
	  "  --register #   -r  Specify register name\n"
	  "\n"
	  "  example:\n"
	  "	Read data from the specify register:\n"
	  "\n"
	  "	$rsysreg -p0 -r MPIDR_EL1\n"
	  "	0x80000002\n"
	  "\n"
	  "	Write data into the specify register:\n"
	  "\n"
	  "	$wsysreg -p0 -r MPIDR_EL1 <data>\n"
	  "	0x80000006\n"
	  , program);
}

/* traverse regs.table file and rearch for destination register */
int search()
{
	char str1[20];
	int op0,op1,cn,cm,op2;
	FILE *fp;
	unsigned int len = global_register->length;
	if(global_register->type == 0){
		op0 = atoi(&global_register->name[1]);
		op1 = atoi(&global_register->name[3]);
		op2 = atoi(&global_register->name[len - 1]);
		cn = atoi(&global_register->name[6]);
		cm = (global_register->name[len-4] == 'c') ? atoi(&global_register->name[len-3]): \
			(atoi(&global_register->name[len-4]));
		global_register->regcode = sys_reg(op0, op1, cn, cm, op2);
		return 0;
	}
	
	if((fp = fopen(TABLEPATH, "r")) == NULL){ 
		printf("%s: No such file or can not read!\n", TABLEPATH); 
		return -1; 
	} 
  
	while (!feof(fp)) { 
		fscanf(fp, "%d %d %d %d %d %s", &op0, &op1, &cn, &cm, &op2, str1);
		//printf("%d %d %d %d %d ", op0, op1, cn, cm, op2);
		if(strcmp(str1, global_register->name) == 0){
			/* find destination register */
			global_register->op0 = op0;
			global_register->op1 = op1;
			global_register->cn = cn;
			global_register->cm = cm;
			global_register->op2 = op2;
			global_register->regcode = sys_reg(op0, op1, cn, cm, op2);
			break;
		}
	} 
      	fclose(fp);
	if(strcmp(str1, global_register->name) != 0){
		/* Not find destination register */
		printf("Not find destination register!\n");
		exit(127);
	}
	return 0;
}


void parse(const char *regstr)
{
	int status;
	global_register = (struct register_t *)malloc(sizeof(struct register_t));
	global_register->length = strlen(regstr);
	global_register->name = (char *)malloc(strlen(regstr));
	strncpy(global_register->name, regstr, global_register->length);
	
	/* check the format of register name */
	if((regstr[0] == 'S') && (regstr[5] == 'c')){
		global_register->type = 0;
	}
	else{
		global_register->type = 1;
	}
	//TABLEPATH = getlogin();
	status = search();
	if(status == -1){
		/* Not find target register in regs.table */
		exit(127);
	}
	printf("register: %s\n", global_register->name);
}

void init()
{
	char *home = getenv("HOME");
	TABLEPATH = (char *)malloc(strlen(home) + strlen(TABLENAME) + 1);
	sprintf(TABLEPATH, "%s/%s", home, TABLENAME);
}

int main(int argc, char *argv[])
{
	uint32_t reg;
	uint64_t data;
	int c, fd;
	int mode = mo_hex;
	int cpu = 0;
	unsigned int highbit = 63, lowbit = 0;
	unsigned long arg;
	char *endarg;
	char msr_file_name[64];
	
	program = argv[0];
	if (argc < 4) {
		/* invalid input */
		usage();
		exit(127);
	}
	init();
	/* delete r */
	while ( (c = getopt_long(argc,argv,"hVxXdouc0p:f:r:",long_options,NULL)) != -1 ) {
		switch ( c ) {
		case 'h':
			usage();
			exit(0);
		case 'V':
			fprintf(stderr, "%s: version %s\n", program, VERSION_STRING);
			exit(0);
		case 'x':
			mode = (mode & ~mo_mask) | mo_hex;
			break;
		case 'X':
			mode = (mode & ~mo_mask) | mo_chx;
			break;
		case 'o':
			mode = (mode & ~mo_mask) | mo_oct;
			break;
		case 'd':
			mode = (mode & ~mo_mask) | mo_dec;
			break;
		case 'r':
			global_register_name = optarg;
			//printf("register: %d\n", strlen(global_register_name));
			if (strlen(global_register_name) == 0) {
				usage();
				exit(127);
			}
			parse(optarg);
			//mode = (mode & ~mo_mask) | mo_raw;
			break;
		case 'u':
			mode = (mode & ~mo_mask) | mo_uns;
			break;
		case 'c':
			mode |= mo_c;
			break;
		case '0':
			mode |= mo_fill;
			break;
		case 'p':
			arg = strtoul(optarg, &endarg, 0);
			if ( *endarg || arg > 255 ) {
				usage();
				exit(127);
			}
			cpu = (int)arg;
			break;
		case 'f':
		{
			if ( sscanf(optarg, "%u:%u", &highbit, &lowbit) != 2 ||
		     		highbit > 63 || lowbit > highbit ) {
		  	usage();
		  	exit(127);
			}
		}
			break;
		default:
			usage();
			exit(127);
		}
	}

	if (optind != argc-1) {
    		/* Should have exactly one argument */
    		usage();
    		exit(127);
  	}

	//reg = strtoul(argv[optind], NULL, 0);
	reg = global_register->regcode;
  	sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
  	fd = open(msr_file_name, O_RDWR);
  	if ( fd < 0 ) {
    		if ( errno == ENXIO ) {
      			fprintf(stderr, "rdmsr: No CPU %d\n", cpu);
			fprintf(stderr, "please enter CPU number: 0~%d\n", get_nprocs()-1);
      			exit(2);
    		} 
		else if ( errno == EIO ) {
      			fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n", cpu);
      			exit(3);
    		}
		else {
      			perror("rdmsr:open");
			fprintf(stderr, "please enter CPU number: 0~%d\n", get_nprocs()-1);
      			exit(127);
    		}
  	}
        /* write */
	data = strtoul(argv[optind], NULL, 0);
	ioctl(fd, 0x01, reg);
	aarch64_write_register(fd, data, sizeof data, reg);

	free(global_register);
	free(TABLEPATH);
	close(fd);
  	exit(0);
}




