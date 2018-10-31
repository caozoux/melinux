#!/usr/bin/env stap
probe kernel.function("chmod_common")
{
	
	printf ("[%s](%d) name:%s\n",
	execname(), pid(), kernel_string($path->dentry->d_iname));
}
