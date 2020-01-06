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
 * author     youngki.lyu@lge.com
 * version    1.0
 * date       2011.05.08
 * note       Additional information.
 *
 */

#ifndef _IPC_REQ_H_
#define _IPC_REQ_H_


#include "vdec_type_defs.h"


#ifdef __cplusplus
extern "C" {
#endif



typedef enum
{
	IPC_REQ_ID_NOTI_DECINFO = 0,
	IPC_REQ_ID_NOTI_DISPINFO,
	IPC_REQ_ID_REPORT_CPB,
	IPC_REQ_ID_REPORT_DQ,
	IPC_REQ_ID_REQCMD_RESET,
	IPC_REQ_ID_MCU_READY,
	IPC_REQ_ID_SET_BASETIME,
	IPC_REQ_ID_MAX,
	IPC_REQ_ID_UINT32 = 0x20110803
} E_IPC_REQ_ID_T;


typedef void (*IPC_REQ_RECEIVER_CALLBACK_T)(void *);



void IPC_REQ_Init(void);
BOOLEAN IPC_REQ_Send(E_IPC_REQ_ID_T eIpcReqId, UINT32 ui32BodySize, void *pIpcBody);

BOOLEAN IPC_REQ_Register_ReceiverCallback(E_IPC_REQ_ID_T eIpcReqId, IPC_REQ_RECEIVER_CALLBACK_T fpCallback);
void IPC_REQ_Receive(void);


#ifdef __cplusplus
}
#endif

#endif /* _IPC_REQ_H_ */

