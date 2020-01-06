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

#ifndef _HMA_H
#define _HMA_H

#define HMA_POOL_LEN	16

#ifdef __KERNEL__

int hma_pool_register (const char *name, unsigned long start, int size);
void hma_pool_unregister (const char *name);
int hma_pool_info (const char *name, unsigned long *start, int *size);

unsigned long hma_alloc_prefered_user (const char *poolname,
		int size, unsigned long prefered, const char *user);
unsigned long hma_alloc_prefered (const char *poolname,
		int size, unsigned long prefered);

unsigned long hma_alloc_user (const char *poolname,
		int size, int align, const char *user);
unsigned long hma_alloc (const char *poolname, int size, int align);
void hma_free (const char *poolname, unsigned long addr);

int hma_init (void);
void hma_exit (void);

#endif

#endif
