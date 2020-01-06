
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/io.h>

#include "decoder.h"
#include "log.h"

logm_define (mmcu_proc, log_level_warning);

extern void MMCU_DebugCmd(UINT32 u32nParam, CHAR *apParams[]);
static ssize_t mmcu_proc_write (struct file *file,
		const char __user *buf, size_t size, loff_t *off)
{
	unsigned int n=0, type;
	char tmp[64], *t, *param[10];

	if (size == 0)
		return 0;

	/* we only allow single write */
	if (*off != 0)
		return -EINVAL;

	if (size > sizeof(tmp)-1)
	{
		logm_warning (mmcu_proc, "too long string.\n");
		return -EINVAL;
	}
	if (copy_from_user (tmp, buf, sizeof(tmp)) > 0)
		return -EFAULT;

	tmp[sizeof(tmp)-1] = '\0';
	t = tmp;
	while( t != NULL && n < ARRAY_SIZE(param) )
		param[n++] = strsep(&t, " \t");

	MMCU_DebugCmd(n, param);
	
	return size;
}

static int mmcu_proc_show (struct seq_file *m, void *data)
{
	//int i;

	//for (i=0; i<32; i++)
	//	if( log_cmd[i].set_level != NULL )
	//		seq_printf (m, "%2d : %s\n", i, log_cmd[i].name );

	return 0;
}

static int mmcu_proc_open (struct inode *inode, struct file *file)
{
	return single_open (file, mmcu_proc_show, NULL);;
}

static int mmcu_proc_release (struct inode *inode, struct file *file)
{
	return single_release (inode, file);
}

static struct file_operations mmcu_proc_fops =
{
	.open = mmcu_proc_open,
	.read = seq_read,
	.write = mmcu_proc_write,
	.llseek = seq_lseek,
	.release = mmcu_proc_release,
};

struct proc_dir_entry *mmcu_proc_root_entry;

int mmcu_proc_init(void)
{
	mmcu_proc_root_entry = proc_mkdir ("mmcu", NULL);

	proc_create ("debug", 0640, mmcu_proc_root_entry, &mmcu_proc_fops);

	return 0;
}

void mmcu_proc_cleanup (void)
{
	remove_proc_entry ("debug", mmcu_proc_root_entry);
}

