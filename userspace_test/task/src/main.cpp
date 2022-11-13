#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <gflags/gflags.h>
#include <linux/sched.h>

using namespace std;

//#define SCHED_NORMAL        0
//#define SCHED_FIFO      1
//#define SCHED_RR        2
//#define SCHED_BATCH     3
//#define SCHED_IDLE      5
//#define SCHED_DEADLINE      6

DEFINE_string(policy, "fifo rr idle deadline", "sched policy");
int main(int argc, char** argv)
{
	int rc,old_scheduler_policy;
	int policy = SCHED_NORMAL;
	struct sched_param my_params;
	google::ParseCommandLineFlags(&argc, &argv, true);
	google::ShutDownCommandLineFlags();

	old_scheduler_policy=sched_getscheduler(0);
	
	if (FLAGS_policy == "fifo")
		policy = SCHED_FIFO;
	else if (FLAGS_policy == "rr") 
		policy = SCHED_RR;
	else if (FLAGS_policy == "idle")
		policy = SCHED_IDLE;
	else if (FLAGS_policy == "deadline")
		policy = SCHED_DEADLINE;

	cout<<"policy:"<<policy<<endl;
	my_params.sched_priority=sched_get_priority_max(policy);// 尽可能高的实时优先级

	rc=sched_setscheduler(0,policy, &my_params);
	if(rc<0)
	{
	   perror("sched_setscheduler to %d error", policy);
	   exit(0);
	}

	old_scheduler_policy=sched_getscheduler(0);

	while(1);
	printf("the current scheduler = %d \n",old_scheduler_policy);
	printf("press any key to exit\n");
	getchar();

	return 0;
}
