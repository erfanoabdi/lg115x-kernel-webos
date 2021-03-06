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
 *  driver interface header for sci device. ( used only within kdriver )
 *	this file lists exported function, types for the external modules.
 *
 *  @author		bongrae.cho (bongrae.cho@lge.com)
 *  @version	1.0
 *  @date		2012.02.23
 *
 *  @addtogroup lg1154_sci
 *	@{
 */
#ifndef	_SCI_DRV_H_
#define	_SCI_DRV_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "base_types.h"
//FIXME
//#include "sci_kapi.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define SCI_MODULE  "sci"
#define SCI_MAX_DEVICE  1

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
void SCI_PreInit(void);
int      		SCI_Init					(void);
void     		SCI_Cleanup					(void);



#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _SCI_DRV_H_ */

/** @} */
