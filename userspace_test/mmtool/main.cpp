#include<string.h>
#include "TaskScan.h"
#include "PageMange.h"

int main(int argc, char *argv[])
{
	TaskManage oTaskM;
	MachinePageManage oPageM;
	TaskItem *oTaskI;

	oPageM.KerPageScan();
	oPageM.MachineKerPageFlags();

	oTaskM.ScanTask();

	//oTaskI = oTaskM.GetTaskItem(44101);
	oTaskI = oTaskM.GetTaskItem(101196);
	oTaskI->dump(0);
	oTaskI->TaskPageScan();
	//oTaskM.DumpTaskList(1);
	//

	

	return 0;


}

