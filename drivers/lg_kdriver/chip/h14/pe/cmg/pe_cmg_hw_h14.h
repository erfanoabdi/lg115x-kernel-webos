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

/** @file pe_cmg_hw_h14.h
 *
 *  driver header for picture enhance color management. ( used only within kdriver )
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2012.04.17
 *	@see		
 */

#ifndef	_PE_CMG_HW_H14_H_
#define	_PE_CMG_HW_H14_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "pe_kapi.h"


#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
/**	20130220, sj.youm, check double setting */
#define PE_CMG_H14_CHK_CEN_DOUBLE_SET

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/
/**
 *	pe cmg settings type
 */
typedef struct
{
	LX_PE_CMG_REGION_T		rgn_set[LX_PE_CMG_REGION_NUM];	///< region
	LX_PE_CMG_REGION_CTRL_T	rgn_ctrl[LX_PE_CMG_REGION_NUM];	///< region ctrl
	LX_PE_CMG_GLOBAL_CTRL_T	gbl_ctrl;						///< global ctrl
}
PE_CMG_HW_H14_SETTINGS_T;

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
int PE_CMG_HW_H14_Init(void);
int PE_CMG_HW_H14_SetDebugSettings(LX_PE_DBG_SETTINGS_T *pstParams);
int PE_CMG_HW_H14_SetCenEnable(LX_PE_CMG_ENABLE_T *pstParams);
int PE_CMG_HW_H14_GetCenEnable(LX_PE_CMG_ENABLE_T *pstParams);
int PE_CMG_HW_H14_SetCenRegionEnable(LX_PE_CMG_REGION_ENABLE_T *pstParams);
int PE_CMG_HW_H14_GetCenRegionEnable(LX_PE_CMG_REGION_ENABLE_T *pstParams);
int PE_CMG_HW_H14_SetCenRegion(LX_PE_CMG_REGION_T *pstParams);
int PE_CMG_HW_H14_GetCenRegion(LX_PE_CMG_REGION_T *pstParams);
int PE_CMG_HW_H14_SetCenRegionCtrl(LX_PE_CMG_REGION_CTRL_T *pstParams);
int PE_CMG_HW_H14_GetCenRegionCtrl(LX_PE_CMG_REGION_CTRL_T *pstParams);
int PE_CMG_HW_H14_SetCenGlobalCtrl(LX_PE_CMG_GLOBAL_CTRL_T *pstParams);
int PE_CMG_HW_H14_GetCenGlobalCtrl(LX_PE_CMG_GLOBAL_CTRL_T *pstParams);
int PE_CMG_HW_H14_SetCenColorCtrl(LX_PE_CMG_COLOR_CTRL_T *pstParams);
int PE_CMG_HW_H14_GetCenColorCtrl(LX_PE_CMG_COLOR_CTRL_T *pstParams);
int PE_CMG_HW_H14_SetClearWhiteCtrl(LX_PE_CMG_CW_CTRL_T *pstParams);
int PE_CMG_HW_H14_GetClearWhiteCtrl(LX_PE_CMG_CW_CTRL_T *pstParams);
int PE_CMG_HW_H14_SetClearWhiteGainCtrl(LX_PE_CMG_CW_GAIN_CTRL_T *pstParams);
int PE_CMG_HW_H14_GetClearWhiteGainCtrl(LX_PE_CMG_CW_GAIN_CTRL_T *pstParams);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_CMG_HW_H14_H_ */

