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


/**
 * @file
 *
 *  driver interface header for media MCU device. ( used only within kdriver )
 *	media MCU device will teach you how to make device driver with new platform.
 *
 *  @author		taewoong.kim (taewoong.kim@lge.com)
 *  @version	1.0
 *  @date		2013.05.02
 *
 *  @addtogroup lg1154_mmcu
 * @{
 */

#ifndef	_MMCU_DRV_H_
#define	_MMCU_DRV_H_

#include "base_types.h"

extern	void     MMCU_PreInit(void);
extern	int      MMCU_Init(void);
extern	void     MMCU_Cleanup(void);

extern struct proc_dir_entry *mmcu_proc_root_entry;

UINT32 MMCU_Read(UINT8* pu8Buf, UINT32 u32Size);

#endif /* _MMCU_DRV_H_ */

/** @} */
