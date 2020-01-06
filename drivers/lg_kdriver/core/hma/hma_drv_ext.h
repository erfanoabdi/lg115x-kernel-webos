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

#ifndef _HMA_DRV_EXT_H
#define _HMA_DRV_EXT_H

#include <linux/fs.h>

#include "hma_alloc.h"

typedef int (*hma_drv_hook_open_t) (struct inode *, struct file *);
typedef long (*hma_drv_hook_ioctl_t) (struct file *, unsigned int cmd, unsigned long arg);
typedef int (*hma_drv_hook_release_t) (struct inode *, struct file *);

extern hma_drv_hook_open_t hma_drv_hook_open;
extern hma_drv_hook_ioctl_t hma_drv_hook_ioctl;
extern hma_drv_hook_release_t hma_drv_hook_release;
extern int hma_drv_priv_size;

struct hma_drv_mem_desc
{
	unsigned long paddr;
	void *vaddr;
	int size;
	struct vm_area_struct *vma;
	char poolname[HMA_POOL_LEN];
	int cached;
};

#endif
