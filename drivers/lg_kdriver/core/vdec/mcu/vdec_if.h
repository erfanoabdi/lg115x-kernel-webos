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
 * author     taewoong.kim@lge.com
 * version    0.1
 * date       2013.05.03
 * note       Additional information.
 *
 */

#ifndef _VDEC_IF_H_
#define _VDEC_IF_H_

#include "../../platform/include/type_defs.h"
#include "../vds/disp_q.h"


#ifdef __cplusplus
extern "C" {
#endif

#define VDEC_IF_NUM_OF_CHANNEL        4

typedef struct _VDEC_IF_T VDEC_IF_CH_T;

void VIF_Init(void);
VDEC_IF_CH_T* VIF_Open(UINT32 u32VdecIfCh);
void VIF_SelDe(UINT32 u32VdecIfCh, UINT32 u32DeCh);
void VIF_SelVsync(UINT32 u32VdecIfCh, UINT32 u32DeCh);
void VIF_VsyncCallback(UINT8 u8VdecIfCh, UINT32 ui32SyncField);
void VIF_SetNextFrame(UINT32 u32VdecIfCh, S_DISPQ_BUF_T *pstNextFrame);

#ifdef __cplusplus
}
#endif

#endif /* _VDEC_IF_H_ */

