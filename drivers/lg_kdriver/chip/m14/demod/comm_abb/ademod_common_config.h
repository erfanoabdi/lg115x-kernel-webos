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


/*****************************************************************************
**
**  Name: ademod_common_config.h
**
**  Description:    ABB Video Processing block.
**
**  Functions
**  Implemented:    LX_ADEMOD_Result  
**
**  References:     
**
**  Exports:        N/A
**
**  Dependencies:  ademod_common.h   - for basic FM types,
**
**  Revision History:
**
**     Date        Author          Description
**  -------------------------------------------------------------------------
**   31-07-2013   Jeongpil Yun    Initial draft.
*****************************************************************************/
#ifndef _ADEMOD_COMMON_CONFIG_H_
#define _ADEMOD_COMMON_CONFIG_H_

#include "ademod_common.h"
#include "ademod_common_demod.h"
//#include "fm2050_config.h"
#include "ademod_m14_config.h"

#if defined( __cplusplus )
extern "C"                     /* Use "C" external linkage                  */
{
#endif

// All Fresco devices
#define LX_ADEMOD_DEVICE_CNT					(1)  

#if !defined( LX_ADEMOD_DEVICE_CNT )
#define LX_ADEMOD_DEVICE_CNT					(0)
#endif

// Host or Direct mode of operation 1-Host, 0-Direct.


#if defined( __cplusplus )
}
#endif

#endif  //_ADEMOD_COMMON_CONFIG_H_
