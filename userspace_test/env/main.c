#include<stdio.h>


int main(int argc, char *argv[])
{
	char *env=NULL;
	env=getenv("TMP");
	if (env)
		printf("%s\n", env);
	return 0;
}
