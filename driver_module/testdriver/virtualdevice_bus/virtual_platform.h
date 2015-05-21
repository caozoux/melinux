#ifndef _VIRTUAL_BUS_H_
#define _VIRTUAL_BUS_H_
#include<linux/device.h>

struct virtualme_platform_dev {
	struct device *dev;
	char * name;
};

struct virtualme_platform_drv {
	struct driver *drv;
	char * name;
};

struct virme_bus_data {
	struct device *dev;
};

#endif
