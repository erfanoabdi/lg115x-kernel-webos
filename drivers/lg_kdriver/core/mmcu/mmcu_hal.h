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
 * author     youngki.lyu@lge.com
 * version    0.1
 * date       2011.02.23
 * note       Additional information.
 *
 */

#ifndef _MMCU_HAL_
#define _MMCU_HAL_

#include <linux/kernel.h>
#include "base_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define MMCU_IRQ_NUM				(32+48)

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/
typedef enum{
	MCU_INTR_IPC	= 0,
	MCU_INTR_DMA	= 1,
} MCU_INTR_MODE;

typedef enum {
	MMCU_IPC_PIPECMD,
	MMCU_IPC_SET_CMDPIPE,
	MMCU_IPC_SET_LOCKDB
} MMCU_IPC_CMD;

typedef void (*MMCU_HAL_ISR_FN)(UINT32, void*);

typedef struct {
	UINT32				ver;			// 0x0000 : ''
	UINT32				cmd;			// 0x0000 : ''
	UINT32				param0;			// 0x0000 : ''
	UINT32				param1;			// 0x0000 : ''
} reg_mcu_ipc_t;
/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Generic Usage Functions
----------------------------------------------------------------------------------------*/
void	MCU_HAL_Init(void);
void	MCU_HAL_SegRunStall(BOOLEAN bStall);
void	MCU_HAL_Reset(void);
UINT32	MCU_HAL_GetStatus(void);
void	MCU_HAL_InitSystemMemory(UINT32 ui32AddrSrom, UINT32 ui32AddrSram0, UINT32 ui32AddrSram1, UINT32 ui32AddrSram2);
void	MCU_HAL_SetExtIntrEvent(void);
void	MCU_HAL_EnableExtIntr(MCU_INTR_MODE eMode);
void	MCU_HAL_DisableExtIntr(MCU_INTR_MODE eMode);
void	MCU_HAL_ClearExtIntr(MCU_INTR_MODE eMode);
void	MCU_HAL_DisableExtIntrAll(void);
BOOLEAN	MCU_HAL_IsExtIntrEnable(MCU_INTR_MODE eMode);
BOOLEAN	MCU_HAL_GetExtIntrStatus(MCU_INTR_MODE eMode);
void	MCU_HAL_SetExtIntrStatus(MCU_INTR_MODE eMode, BOOLEAN bValue);
void	MCU_HAL_SetIntrEvent(void);
void	MCU_HAL_EnableInterIntr(MCU_INTR_MODE eMode);
void	MCU_HAL_DisableInterIntr(MCU_INTR_MODE eMode);
void	MCU_HAL_ClearInterIntr(MCU_INTR_MODE eMode);
void	MCU_HAL_DisableInterIntrAll(void);
BOOLEAN	MCU_HAL_IsInterIntrEnable(MCU_INTR_MODE eMode);
BOOLEAN	MCU_HAL_GetInterIntrStatus(MCU_INTR_MODE eMode);
void	MCU_HAL_SetInterIntrStatus(MCU_INTR_MODE eMode, BOOLEAN bValue);
UINT32	MCU_HAL_GetVersion(void);
void* MCU_HAL_GetIpcData(void);
void	MCU_HAL_CodeDown(UINT32 SROMAddr, UINT32 SROMSize, UINT8 *mcu_code, UINT32 mcu_code_size);
#ifdef __cplusplus
}
#endif

#endif // endif of _MMCU_HAL_

