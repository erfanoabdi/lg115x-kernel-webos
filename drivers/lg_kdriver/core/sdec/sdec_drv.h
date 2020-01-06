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
 *  sdec driver
 *
 *  @author	Jihoon Lee ( gaius.lee@lge.com)
 *  @author	Jinhwan Bae ( jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010-03-30
 *  @note		Additional information.
 */

#ifndef	_SDEC_DRV_H_
#define	_SDEC_DRV_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "debug_util.h"
#include "sdec_cfg.h"
#include "sdec_kapi.h"
#include "sdec_io.h"

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
#define LGSI_BUG_FIX

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/
#define GPB_BASE_ADDR_MASK		0xF0000000

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
extern	void     SDEC_PreInit(void);
extern	int      SDEC_Init(void);
extern	void     SDEC_Cleanup(void);

extern 	int 	SDEC_ResumeInit(S_SDEC_PARAM_T *stpSdecParam);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern	int		g_sdec_debug_fd;
//This enumeration describes the return type type of the DDI function.

/*----------------------------------------------------------------------------------------
   enum
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
   structre
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
   API
----------------------------------------------------------------------------------------*/



#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _SDEC_DRV_H_ */

/** @} */
