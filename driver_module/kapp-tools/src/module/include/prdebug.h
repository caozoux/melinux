#ifndef __PRDEBUG_H__
#define __PRDEBUG_H__


#define DEBUGLEVE 1

#define level_debug(level, fmt, arg...) \
	do {  \
		if (level >= DEBUGLEVE) \
			printk(fmt, ##arg); \
	} while(0)

#define MEDEBUG(fmt, args...) 	level_debug(1, fmt, ## args)
#define MEINFO(fmt,args...) 	level_debug(2, fmt, ## args)
#define MEWARN(fmt,args...) 	level_debug(3, fmt, ## args)
#define MEERR(fmt,args...) 		level_debug(4, fmt, ## args)

#endif /* ifndef __PRDEBUG_H__ */
