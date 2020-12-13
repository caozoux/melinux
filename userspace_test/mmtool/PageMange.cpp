#include "PageMange.h"

void *MachinePageManage::mMachineMap = NULL;
void *MachinePageManage::mPageFlagsBuf = NULL;
uint64_t MachinePageManage::mPageFlagsSize = 0;
