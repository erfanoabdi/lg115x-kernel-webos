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

/** @file pe_dcm_hw_m14.h
 *
 *  driver header for picture enhance dynamic contrast module functions. ( used only within kdriver )
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2011.06.11
 *	@see		
 */

#ifndef	_PE_DCM_HW_M14_H_
#define	_PE_DCM_HW_M14_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "pe_kapi.h"
#include "pe_tsk_m14.h"

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
#define PE_DCM_HW_M14_LUT_STEP				16
#define PE_DCM_HW_M14_LUT_MAX				33

#define PE_DCM_HW_M14_SAT_STATUS_TH			220

#define PE_DCM_HW_M14_SAT_STATUS_TH_DEL		3
#define PE_DCM_HW_M14_SAT_STATUS_DTV_TH		192
#define PE_DCM_HW_M14_SAT_STATUS_ATV_TH		185

#define PE_DCM_HW_M14_SAT_STATUS_DTV_HIGH_TH	(PE_DCM_HW_M14_SAT_STATUS_DTV_TH+PE_DCM_HW_M14_SAT_STATUS_TH_DEL)
#define PE_DCM_HW_M14_SAT_STATUS_DTV_LOW_TH		(PE_DCM_HW_M14_SAT_STATUS_DTV_TH-PE_DCM_HW_M14_SAT_STATUS_TH_DEL)
#define PE_DCM_HW_M14_SAT_STATUS_ATV_HIGH_TH	(PE_DCM_HW_M14_SAT_STATUS_ATV_TH+PE_DCM_HW_M14_SAT_STATUS_TH_DEL)
#define PE_DCM_HW_M14_SAT_STATUS_ATV_LOW_TH		(PE_DCM_HW_M14_SAT_STATUS_ATV_TH-PE_DCM_HW_M14_SAT_STATUS_TH_DEL)

#define PE_DCM_HW_M14_BLUR_ORG_VAL			0xf

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/
/**
 *	pe dcm module settings type
 */
typedef struct
{
	UINT32 cur_blur_gain;		///< current blur gain
	UINT32 pre_blur_gain;		///< previous blur gain
}
PE_DCM_HW_M14_SETTINGS_T;

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
int PE_DCM_HW_M14_Init(void);
int PE_DCM_HW_M14_SetDebugSettings(LX_PE_DBG_SETTINGS_T *pstParams);
int PE_DCM_HW_M14_SetDceConfig(LX_PE_DCM_DCE_CONF_T *pstParams);
int PE_DCM_HW_M14_GetDceConfig(LX_PE_DCM_DCE_CONF_T *pstParams);
int PE_DCM_HW_M14_SetDceLut(LX_PE_DCM_DCE_LUT_T *pstParams);
int PE_DCM_HW_M14_GetDceLut(LX_PE_DCM_DCE_LUT_T *pstParams);
int PE_DCM_HW_M14_SetDseLut(LX_PE_DCM_DSE_LUT_T *pstParams);
int PE_DCM_HW_M14_GetDseLut(LX_PE_DCM_DSE_LUT_T *pstParams);
int PE_DCM_HW_M14_SetDceSmoothCtrl(void *pstParams);
int PE_DCM_HW_M14_GetDceSmoothCtrl(void *pstParams);
int PE_DCM_HW_M14_SetBlurGain(PE_TSK_M14_CFG_T *pCfg);
int PE_DCM_HW_M14_SetDceBlendingCtrl(LX_PE_DCM_BLENDING_T *pstParams);
int PE_DCM_HW_M14_GetDceBlendingCtrl(LX_PE_DCM_BLENDING_T *pstParams);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_DCM_HW_M14_H_ */
