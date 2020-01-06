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
 *  @author		won.hur (won.hur@lge.com)
 *  @version	1.0
 *  @date		2012.04.17
 *
 *  @addtogroup lg115x_vbi
 *	@{
 */

#ifndef	_VBI_DRV_H_
#define	_VBI_DRV_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "debug_util.h"
#include "vbi_cfg.h"
#include "vbi_kapi.h"

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
#define	VBI_PRINT(format, args...)			DBG_PRINT( g_vbi_debug_fd, LX_LOGM_LEVEL_NOTI, format, ##args)
#define	VBI_TRACE(format, args...)			DBG_PRINT( g_vbi_debug_fd, LX_LOGM_LEVEL_TRACE, format, ##args)

#define	TTX_PRINT						VBI_PRINT	
#define	TTX_TRACE 						VBI_TRACE	
#define	CC_PRINT						VBI_PRINT
#define	CC_TRACE						VBI_TRACE

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
extern	void     VBI_PreInit(void);
extern	int      VBI_Init(void);
extern	void     VBI_Cleanup(void);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern	int		g_vbi_debug_fd;

extern void    VBI_InitCfg ( void );

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _VBI_DRV_H_ */

/** @} */
