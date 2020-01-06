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
 *  driver interface header for sample device. ( used only within kdriver )
 *	sample device will teach you how to make device driver with new platform.
 *
 *  @author		root
 *  @version	1.0 
 *  @date		2009.11.15 
 *
 *  @addtogroup lg1150_sample
 *	@{
 */

#ifndef	_SAMPLE_DRV_H_
#define	_SAMPLE_DRV_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "debug_util.h"
#include "sample_cfg.h"
#include "sample_kapi.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define	SAMPLE_PRINT(format, args...)		DBG_PRINT(  g_sample_debug_fd, 0, format, ##args)
#define	SAMPLE_TRACE(format, args...)		DBG_PRINTX( g_sample_debug_fd, 0, format, ##args)

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
extern	int      SAMPLE_Init(void);
extern	void     SAMPLE_Cleanup(void);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern	int		g_sample_debug_fd;

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _SAMPLE_DRV_H_ */

/** @} */
