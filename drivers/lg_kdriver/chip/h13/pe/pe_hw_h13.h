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

/** @file pe_hw_h13.h
 *
 *  hw header for picture enhance. ( used only within kdriver )
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2013.04.28
 *	@see		
 */

#ifndef	_PE_HW_H13_H_
#define	_PE_HW_H13_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "pe_cfg.h"
#include "pe_reg.h"
#include "pe_def.h"
#include "pe_etc.h"
#include "pe_fwi.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
/* h13b0 use pe0 apl for dce */
#define PE_HW_H13B_USE_PE0_APL_FOR_DCE

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_HW_H13_H_ */
