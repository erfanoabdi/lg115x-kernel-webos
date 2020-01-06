#ifndef _LOG_H
#define _LOG_H

#ifdef __KERNEL__
#include <linux/atomic.h>
#include <linux/wait.h>
#endif

//#define NO_LOGFUNNEL
//#define NO_LOGFUNNEL_LEVEL	log_level_warning


/**
 * \defgroup loghelper log helper
 */

/**
 * \ingroup loghelper
 * \brief log level을 정한다.
 */
enum log_level
{
	log_level_error	= 0,
	log_level_warning,
	log_level_noti,
	log_level_info,
	log_level_debug,
	log_level_trace,

	log_level_user1,
	log_level_user2,
	log_level_user3,

	log_level_max = log_level_user3,
};

struct log_module
{
	char *name;

	unsigned int mask;
	enum log_level _level;

#ifdef __KERNEL__
	int user_land;
	atomic_t user;
	struct log_group *group;
#else
	int index;
#endif
	struct log_module *next;
};

int log_writef (struct log_module *module, enum log_level level,
		const char *fmt, ...) __attribute__((__format__ (__printf__,3,4)));
#ifdef __KERNEL__
int log_writeb (struct log_module *module, enum log_level level,
		const void *data, int size, const char *filename);
int log_set_mask (const char *name, unsigned int mask);
#endif

#define LOG_FORMAT_PREFIX	"%20s %-4d:"
#define log_printl(s,l,f,a...) do{ \
	if (s.mask & (1<<(l))) \
	log_writef (&s, (l), \
			LOG_FORMAT_PREFIX f, __func__, __LINE__, ##a); \
	}while(0)


#ifdef LOG_NAME

/*
 * Old interface. Only one log module can be defined in a source file. Do not
 * use this style of log print any more.
 */

#ifndef LOG_LEVEL
#define LOG_LEVEL	log_level_warning
#endif


#define __LOG_NAME(n)	#n
#define _LOG_NAME(n)	__LOG_NAME(n)

static struct log_module log_module __attribute__((unused)) =
{
	.name = _LOG_NAME (LOG_NAME),
	.mask = 0xffffffff,
	._level = LOG_LEVEL,
#ifndef __KERNEL__
	.index = -1,
#endif
};


#ifdef NO_LOGFUNNEL

#define log_printn(l,f,a...)	do{ \
	if (log_enabled(l)) \
		_log_print (f,##a); \
	} while(0)

#define log_error(f,a...)	log_printn(log_level_error,f,##a)
#define log_warning(f,a...)	log_printn(log_level_warning,f,##a)
#define log_noti(f,a...)	log_printn(log_level_noti,f,##a)
#define log_info(f,a...)	log_printn(log_level_info,f,##a)
#define log_debug(f,a...)	log_printn(log_level_debug,f,##a)
#define log_trace(f,a...)	log_printn(log_level_trace,f,##a)

#define log_user1(f,a...)	log_printn(log_level_user1,f,##a)
#define log_user2(f,a...)	log_printn(log_level_user2,f,##a)
#define log_user3(f,a...)	log_printn(log_level_user3,f,##a)

#define log_enabled(l)		(NO_LOGFUNNEL_LEVEL>=l)

#else

#define log_error(f,a...)	log_printl(log_module,log_level_error,f,##a)
#define log_warning(f,a...)	log_printl(log_module,log_level_warning,f,##a)
#define log_noti(f,a...)	log_printl(log_module,log_level_noti,f,##a)
#define log_info(f,a...)	log_printl(log_module,log_level_info,f,##a)
#define log_debug(f,a...)	log_printl(log_module,log_level_debug,f,##a)
#define log_trace(f,a...)	log_printl(log_module,log_level_trace,f,##a)

#define log_user1(f,a...)	log_printl(log_module,log_level_user1,f,##a)
#define log_user2(f,a...)	log_printl(log_module,log_level_user2,f,##a)
#define log_user3(f,a...)	log_printl(log_module,log_level_user3,f,##a)

#define log_enabled(l)		(LOG_LEVEL<=(l))

#endif

#endif



#ifdef __KERNEL__
#define logm_define(n,l) \
	struct log_module log_module_##n __attribute__((unused)) = \
	{ \
		.name = #n, \
		.mask = 0xffffffff, \
		._level = l, \
	}
#else
#define logm_define(n,l) \
	struct log_module log_module_##n __attribute__((unused)) = \
	{ \
		.name = #n, \
		.mask = 0xffffffff, \
		._level = l, \
		.index = -1, \
	}
#endif



#ifdef NO_LOGFUNNEL

#ifdef __KERNEL__
#define _log_print(f,a...)	printk(f,##a)
#else
#define _log_print(f,a...)	printf(f,##a)
#endif

#define logm_printn(n,l,f,a...)	do{ \
	if (logm_enabled(n,l)) \
		_log_print (f,##a); \
	} while(0)

#define logm_error(n,f,a...)	logm_printn(n,log_level_error,f,##a)
#define logm_warning(n,f,a...)	logm_printn(n,log_level_warning,f,##a)
#define logm_noti(n,f,a...)	logm_printn(n,log_level_noti,f,##a)
#define logm_info(n,f,a...)	logm_printn(n,log_level_info,f,##a)
#define logm_debug(n,f,a...)	logm_printn(n,log_level_debug,f,##a)
#define logm_trace(n,f,a...)	logm_printn(n,log_level_trace,f,##a)

#define logm_enabled(n,l)	({ \
	extern struct log_module log_module_##n; \
	(log_module_##n._level >= (l)); \
	})

#define logm_binary(n,l,d,s,f)	do{}while(0)
#define logm_level_set(n,l)	do{extern struct log_module log_module_##n; \
	log_module_##n._level = l; \
	}while(0)

#else

#define logm_error(n,f,a...)	do { \
	extern struct log_module log_module_##n; \
	log_printl(log_module_##n,log_level_error,f,##a); \
	} while (0)
#define logm_warning(n,f,a...)	do { \
	extern struct log_module log_module_##n; \
	log_printl(log_module_##n,log_level_warning,f,##a); \
	} while (0)
#define logm_noti(n,f,a...)	do { \
	extern struct log_module log_module_##n; \
	log_printl(log_module_##n,log_level_noti,f,##a); \
	} while (0)
#define logm_info(n,f,a...)	do { \
	extern struct log_module log_module_##n; \
	log_printl(log_module_##n,log_level_info,f,##a); \
	} while (0)
#define logm_debug(n,f,a...)	do { \
	extern struct log_module log_module_##n; \
	log_printl(log_module_##n,log_level_debug,f,##a); \
	} while (0)
#define logm_trace(n,f,a...)	do { \
	extern struct log_module log_module_##n; \
	log_printl(log_module_##n,log_level_trace,f,##a); \
	} while (0)

#define logm_enabled(n,l)	({ \
	extern struct log_module log_module_##n; \
	(log_module_##n.mask&(1<<(l))); \
	})

#define logm_binary(n,l,d,s,f)	do { \
	extern struct log_module log_module_##n; \
	if (log_module_##n.mask&(1<<(l))) \
		log_writeb (&log_module_##n,l,d,s,f); \
	}

#define logm_level_set(n,l)	do{}while(0)
#endif


#endif
