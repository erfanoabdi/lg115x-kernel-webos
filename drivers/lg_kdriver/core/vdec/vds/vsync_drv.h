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
 * version    0.1
 * date       2011.11.08
 * note       Additional information.
 *
 */

#ifndef _VDEC_VSYNC_DRV_H_
#define _VDEC_VSYNC_DRV_H_

#include "vdec_type_defs.h"

#define		VDISP_MAX_NUM_OF_MULTI_CHANNEL		3
#define		VDISP_INVALID_CHANNEL				0xFF


#ifdef __cplusplus
extern "C" {
#endif


typedef void (*VSYNC_CB)(UINT8, UINT8, UINT8[], UINT8);

void VSync_Init(VSYNC_CB fnCallback);
UINT8 VSync_Open(UINT8 u8VdispCh, BOOLEAN bIsDualDecoding,
		UINT32 ui32FrameRateRes, UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced);
void VSync_Close(UINT8 ui8VSyncCh, UINT8 ui8LipSyncCh);
int VSync_IsActive (UINT8 ui8VSyncCh);
UINT32 VSync_CalFrameDuration(UINT32 ui32FrameRateRes, UINT32 ui32FrameRateDiv);
BOOLEAN VSync_SetNextVsyncField(UINT8 ui8VSyncCh, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced);
BOOLEAN VSync_SetVsyncField(UINT8 ui8VSyncCh, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced);
UINT32 VSync_GetVsyncIntv(UINT8 ui8VSyncCh);
void VSync_SetDcoFrameRate(UINT8 ui8VSyncCh, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv);
void VSync_SetPhaseShift(UINT8 ui8VSyncCh, SINT32 i32ShiftDelta90K);
void VSync_SetMaxShift(UINT8 ui8VSyncCh, SINT32 max);
void VDEC_ISR_VSync_Task(void);
UINT32 VSYNC_GetFieldDuration(UINT8 ui8VSyncCh);
void VSYNC_SetVVsyncTimer(UINT8 ui8VSyncCh, UINT32 u32Time90K);
void VSYNC_Suspend(void);
void VSYNC_Resume(void);




#ifdef __cplusplus
}
#endif

#endif /* _VDEC_VSYNC_DRV_H_ */

