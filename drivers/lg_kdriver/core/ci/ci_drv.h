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
 *  driver interface header for ci device. ( used only within kdriver )
 *	ci device will teach you how to make device driver with new platform.
 *
 *  @author		Srinivasan Shanmugam	(srinivasan.shanmugam@lge.com)
  * @author		Hwajeong Lee (hwajeong.lee@lge.com)
 *  @author		Jinhwan Bae (jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2009.12.30
 *
 *  @addtogroup lg1150_ci
 *	@{
 */

#ifndef	_CI_DRV_H_
#define	_CI_DRV_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "debug_util.h"
#include "ci_defs.h"
#include "ci_kapi.h"

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
#if 0 // before '13 logm jinhwan.bae
#if 1		// for no display a error message
#define	CI_PRINT(format, args...)		DBG_PRINT(  g_ci_debug_fd, 0, format, ##args)
#else 
#define	CI_PRINT(format, args...)		/* NOP */
#endif


#define	CI_TRACE(format, args...)		DBG_PRINTX( g_ci_debug_fd, 0, format, ##args)
#endif

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
extern	void     CI_PreInit(void);
extern	int      CI_Init(void);
extern	void     CI_Cleanup(void);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern	int		g_ci_debug_fd;

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _CI_DRV_H_ */

/** @} */
