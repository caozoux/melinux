#include<stdio.h>


unsigned char buf[12];
void main(void)
{
#if 0
//ins:7100003f 540006ad
//ins:1628cbf4 540006ad
   	buf[0]=0x3f;	
   	buf[1]=0x00;	
   	buf[2]=0x00;	
   	buf[3]=0x71;	
   	buf[4]=0xad;	
   	buf[5]=0x06;	
   	buf[6]=0x00;	
   	buf[7]=0x54;	
   	buf[8]=0xf4;	
   	buf[9]=0xcb;	
   	buf[10]=0x28;	
   	buf[11]=0x16;	
	printf("%p\n", buf);
#else
	unsigned long long va1, va2;
	va1 = 0xffff20030;
	va2 = 0x030;
	printf("%llx\n", va2 + ~(va1));
#endif
}

