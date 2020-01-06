
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include "log.h"
#include "logfunnel.h"

#if 0
#define debug(fmt, arg...)	printf ("%s.%d: " fmt, __func__, __LINE__, ##arg)
#else
#define debug(fmt, arg...)	do{}while(0)
#endif
#define error(fmt, arg...)	printf ("%s.%d: " fmt, __func__, __LINE__, ##arg)

static int log_dev = -1;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t tid;
static int reader_working;
static int module_num;
static struct log_module module_root;

int log_opendev (int mode)
{
	int dev;

	debug ("try to open /dev/logfunnel\n");
	dev = open ("/dev/logfunnel", mode);

	if (dev < 0)
	{
		debug ("try to open /dev/shm/logfunnel\n");
		dev = open ("/dev/shm/logfunnel", mode);
	}

	if (dev < 0)
	{
		FILE *p;

		p = fopen ("/proc/devices", "rt");
		if (!p)
			perror ("loghelper:fopen");
		else
		{
			while (1)
			{
				char buf[256], *b;
				int a;
				int major;

				/* get a line */
				b = fgets (buf, sizeof(buf), p);
				if (!b)
					break;

				/* remove tailing newline */
				a=strlen (b);
				while (a>0 && b[a-1] == '\n')
				{
					b[a-1] = 0;
					a--;
				}

				/* get major number */
				b = buf;
				major = strtol (b, &b, 10);

				/* get the name */
				b++;
				if (!strcmp (b, "logfunnel"))
				{
					debug ("found major %d\n", major);

					unlink ("/dev/shm/logfunnel");
					if (mknod ("/dev/shm/logfunnel",
								S_IFCHR |
								S_IRUSR | S_IWUSR |
								S_IRGRP | S_IWGRP |
								S_IWOTH,
								makedev (major, 0)
						  ) < 0)
						perror ("loghelper:mknod");
					else
						dev = open ("/dev/shm/logfunnel", mode);

					break;
				}
			}

			fclose (p);
		}
	}

	return dev;
}

static void *mask_reader (void *arg)
{
	unsigned int *m;
	int len;
	struct logfunnel_get_mask g;

	/* alloc initial buffer */
	len = module_num;
	m = malloc (sizeof (*m)*len);
	if (m == NULL)
	{
		error ("Oops\n");
		return NULL;
	}

	while (1)
	{
		int ret;
		int a;
		struct log_module *l;

		g.masks = m;
		g.masks_num = len;
		ret = ioctl (log_dev, LF_GET_MASK, &g);
		if (ret < 0)
		{
			error ("Oops\n");
			break;
		}

		if (g.masks_num != len)
		{
			unsigned int *_m;
			debug ("got %d modules(%d)\n", g.masks_num, len);

			_m = realloc (m, sizeof (*m) * g.masks_num);
			if (_m == NULL)
			{
				error ("Oops\n");
				break;
			}
			m = _m;

			for (a=len; a<g.masks_num; a++)
				m[a] = 0xffffffff;

			len = g.masks_num;

			continue;
		}

		pthread_mutex_lock (&lock);
		for (l=module_root.next, a=0; a<g.masks_num; a++, l=l->next)
		{
			debug ("set %s(%d) module mask %08x\n",
					l->name, a,
					g.masks[a]);
			if (l->index != a)
			{
				error ("Oops %d %d\n", l->index, a);
				break;
			}
			l->mask = g.masks[a];
		}
		pthread_mutex_unlock (&lock);
	}
	if (m)
		free (m);

	return NULL;
}

static void log_write (struct log_module *m,
		enum log_level l, enum log_type t,
		const unsigned char *data, int size)
{
	struct logfunnel_write w;
	int ret;


	if (log_dev < 0)
	{
		error ("no logfunnel device to write\n");
		return;
	}

	if (size <= 0)
		return;

	w.index = m->index;
	w.level = l;
	w.type = t;
	w.data = data;
	w.size = size;

	ret = ioctl (log_dev, LF_WRITE, &w);
	if (ret < 0)
		perror ("LF_WRITE");
}

static int init_log (struct log_module *m)
{
	struct logfunnel_get_mask g;
	int ret;

	if (log_dev < 0)
	{
		log_dev = log_opendev (O_WRONLY);
		debug ("we got log device %d\n", log_dev);
	}

	/* check we have added to the device */
	if (log_dev >= 0 && m->index < 0)
	{
		struct logfunnel_new new;

		new.level = m->_level;
		new.index = -1;
		new.name = m->name;
		new.name_len = strlen (m->name);
		ret = ioctl (log_dev, LF_NEW_MODULE, &new);
		if (ret < 0)
			perror ("LF_NEW_MODULE");
		else
		{
			struct log_module *t;

			m->index = new.index;

			debug ("log module added on %s(%d), %d\n",
					m->name, m->index,
					module_num);
			module_num ++;

			for (t=&module_root; t->next; t=t->next);
			t->next = m;
		}
	}

	if (!reader_working && log_dev >= 0 && module_num > 0)
	{
		ret = pthread_create (&tid, NULL, mask_reader, NULL);
		if (ret)
		{
			error ("pthread_create failed, %d\n", ret);
			close (log_dev);
			log_dev = -1;
		}
		else
			reader_working = 1;
	}

	/* get my mask */
	if (log_dev >= 0 && m->index >= 0)
	{
		unsigned int *_m;

		g.masks = NULL;
		g.masks_num = 0;
		ret = ioctl (log_dev, LF_GET_MASK, &g);
		if (ret < 0)
		{
			error ("LF_GET_MASK failed. %d, %s\n",
					ret, strerror(errno));

			return ret;
		}

		debug ("got %d modules\n", g.masks_num);
		_m = malloc (sizeof (*_m) * g.masks_num);
		if (_m == NULL)
		{
			error ("malloc failed %s\n", strerror (errno));
			return -1;
		}

		memset (_m, 0, sizeof (*_m) * g.masks_num);
		g.masks = _m;
		ret = ioctl (log_dev, LF_GET_MASK, &g);
		if (ret < 0)
			error ("LF_GET_MASK failed. %s\n",
					strerror (errno));

		m->mask = g.masks[m->index];
		debug ("my log mask %08x\n", m->mask);

		free (_m);
	}

	return 0;
}

int log_writef (struct log_module *m, enum log_level level,
		const char *fmt, ...)
{
	va_list args;
	unsigned char *buf;
	int size = 128;

	if (log_dev < 0 || m->index < 0)
	{
		/* check we have the device */
		pthread_mutex_lock (&lock);
		init_log (m);
		pthread_mutex_unlock (&lock);

		if (log_dev < 0 || m->index < 0)
		{
			error ("failed to initialize log module. %d, %d\n",
					log_dev, m->index);
			return -1;
		}
	}

	if ((m->mask & (1<<level)) == 0)
	{
		debug ("skip this log %08x, %d\n",
				m->mask, level);
		return 0;
	}

	buf = malloc (size);
	if (buf == NULL)
	{
		perror ("loghelper:malloc");
		return -1;
	}

	while (1)
	{
		int n;
		unsigned char *_buf;

		va_start (args, fmt);
		n = vsnprintf ((char*)buf, size, fmt, args);
		va_end (args);

		if (n > -1 && n < size)
		{
			log_write (m, level, log_type_aline,
					buf, n);
			free (buf);
			return 0;
		}

		size *= 2;

		_buf = realloc (buf, size);
		if (_buf == NULL)
		{
			perror ("loghelper:realloc");
			free (buf);
			return -1;
		}
		buf = _buf;
	}
}

