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

/** @file pe_csc_hw_h14.h
 *
 *  driver header for picture enhance csc functions. ( used only within kdriver )
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2012.03.15
 *	@see		
 */

#ifndef	_PE_CSC_HW_H14_H_
#define	_PE_CSC_HW_H14_H_

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

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
int PE_CSC_HW_H14_Init(void);
int PE_CSC_HW_H14_SetDebugSettings(LX_PE_DBG_SETTINGS_T *pstParams);
int PE_CSC_HW_H14_SetColorGamut(LX_PE_CSC_GAMUT_T *pstParams);
int PE_CSC_HW_H14_GetColorGamut(LX_PE_CSC_GAMUT_T *pstParams);
int PE_CSC_HW_H14_SetPostCsc(LX_PE_CSC_POST_T *pstParams);
int PE_CSC_HW_H14_GetPostCsc(LX_PE_CSC_POST_T *pstParams);
int PE_CSC_HW_H14_SetInputCsc(LX_PE_CSC_INPUT_T *pstParams);
int PE_CSC_HW_H14_GetInputCsc(LX_PE_CSC_INPUT_T *pstParams);
int PE_CSC_HW_H14_SetxvYCC(LX_PE_CSC_XVYCC_T *pstParams);
int PE_CSC_HW_H14_GetxvYCC(LX_PE_CSC_XVYCC_T *pstParams);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_CSC_HW_H14_H_ */
