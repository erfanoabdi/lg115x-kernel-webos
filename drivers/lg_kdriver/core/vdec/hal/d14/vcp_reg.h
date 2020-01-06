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
 *  VCP register details. ( used only within kdriver )
 *
 *  author     Taewoong Kim (taewoong.kim@lge.com)
 *  version    1.0
 *  date       2013.11.05
 *
 */

#ifndef _VCP_REG_H_
#define _VCP_REG_H_
//#define D14
#include "vdec_type_defs.h"



extern volatile UINT32 stpVcpReg;
/*----------------------------------------------------------------------------------------
    Control Constants
----------------------------------------------------------------------------------------*/
#define VDEC_VCP_BASE		(VDEC_BASE + 0x62000) //D14 Fix

// D14 vcp fix
#define VCP_CH_PIC_INIT(i)		(*(volatile UINT32 *)(stpVcpReg + 0x200*i + 0x000)) 
#define VCP_CH_PIC_START(i)		(*(volatile UINT32 *)(stpVcpReg + 0x200*i + 0x004))
#define VCP_CH_VCP_INTR_REG(i)      (*(volatile UINT32 *)(stpVcpReg + 0x200*i + 0x040))
#define VCP_CH_VCP_INTR_MASK(i)      (*(volatile UINT32 *)(stpVcpReg + 0x200*i + 0x044))	
/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
void VCP_Init(void);

#ifdef __cplusplus
extern "C" {
#endif
/** @} *//* end of macro documentation */

#ifdef __cplusplus
}
#endif

#endif	/* _VCP_REG_H_ */

/* from 'vdo-REG.csv' 20120326 19:08:42   대한민국 표준시 by getregs v2.7 */
