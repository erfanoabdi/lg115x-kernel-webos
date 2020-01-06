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

/** @file pe_win_hw.h
 *
 *  driver header for picture enhance window control functions. ( used only within kdriver )
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2012.04.16
 *	@see		
 */

#ifndef	_PE_WIN_HW_H_
#define	_PE_WIN_HW_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
/* the latest chip : the upper position */
#ifdef USE_PE_KDRV_CODES_FOR_M14
#include "pe_win_hw_m14.h"
#endif
#ifdef USE_PE_KDRV_CODES_FOR_H14
#include "pe_win_hw_h14.h"
#endif
#ifdef USE_PE_KDRV_CODES_FOR_H13
#include "pe_win_hw_h13.h"
#endif

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

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_WIN_HW_H_ */

