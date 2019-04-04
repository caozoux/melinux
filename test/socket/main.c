#include <stdio.h>

int main(int argc, char *argv[])
{
	int t[12];
	register int a asm("%eax");
	register int *b asm("%ebx") = t;
	asm("add %1, %0" : "r"(a) : "r"(b));

	return 0;
}
