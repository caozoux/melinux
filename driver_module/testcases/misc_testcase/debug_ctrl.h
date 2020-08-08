#ifndef __DEBUG_CTRL_H__
#define __DEBUG_CTRL_H__

#define DEBUGLEVE 1

#define level_debug(level, fmt, arg...) \
	if (level >= DEBUGLEVE) \
		printk(fmt, ##arg);

#undef DEBUG
#undef INFO
#undef WARN
#undef ERR

#define DEBUG(fmt, args...) level_debug(1, fmt, ## args)
#define INFO(fmt,args...) 	level_debug(2, fmt, ## args)
#define WARN(fmt,args...) 	level_debug(3, fmt, ## args)
#define ERR(fmt,args...) 	level_debug(4, fmt, ## args)

#endif
