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
 *  driver interface header for pvr device. ( used only within kdriver )
 *	pvr device will teach you how to make device driver with new platform.
 *
 *  @author		kyungbin.pak
 *  @version	1.0
 *  @date		2009.11.15
 *
 *  @addtogroup lg1150_pvr
 *	@{
 */

#ifndef	_PVR_DRV_H_
#define	_PVR_DRV_H_


/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "debug_util.h"
#include "pvr_cfg.h"
#include "pvr_kapi.h"

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
#if 0 // before logm '13 jinhwan.bae
/*
 * The print macros follow the LX_PVR_PRINT_MASK_T defined in pvr_drv.h
 */
#define	PVR_PRINT(format, args...)		DBG_PRINT(  g_pvr_debug_fd, 0, format, ##args)
/* Trace are used for tracking IOCTL calls */
#define	PVR_TRACE(format, args...)		DBG_PRINTX( g_pvr_debug_fd, 1, format, ##args)
#define	PVR_REG_PRINT(format, args...)	DBG_PRINT(  g_pvr_debug_fd, 2, format, ##args)
/* Not good to enable prints from ISR, only need to be enabled for critical debugging purposes */
#define	PVR_ISR_PRINT(format, args...)	DBG_PRINT(  g_pvr_debug_fd, 3, format, ##args)
#else
#define PVR_ERROR		LX_LOGM_LEVEL_ERROR			/* 0 */
#define PVR_WARNING		LX_LOGM_LEVEL_WARNING		/* 1 */
#define PVR_NOTI		LX_LOGM_LEVEL_NOTI			/* 2 */
#define PVR_INFO		LX_LOGM_LEVEL_INFO			/* 3 */
#define PVR_DEBUG		LX_LOGM_LEVEL_DEBUG			/* 4 */
#define PVR_TRACE		LX_LOGM_LEVEL_TRACE			/* 5 */
#define PVR_DRV			(LX_LOGM_LEVEL_TRACE + 1)	/* 6 */
#define PVR_NORMAL		(LX_LOGM_LEVEL_TRACE + 2)	/* 7 */
#define PVR_ISR			(LX_LOGM_LEVEL_TRACE + 3)	/* 8 */
#define PVR_UPLOAD		(LX_LOGM_LEVEL_TRACE + 4)	/* 9 */
#define PVR_PIE			(LX_LOGM_LEVEL_TRACE + 5)	/* 10 */
#define PVR_REGISTER	(LX_LOGM_LEVEL_TRACE + 6)	/* 11 */
#define PVR_PIE_DEBUG	(LX_LOGM_LEVEL_TRACE + 7)	/* 12 */

#define PVR_KDRV_LOG(_idx, format, args...) 	DBG_PRINT( g_pvr_debug_fd, 	_idx, 			format "\n", ##args)
#define	PVR_REG_PRINT(format, args...)			DBG_PRINT( g_pvr_debug_fd, 	PVR_REGISTER, 	format, ##args)
#define	PVR_ISR_PRINT(format, args...)			DBG_PRINT(  g_pvr_debug_fd, PVR_ISR, 		format, ##args)

#endif

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
extern	void     PVR_PreInit(void);
extern	int      PVR_Init(void);
extern	void     PVR_Cleanup(void);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern	int		g_pvr_debug_fd;

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PVR_DRV_H_ */

/** @} */
