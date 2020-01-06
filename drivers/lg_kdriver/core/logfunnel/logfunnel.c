
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <linux/version.h>

#include "log.h"
#include "logfunnel.h"

//#define USE_MISCDEVICE

#if 0
#define debug(fmt, arg...)	do{printk (KERN_DEBUG "%s.%d: " fmt, __func__, __LINE__, ##arg);}while(0)
#else
#if 1
static int debug_level;
module_param_named (debug, debug_level, int, 0644);
#define debug(fmt, arg...)	do{if (debug_level)printk (KERN_DEBUG "%s.%d: " fmt, __func__, __LINE__, ##arg);}while(0)
#else
#define debug(fmt, arg...)	do{}while(0)
#endif
#endif
#define error(fmt, arg...)	do{printk (KERN_ERR "%s.%d: " fmt, __func__, __LINE__, ##arg);}while(0)

static int force_printk;
module_param (force_printk, int, 0644);

/*
 * case 1:
 * |------------------------------------------------------------|
 *    <size1>< --- data1 --- > 
 *    -> tail                  -> head
 *
 * case 2:
 * |------------------------------------------------------------|
 *                             <size2>< --- data2 --- >  <-1>
 *  -> head                    -> tail
 *
 * - head and tail are always aligned on 4 bytes.
 * - sizes are 4 bytes.
 *         start at 4 bytes aligned position.
 * if there is not enough space at the end of buffer, put the size -1 and start
 * from first.
 */
static unsigned char *buffer;
static int buffer_size = 64*1024;
static int buffer_head, buffer_tail;
static int buffer_dropped;
static int buffer_write_counter;
static int buffer_last_readed_counter;
module_param_named (dropped, buffer_dropped, int, 0440);

static DEFINE_SPINLOCK (buffer_lock);
static DECLARE_WAIT_QUEUE_HEAD (wait);

struct log_group
{
	char *name;
	unsigned int mask;
	struct log_module members;

	struct log_group *next;
};

static struct log_group root_group;
static DEFINE_SPINLOCK (group_lock);

static struct proc_dir_entry *proc_entry;
static int user;

static DECLARE_WAIT_QUEUE_HEAD (mask_changed);

struct header
{
	unsigned long long clock;
	struct log_module *module;
	enum log_level level;

	/* data type and size that fallowing this header */
	enum log_type type;
	int size;

	/* for binary */
	const void *bin_data;
	int bin_size;

	/* continuty counter to check buffer drop */
	int count;
};

struct logf
{
	struct mutex modules_lock;
	struct log_module **modules;
	int modules_num;

	unsigned int user_mask;
	enum log_level default_level;

	/* for log reader */
	int enable_binary_read;
};

/*
 * get a line from the buffer.
 *
 * call in lock
 */
static int get (struct header *header, unsigned char **data)
{
	if (buffer_tail == buffer_head)
	{
		debug ("Ooops. no data.\n");
		return -1;
	}

	/* read */
	if (data)
		*data = buffer+buffer_tail+sizeof(struct header);

	if (header)
	{
		struct header *_header;

		_header = (struct header*)(buffer+buffer_tail);
		*header = *_header;
	}

	return 0;
}

enum mask_set_type
{
	mask__set,
	mask_add,
	mask_del,
};

static void mask_set (struct log_group *g, enum mask_set_type type, unsigned int mask)
{
	unsigned long flag;
	int a, l;
	struct log_module *m;

	if (!mask)
		return;

	if (type == mask_add)
	{
		debug ("%s: add %08x to %08x\n",
				g->name, mask, g->mask);
		g->mask |= mask;
		goto done;
	}

	if (type == mask_del)
	{
		debug ("%s: del %08x from %08x\n",
				g->name, mask, g->mask);
		g->mask &= ~mask;
		goto done;
	}

	l = 0;
	for (a=0; a<32; a++)
		if (mask & (1<<a))
			l = a;

	debug ("%s: level %d\n", g->name, l);

	for (a=31; a>l; a--)
		g->mask &= ~(1<<a);

	for (a=l; a>=0; a--)
		g->mask |= 1<<a;

	debug ("%s: mask %08x\n", g->name, g->mask);

done:
	spin_lock_irqsave (&group_lock, flag);
	for (m=&g->members; m->next; m=m->next)
		m->next->mask = g->mask;
	spin_unlock_irqrestore (&group_lock, flag);

	wake_up (&mask_changed);
}

static const char *log_name (int bit)
{
	static char buf[32];

	switch (bit)
	{
	case log_level_error: return "error";
	case log_level_warning: return "warning";
	case log_level_noti: return "noti";
	case log_level_info: return "info";
	case log_level_debug: return "debug";
	case log_level_trace: return "trace";

	case log_level_user1: return "user1";
	case log_level_user2: return "user2";
	case log_level_user3: return "user3";
	}

	sprintf (buf, "bit%d", bit);

	return buf;
}

static int mask_show (struct seq_file *s, void *data)
{
	struct log_group *g = (struct log_group*)s->private;
	int a;
	char *sep = "";

	seq_printf (s, "%s: ", g->name);

	for (a=0; a<32; a++)
	{
		if (g->mask & (1<<a))
		{
			seq_printf (s, "%s%s", sep, log_name(a));
			sep = ", ";
		}
	}

	seq_printf (s, "\n");

	return 0;
}

static int mask_open (struct inode *inode, struct file *file)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
	struct proc_dir_entry *dp;

	dp = PDE (inode);

	return single_open (file, mask_show, dp->data);
#else	
	return single_open (file, mask_show, PDE_DATA(inode));
#endif
}

static int mask_release (struct inode *inode, struct file *file)
{
	return single_release (inode, file);
}

static int set_group_mask (struct log_group *g, const char *buf, int size)
{
	int cnt = 0;
	const char *p;
	unsigned int mask;
	int sep;
	enum mask_set_type set = mask__set;

	p = buf;

	mask = 0;
	sep = 1;

	while (cnt < size)
	{
		if (*p == '+')
		{
			mask_set (g, set, mask);
			mask = 0;
			set = mask_add;
			sep = 1;

			p ++;
			cnt ++;
		}
		else if (*p == '-')
		{
			mask_set (g, set, mask);
			mask = 0;
			set = mask_del;
			sep = 1;

			p ++;
			cnt ++;
		}
		else if (
				*p == ',' ||
				*p == ' ' ||
				*p == '\t' ||
				*p == '\n' ||
				*p == 0
			)
		{
			sep = 1;

			p ++;
			cnt ++;
		}
		else if (sep)
		{
			struct
			{
				const char *name;
				int len;
				enum log_level level;
			} level_table[] =
			{
#define lt(s,n)	{#s, sizeof(#s), log_level_##n, }
				lt(error, error),
				lt(warning, warning),
				lt(noti, noti),
				lt(info, info),
				lt(debug, debug),
				lt(trace, trace),
				lt(user1, user1),
				lt(user2, user2),
				lt(user3, user3),

				/* simple form */
				lt(e, error),
				lt(w, warning),
				lt(n, noti),
				lt(i, info),
				lt(d, debug),
				lt(t, trace),
				lt(u1, user1),
				lt(u2, user2),
				lt(u3, user3),
#undef lt
			};
			int a;

			for (a=0; a<ARRAY_SIZE (level_table); a++)
			{
				int len;

				debug ("name %s, len %d\n",
						level_table[a].name,
						level_table[a].len);

				len = level_table[a].len-1;
				if (size-cnt < len)
				{
					debug ("no data\n");
					continue;
				}

				if (!strncmp (p,level_table[a].name, len))
				{
					debug ("got it\n");
					mask |= 1<<level_table[a].level;

					p += len;
					cnt += len;
					break;
				}
			}
			if (a == ARRAY_SIZE(level_table))
			{
				error ("unknown format. %02x, %c\n", *p, *p);
				return -EINVAL;
			}

			sep = 0;
		}
		else
		{
			error ("unknown format. %02x, %c\n", *p, *p);
			return -EINVAL;
		}
	}

	mask_set (g, set, mask);

	return size;
}

static ssize_t mask_write (struct file *file,
		const char __user *buf, size_t size, loff_t *off)
{
	struct log_group *g;
	int ret;

	g = ((struct seq_file *)file->private_data)->private;

	ret = set_group_mask (g, buf, size);

	return ret;
}

static struct file_operations proc_fops =
{
	.open = mask_open,
	.write = mask_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = mask_release,
};

static struct log_group *search_group (const char *name, enum log_level level)
{
	struct log_group *group;

	for (group = &root_group; group->next; group = group->next)
		if (!strcmp (group->next->name, name))
			break;
	if (group->next == NULL)
	{
		/* no such a group. add new log group */
		int a;

		debug ("make new log group %s, %d\n", name, level);
		group->next = kcalloc (1, sizeof (*group), GFP_KERNEL);
		if (group->next == NULL)
		{
			error ("no mem\n");
			return NULL;
		}
		group->next->name = kmalloc (strlen (name)+1, GFP_KERNEL);
		if (group->next->name == NULL)
		{
			error ("no mem\n");
			kfree (group->next);
			return NULL;
		}
		strcpy (group->next->name, name);
		for (a=0; a<=level; a++)
			group->next->mask |= 1<<a;

		proc_create_data (group->next->name, 0640, proc_entry,
				&proc_fops, group->next);
	}

	return group->next;
}

static int add_log_module_locked (struct log_module *module)
{
	struct log_group *group;

	group = search_group (module->name, module->_level);

	debug ("initialize... %s\n", module->name);

	module->group = group;
	module->mask = group->mask;
	module->next = group->members.next;
	group->members.next = module;

	return 0;
}

static void del_log_module (struct log_module *module)
{
	unsigned long flag;
	struct log_module *now;
	struct log_group *group;

	group = search_group (module->name, log_level_warning);
	spin_lock_irqsave (&group_lock, flag);

	for (now = &group->members; now->next; now = now->next)
	{
		if (now->next == module)
			break;
	}
	if (now->next == NULL)
	{
		error ("not registered module %p\n", module);
		error ("not registered module %s\n", module->name);
	}
	else
		now->next = now->next->next;

	spin_unlock_irqrestore (&group_lock, flag);

	if (module->name)
		kfree (module->name);

	kfree (module);
}

static struct log_module * get_log_module (struct log_module *m)
{
	if (!m->user_land)
		return m;

	atomic_inc (&m->user);

	return m;
}

static void put_log_module (struct log_module *m)
{
	if (!m->user_land)
	       return;

	if (atomic_dec_and_test (&m->user))
	{
		debug ("delete %s from list\n", m->name);
		del_log_module (m);
	}
}

/*
 * drop a line in the buffer.
 *
 * call in lock
 */
static int drop_a_line (void)
{
	int buffer_tail_tmp;
	struct header header;

	/* read size */
	if (get (&header, NULL) < 0)
		return -1;

	/* move tail */
	if (header.size < 0)
		buffer_tail = 0;
	else
	{
		buffer_tail_tmp = buffer_tail;
		buffer_tail += (sizeof(struct header)+header.size + 7)&~7;
		put_log_module (header.module);
		debug ("move tail %4d -> %4d\n", buffer_tail_tmp, buffer_tail);

		if (header.bin_data)
			vfree (header.bin_data);
	}

	/* wrap if its the end */
	if ((get (&header, NULL) == 0) && (header.size == -1))
		buffer_tail = 0;

	buffer_dropped ++;
	debug ("drop a line. %d line dropped\n", buffer_dropped);

	return 0;
}

/*
 * put a line into the buffer.
 *
 * call in lock
 */
static int put (struct log_module *module,
		enum log_level level, enum log_type type,
		const void *bin_data, int bin_size,
		const char *str, int len)
{
	int buffer_head_tmp;
	struct header *header;

	/* too big size, fail */
	if (buffer_size < sizeof(struct header)*2+len+7)
	{
		debug ("too big len. %d\n", len);
		return -1;
	}

	/* check the buffer space.
	 * if there is not enough space, drop a line at tail.
	 */
	/* case1 */
	if (buffer_tail <= buffer_head)
	{
case1:
		if (buffer_size < buffer_head+sizeof(struct header)*2+len+7)
		{
			header = (struct header*)(buffer+buffer_head);
			header->size = -1;

			if (buffer_tail == 0)
				drop_a_line ();

			/* now, it becomes case2 */
			buffer_head = 0;
		}
	}

	/* case2 */
	if (buffer_head < buffer_tail)
	{
		while (buffer_tail - buffer_head <= sizeof(struct header)+len+7)
		{
			debug ("head %4d\n", buffer_head);
			drop_a_line ();

			if (buffer_tail < buffer_head)
				goto case1;

			if (buffer_tail == buffer_head)
			{
				buffer_tail = buffer_head = 0;
				break;
			}
		}
	}

	/* now, its safe to store the data.
	 * lets copy the data.
	 */
	header = (struct header*)(buffer+buffer_head);
	header->clock = local_clock ();
	header->module = get_log_module (module);
	header->level = level;
	header->type = type;
	header->size = len;
	header->bin_data = bin_data;
	header->bin_size = bin_size;
	header->count = ++buffer_write_counter;
	memcpy (buffer+buffer_head+sizeof(struct header), str, len);

	buffer_head_tmp = buffer_head;
	buffer_head += (sizeof(struct header)+len + 7)&~7;
	debug ("move head %4d -> %4d\n", buffer_head_tmp, buffer_head);

	return 0;
}

/*
 * put a line into the buffer.
 *
 * initialize the module if it is not initialized. and write the buffer to the
 * queue.
 */
static int put_a_line (struct log_module *module,
		enum log_level level, enum log_type type,
		const void *bin_data, int bin_size,
		const char *str, int len)
{
	unsigned long flag;
	int ret;
	void *data;

	if (module->group == NULL)
	{
		ret = 0;
		spin_lock_irqsave (&group_lock, flag);
		if (module->group == NULL)
		{
			debug ("initialize..\n");
			ret = add_log_module_locked (module);
			get_log_module (module);
			if (ret < 0)
			{
				error ("failed to add\n");
			}
			else if ((module->mask & (1<<level)) == 0)
			{
				debug ("no print\n");
				ret = -EINVAL;
			}
		}
		spin_unlock_irqrestore (&group_lock, flag);
		if (ret < 0)
			return ret;
	}

	if (bin_data && bin_size)
	{
		debug ("allocate %d for binary\n", bin_size);
		data = vmalloc (bin_size);
	}
	else
		data = NULL;

	if (data)
		memcpy (data, bin_data, bin_size);

	spin_lock_irqsave (&buffer_lock, flag);
	ret = put (module, level, type,
			data, bin_size,
			str, len);
	wake_up (&wait);
	spin_unlock_irqrestore (&buffer_lock, flag);

	return ret;
}

static int logfunnel_open (struct inode *inode, struct file *file)
{
	struct logf *logf;

	debug ("open\n");

	logf = kcalloc (1, sizeof (*logf), GFP_KERNEL);
	if (logf == NULL)
	{
		error ("kmalloc failed\n");
		return -ENOMEM;
	}

	mutex_init (&logf->modules_lock);
	logf->user_mask = 0xffffffff;
	logf->default_level = log_level_debug;

	file->private_data = logf;

	user ++;
	debug ("user %d\n", user);

	return 0;
}

static int logfunnel_release (struct inode *inode, struct file *file)
{
	struct logf *logf = file->private_data;
	int a;

	debug ("release\n");

	user --;
	debug ("user %d\n", user);

	for (a=0; a<logf->modules_num; a++)
		put_log_module (logf->modules[a]);
	if (logf->modules)
		kfree (logf->modules);
	kfree (logf);

	return 0;
}

static int is_mask_changed (struct logf *logf, struct logfunnel_get_mask *mask)
{
	int a;

	if (mask->masks_num != logf->modules_num)
		return 1;

	for (a=0; a<logf->modules_num; a++)
		if (mask->masks[a] != logf->modules[a]->mask)
			return 1;

	return 0;
}

static int get_mask (struct logf *logf, struct logfunnel_get_mask *mask)
{
	int ret;

	ret = access_ok (VERIFY_READ|VERIFY_WRITE,
			mask->masks, sizeof(mask->masks[0])*mask->masks_num);
	if (!ret)
	{
		error ("Oops\n");
		return -EFAULT;
	}

	ret = wait_event_interruptible (mask_changed,
			is_mask_changed (logf, mask));
	if (ret != 0)
		return ret;

	mutex_lock (&logf->modules_lock);
	if (mask->masks_num == logf->modules_num)
	{
		int a;

		for (a=0; a<logf->modules_num; a++)
		{
			mask->masks[a] = logf->modules[a]->mask;
			debug ("mask %s(%d): %08x\n",
					logf->modules[a]->name, a, logf->modules[a]->mask);
		}
	}
	else
		mask->masks_num = logf->modules_num;
	mutex_unlock (&logf->modules_lock);

	return 0;
}

static int new_module (struct logf *logf, struct logfunnel_new *new)
{
	int ret;
	struct log_module **ms, *m;

	ret = access_ok (VERIFY_READ,
			new->name, new->name_len+1);
	if (!ret)
	{
		error ("Oops\n");
		return -EFAULT;
	}

	if (new->name[new->name_len])
	{
		error ("not null terminated\n");
		return -EINVAL;
	}

	ms = kmalloc (sizeof (struct log_module*)*(logf->modules_num+1),
			GFP_KERNEL);
	if (!ms)
		return -ENOMEM;

	if (logf->modules)
		memcpy (ms, logf->modules,
				sizeof (struct log_module*)*logf->modules_num);

	ms[logf->modules_num] = m = kcalloc (1, sizeof (struct log_module),
			GFP_KERNEL);
	m->user_land = 1;
	m->name = kmalloc (new->name_len+1, GFP_KERNEL);
	strcpy (m->name, new->name);
	m->_level = new->level;
	get_log_module (m);

	kfree (logf->modules);
	logf->modules = ms;

	new->index = logf->modules_num;
	logf->modules_num ++;

	ret = put_a_line (m, log_level_max+1,
			log_type_aline, NULL, 0,
			"module_added", 12);
	wake_up (&mask_changed);

	debug ("new module, level %d, index %d, %s\n",
			m->_level, new->index, new->name);

	return 0;
}

static int write_log (struct logf *logf, struct logfunnel_write *w)
{
	int ret;

	ret = access_ok (VERIFY_READ,
			w->data, w->size);
	if (!ret)
	{
		error ("Oops\n");
		return -EFAULT;
	}

	if (w->index < 0 ||
			w->index >= logf->modules_num)
	{
		error ("Oops\n");
		return -EINVAL;
	}

	if (w->data[w->size-1] == '\n')
		w->size --;

	ret = put_a_line (logf->modules[w->index],
			w->level, w->type,
			NULL, 0,
			w->data, w->size);

	return ret;
}

static long logfunnel_ioctl (struct file *file,
		unsigned int cmd, unsigned long arg)
{
	union
	{
		struct logfunnel_new new;
		struct logfunnel_get_mask get_mask;
		struct logfunnel_write write;
	} a;
	int ret = 0;
	struct logf *logf = file->private_data;

	debug ("ioctl\n");

	if (_IOC_TYPE(cmd) != LF_TYPE)
	{
		error ("invalid magic. magic=0x%02x\n",
				_IOC_TYPE(cmd) );
		return -ENOIOCTLCMD;
	}
	if (_IOC_DIR(cmd) & _IOC_WRITE)
	{
		int r;

		r = copy_from_user (&a, (void*)arg, _IOC_SIZE(cmd));
		if (r)
		{
			error ("copy_from_user failed\n");
			return -EFAULT;
		}
	}

	switch (cmd)
	{
	case LF_NEW_MODULE:
		mutex_lock (&logf->modules_lock);
		ret = new_module (logf, &a.new);
		mutex_unlock (&logf->modules_lock);
		break;

	case LF_WRITE:
		ret = write_log (logf, &a.write);
		break;

	case LF_GET_MASK:
		ret = get_mask (logf, &a.get_mask);
		break;

	default:
		error ("unknown ioctl command %08x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	if (ret >= 0 && _IOC_DIR(cmd) & _IOC_READ)
	{
		int r;

		r = copy_to_user ((void*)arg, &a, _IOC_SIZE(cmd));
		if (r)
		{
			error ("copy_to_user failed\n");
			return -EFAULT;
		}
	}

	return ret;
}

static const char *level_name (enum log_level l)
{
	switch (l)
	{
	default:
	case log_level_error:
		return "ERROR";
	case log_level_warning:
		return "WARN";
	case log_level_noti:
		return "NOTI";
	case log_level_info:
		return "INFO";
	case log_level_debug:
		return "DEBUG";
	case log_level_trace:
		return "TRACE";

	case log_level_user1:
		return "USER1";
	case log_level_user2:
		return "USER2";
	case log_level_user3:
		return "USER3";
	}
}

static int get_a_line_locked (struct logf *logf,
		char *data, int readed, int size)
{
	unsigned long long t;
	unsigned long nanosec_rem;
	char *line;
	int length;
	int buffer_tail_tmp;
	int tlen;
	struct header header;
	char *buf = data+readed;
	int len = size-readed;
	int ret;

	if (get (&header, (unsigned char**)&line) < 0)
	{
		/* no more data */
		return -EIO;
	}

	t = header.clock;
	length = header.size;
	debug ("length %d\n", length);

	if (length < 0)
	{
		buffer_tail = 0;
		return -EAGAIN;
	}

	nanosec_rem = do_div(t, 1000000000);

	if (header.type == log_type_aline)
	{
		tlen = 6+26;
		if (tlen+length+1 > len)
		{
			/* not enough buffer */
			debug ("not enough buffer\n");
			return -ENOMEM;
		}

		tlen = snprintf (buf, len,
				"[%5lu.%06lu] %10s %-5s ",
				(unsigned long)t, nanosec_rem/1000,
				header.module->name,
				level_name (header.level));
		if (len == tlen)
		{
			debug ("need more buffer\n");
			return -ENOMEM;
		}

		if (tlen+length+1 > len)
		{
			/* not enough buffer */
			debug ("not enough buffer\n");
			return -ENOMEM;
		}

		/* copy data */
		memcpy (buf+tlen, line, length);

		/* put tailing new line */
		buf[tlen+length] = '\n';
		ret = length + tlen+1;
	}
	else if (header.bin_data)
	{
		int data_size;

		debug ("binary...\n");
		if (!logf->enable_binary_read)
		{
			drop_a_line ();
			return -EAGAIN;
		}

		if (readed)
			return -ENOMEM;

		data_size = 1+sizeof(int)+	/* magic and data size */
			strlen(header.module->name)+1+
			length+1+		/* recommended filename size */
			header.bin_size;	/* binary size */
		debug ("size %d\n", data_size);

		if (data_size > size)
			return -ENOMEM;

		/* magic */
		buf[0] = 0;
		buf ++;

		/* data size */
		memcpy (buf, &data_size, sizeof(data_size));
		buf += sizeof (data_size);

		/* module name */
		strcpy (buf, header.module->name);
		buf += strlen (header.module->name)+1;

		/* recommended filename */
		memcpy (buf, line, length);
		buf += length;
		buf[0] = 0;
		buf ++;

		memcpy (buf, header.bin_data,
				header.bin_size);

		ret = data_size;
	}
	else
	{
		error ("Oops\n");
		ret = 0;
	}

	if (header.bin_data)
		vfree (header.bin_data);

	buffer_tail_tmp = buffer_tail;
	buffer_tail += (sizeof(struct header) + length + 7) & ~7;
	put_log_module (header.module);
	debug ("move tail %4d -> %4d\n", buffer_tail_tmp, buffer_tail);

	if ((header.count-1) != buffer_last_readed_counter)
		error ("buffer dropped. %d\n",
				header.count-buffer_last_readed_counter-1);
	buffer_last_readed_counter = header.count;

	return ret;
}

static ssize_t logfunnel_read (struct file *file,
		char __user *data, size_t size, loff_t *off)
{
	int ret, readed;
	unsigned long flag;
	struct logf *logf = file->private_data;

	debug ("read %zd bytes\n", size);

	if (!access_ok (VERIFY_WRITE, data, size))
	{
		error ("access failed.\n");
		return -EINVAL;
	}

	/* wait a data */
	ret = wait_event_interruptible (wait,
			(buffer_tail != buffer_head));
	if (ret != 0)
		return ret;

	debug ("got data. %d, %d\n", buffer_tail, buffer_head);

	readed = 0;
	spin_lock_irqsave (&buffer_lock, flag);
	do
	{
		ret = get_a_line_locked (logf, data, readed, size);
		if (ret == -ENOMEM)
			break;

		if (ret > 0)
			readed += ret;
	}
	while (buffer_tail != buffer_head);
	spin_unlock_irqrestore (&buffer_lock, flag);

	debug ("done\n");

	if (ret == -ENOMEM && readed == 0)
	{
		debug ("no memory\n");
		readed = ret;
	}

	return readed;
}

#if 0
static ssize_t logfunnel_write (struct file *file,
		const char __user *data, size_t size, loff_t *off)
{
	int written;
	int ret;
	struct logf *logf = file->private_data;

	debug ("write %zd bytes\n", size);
	written = 0;
	while (written < size)
	{
		int length;
		char c;

		/* check length */
		length=0;
		do
		{
			ret = get_user (c, data+written+length);
			if (ret < 0 || c == '\n' || c == 0)
				break;
			length++;
		}
		while (written+length < size && ret >= 0);
		if (ret < 0)
			break;
		debug ("length %d\n", length);

		/* put the data */
		put_a_line (&logf->module, logf->default_level, log_type_aline,
				NULL, 0,
				data+written, length);
		written += length;

		/* skip newline */
		if (written < size)
		{
			ret = get_user (c, data+written);
			if (ret >= 0 && (c == '\n' || c == 0))
				written ++;
		}
	}

	debug ("done\n");

	return written;
}
#endif

static unsigned int logfunnel_poll (struct file *file,
		struct poll_table_struct *pt)
{
	poll_wait (file, &wait, pt);

	if (buffer_tail != buffer_head)
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

static struct file_operations logfunnel_fops =
{
	.owner		= THIS_MODULE,
	.open		= logfunnel_open,
	.release	= logfunnel_release,
	.unlocked_ioctl	= logfunnel_ioctl,
	.read		= logfunnel_read,
//	.write		= logfunnel_write,
	.poll		= logfunnel_poll,
};

static int printk_level (enum log_level l)
{
	switch (l)
	{
	case log_level_error:	// KERN_ERR
		return 3;
	case log_level_warning:
		return 4;
	case log_level_noti:
		return 5;
	case log_level_info:
		return 6;

	default:
	case log_level_debug:
	case log_level_trace:
		return 7;
	}
}

int log_writef (struct log_module *module, enum log_level level,
		const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len = 256;
	int n = 0;

	debug ("write, name %s\n", module->name);

	while (1)
	{
		debug ("try buffer size %d\n", len);
		buf = kmalloc (len, GFP_KERNEL);
		if (buf == NULL)
		{
			error ("no memory for %d sized log write\n", len);
			n = -ENOMEM;
			break;
		}

		va_start (args, fmt);
		n = vsnprintf (buf, len, fmt, args);
		va_end (args);

		if (n < len)
		{
			debug ("got %d size string\n", n);
			if (buf[n-1] == '\n')
			{
				debug ("remove tailing newline\n");
				n--;
			}

			put_a_line (module, level, log_type_aline,
					NULL, 0,
					buf, n);

			if (!user || force_printk)
				printk ("<%d> %10s %-5s %s",
						printk_level (level),
						module->name,
						level_name (level),
						buf);

			break;
		}

		kfree (buf);
		len *= 2;
	}

	if (buf)
		kfree (buf);

	return n;
}
EXPORT_SYMBOL (log_writef);

/*
 * do not call in interrupt context. not safe inside interrupt.
 */
int log_writeb (struct log_module *module, enum log_level level,
		const void *data, int size, const char *filename)
{
	int ret;

	ret = put_a_line (module, level, log_type_binary,
			data, size,
			filename, strlen(filename));
	return ret;
}
EXPORT_SYMBOL (log_writeb);

int log_set_mask (const char *name, unsigned int mask)
{
	unsigned long flag;
	struct log_group *g;
	struct log_module *m;

	spin_lock_irqsave (&group_lock, flag);
	g = search_group (name, log_level_warning);

	for (m=&g->members; m->next; m=m->next)
		m->next->mask = mask;
	spin_unlock_irqrestore (&group_lock, flag);

	return 0;
}
EXPORT_SYMBOL (log_set_mask);


static int status_show (struct seq_file *s, void *data)
{
	int filled;

	filled = buffer_head - buffer_tail;
	if (filled < 0)
		filled += buffer_size;

#define seq_print_status(v,f)	seq_printf (s, #v"="f"\n", v)
	seq_print_status (buffer_head, "%d");
	seq_print_status (buffer_tail, "%d");
	seq_print_status (buffer_size, "%d");
	seq_print_status (filled, "%d");
	seq_print_status (force_printk, "%d");
	seq_print_status (buffer_dropped, "%d");
	seq_print_status (buffer_write_counter, "%d");
	seq_print_status (debug_level, "%d");
#undef seq_print_status

	return 0;
}

static int status_open (struct inode *inode, struct file *file)
{
	return single_open (file, status_show, NULL);
}

static ssize_t status_write (struct file *file,
		const char __user *buf, size_t size, loff_t *off)
{
	char *str;
	char *name;
	char *sep, *p;
	int cnt;
	int ret = size;
	struct log_group *g;
	unsigned long flag;

	if (*off != 0)
	{
		error ("nonzero offset\n");
		return -EINVAL;
	}

	str = vmalloc (size);
	memcpy (str, buf, size);
	str[size-1] = 0;

	cnt = 0;
	name = str;
	p = str;
	while (cnt<size && *p != ':')
	{
		p ++;
		cnt ++;
	}

	if (cnt == size || *p != ':')
	{
		error ("no name seperator. "
				"write something like \"vdec:debug\".\n");
		ret = -EINVAL;
		goto failed;
	}

	*p = 0;
	sep = p+1;

	while (
			*name == ' ' ||
			*name == '\t' ||
			*name == '\n'
	      )
		name ++;

	while (str < p && (
				*(p-1) == ' ' ||
				*(p-1) == '\t' ||
				*(p-1) == '\n'
			  ))
	{
		p --;
		*p = 0;
	}

	debug ("group name \"%s\", mask \"%s\"\n",
			name, sep);

	spin_lock_irqsave (&group_lock, flag);
	g = search_group (name, log_level_warning);
	spin_unlock_irqrestore (&group_lock, flag);

	set_group_mask (g, sep, size-(sep-str));

	ret = size;

failed:
	vfree (str);
	return ret;
}

static struct file_operations proc_status_fops =
{
	.open = status_open,
	.read = seq_read,
	.write = status_write,
	.llseek = seq_lseek,
	.release = single_release,
};


#ifdef USE_MISCDEVICE
#include <linux/miscdevice.h>

static struct miscdevice logfunnel_dev =
{
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "logfunnel",
	.fops	= &logfunnel_fops,
};

#else
static int logfunnel_major = 1820;
module_param_named (major, logfunnel_major, int, 0444);
#endif

static void logfunnel_exit (void)
{
#ifdef USE_MISCDEVICE
	misc_deregister (&logfunnel_dev);
#else
	unregister_chrdev (logfunnel_major, "logfunnel");
#endif

	vfree (buffer);

	remove_proc_entry ("status", proc_entry);
	remove_proc_entry ("log", NULL);
}

static int logfunnel_init (void)
{
	int ret;

	proc_entry = proc_mkdir ("log", NULL);
	proc_create ("status", 0440, proc_entry,
			&proc_status_fops);

	buffer = vmalloc (buffer_size);
	if (buffer == NULL)
	{
		printk (KERN_ERR "logfunnel: no memory.\n");
		logfunnel_exit ();
		return -ENOMEM;
	}

#ifdef USE_MISCDEVICE
	ret = misc_register (&logfunnel_dev);
	if (ret != 0)
	{
		printk (KERN_ERR "logfunnel: misc_register failed. %d\n", ret);
		return ret;
	}
	printk (KERN_DEBUG "logfunnel: misc minor %d\n", logfunnel_dev.minor);
#else
	ret = register_chrdev (logfunnel_major, "logfunnel", &logfunnel_fops);
	if (ret < 0)
		return ret;

	if (logfunnel_major == 0)
		logfunnel_major = ret;
	ret = 0;
	printk (KERN_DEBUG "logfunnel: major %d\n", logfunnel_major);
#endif

	return ret;
}

MODULE_LICENSE ("GPL");
#if defined(CONFIG_LG_BUILTIN_KDRIVER) && defined(CONFIG_LGSNAP)
user_initcall_grp("kdrv",logfunnel_init);
#else
module_init (logfunnel_init);
#endif
module_exit (logfunnel_exit);

