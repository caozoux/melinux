#ifndef __PRDEBUG_H__
#define __PRDEBUG_H__


#define DEBUGLEVE 1

#define level_debug(level, fmt, arg...) \
	do {  \
		if (level >= DEBUGLEVE) \
			printk(fmt, ##arg); \
	} while(0)

#define DBG(fmt, args...) 	level_debug(1, fmt, ## args)
#define MEINFO(fmt,args...) 	level_debug(2, fmt, ## args)
#define MEWARN(fmt,args...) 	level_debug(3, fmt, ## args)
#define ERR(fmt,args...) 		printk(fmt, args);

#endif /* ifndef __PRDEBUG_H__ */
