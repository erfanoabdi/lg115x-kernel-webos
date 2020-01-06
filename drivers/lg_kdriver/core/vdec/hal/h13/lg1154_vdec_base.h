
/*
 * SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2010 by LG Electronics Inc.
 *  
 *  This program is free software; you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License 
 *  version 2 as published by the Free Software Foundation.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 *   GNU General Public License for more details.
*/ 

/** @file
 *
 *  Brief description.
 *  Detailed description starts here.
 *
 *  @author     seokjoo.lee
 *  @version    1.0
 *  @date       2011-04-05
 *  @note       Additional information.
 */

#ifndef	_H13_VDEC_REG_BASE_H_
#define	_H13_VDEC_REG_BASE_H_


#ifdef __XTENSA__
#ifdef H13_VDEC_BASE
#undef H13_VDEC_BASE
#endif
#define 	H13_VDEC_BASE					(0xC0000000 + 0x004000)
#else
//#define 	H13_VDEC_BASE					(0xC0000000 + 0x004000)
#include "os_util.h"
#endif

#define MCU_ROM_BASE				0x50000000
#define MCU_RAM0_BASE				0x60000000
#define MCU_RAM1_BASE				0x80000000
#define MCU_RAM2_BASE				0xa0000000
#define MCU_REG_BASE				0xf0000000

#define MCU_ROM_MAX_SIZE			0x10000000
#define MCU_RAM_MAX_SIZE			0x20000000

#define VDEC_REG_OFFSET				0x00004000

#define	VDEC_MAIN_ISR_HANDLE		H13_IRQ_VDEC0
#define	VDEC_MCU_ISR_HANDLE			H13_IRQ_VDEC1

#define	PDEC_NUM_OF_CHANNEL			3
#define	SRCBUF_NUM_OF_CHANNEL		2
#define AUIB_NUM_OF_CHANNEL			(PDEC_NUM_OF_CHANNEL + 0)	// <= (PDEC_NUM_OF_HW_CHANNEL + SRCBUF_NUM_OF_CHANNEL)
#define CPB_NUM_OF_CHANNEL			AUIB_NUM_OF_CHANNEL
#define	LQC_NUM_OF_CHANNEL			4
#define	MSVC_NUM_OF_CORE			1
#define	MSVC_NUM_OF_CHANNEL			4
#define	NULLVC_NUM_OF_CHANNEL		1
#define	LIPSYNC_NUM_OF_CHANNEL		3	// MAX: LQC_NUM_OF_CHANNEL
#define	DE_IF_NUM_OF_CHANNEL		2
#define	DE_VDO_NUM_OF_CHANNEL		2
#define	DSTBUF_NUM_OF_CHANNEL		1
#define	VDISP_NUM_OF_CHANNEL			DE_IF_NUM_OF_CHANNEL

#define	NUM_OF_MSVC_CHANNEL			(MSVC_NUM_OF_CORE * MSVC_NUM_OF_CHANNEL)
#define	NUM_OF_SWDEC_CHANNEL		1

#define	VDEC_BASE_CHANNEL_SWDEC		(NUM_OF_MSVC_CHANNEL)

#define	VDEC_NUM_OF_CORE			(MSVC_NUM_OF_CORE + NUM_OF_SWDEC_CHANNEL)
#define	VDEC_NUM_OF_CHANNEL			(NUM_OF_MSVC_CHANNEL + NUM_OF_SWDEC_CHANNEL)

/* For B0 codes */
#define MAX_WIDTH					2048
#define MAX_HEIGHT					1088

#define MAX_NUM_OF_TILED_BUFFER		22

#endif /* _H13_VDEC_REG_BASE_H_ */

