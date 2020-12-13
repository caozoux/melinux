#ifndef __MMPAGECOMMON_H__

#define __MMPAGECOMMON_H__

#ifdef DEBUG
#define DBG(fmt, ...) printf(fmt, ##__VA_ARGS__);
#else
#define DBG(fmt, ...)
#endif

#define INFO(fmt, ...) printf(fmt, ##__VA_ARGS__);
#define WARN(fmt, ...) printf(fmt, ##__VA_ARGS__);
#define ERR(fmt, ...) printf(fmt, ##__VA_ARGS__);

#endif

