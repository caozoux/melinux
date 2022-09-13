#ifndef __KSYS_DATA_H__
#define __KSYS_DATA_H__

#include <ksysd_ioctl.h>
#include <prdebug.h>

struct ioctl_ksdata {
	enum ioctl_cmdtype cmd;
	int subcmd;
	void *data;
	int len;
};

struct ioctl_ksdata_ktrace {
	int sub_cmd;
	void *data;
	int len;
};

struct ksysd_data {
	const struct device *dev;
};

struct ksys_private_data {

};

#define FUNC_UNIT(name) \
	int name##_unit_ioctl_func(unsigned int  cmd, unsigned long addr, struct ioctl_ksdata *data); \
	int name##_unit_init(void); \
	int name##_unit_exit(void); 

typedef int (*ksysd_unit_fn)(void);
typedef int (*ksysd_unit_ioctl_fn)(unsigned int cmd, unsigned long size, struct ioctl_ksdata *data);
struct ksysd_uint_item {
	const char *u_name;
	enum ioctl_cmdtype type;
	ksysd_unit_ioctl_fn ioctl;
	ksysd_unit_fn init;
	ksysd_unit_fn exit;
};

#define KSYSD_UNIT(name, utype)  { \
		.u_name =	#name, \
		.type =	utype, \
		.ioctl = name##_unit_ioctl_func,	 \
		.init =	 name##_unit_init, \
		.exit =	 name##_unit_exit, \
	}


int base_func_init(void);
FUNC_UNIT(kprobe);
FUNC_UNIT(ktrace);
#endif /* ifndef __KSYS_DATA_H__ */
