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
 *  @author		murugan.d
 *  @version	1.0 
 *  @date		2010.04.09 
 *
 *  @addtogroup lg1150_pvr
 *	@{
 */

#ifndef	_PVR_DEV_H_
#define	_PVR_DEV_H_

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
//Basic function error codes
#define PVR_OK		0
#define PVR_FAILURE	1


/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define DVR_DN_MAX_PKT_CNT	(ui32DvrDnMaxPktCount) /* Defined in DD_DVR.c, The value is modified when download buf size is modified */
#define DVR_DN_MAX_BUF_CNT	(ui32DvrDnMaxBufLimit) /* Defined in DD_DVR.c, The value is modified when download buf size is modified */

#define DVR_DN_MIN_PKT_CNT	(ui32DvrDnMinPktCount)
#define DVR_DN_MIN_BUF_CNT  (ui32DvrDnMinBufLimit)

/* Maximum 3 indices can be used in the SCD array */
#define DVR_SCD_INDEX_MAX	3

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

typedef struct {
    UINT32	ui32DnUnitBuf;
    UINT32	ui32DnOverFlowErr;
    UINT32	ui32UpOverlapErr;
    UINT32	ui32UpAlmostEmpty;
} DVR_ErrorStatus_T;

typedef enum
{
	 /* 0x00 */ DVR_EVT_UNIT_BUFF	/* Download	*/
	,/* 0x01 */ DVR_EVT_UL_EPTY		/* Upload */
	,/* 0x02 */ DVR_EVT_UL_OVERLAP	/* Upload */
	,/* 0x03 */ DVR_EVT_UL_FULL		/* Upload */
	,/* 0x04 */ DVR_EVT_UL_ERR_SYNC /* Upload */
	,/* 0x05 */ DVR_EVT_PIE_SCD 	/* PIE */
}DVR_EVENT_T;



/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern UINT32 ui32DvrDnMinPktCount;
extern UINT32 ui32DvrDnMaxPktCount;

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PVR_DEV_H_ */

/** @} */

