/*
	SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
	Copyright(c) 2013 by LG Electronics Inc.

	This program is free software; you can redistribute it and/or 
	modify it under the terms of the GNU General Public License
	version 2 as published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
	GNU General Public License for more details.
*/ 


#ifndef _LOGM_H
#define _LOGM_H

struct logmm_module
{
	int index;
	unsigned int *mask;

	const char *name;
};

enum log_level
{
	log_level_error	= 0,
	log_level_warning,
	log_level_noti,
	log_level_info,
	log_level_debug,
	log_level_trace,
};

extern unsigned int logmm_dummy_mask;

#define logm_define(n,l) \
	struct logmm_module logmm_module_##n __attribute__((unused)) = \
	{ \
		.name = #n, \
		.mask = &logmm_dummy_mask, \
	}

#define logm_enabled(n,l)	({ \
		extern struct logmm_module logmm_module_##n; \
		(*logmm_module_##n.mask & (1<<l)); \
		})

extern int logmm_printl(struct logmm_module *, enum log_level, const char *func, int line, const char *format, ...);
#define _logmm_print(n,l,f,a...) do{ \
	if (logm_enabled(n,l)) \
		logmm_printl(&logmm_module_##n, l, __func__, __LINE__, f, ##a); \
	}while(0)

#define logm_error(n,f,a...)	_logmm_print(n,log_level_error,f,##a)
#define logm_warning(n,f,a...)	_logmm_print(n,log_level_warning,f,##a)
#define logm_noti(n,f,a...)	_logmm_print(n,log_level_noti,f,##a)
#define logm_info(n,f,a...)	_logmm_print(n,log_level_info,f,##a)
#define logm_debug(n,f,a...)	_logmm_print(n,log_level_debug,f,##a)
#define logm_trace(n,f,a...)	_logmm_print(n,log_level_trace,f,##a)

#define logm_level_set(n,l)	do{}while(0)

#endif

