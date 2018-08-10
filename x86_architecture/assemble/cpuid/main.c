#include <stdio.h>


void test1()
{
	int result, input=2;
	__asm__volatile__("move %1,%0" : "=r"(result): "m"(input));
	printf("zz %s result:%08x input:%08x \n",__func__, (int)result, (int)input);
}
int main()
{
	int a;
	printf("main start\n");
	return 0;
}
