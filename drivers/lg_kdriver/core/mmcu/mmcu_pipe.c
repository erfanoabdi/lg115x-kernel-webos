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
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/string.h>

#include <linux/delay.h>

#include "mmcu_hal.h"
#include "mmcu_pipe.h"

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/
#define MPIPE_MAX_PIPE				(10)
#define LOG_NAME					mmcu_pipe

typedef struct {
	CHAR		szName[MPIPE_MAX_NAME_LEN+1];
	UINT8 *pu8BufBasePhs;
	UINT32 u32BufSize;
	UINT32 u32WrIdx;
	UINT32 u32RdIdx;
} MMCU_PIPE_INFO_T;


static MMCU_PIPE_T astPipeList[MPIPE_MAX_PIPE];
extern volatile reg_mcu_ipc_t*	_gpstRegMcuIpc;

void MPIPE_Flush(MMCU_PIPE_T *pstPipe)
{
	_gpstRegMcuIpc->cmd = MMCU_IPC_PIPECMD;
	_gpstRegMcuIpc->param0 = pstPipe->u32Num;
	MCU_HAL_SetInterIntrStatus(MCU_INTR_IPC, TRUE);
	MCU_HAL_SetExtIntrEvent();
}

void _MMCU_SetPipeThruIpc(MMCU_PIPE_T *pstPipe, UINT8 *buf, UINT32 u32BufSize)
{ 
	UINT8 *pu8PipeInfoAddrPhs;
	volatile MMCU_PIPE_INFO_T *pstPipeInfo;

	pu8PipeInfoAddrPhs = buf;
	buf += sizeof(MMCU_PIPE_INFO_T);
	u32BufSize -= sizeof(MMCU_PIPE_INFO_T);

	pstPipeInfo = (volatile MMCU_PIPE_INFO_T *)ioremap((UINT32)pu8PipeInfoAddrPhs,
				sizeof(MMCU_PIPE_INFO_T));

	// shared mem info ////
	strncpy((CHAR*)pstPipeInfo->szName, pstPipe->szName, MPIPE_MAX_NAME_LEN);
	pstPipeInfo->szName[MPIPE_MAX_NAME_LEN] = '\0';
	pstPipeInfo->pu8BufBasePhs = buf;
	pstPipeInfo->u32BufSize = u32BufSize;
	pstPipeInfo->u32WrIdx = 0;
	pstPipeInfo->u32RdIdx = 0;
	///////////////////////

	pstPipe->pu8BufBase = (UINT8*)ioremap((UINT32)buf, u32BufSize);
	pstPipe->u32BufSize = u32BufSize;
	pstPipe->pu32WrIdx = &pstPipeInfo->u32WrIdx;
	pstPipe->pu32RdIdx = &pstPipeInfo->u32RdIdx;

	// Send info thru IPC
	_gpstRegMcuIpc->cmd = MMCU_IPC_SET_CMDPIPE;
	_gpstRegMcuIpc->param0 = (UINT32)pu8PipeInfoAddrPhs;
	_gpstRegMcuIpc->param1 = sizeof(MMCU_PIPE_INFO_T);

	MCU_HAL_SetInterIntrStatus(MCU_INTR_IPC, TRUE);
	MCU_HAL_SetExtIntrEvent();
}


void MPIPE_Init(void)
{
	UINT32 i;

	printk("Init MMCU Pipe\n");

	for( i=0; i<MPIPE_MAX_PIPE; i++ )
	{
		astPipeList[i].bValid = FALSE;
		astPipeList[i].u32Num = i;
	}
}


MMCU_PIPE_T* MPIPE_MakePipe(CHAR *szPipeName, UINT8 *buf, UINT32 u32BufSize)
{
	UINT32 i;
	MMCU_PIPE_T *pstPipe = NULL;
	char	szName[MPIPE_MAX_NAME_LEN+1];
	
	strncpy(szName, szPipeName, MPIPE_MAX_NAME_LEN);
	szName[MPIPE_MAX_NAME_LEN] = '\0';

	for(i=0; i<MPIPE_MAX_PIPE; i++)
	{
		if( astPipeList[i].bValid )
		{
			if( !strcmp(astPipeList[i].szName, szName) )
			{
				printk("already exist pipe name '%s'\n", szName);

				return &astPipeList[i];
			}
		}
		else
		{
			if( pstPipe == NULL )
				pstPipe = &astPipeList[i];
		}
	}

	if( pstPipe == NULL )
	{
		printk("not enough pipe slot\n");
		return NULL;
	}

	pstPipe->bValid = TRUE;
	strcpy(pstPipe->szName, szName);
	_MMCU_SetPipeThruIpc(pstPipe, buf, u32BufSize);

	return pstPipe;
}

MMCU_PIPE_T* MPIPE_Open(CHAR *szPipeName, UINT32 u32Op)
{
	UINT32 i;
	MMCU_PIPE_T *pstPipe = NULL;

	for(i=0; i<MPIPE_MAX_PIPE; i++)
	{
		if( astPipeList[i].bValid )
		{
			if( !strcmp(astPipeList[i].szName, szPipeName) )
			{
				pstPipe = &astPipeList[i];
				break;
			}
		}
	}

	if( i == MPIPE_MAX_PIPE )
		printk("can't find '%s' pipe\n", szPipeName);

	return pstPipe;
}

void _mmcu_pipe_enqueue(volatile MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize)
{
	if( *(pstPipe->pu32WrIdx) + u32DataSize >= pstPipe->u32BufSize )
	{
		UINT32 u32ChunkSize;

		u32ChunkSize = pstPipe->u32BufSize - *(pstPipe->pu32WrIdx);

		memcpy(pstPipe->pu8BufBase + *(pstPipe->pu32WrIdx), pu8Data, u32ChunkSize);
		*(pstPipe->pu32WrIdx) = 0;
		pu8Data += u32ChunkSize;
		u32DataSize -= u32ChunkSize;
	}

	memcpy(pstPipe->pu8BufBase + *(pstPipe->pu32WrIdx), pu8Data, u32DataSize);
	*(pstPipe->pu32WrIdx) += u32DataSize;

	return;
}

BOOLEAN _mmcu_pipe_dequeue(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize)
{
	UINT32 u32UsedSize;
	// size check
	if( *(pstPipe->pu32WrIdx) >=  *(pstPipe->pu32RdIdx) )
	{
		u32UsedSize = *(pstPipe->pu32WrIdx) - *(pstPipe->pu32RdIdx);
	}
	else
	{
		u32UsedSize = *(pstPipe->pu32WrIdx) + pstPipe->u32BufSize - *(pstPipe->pu32RdIdx);
	}

	if( u32UsedSize < u32DataSize )
	{
		printk("Not enough data, req %d  remained %d (%d/%d)\n",
				u32DataSize, u32UsedSize, *(pstPipe->pu32WrIdx), *(pstPipe->pu32RdIdx));
		return FALSE;
	}

	if( *(pstPipe->pu32RdIdx) + u32DataSize >= pstPipe->u32BufSize )
	{
		UINT32 u32ChunkSize;

		u32ChunkSize = pstPipe->u32BufSize - *(pstPipe->pu32RdIdx);

		memcpy(pu8Data, pstPipe->pu8BufBase + *(pstPipe->pu32RdIdx), u32ChunkSize);
		*(pstPipe->pu32RdIdx) = 0;
		pu8Data += u32ChunkSize;
		u32DataSize -= u32ChunkSize;
	}

	memcpy(pu8Data, pstPipe->pu8BufBase + *(pstPipe->pu32RdIdx), u32DataSize);

	*(pstPipe->pu32RdIdx) += u32DataSize;
	if( *(pstPipe->pu32RdIdx) >= pstPipe->u32BufSize )
		*(pstPipe->pu32RdIdx) -= pstPipe->u32BufSize;

	return TRUE;
}

UINT32 MPIPE_Write(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize)
{
	_mmcu_pipe_enqueue(pstPipe, pu8Data, u32DataSize);

	return 0;
}

UINT32 MPIPE_Read(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize)
{
	_mmcu_pipe_dequeue(pstPipe, pu8Data, u32DataSize);

	return 0;
}

UINT32 MPIPE_GetUsedSize(MMCU_PIPE_T *pstPipe)
{
	UINT32 u32UsedSize;

	// size check
	if( *(pstPipe->pu32WrIdx) >=  *(pstPipe->pu32RdIdx) )
	{
		u32UsedSize = *(pstPipe->pu32WrIdx) - *(pstPipe->pu32RdIdx);
	}
	else
	{
		u32UsedSize = *(pstPipe->pu32WrIdx) + pstPipe->u32BufSize - *(pstPipe->pu32RdIdx);
	}

	return u32UsedSize;
}

UINT32 MPIPE_GetEmptySize(MMCU_PIPE_T *pstPipe)
{
	UINT32 u32EmptySize;

	u32EmptySize = pstPipe->u32BufSize - MPIPE_GetUsedSize(pstPipe);

	return u32EmptySize;
}

UINT32 MPIPE_RegisterCallback(MMCU_PIPE_T *pstPipe, MPIPE_CALLBACK_FN pfCallBack)
{
	pstPipe->pfnCallBack = pfCallBack;

	return 0;
}

SINT32 MPIPE_Isr(void *data)
{
	UINT32 u32PipeNo;

	u32PipeNo = _gpstRegMcuIpc->param0;

	if( !astPipeList[u32PipeNo].bValid )
	{
		return -1;
	}

	if( astPipeList[u32PipeNo].pfnCallBack != NULL )
		astPipeList[u32PipeNo].pfnCallBack(&astPipeList[u32PipeNo]);

	return 0;
}
