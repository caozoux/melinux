#ifndef __KTRACE_IOCTL_H__
#define __KTRACE_IOCTL_H__

enum IOCTL_USEKINJECT_SUB{
	IOCTL_USEKINJECT_NONE = 0,
	IOCTL_USEKINJECT_TEST,
};

enum IOCTL_USEKINJECT_SUBTEST{
	IOCTL_USEKINJECT_TEST_NONE = 0,
	IOCTL_USEKINJECT_TEST_STATICKEY,
	IOCTL_USEKINJECT_HRTIMER,
};

struct kinject_ioctl {
	int enable;
	struct inject_test {
		enum IOCTL_USEKINJECT_SUBTEST cmd;
	} test_data;
	//int threshold;
};

#endif /* ifndef __KTRACE_IOCTL_H__ */

