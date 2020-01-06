#ifndef _LOG_FUNNEL_H
#define _LOG_FUNNEL_H

#include <linux/ioctl.h>

#include "log.h"

#define LF_TYPE		'l'

#define LF_NEW_MODULE	_IOWR(LF_TYPE, 1, struct logfunnel_new)
#define LF_WRITE	_IOW (LF_TYPE, 2, struct logfunnel_write)
#define LF_GET_MASK	_IOWR(LF_TYPE, 3, struct logfunnel_get_mask)


enum log_type
{
	log_type_aline,
	log_type_binary,
};

struct logfunnel_new
{
	enum log_level level;	/* default level when it created */
	int index;		/* index that was creaded */
	char *name;
	int name_len;
};

struct logfunnel_write
{
	int index;

	enum log_level level;
	enum log_type type;

	const unsigned char *data;
	int size;
};

struct logfunnel_get_mask
{
	unsigned int *masks;
	int masks_num;
};

#endif
