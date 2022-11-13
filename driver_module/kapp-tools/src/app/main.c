#include<stdio.h>

#define TESTM(name) "mm"#name"_"__FILE__

int main(int argc, char *argv[])
{
	printf("%s\n", TESTM(aa));	
	return 0;
}
