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
 * author     youngki.lyu (youngki.lyu@lge.com)
 * version    1.0
 * date       2010.05.08
 * note       Additional information.
 *
 * @addtogroup lg1152_vdec
 * @{
 */

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/
#define		IPC_CMD_BUF_SIZE			0x1000

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include "ipc_cmd.h"
#include "../hal/lg1152/ipc_reg_api.h"
#include "vdec_shm.h"

#ifndef __XTENSA__
#include <linux/kernel.h>
#include <asm/string.h>	// memset
#endif

#include "os_adap.h"

#include "../hal/lg1152/top_hal_api.h"	// for IPC Interrupt
#include "../hal/lg1152/lq_hal_api.h"	// for IPC Interrupt

#include "../hal/lg1152/mcu_hal_api.h"
#include "vdec_print.h"


/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/
#define		IPC_CMD_MAGIC_CODE		0x19C0C3D0
#define		IPC_CMD_MAGIC_CLEAR		0x0D3C0C91
#define		IPC_CMD_PADDING_BITS	0xC3DF9ADE

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/
#define IPC_ISR_ID 		(LQC_NUM_OF_CHANNEL-1)

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Functions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/
static struct
{
	UINT32 		ui32PhyBaseAddr;	// constant
	UINT32 		*pui32VirBasePtr;	// constant
	UINT32 		ui32BufSize;			// constant
}gsIpcCmd;

#ifdef __XTENSA__
static IPC_CMD_RECEIVER_CALLBACK_T 		fpIpcCmd_Receiver[IPC_CMD_ID_MAX];
#endif


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
void IPC_CMD_Init(void)
{
	UINT32	ui32MemPtr;
	UINT32	ui32MemSize;

	ui32MemPtr = VDEC_SHM_Malloc(IPC_CMD_BUF_SIZE);
	ui32MemSize = IPC_CMD_BUF_SIZE;

	IPC_REG_CMD_SetBaseAddr(ui32MemPtr);
	IPC_REG_CMD_SetBufSize(ui32MemSize);
	IPC_REG_CMD_SetWrOffset(0);
	IPC_REG_CMD_SetRdOffset(0);
	VDEC_KDRV_Message(_TRACE, "[IPC][CMD] Base:0x%X, Size: 0x%X, %s", ui32MemPtr, ui32MemSize, __FUNCTION__ );

	gsIpcCmd.ui32PhyBaseAddr = ui32MemPtr;
	gsIpcCmd.ui32BufSize = ui32MemSize;
	gsIpcCmd.pui32VirBasePtr = VDEC_TranselateVirualAddr(ui32MemPtr, ui32MemSize);
#ifndef __XTENSA__
	memset(gsIpcCmd.pui32VirBasePtr , 0x0, ui32MemSize);
#endif

	LQC_HAL_Init(IPC_ISR_ID);
	LQ_HAL_Enable(IPC_ISR_ID);
}

#ifndef __XTENSA__
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
static BOOLEAN _IPC_CMD_CheckBelongToAddress(UINT32 ui32StartAddr, UINT32 ui32EndAddr, UINT32 ui32TargetAddr)
{
	if( ui32StartAddr <= ui32EndAddr )
	{
		if( (ui32TargetAddr > ui32StartAddr) &&
			(ui32TargetAddr <= ui32EndAddr) )
			return TRUE;
	}
	else
	{
		if( (ui32TargetAddr > ui32StartAddr) ||
			(ui32TargetAddr <= ui32EndAddr) )
			return TRUE;
	}

	return FALSE;
}

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
static UINT32 _IPC_CMD_CheckOverflow(UINT32 ui32CmdSize)
{
	UINT32		ui32WrOffset, ui32WrOffset_Org;
	UINT32		ui32RdOffset;

	if( ui32CmdSize > gsIpcCmd.ui32BufSize )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Overflow - Too Big CMD Message Size: %d", ui32CmdSize );
		return IPC_REG_INVALID_OFFSET;
	}

	ui32WrOffset_Org = IPC_REG_CMD_GetWrOffset();
	ui32WrOffset = ui32WrOffset_Org;
	ui32RdOffset = IPC_REG_CMD_GetRdOffset();

	if( (ui32WrOffset + ui32CmdSize) >= gsIpcCmd.ui32BufSize )
	{
		VDEC_KDRV_Message(DBG_IPC, "[IPC][DBG][CMD] Write Wraparound - Wr:%d, Buf Size:%d", ui32WrOffset, gsIpcCmd.ui32BufSize );
		ui32WrOffset = 0;
	}

	if( _IPC_CMD_CheckBelongToAddress(ui32WrOffset_Org, ui32WrOffset + ui32CmdSize, ui32RdOffset) == TRUE )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Overflow - Write:0x%X, Size:0x%X, Read:0x%X", ui32WrOffset, ui32CmdSize, ui32RdOffset );
		return IPC_REG_INVALID_OFFSET;
	}

	if( ui32WrOffset != ui32WrOffset_Org )
	{
		UINT32	ui32WrIndex;

		VDEC_KDRV_Message(DBG_IPC, "[IPC][DBG][CMD] Padding - Wr:%d, Buf Size:%d", ui32WrOffset, gsIpcCmd.ui32BufSize );

		for( ui32WrIndex = ui32WrOffset_Org; ui32WrIndex < gsIpcCmd.ui32BufSize; ui32WrIndex += 4 )
			gsIpcCmd.pui32VirBasePtr[ui32WrIndex>>2] = IPC_CMD_PADDING_BITS;
	}

	return ui32WrOffset;
}

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
BOOLEAN IPC_CMD_Send(E_IPC_CMD_ID_T eIpcCmdId, UINT32 ui32BodySize, void *pIpcBody)
{
	UINT32		*pui32WrVirPtr;
	const UINT32	ui32CmdHeaderSize = 12;	// 4 * 3 : Header Size(MAGIC CODE + Header Type + Body Length)
	UINT32		ui32WrOffset, ui32WrOffset_Org;
	UINT32		ui32WriteConfirm;
	UINT32		ui32WriteFailRetryCount = 0;

	if( !gsIpcCmd.pui32VirBasePtr )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Not Initialised");
		return FALSE;
	}

	// 1. Check Buffer Overflow
	ui32WrOffset = _IPC_CMD_CheckOverflow(ui32CmdHeaderSize + ui32BodySize);
	if( ui32WrOffset == IPC_REG_INVALID_OFFSET )
		return FALSE;

	pui32WrVirPtr = gsIpcCmd.pui32VirBasePtr;

	ui32WrOffset_Org = ui32WrOffset;

IPC_CMD_Write_Retry :
	// 2. Write Header
	pui32WrVirPtr[ui32WrOffset>>2] = IPC_CMD_MAGIC_CODE;
	ui32WrOffset += 4;
	pui32WrVirPtr[ui32WrOffset>>2] = (UINT32)eIpcCmdId;
	ui32WrOffset += 4;

	// 3. Write Length
	pui32WrVirPtr[ui32WrOffset>>2] = ui32BodySize;
	ui32WrOffset += 4;

	// 4. Write Body
	memcpy((void *)&pui32WrVirPtr[ui32WrOffset>>2], (void *)pIpcBody, ui32BodySize);
	ui32WrOffset += ui32BodySize;

	ui32WrOffset = IPC_REG_CEILING_4BYTES(ui32WrOffset);
	if( ui32WrOffset >= gsIpcCmd.ui32BufSize )
		ui32WrOffset = 0;

	// 5. Update Write IPC Register
	IPC_REG_CMD_SetWrOffset(ui32WrOffset);

	// 6. Confirm Writing
	ui32WriteConfirm = pui32WrVirPtr[ui32WrOffset_Org>>2];
	if( ui32WriteConfirm != IPC_CMD_MAGIC_CODE )
	{
		ui32WriteFailRetryCount++;
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Fail to Write - Retry Count: %d, Wr:0x%X", ui32WriteFailRetryCount, ui32WrOffset);
		if( ui32WriteFailRetryCount < 3 )
		{
			ui32WrOffset = ui32WrOffset_Org;
			goto IPC_CMD_Write_Retry;
		}
	}

//	MCU_HAL_SetInterIntrStatus(MCU_ISR_IPC, 1);	// internal interrupt : mcu receive
//	MCU_HAL_SetExtIntrEvent();
//	IPC_CMD_KickISR(0, 0, 0, 0);

	VDEC_KDRV_Message(DBG_IPC, "[IPC][CMD] Send CMD Id: %d, Wr:0x%X-->0x%X", eIpcCmdId, ui32WrOffset_Org, ui32WrOffset);

	return TRUE;
}
#else
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
BOOLEAN IPC_CMD_Register_ReceiverCallback(E_IPC_CMD_ID_T eIpcCmdId, IPC_CMD_RECEIVER_CALLBACK_T fpCallback)
{
	if( eIpcCmdId >= IPC_CMD_ID_MAX )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] IPC CMD ID: %d, %s", eIpcCmdId, __FUNCTION__ );
		return FALSE;
	}

	if( fpIpcCmd_Receiver[eIpcCmdId] != NULL )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Callback of IPC CMD ID(%d) not Empth", eIpcCmdId );
	}

	fpIpcCmd_Receiver[eIpcCmdId] = fpCallback;

	return TRUE;
}

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
static UINT32 _IPC_CMD_VerifyMagicCode(UINT32 *pui32RdVirPtr, UINT32 ui32RdOffset)
{
	UINT32			ui32RdOffset_Org;
	UINT32			ui32ReadFailRetryCount = 0;

	ui32RdOffset_Org = ui32RdOffset;

_IPC_CMD_VerifyMagicCode_Retry :
	if( pui32RdVirPtr[ui32RdOffset>>2] == IPC_CMD_MAGIC_CODE )
	{
		pui32RdVirPtr[ui32RdOffset>>2] = IPC_CMD_MAGIC_CLEAR;
		return ui32RdOffset;
	}

//	VDEC_KDRV_Message(DBG_IPC, "[IPC][Dbg][CMD] Not Found MAGIC CODE - Read Offset: 0x%X", ui32RdOffset );

	if( pui32RdVirPtr[ui32RdOffset>>2] == IPC_CMD_PADDING_BITS )
	{
		VDEC_KDRV_Message(DBG_IPC, "[IPC][DBG][CMD] Padding - Rd:%d, Buf Size:%d", ui32RdOffset, gsIpcCmd.ui32BufSize );

		ui32RdOffset = 0;
		if( pui32RdVirPtr[ui32RdOffset>>2] == IPC_CMD_MAGIC_CODE )
		{
			VDEC_KDRV_Message(DBG_IPC, "[IPC][DBG][CMD] WrapAround - Rd:%d, Buf Size:%d", ui32RdOffset, gsIpcCmd.ui32BufSize );
			pui32RdVirPtr[ui32RdOffset>>2] = IPC_CMD_MAGIC_CLEAR;
			return ui32RdOffset;
		}
	}

	ui32ReadFailRetryCount++;
	if( ui32ReadFailRetryCount < 0x10 )
	{
		VDEC_KDRV_Message(DBG_IPC, "[IPC][DBG][CMD] Retry to Verify Magic Code: %d", ui32ReadFailRetryCount );
		ui32RdOffset = ui32RdOffset_Org;
		goto _IPC_CMD_VerifyMagicCode_Retry;
	}

	VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] No MAGIC CODE: 0x%08X - Rd: 0x%X, BufSize: 0x%X", pui32RdVirPtr[ui32RdOffset>>2], ui32RdOffset, gsIpcCmd.ui32BufSize );

	return IPC_REG_INVALID_OFFSET;
}

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
static UINT32 _IPC_CMD_FindNextMagicCode(UINT32 *pui32RdVirPtr, UINT32 ui32RdOffset, UINT32 ui32WrOffset)
{
	while( ui32RdOffset != ui32WrOffset )
	{
		if( pui32RdVirPtr[ui32RdOffset>>2] == IPC_CMD_MAGIC_CODE )
		{
			pui32RdVirPtr[ui32RdOffset>>2] = IPC_CMD_MAGIC_CLEAR;
			return ui32RdOffset;
		}
		else if( pui32RdVirPtr[ui32RdOffset>>2] == IPC_CMD_PADDING_BITS )
		{
			UINT32	ui32RdOffset_i = ui32RdOffset;

			while( ui32RdOffset_i < gsIpcCmd.ui32BufSize )
			{
				if( pui32RdVirPtr[ui32RdOffset_i>>2] != IPC_CMD_PADDING_BITS )
				{
					VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] No Next PADDING CODE: 0x%X - Rd:0x%X(0x%X), BufSize:0x%X", pui32RdVirPtr[ui32RdOffset>>2], ui32RdOffset, ui32RdOffset_i, gsIpcCmd.ui32BufSize);
				}

//				VDEC_KDRV_Message(CMD_IPC, "[IPC][DBG][CMD] PADDING BITS - Rd: 0x%X", ui32RdOffset );

				ui32RdOffset_i += 4;
				if( ui32RdOffset_i == ui32WrOffset )
				{
					VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] PADDING CODE Area: 0x%X - Rd:0x%X(0x%X), Wr:0x%X", pui32RdVirPtr[ui32RdOffset>>2], ui32RdOffset, ui32RdOffset_i, ui32WrOffset);
					break;
				}
			}
			ui32RdOffset = ui32RdOffset_i;

			if( ui32RdOffset == gsIpcCmd.ui32BufSize )
				ui32RdOffset = 0;
		}
		else
		{
			ui32RdOffset += 4;
			if( (ui32RdOffset + 4) >= gsIpcCmd.ui32BufSize )
				ui32RdOffset = 0;
		}
	}

	VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Rd: 0x%X, Wr:0x%X - Data:0x%X", ui32RdOffset, ui32WrOffset, pui32RdVirPtr[ui32RdOffset>>2]);

	return IPC_REG_INVALID_OFFSET;
}

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
static UINT32 _IPC_CMD_Receive(void)
{
	UINT32		ui32WrOffset;	// byte size
	UINT32		ui32RdOffset, ui32RdOffset_Org;	// byte size
	E_IPC_CMD_ID_T eIpcCmdId;
	UINT32		ui32BodySize;
	volatile UINT32	*pui32RdVirPtr;
	volatile UINT32	ui32RdOffset_Aligned;

//	IPC_CMD_ReceiveISR(NULL, NULL, NULL, NULL);

	if( !gsIpcCmd.pui32VirBasePtr )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Not Initialised" );
		return 0;
	}

	ui32WrOffset = IPC_REG_CMD_GetWrOffset();
	ui32RdOffset = IPC_REG_CMD_GetRdOffset();
	ui32RdOffset_Org = ui32RdOffset;

	if( ui32RdOffset == ui32WrOffset )
		return 0;

	pui32RdVirPtr = gsIpcCmd.pui32VirBasePtr;

	// 1. Check Magic Code
	ui32RdOffset = _IPC_CMD_VerifyMagicCode((UINT32 *)pui32RdVirPtr, ui32RdOffset);
	if( ui32RdOffset == IPC_REG_INVALID_OFFSET )
	{
		ui32RdOffset = ui32RdOffset_Org;

		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Try to Find Next MAGIC Code - Rd:0x%X, Wr:0x%X", ui32RdOffset, ui32WrOffset);

		ui32RdOffset = _IPC_CMD_FindNextMagicCode((UINT32 *)pui32RdVirPtr, ui32RdOffset, ui32WrOffset);
		if( ui32RdOffset == IPC_REG_INVALID_OFFSET )
		{
			ui32RdOffset = ui32RdOffset_Org;
			VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Not Found Next MAGIC Code - Rd:0x%X, Wr:0x%X", ui32RdOffset, ui32WrOffset);
			return 0;
		}
		else
		{
			VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Found Next MAGIC Code - Rd:0x%X, Wr:0x%X", ui32RdOffset, ui32WrOffset);
		}
	}
	ui32RdOffset += 4;

	// 2. Read Header
	eIpcCmdId = (E_IPC_CMD_ID_T)pui32RdVirPtr[ui32RdOffset>>2];
	ui32RdOffset += 4;

	if( eIpcCmdId >= IPC_CMD_ID_MAX )
	{
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] IPC CMD ID: %u - Rd:0x%X, Wr:0x%X", eIpcCmdId, ui32RdOffset_Org, ui32WrOffset);
		return 0;
	}

	// 3. Read Length
	ui32BodySize = pui32RdVirPtr[ui32RdOffset>>2];
	ui32RdOffset += 4;

	// 5. Update Read IPC Register
	ui32RdOffset_Aligned = ui32RdOffset + IPC_REG_CEILING_4BYTES(ui32BodySize);
	if( ui32RdOffset_Aligned >= gsIpcCmd.ui32BufSize )
		ui32RdOffset_Aligned = 0;
	IPC_REG_CMD_SetRdOffset(ui32RdOffset_Aligned);

	// 4. Call Registered Callback Function
	if( fpIpcCmd_Receiver[eIpcCmdId] == NULL )
		VDEC_KDRV_Message(ERROR, "[IPC][Err][CMD] Callback of IPC CMD ID(%d) not Empth", eIpcCmdId);
	else
		fpIpcCmd_Receiver[eIpcCmdId]( (void *)&pui32RdVirPtr[ui32RdOffset>>2] );

//	ui32RdOffset += IPC_REG_CEILING_4BYTES(ui32BodySize);
//	if( ui32RdOffset >= gsIpcCmd.ui32BufSize )
//		ui32RdOffset = 0;

	// 5. Update Read IPC Register
//	IPC_REG_CMD_SetRdOffset(ui32RdOffset);

	VDEC_KDRV_Message(DBG_IPC, "[IPC][CMD] Receive CMD Id: %d, Rd:0x%X-->0x%X", eIpcCmdId, ui32RdOffset_Org, ui32RdOffset_Aligned);

	return 1;
}


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
void IPC_CMD_Receive(void)
{
	while(_IPC_CMD_Receive());
}
#endif

// UINT32 wParam0  : usable just 23Bits data
// UINT32 wParam1  : usable just 28Bits data
// UINT32 wParam2  : usable just 28Bits data
// UINT32 wParam2  : usable just 32Bits data

void IPC_CMD_KickISR(UINT32 wParam0, UINT32 wParam1, UINT32 wParam2, UINT32 wParam3)
{
	LIPSYNC_QUEUE_T IPCCmd;

	/*	COMMAND 0		[31:9] au_start_address		*/
	IPCCmd.ui32AuStartAddr = ((wParam0&0x007FFFFF)<<9);			// 512bytes aligned size
	IPCCmd.ui32EndAddrCheckMode = 0x3;
	IPCCmd.ui32AuType = 0;

	/*	COMMAND 1 		[27:0] dts	*/
	IPCCmd.ui32IgnoreDTSMatching = 1;
	IPCCmd.ui32RunDTS = wParam1;

	/*	COMMAND 2		[27:0] pts	*/
	IPCCmd.ui32PTSerr = 0;
	IPCCmd.ui32PTS = wParam2;

	IPCCmd.ui32wParam = wParam3;

	LQ_HAL_PushData(IPC_ISR_ID, &IPCCmd);
}

void IPC_CMD_ReceiveISR(UINT32 *pwParam0, UINT32 *pwParam1, UINT32 *pwParam2, UINT32 *pwParam3)
{
	LIPSYNC_QUEUE_T IPCCmd;
	LQ_HAL_GetData(IPC_ISR_ID, &IPCCmd);
	LQ_HAL_UpdateRdPtr(IPC_ISR_ID);

	if(pwParam0 != NULL)
	{
		*pwParam0 = (IPCCmd.ui32AuStartAddr>>9);			// 512bytes aligned size
	}

	if(pwParam1 != NULL)
	{
		*pwParam1 = IPCCmd.ui32RunDTS;
	}

	if(pwParam2 != NULL)
	{
		*pwParam2 = IPCCmd.ui32PTS;
	}

	if(pwParam3 != NULL)
	{
		*pwParam3 = IPCCmd.ui32wParam;
	}
}

/*
void example_receive_control(void)
{
	UINT32 wParam0, wParam1, wParam2, wParam3;

	IPC_CMD_ReceiveISR(&wParam0, &wParam1, &wParam2, &wParam3);
//	printk("IPC Receive Param :: 0x%x, 0x%x, 0x%x, 0x%x\n", wParam0, wParam1, wParam2, wParam3);
}
*/
/** @} */

