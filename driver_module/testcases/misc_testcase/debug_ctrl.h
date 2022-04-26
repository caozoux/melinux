#ifndef __DEBUG_CTRL_H__
#define __DEBUG_CTRL_H__

#define DEBUGLEVE 1

#define level_debug(level, fmt, arg...) \
	if (level >= DEBUGLEVE) \
		printk(fmt, ##arg);

#define MEDEBUG(fmt, args...) level_debug(1, fmt, ## args)
#define MEINFO(fmt,args...) 	level_debug(2, fmt, ## args)
#define MEWARN(fmt,args...) 	level_debug(3, fmt, ## args)
#define MEERR(fmt,args...) 	level_debug(4, fmt, ## args)

#endif
