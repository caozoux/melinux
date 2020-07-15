
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include "version.h"
#include "sysreg.h"

extern const char *program;
extern int decdigits[];
void aarch64_read_register(int fd, uint64_t *data, int size, uint32_t reg);
void aarch64_printf(int mode, uint64_t data, unsigned int highbit,
		unsigned int lowbit);

/* support aarch64 */
void aarch64_read_register(int fd, uint64_t *data, int size, uint32_t reg)
{
	uint64_t data_read;
  	if (pread(fd, &data_read, sizeof data_read, reg) != sizeof(data_read)) {
    		perror("rdmsr:pread");
		fprintf(stderr,
			"This register is UNDEFINED or unreadable currently!\n");
    		exit(127);
  	}
	*data = data_read;
}

void aarch64_printf(int mode, uint64_t data,unsigned int highbit,
		unsigned int lowbit)
{
	unsigned int bits;
	int width;
	char *pat;
  	bits = highbit-lowbit+1;
  	if ( bits < 64 ) {
		/* Show only part of register */
    		data >>= lowbit;
    		data &= (1ULL << bits)-1;
  	}

  	pat = NULL;
  	width = 1;			/* Default */
  	switch(mode) {
  	case mo_hex:
    		pat = "%*llx\n";
    		break;
  	case mo_chx:
    		pat = "%*llX\n";
    		break;
  	case mo_dec:
  	case mo_dec|mo_c:
  	case mo_dec|mo_fill|mo_c:
    		/* Make sure we get sign correct */
    		if ( data & (1ULL << (bits-1)) ) {
      			data &= ~(1ULL << (bits-1));
      			data = -data;
    		}
    		pat = "%*lld\n";
    		break;
  	case mo_uns:
    		pat = "%*llu\n";
    		break;
  	case mo_oct:
    		pat = "%*llo\n";
    		break;
  	case mo_hex|mo_c:
    		pat = "0x%*llx\n";
    		break;
  	case mo_chx|mo_c:
    		pat = "0x%*llX\n";
    		break;
  	case mo_oct|mo_c:
    		pat = "0%*llo\n";
    		break;
  	case mo_uns|mo_c:
  	case mo_uns|mo_fill|mo_c:
    		pat = "%*lluU\n";
    		break;
  	case mo_hex|mo_fill:
    		pat = "%0*llx\n";
    		width = (bits+3)/4;
    		break;
  	case mo_chx|mo_fill:
    		pat = "%0*llX\n";
		width = (bits+3)/4;
		break;
  	case mo_dec|mo_fill:
		/* Make sure we get sign correct */
		if ( data & (1ULL << (bits-1)) ) {
			data &= ~(1ULL << (bits-1));
			data = -data;
		}
    		pat = "%0*lld\n";
    		width = decdigits[bits-1]+1;
    		break;
	case mo_uns|mo_fill:
		pat = "%0*llu\n";
		width = decdigits[bits];
		break;
  	case mo_oct|mo_fill:
		pat = "%0*llo\n";
		width = (bits+2)/3;
		break;
  	case mo_hex|mo_fill|mo_c:
		pat = "0x%0*llx\n";
		width = (bits+3)/4;
		break;
	case mo_chx|mo_fill|mo_c:
		pat = "0x%0*llX\n";
		width = (bits+3)/4;
		break;
  	case mo_oct|mo_fill|mo_c:
		pat = "0%0*llo\n";
		width = (bits+2)/3;
		break;
  	case mo_raw:
  	case mo_raw|mo_fill:
		fwrite(&data,sizeof data,1,stdout);
		break;
	case mo_raw|mo_c:
	case mo_raw|mo_fill|mo_c:
    	{
		unsigned char *p = (unsigned char *)&data;
		int i;
		for ( i = 0 ; i < sizeof data ; i++ ) {
			printf("%s0x%02x", i?",":"{", (unsigned int)(*p++));
      		}
      		printf("}\n");
    	}
  	break;
  	default:
		fprintf(stderr,
			"%s: Impossible case, line %d\n", program, __LINE__);
		exit(127);
  	}

  	if ( width < 1 )
    		width = 1;

  	if ( pat ){
    		printf(pat, width, data);
  	}

}


