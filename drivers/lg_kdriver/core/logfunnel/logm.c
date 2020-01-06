
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "logm_kapi.h"

#include "logm.h"

#if 0
#define debug(fmt, arg...)	printf ("%s.%d: " fmt, __func__, __LINE__, ##arg)
#else
#define debug(fmt, arg...)	do{}while(0)
#endif
#define error(fmt, arg...)	printf ("%s.%d: " fmt, __func__, __LINE__, ##arg)

/* default log mask */
unsigned int logmm_dummy_mask = 0xffffffff;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int logm = -1;
static LX_LOGM_LOGOBJ_T *objs;
static unsigned int mapsize;

static int init_logm (struct logmm_module *m)
{
	struct logm_obj_reg reg;
	int ret;

	if (logm < 0)
		logm = open ("/dev/lg/logm", O_RDWR);
	if (logm < 0)
	{
		error ("no logm device. %s\n",
				strerror (errno));
		return logm;
	}

	if (!objs)
	{
		mapsize = 0;
		ret = ioctl (logm, LOGM_IOR_OBJMAP_SIZE, &mapsize);
		if (ret < 0)
		{
			error ("LOGM_IOR_OBJMAP_SIZE error. %s\n",
					strerror (errno));
			return ret;
		}
		if (mapsize == 0)
		{
			error ("zero map size.\n");
			return -1;
		}
		debug ("map size %d\n", mapsize);

		objs = mmap (0, mapsize, PROT_READ, MAP_SHARED, logm, 0);
		if (objs == MAP_FAILED)
		{
			error ("map failed. %s\n", strerror (errno));
			objs = NULL;
			return -1;
		}
		debug ("mapped %p\n", objs);
	}

	strncpy (reg.name, m->name, LOGM_MODULE_NAME_MAX);
	reg.name[LOGM_MODULE_NAME_MAX-1] = 0;
	reg.ctrl = LOGM_OBJ_REGISTER;
	reg.fd = 0;

	ret = ioctl (logm, LOGM_IOWR_REG_OBJ, &reg);
	if (ret < 0)
	{
		error ("LOGM_IOWR_REG_OBJ error. %s\n", strerror (errno));
		return ret;
	}

	debug ("index %d for %s\n", reg.fd, m->name);
	m->index = reg.fd;
	m->mask = &objs[m->index].mask;

	return ret;
}

static void destroy_logm (void) __attribute__((destructor));
static void destroy_logm (void)
{
	if (logm >= 0)
	{
		close (logm);
		if (objs)
			munmap (objs, mapsize);

	}

	logm = -1;
	objs = NULL;
}

static int log_write (int fd,
		LX_LOGM_LOGLEVEL_T level ,LX_LOGM_LOGTYPE_T type,
		const unsigned char *data, int size)
{
	struct logm_user_log user;
	int ret;

	if (size <= 0)
		return 0;

	user.fd = fd;
	user.type = type;
	user.level = level;
	user.data = data;
	user.size = size;

	ret = ioctl (logm, LOGM_IOW_USER_WRITE, &user);
	if (ret < 0)
		error ("LOGM_IOW_USER_WRITE error. %s\n", strerror (errno));

	return ret;
}

int logmm_printl (struct logmm_module *m, enum log_level l,
		const char *func, int line,
		const char *format, ...)
{
	va_list args;
	unsigned char *buf;
	int len = 256;

	if (!m->index)
	{
		pthread_mutex_lock (&lock);
		if (!m->index)
		{
			/* not initialized one. initialize... */

			init_logm (m);
		}
		pthread_mutex_unlock (&lock);

		if (!(*m->mask & (1<<l)))
		{
			debug ("no print.\n");
			return 0;
		}
	}

	buf = malloc (len);
	if (!buf)
	{
		error ("Oops. %s\n", strerror (errno));
		return -1;
	}

	while (1)
	{
		int n = 0;
		int i = 0;
		unsigned char *_buf;

#define LOG_FORMAT_PREFIX	"%-20.20s:%4d ] "
		i = snprintf ((char*)buf, len, LOG_FORMAT_PREFIX, func, line);

		va_start (args, format);
		n = vsnprintf ((char*)buf + i, len - i, format, args);
		va_end (args);
		n += i;

		if (n > -1 && n < len)
		{
			LX_LOGM_LOGTYPE_T level;
			int ret;

			level = l-(log_level_error-LX_LOGM_LEVEL_ERROR);

			ret = log_write (m->index, level,
					LX_LOGM_LOGTYPE_0, buf, n);
			free (buf);
			return ret;
		}

		len *= 2;

		_buf = realloc (buf, len);
		if (!_buf)
		{
			error ("Oops. %s\n", strerror (errno));
			free (buf);
			return -1;
		}

		buf = _buf;
	}

	return 0;
}

