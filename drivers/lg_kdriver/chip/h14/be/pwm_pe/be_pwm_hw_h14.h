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


/** @file
 *
 *  PWM reg header file for BE device
 *	
 *  author		dj911.kim
 *  version		1.0
 *  date		2013.04.03
 *  note		Additional information.
 *
 *  @addtogroup BE
 *	@{
 */

#ifndef _BE_PWM_HW_H14_H_
#define _BE_PWM_HW_H14_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include "be_kapi.h"

#include "base_types.h"

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
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
int BE_PWM_HW_H14_Init(void);
int BE_PWM_HW_H14_SetPwmControl(BE_PWM_CTRL_T *pstParams);
int BE_PWM_HW_H14_SetPwmAdaptFreqControl(BE_PWM_ADAPT_FREQ_CTRL_T *pstParams);
int BE_PWM_HW_H14_SetPwmFrequency(BE_PWM_FREQ_T *pstParams);
int BE_PWM_HW_H14_SetPwmDutyCycle(BE_PWM_DUTY_T *pstParams);
int BE_PWM_HW_H14_GetPwmInfo(BE_EXTERNAL_PWM_T *pstParams);

int BE_PWM_HW_H14_SetPwmSync(BE_PWM_FREQ_T *pstParams);

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

#endif /* _BE_PWM_HW_H14_H_ */

/** @} */



