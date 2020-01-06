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
 *
 * author     taewoong.kim@lge.com
 * version    0.1
 * date       2013.08.29
 * note       Additional information.
 *
 */

#ifndef _VDISP_CONFIG_H_
#define _VDISP_CONFIG_H_

#define VDISP_NUM_OF_CH		4

#ifndef __XTENSA__
	#ifdef VDEC_USE_MCU
		#define ARM_USING_MMCU
	#else
		#define ARM_NOT_USING_MMCU
	#endif
#else
	#ifdef VDEC_USE_MCU
		#define MCU_USING_MMCU
	#else
		#define MCU_NOT_USING_MMCU
	#endif
#endif

#endif /* _VDISP_CONFIG_H_ */

