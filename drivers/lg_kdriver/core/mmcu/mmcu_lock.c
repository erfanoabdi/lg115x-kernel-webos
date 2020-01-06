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
#ifdef __XTENSA__
#define mb()
#else
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/string.h>

#include <linux/delay.h>
#endif

#include "hma_alloc.h"
#include "mmcu_hal.h"
#include "mmcu_lock.h"

#include "log.h"

logm_define (mmcu_lock, log_level_warning);
/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/
#define MLOCK_MAX_LOCK				(4)
#define MLOCK_MAX_NAME_LEN			(19)

struct _MMCU_LOCK_T{
	volatile char		szName[MLOCK_MAX_NAME_LEN+1];
	volatile BOOLEAN		bValid;
	volatile unsigned int flag[2];   // 0: for ARM, 1: for MCU
	volatile unsigned int turn;
};

static MMCU_LOCK_T *astLock;

extern volatile reg_mcu_ipc_t*	_gpstRegMcuIpc;
void _MMCU_SetLockDbThruIpc(UINT8 *buf, UINT32 u32BufSize)
{
	// Send info thru IPC
	_gpstRegMcuIpc->cmd = MMCU_IPC_SET_LOCKDB;
	_gpstRegMcuIpc->param0 = (UINT32)buf;
	_gpstRegMcuIpc->param1 = u32BufSize;

	MCU_HAL_SetInterIntrStatus(MCU_INTR_IPC, TRUE);
	MCU_HAL_SetExtIntrEvent();
}

#define MLOCK_DBBUF_SIZE	(sizeof(MMCU_LOCK_T)*MLOCK_MAX_LOCK)
void MLOCK_Init(UINT8 *buf, UINT32 u32BufSize)
{
	UINT32 i;
	MMCU_LOCK_T *pstLock;

	if (buf==NULL)
	{
		buf = (UINT8*)hma_alloc_user("mmcu", MLOCK_DBBUF_SIZE,
				1 << 4, "mmcu_lock");
		u32BufSize = MLOCK_DBBUF_SIZE;

		if (buf==NULL)
		{
			logm_error(mmcu_lock, "No HMA mem for mmcu lock sys!!!\n");
		}
	}

	astLock = (MMCU_LOCK_T*)ioremap((UINT32)buf, u32BufSize);

	printk("Init MMCU Lock \n");

	for( i=0; i<MLOCK_MAX_LOCK; i++ )
	{
		pstLock = &astLock[i];
		pstLock->bValid = FALSE;
		pstLock->flag[0] = FALSE;
		pstLock->flag[1] = FALSE;
	}

	_MMCU_SetLockDbThruIpc(buf, u32BufSize);

	MLOCK_MakeLock("vdo_lock");
}

MMCU_LOCK_T* MLOCK_MakeLock(CHAR *szLockName)
{
	UINT32 i;
	MMCU_LOCK_T *pstLock, *pstRet = NULL;
	char		szName[MLOCK_MAX_NAME_LEN+1];

	strncpy(szName, szLockName, MLOCK_MAX_NAME_LEN);
	szName[MLOCK_MAX_NAME_LEN] = '\0';

	for(i=0; i<MLOCK_MAX_LOCK; i++)
	{
		pstLock = &astLock[i];
		if( pstLock->bValid )
		{
			if( !strcmp((char*)pstLock->szName, szName) )
			{
				printk("already exist lock name '%s'\n", szName);

				pstRet = pstLock;
				break;
			}
		}
		else
		{
			if (pstRet == NULL)
				pstRet = pstLock;
		}
	}

	if( pstRet==NULL )
	{
		printk("not enough lock slot\n");
		return NULL;
	}

	pstRet->bValid = TRUE;
	strcpy((char*)pstRet->szName, szName);

	return pstRet;
}

MMCU_LOCK_T* MLOCK_Open(CHAR *szLockName)
{
	UINT32 i;
	MMCU_LOCK_T *pstLock = NULL;

	for(i=0; i<MLOCK_MAX_LOCK; i++)
	{
		pstLock = &astLock[i];
		if( pstLock->bValid )
		{
			if( !strcmp((char*)pstLock->szName, szLockName) )
				break;
		}
	}

	if (i == MLOCK_MAX_LOCK)
	{
		logm_error(mmcu_lock, "can't find '%s' lock\n", szLockName);
		pstLock = NULL;
	}

	return pstLock;
}

UINT32 u32nLockWait=0;
#ifdef __XTENSA__
#define CUR_CPU_NUM		1
#define EXT_CPU_NUM		0
#else
#define CUR_CPU_NUM		0
#define EXT_CPU_NUM		1
#endif
void MLOCK_Lock(MMCU_LOCK_T *pstLock)
{
	BOOLEAN bWait = FALSE;

	pstLock->flag[CUR_CPU_NUM] = TRUE;
	mb();
	pstLock->turn = 0;
	mb();
	while(pstLock->flag[EXT_CPU_NUM] && pstLock->turn == 0)
	{
		bWait = TRUE;
	}

	mb();
	if(bWait)
		u32nLockWait++;
}

void MLOCK_Unlock(MMCU_LOCK_T *pstLock)
{
	mb();
	pstLock->flag[CUR_CPU_NUM] = FALSE;
}
