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
 * version    1.0
 * date       2011.05.08
 * note       Additional information.
 *
 */

#ifndef _IPC_CMD_H_
#define _IPC_CMD_H_


#include "vdec_type_defs.h"


#ifdef __cplusplus
extern "C" {
#endif



typedef enum
{
	IPC_CMD_ID_DISP_Q_REGISTER = 0,
	IPC_CMD_ID_DISP_CLEAR_Q_REGISTER,
	IPC_CMD_ID_NOTI_REGISTER,
	IPC_CMD_ID_AUIB_REGISTER,
	IPC_CMD_ID_CPB_REGISTER,
	IPC_CMD_ID_MAX,
	IPC_CMD_ID_UINT32 = 0x20110803
} E_IPC_CMD_ID_T;


typedef void (*IPC_CMD_RECEIVER_CALLBACK_T)(void *);



void IPC_CMD_Init(void);

#ifndef __XTENSA__
BOOLEAN IPC_CMD_Send(E_IPC_CMD_ID_T eIpcCmdId, UINT32 ui32BodySize, void *pIpcCmdBody);
#else
BOOLEAN IPC_CMD_Register_ReceiverCallback(E_IPC_CMD_ID_T eIpcCmdId, IPC_CMD_RECEIVER_CALLBACK_T fpCallback);
void IPC_CMD_Receive(void);
#endif

void IPC_CMD_KickISR(UINT32 wParam0, UINT32 wParam1, UINT32 wParam2, UINT32 wParam3);
void IPC_CMD_ReceiveISR(UINT32 *pwParam0, UINT32 *pwParam1, UINT32 *pwParam2, UINT32 *pwParam3);



#ifdef __cplusplus
}
#endif

#endif /* _IPC_CMD_H_ */

