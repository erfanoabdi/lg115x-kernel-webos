/*
 * SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2013 by LG Electronics Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/** @file
 *
 * main driver implementation for de device.
 * de device will teach you how to make device driver with new platform.
 *
 * author     youngki.lyu (youngki.lyu@lge.com)
 * version    1.0
 * date       2010.07.30
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
#include "ipc_callback.h"
#include "ipc_req.h"

#include <linux/kernel.h>
#include <asm/string.h>	// memset

#include "log.h"
logm_define (vdec_ipcc, log_level_warning);


/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/

typedef enum
{
	IPC_CALLBACK_ID_NOTI_DECODE = 0,
	IPC_CALLBACK_ID_NOTI_DISPLAY,
	IPC_CALLBACK_ID_REPORT_CPB,
	IPC_CALLBACK_ID_REPORT_DQ,
	IPC_CALLBACK_ID_REQCMD_RESET,
	IPC_CALLBACK_ID_MAX,
	IPC_CALLBACK_ID_UINT32 = 0x20110803
} E_IPC_CALLBACK_ID_T;

typedef struct
{
	UINT32		ui32IpcNotiId;
	BOOLEAN		bEnable;
} S_IPC_CALLBACK_REGISTER_T;


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
static UINT32 ui32EnableNoti = 0x0;



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
BOOLEAN IPC_CALLBACK_Register_DecInfo(IPC_CALLBACK_DECINFO_CB_T fpCallback)
{
	if( IPC_REQ_Register_ReceiverCallback(IPC_REQ_ID_NOTI_DECINFO, (IPC_REQ_RECEIVER_CALLBACK_T)fpCallback) == FALSE )
		return FALSE;

	ui32EnableNoti |= (1 << IPC_CALLBACK_ID_NOTI_DECODE);

	logm_debug (vdec_ipcc, "[IPC_CALLBACK][DBG] EnableNoti:0x%08X %s", ui32EnableNoti, __FUNCTION__ );

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
BOOLEAN IPC_CALLBACK_Register_CpbStatus(IPC_CALLBACK_CPBSTATUS_CB_T fpCallback)
{
	if( IPC_REQ_Register_ReceiverCallback(IPC_REQ_ID_REPORT_CPB, (IPC_REQ_RECEIVER_CALLBACK_T)fpCallback) == FALSE )
		return FALSE;

	ui32EnableNoti |= (1 << IPC_CALLBACK_ID_REPORT_CPB);

	logm_debug (vdec_ipcc, "[IPC_CALLBACK][DBG] EnableNoti:0x%08X %s", ui32EnableNoti, __FUNCTION__ );

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
BOOLEAN IPC_CALLBACK_Register_ReqReset(IPC_CALLBACK_REQUESTCMD_CB_T fpCallback)
{
	if( IPC_REQ_Register_ReceiverCallback(IPC_REQ_ID_REQCMD_RESET, (IPC_REQ_RECEIVER_CALLBACK_T)fpCallback) == FALSE )
		return FALSE;

	ui32EnableNoti |= (1 << IPC_CALLBACK_ID_REQCMD_RESET);

	logm_debug (vdec_ipcc, "[IPC_CALLBACK][DBG] EnableNoti:0x%08X %s", ui32EnableNoti, __FUNCTION__ );

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
void IPC_CALLBACK_Init(void)
{
	logm_debug (vdec_ipcc, "[IPC_CALLBACK][DBG] %s", __FUNCTION__ );

	ui32EnableNoti = 0x0;

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
BOOLEAN IPC_CALLBACK_Noti_DecInfo(S_IPC_CALLBACK_BODY_DECINFO_T *pDecInfo)
{
	if( !(ui32EnableNoti & (1 << IPC_CALLBACK_ID_NOTI_DECODE)) )
		return FALSE;

	return IPC_REQ_Send(IPC_REQ_ID_NOTI_DECINFO, sizeof(S_IPC_CALLBACK_BODY_DECINFO_T), (void *)pDecInfo);
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
BOOLEAN IPC_CALLBACK_Report_CpbStatus(S_IPC_CALLBACK_BODY_CPBSTATUS_T *pCpbStatus)
{
	if( !(ui32EnableNoti & (1 << IPC_CALLBACK_ID_REPORT_CPB)) )
		return FALSE;

	return IPC_REQ_Send(IPC_REQ_ID_REPORT_CPB, sizeof(S_IPC_CALLBACK_BODY_CPBSTATUS_T), (void *)pCpbStatus);
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
BOOLEAN IPC_CALLBACK_RequestCmd_Reset(S_IPC_CALLBACK_BODY_REQUEST_CMD_T *pReqCmd)
{
	if( !(ui32EnableNoti & (1 << IPC_CALLBACK_ID_REQCMD_RESET)))
		return FALSE;

	return IPC_REQ_Send(IPC_REQ_ID_REQCMD_RESET, sizeof(S_IPC_CALLBACK_BODY_REQUEST_CMD_T), (void *)pReqCmd);
}

/** @} */

