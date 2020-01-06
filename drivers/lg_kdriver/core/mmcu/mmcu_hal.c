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
 * author     sooya.joo@lge.com
 * version    0.1
 * date       2010.03.11
 * note       Additional information.
 *
 * @addtogroup lg1152_vdec
 * @{
 */

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <linux/io.h>
#include "os_util.h"
#include "mmcu_reg.h"
#include "mmcu_hal.h"

#include "log.h"

#if 0
#include "base_types.h"
#include "../../../mcu/os_adap.h"
#include "../lg1154_vdec_base.h"
#include "../mcu_hal_api.h"
#include "mcu_reg.h"

#endif
/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/
#define	H13_VDEC_BASE				(0xC0000000 + 0x004000)
#define MMCU_REG_BASE				(H13_VDEC_BASE + 0x0600)
#define MMCU_IPC_REG_BASE			(H13_VDEC_BASE + 0x0800)
#if 0
/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Functions
 *---------------------------------------------------------------------------------------*/

#endif
/*----------------------------------------------------------------------------------------
 *   global Variables
 *---------------------------------------------------------------------------------------*/
//logm_define(mmcu_hal, log_level_warning);
logm_define(mmcu_hal, log_level_info);

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/
static volatile reg_mcu_top_t*	_gpstRegMcuTop;
volatile UINT32*		_gpstRegIpcMem;
volatile reg_mcu_ipc_t*	_gpstRegMcuIpc;
static MMCU_HAL_ISR_FN gfpMcuIsr = NULL;
/*========================================================================================
	Implementation Group
========================================================================================*/
/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
void MCU_HAL_Init(void)
{
	UINT32			chip_rev;

	_gpstRegMcuIpc = (volatile reg_mcu_ipc_t *)ioremap(MMCU_IPC_REG_BASE, 0x100);
	_gpstRegIpcMem = (volatile UINT32*)_gpstRegMcuIpc + sizeof(reg_mcu_ipc_t)/4;

	_gpstRegMcuTop = (volatile reg_mcu_top_t *)ioremap(MMCU_REG_BASE, 0x800);

	chip_rev = lx_chip_rev();
	*_gpstRegIpcMem = chip_rev;	
}

void MCU_HAL_InitSystemMemory(UINT32 ui32AddrSrom, UINT32 ui32AddrSram0, UINT32 ui32AddrSram1, UINT32 ui32AddrSram2)
{
	_gpstRegMcuTop->srom_offset.srom_offset = ui32AddrSrom;
	_gpstRegMcuTop->sram_offset_0.sram_offset_0 = ui32AddrSram0;
	_gpstRegMcuTop->sram_offset_1.sram_offset_1 = ui32AddrSram1;
	_gpstRegMcuTop->sram_offset_2.sram_offset_2 = ui32AddrSram2;
}

void MCU_HAL_Reset(void)
{
	_gpstRegMcuTop->proc_ctrl.runstall = 1;
	_gpstRegMcuTop->proc_ctrl.sw_reset = 1;

	while( _gpstRegMcuTop->proc_ctrl.sw_reset == 1 ){}

	_gpstRegMcuTop->proc_ctrl.runstall = 0;
}

void* MCU_HAL_GetIpcData(void)
{
	return _gpstRegMcuIpc;
}

void MCU_HAL_CodeDown(UINT32 ui32SromAddr, UINT32 ui32SromSize, UINT8* pui8McuCode, UINT32 ui32McuCodeSize)
{
	UINT8*	pui8RegPtr;
	UINT32	ui32FwVirAddr, ui32Count;

	ui32FwVirAddr = (UINT32)ioremap(ui32SromAddr, ui32SromSize);

	logm_noti(mmcu_hal, "Vdec MCU base address-virt[0x%08X]\n", ui32SromAddr);

	_gpstRegMcuTop->proc_ctrl.runstall = 1;

	//	VDEC MCU FirmwareDown
	pui8RegPtr = (UINT8 *)ui32FwVirAddr;
	for( ui32Count = 0; ui32Count < ui32McuCodeSize; ++ui32Count )
	{
		*pui8RegPtr = pui8McuCode[ui32Count];
		pui8RegPtr++;
	}

	MCU_HAL_Reset();

	iounmap((void *)ui32FwVirAddr);
	logm_noti(mmcu_hal, "VDEC MCU is Loaded\n");
}



void	MCU_HAL_SegRunStall(BOOLEAN bStall)
{
	_gpstRegMcuTop->proc_ctrl.runstall = !!bStall;
}


UINT32	MCU_HAL_GetStatus(void)
{
	UINT32	ui32McuStatus = 0;

	_gpstRegMcuTop->proc_ctrl.pdbg_en = 1;
	ui32McuStatus = _gpstRegMcuTop->pdbg_data.pdbg_data;
	_gpstRegMcuTop->proc_ctrl.pdbg_en = 0;

	return ui32McuStatus;
}

void	MCU_HAL_SetExtIntrEvent(void)
{
	_gpstRegMcuTop->e_intr_ev.ipc = 1;
}

void	MCU_HAL_EnableExtIntr(MCU_INTR_MODE eMode)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->e_intr_en.ipc = 1;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->e_intr_en.dma = 1;
		break;

	default:
		break;
	}
}

void	MCU_HAL_DisableExtIntr(MCU_INTR_MODE eMode)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->e_intr_en.ipc = 0;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->e_intr_en.dma = 0;
		break;

	default:
		break;
	}
}

void	MCU_HAL_ClearExtIntr(MCU_INTR_MODE eMode)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->e_intr_cl.ipc = 1;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->e_intr_cl.dma = 1;
		break;

	default:
		break;
	}
}

void	MCU_HAL_DisableExtIntrAll(void)
{
	_gpstRegMcuTop->e_intr_en.ipc = 0;
	_gpstRegMcuTop->e_intr_en.dma = 0;
	_gpstRegMcuTop->e_intr_cl.ipc = 1;
	_gpstRegMcuTop->e_intr_cl.dma = 1;
}

BOOLEAN	MCU_HAL_IsExtIntrEnable(MCU_INTR_MODE eMode)
{
	BOOLEAN bIntrEnabled = 0;

	switch( eMode )
	{
	case MCU_INTR_IPC:
		bIntrEnabled = !!_gpstRegMcuTop->e_intr_en.ipc;
		break;

	case MCU_INTR_DMA:
		bIntrEnabled = !!_gpstRegMcuTop->e_intr_en.dma;
		break;

	default:
		break;
	}

	return bIntrEnabled;
}

BOOLEAN	MCU_HAL_GetExtIntrStatus(MCU_INTR_MODE eMode)
{
	BOOLEAN bIntrStatus = 0;

	switch( eMode )
	{
	case MCU_INTR_IPC:
		bIntrStatus = !!_gpstRegMcuTop->e_intr_st.ipc;
		break;

	case MCU_INTR_DMA:
		bIntrStatus = !!_gpstRegMcuTop->e_intr_st.dma;
		break;

	default:
		break;
	}

	return bIntrStatus;
}

void	MCU_HAL_SetExtIntrStatus(MCU_INTR_MODE eMode, BOOLEAN bValue)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->e_intr_st.ipc = !!bValue;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->e_intr_st.dma = !!bValue;
		break;

	default:
		break;
	}
}

void	MCU_HAL_SetIntrEvent(void)
{
	_gpstRegMcuTop->i_intr_st.ipc = 1;
}

void	MCU_HAL_EnableInterIntr(MCU_INTR_MODE eMode)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->i_intr_en.ipc = 1;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->i_intr_en.dma = 1;
		break;

	default:
		break;
	}
}

void	MCU_HAL_DisableInterIntr(MCU_INTR_MODE eMode)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->i_intr_en.ipc = 0;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->i_intr_en.dma = 0;
		break;

	default:
		break;
	}
}

void	MCU_HAL_ClearInterIntr(MCU_INTR_MODE eMode)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->i_intr_cl.ipc = 1;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->i_intr_cl.dma = 1;
		break;

	default:
		break;
	}
}

void	MCU_HAL_DisableInterIntrAll(void)
{
	_gpstRegMcuTop->i_intr_en.ipc = 0;
	_gpstRegMcuTop->i_intr_en.dma = 0;
	_gpstRegMcuTop->i_intr_cl.ipc = 1;
	_gpstRegMcuTop->i_intr_cl.dma = 1;
}

BOOLEAN	MCU_HAL_IsInterIntrEnable(MCU_INTR_MODE eMode)
{
	BOOLEAN bIntrEnabled = 0;

	switch( eMode )
	{
	case MCU_INTR_IPC:
		bIntrEnabled = !!_gpstRegMcuTop->i_intr_en.ipc;
		break;

	case MCU_INTR_DMA:
		bIntrEnabled = !!_gpstRegMcuTop->i_intr_en.dma;
		break;

	default:
		break;
	}

	return bIntrEnabled;
}

BOOLEAN	MCU_HAL_GetInterIntrStatus(MCU_INTR_MODE eMode)
{
	BOOLEAN bIntrStatus = 0;

	switch( eMode )
	{
	case MCU_INTR_IPC:
		bIntrStatus = !!_gpstRegMcuTop->i_intr_st.ipc;
		break;

	case MCU_INTR_DMA:
		bIntrStatus = !!_gpstRegMcuTop->i_intr_st.dma;
		break;

	default:
		break;
	}

	return bIntrStatus;
}

void	MCU_HAL_SetInterIntrStatus(MCU_INTR_MODE eMode, BOOLEAN bValue)
{
	switch( eMode )
	{
	case MCU_INTR_IPC:
		_gpstRegMcuTop->i_intr_st.ipc = !!bValue;
		break;

	case MCU_INTR_DMA:
		_gpstRegMcuTop->i_intr_st.dma = !!bValue;
		break;

	default:
		break;
	}
}

UINT32	MCU_HAL_GetVersion(void)
{
	return _gpstRegMcuTop->dec_mcu_ver.yyyymmdd;
}

/** @} */

