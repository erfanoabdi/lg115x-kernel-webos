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

#ifndef _MMCU_PIPE_H_
#define _MMCU_PIPE_H_

#include "base_types.h"

#define MPIPE_MAX_NAME_LEN			(19)

typedef struct _MMCU_PIPE_T MMCU_PIPE_T;

typedef void (*MPIPE_CALLBACK_FN)(MMCU_PIPE_T*);
struct _MMCU_PIPE_T {
	UINT32		u32Num;
	CHAR		szName[MPIPE_MAX_NAME_LEN+1];
	BOOLEAN		bValid;
	UINT8		*pu8BufBase;
	UINT32		u32BufSize;
	volatile UINT32	*pu32WrIdx;
	volatile UINT32	*pu32RdIdx;
	MPIPE_CALLBACK_FN	pfnCallBack;
};

#define MMCU_PIPE_ENTRY_SZ	(sizeof(MMCU_PIPE_ENTRY_T))

void MPIPE_Init(void);
SINT32 MPIPE_Isr(void *data);

#ifdef INCLUDE_KDRV_MMCU
UINT32 MPIPE_RegisterCallback(MMCU_PIPE_T *pstPipe, MPIPE_CALLBACK_FN pfCallBack);
MMCU_PIPE_T* MPIPE_MakePipe(CHAR *szPipeName, UINT8 *buf, UINT32 u32BufSize);
MMCU_PIPE_T* MPIPE_Open(CHAR *szPipeName, UINT32 u32Op);

UINT32 MPIPE_Write(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize);
UINT32 MPIPE_Read(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize);
UINT32 MPIPE_GetUsedSize(MMCU_PIPE_T *pstPipe);
UINT32 MPIPE_GetEmptySize(MMCU_PIPE_T *pstPipe);
void MPIPE_Flush(MMCU_PIPE_T *pstPipe);
#else
UINT32 MPIPE_RegisterCallback(MMCU_PIPE_T *pstPipe, MPIPE_CALLBACK_FN pfCallBack)
{
	return 0;
}
MMCU_PIPE_T* MPIPE_MakePipe(CHAR *szPipeName, UINT8 *buf, UINT32 u32BufSize)
{
	return NULL;
}
MMCU_PIPE_T* MPIPE_Open(CHAR *szPipeName, UINT32 u32Op)
{
	return NULL;
}
UINT32 MPIPE_Write(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize)
{
	return 0;
}
UINT32 MPIPE_Read(MMCU_PIPE_T *pstPipe, UINT8 *pu8Data, UINT32 u32DataSize)
{
	return 0;
}
UINT32 MPIPE_GetUsedSize(MMCU_PIPE_T *pstPipe)
{
	return 0;
}
UINT32 MPIPE_GetEmptySize(MMCU_PIPE_T *pstPipe)
{
	return 0;
}
void MPIPE_Flush(MMCU_PIPE_T *pstPipe)
{
	return;
}
#endif



#endif //_MMCU_PIPE_H_

