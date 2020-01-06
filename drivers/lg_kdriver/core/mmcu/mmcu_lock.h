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


/** @file
 *
 * main driver implementation for de device.
 * de device will teach you how to make device driver with new platform.
 *
 * author     taewoong.kim (taewoong.kim@lge.com)
 * version    1.0
 * date       2013.05.08
 * note       Additional information.
 *
 * @addtogroup lg115x_mmcu
 * @{
 */

#ifndef _MMCU_LOCK_H_
#define _MMCU_LOCK_H_

#include "base_types.h"

typedef struct _MMCU_LOCK_T MMCU_LOCK_T;

#ifdef INCLUDE_KDRV_MMCU
void MLOCK_Init(UINT8 *buf, UINT32 u32BufSize);
MMCU_LOCK_T* MLOCK_MakeLock(CHAR *szLockName);
MMCU_LOCK_T* MLOCK_Open(CHAR *szLockName);

void MLOCK_Lock(MMCU_LOCK_T *pstLock);
void MLOCK_Unlock(MMCU_LOCK_T *pstLock);

#define mcu_lock_open(name)	MLOCK_Open(name)
#define mcu_lock(lock)		MLOCK_Lock(lock)
#define mcu_unlock(lock)	MLOCK_Unlock(lock)

#else
MMCU_LOCK_T* MLOCK_MakeLock(CHAR *szLockName)
{
	return NULL;
}
MMCU_LOCK_T* MLOCK_Open(CHAR *szLockName)
{
	return NULL;
}
void MLOCK_Lock(MMCU_LOCK_T *pstLock)
{
	return;
}
void MLOCK_Unlock(MMCU_LOCK_T *pstLock)
{
	return;
}

#define mcu_lock_open(name)	NULL
#define mcu_lock(lock)
#define mcu_unlock(lock)
#endif

#endif //_MMCU_LOCK_H_

