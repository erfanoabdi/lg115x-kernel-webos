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
 *  Brief description.
 *  Detailed description starts here.
 *
 *  @author     junghyun.son@lge.com
 *  @version    1.0
 *  @date       2013-09-25
 *  @note       Additional information.
 */

#ifndef	_H15_VDEC_REG_BASE_H_
#define	_H15_VDEC_REG_BASE_H_

#ifdef	__linux__
#include <linux/io.h>

#include "os_util.h"
#include "hma_alloc.h"
#endif	//__linux__

#ifdef VDEC_BASE
#undef VDEC_BASE
#endif

// Register base address
#define VDEC_REG_BASE				(0xC8450000)
#define VDEC_REG_MCU_RC				(VDEC_REG_BASE + 0x00000700)
#define VDEC_REG_MCU_IPC			(VDEC_REG_BASE + 0x00000800)
#define VDEC_REG_MCU_DMA			(VDEC_REG_BASE + 0x00000D00)
#define VDEC_REG_BASE_CNM			(VDEC_REG_BASE + 0x00001000)
#define VDEC_REG_RC0_BASE			(VDEC_REG_BASE + 0x00001500)
#define VDEC_REG_RC1_BASE			(VDEC_REG_BASE + 0x00001600)
#define VDEC_REG_BASE_SYNC			(VDEC_REG_BASE + 0x00001700)
#define VDEC_REG_BASE_PDEC			(VDEC_REG_BASE + 0x00001800)

#define VDEC_REG_BASE_HEVC			(VDEC_REG_BASE + 0x00002000)

// IRQ number : fixme
#define VDEC_IRQ_NUM_PDEC			(32 + 44)
#define VDEC_IRQ_NUM_CNM			(32 + 44)
#define VDEC_IRQ_NUM_SYNC			(32 + 44)
#define VDEC_IRQ_NUM_HEVC			(32 + 46)
#define VDEC_IRQ_NUM_G1_0			(32 + 47)
#define VDEC_IRQ_NUM_HEVC_1			(32 + 48)
#define VDEC_IRQ_NUM_G1_1			(32 + 49)

// Core number
#define VDEC_CORE_NUM_CNM			(1)
#define VDEC_CORE_NUM_HEVC			(2)
#define VDEC_CORE_NUM_GX			(3)
#define VDEC_NUM_OF_CORES			VDEC_CORE_NUM_GX

// vdec register base
#define VDEC_BASE				VDEC_REG_BASE

#define VDEC_GET_DE_IF_BASE(rev, ch, base)	\
do {		\
		base = (VDEC_BASE + 0x1C00 + (ch * 0x40));		\
} while(0)
#define VDEC_GET_VSYNC_BASE(rev, base)	\
do {									\
		base = VDEC_REG_BASE_SYNC;			\
} while(0)

#define PDEC_BASE(ch)				(VDEC_REG_BASE_PDEC + (ch*0x100))
#define CNM_BASE(ch)				VDEC_REG_BASE_CNM
#define HEVC_REG_BASE(ch)			VDEC_REG_BASE_HEVC

#define	PDEC_NUM_OF_CHANNEL			3

#define	MSVC_NUM_OF_CORE			1
#define	MSVC_NUM_OF_CHANNEL			4

#define HEVC_NUM_OF_CORE			1
#define HEVC_NUM_OF_CHANNEL			1
#define HEVC__REG_SIZE				0xC00 // not defined yet

#define G1_NUM_OF_CORE				2
#define G2_NUM_OF_CORE				1

#define	DE_NUM_OF_CHANNEL			4
#define	LQC_NUM_OF_CHANNEL			4
#define DE_IPC_NUM_OF_REG			DE_NUM_OF_CHANNEL

#define LIPSYNC_NUM_OF_CHANNEL		4
#define	DE_VDO_NUM_OF_CHANNEL		2
#define	DSTBUF_NUM_OF_CHANNEL		1
#define	VDISP_NUM_OF_CHANNEL		4
#define	VSYNC_HW_NUM_OF_CH			4
#define	DE_IPC_NUM_OF_CHANNEL		4

#define	NUM_OF_MSVC_CHANNEL			(MSVC_NUM_OF_CORE * MSVC_NUM_OF_CHANNEL)

#define	VDEC_NUM_OF_CHANNEL			(NUM_OF_MSVC_CHANNEL + HEVC_NUM_OF_CHANNEL)

/* For B0 codes */
#define MAX_WIDTH					2048
#define MAX_HEIGHT					1088

#define MAX_NUM_OF_TILED_BUFFER		22

#define VDEC_CPB_SIZE			0x700000

#define UNKNOWN_PTS				0xFFFFFFFE

#ifdef	__linux__
#define VDEC_BUS_OFFSET				0x00000000
#define vdec_alloc(size,align,name)	({ \
		unsigned long r = hma_alloc_user ("vdec", size, align, name); \
		if (r) \
		r -= VDEC_BUS_OFFSET; \
		r; \
	})
#define vdec_free(addr)			hma_free ("vdec", addr+VDEC_BUS_OFFSET)

#define vdec_remap(addr,size)		ioremap (addr+VDEC_BUS_OFFSET, size)
#define vdec_unmap(addr)		iounmap (addr)
#endif	//__linux__

#define LX_CHIP_D13		0xd3
#define LX_CHIP_D14		0xd4
#define vdec_chip()		lx_chip()
#define VDEC_CHIP_D13		LX_CHIP_D13
#define VDEC_CHIP_D14		LX_CHIP_D14
#define VDEC_CHIP_H13		LX_CHIP_H13
#define VDEC_CHIP_M14		LX_CHIP_M14
#define VDEC_CHIP_H14		LX_CHIP_H14
#define VDEC_CHIP_H15		LX_CHIP_H15

#define vdec_chip_rev()		lx_chip_rev()
#define VDEC_CHIP_REV(a,b)	LX_CHIP_REV(a,b)

#define VDEC_COMP_CHIP(a,b)	LX_COMP_CHIP(a,b)

#ifdef CONFIG_VDEC_ON_FPGA
#define VDEC_HW_RATE		(80)
#else
#define VDEC_HW_RATE		(1)
#endif

#define CNM_CORE_FREQUENCY	(320)

#endif /* _H15_VDEC_REG_BASE_H_ */

