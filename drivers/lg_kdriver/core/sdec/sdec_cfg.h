/*
 *  SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA 
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
 *  main configuration file for sdec device
 *  sdec device will teach you how to make device driver with new platform.
 *
 *  @author	Jihoon Lee ( gaius.lee@lge.com)
 *  @author	Jinhwan Bae ( jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010-03-30
 *  @note		Additional information.
 */


#ifndef	_SDEC_CFG_H_
#define	_SDEC_CFG_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "base_types.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define	SDEC_MODULE			"sdec"
#define SDEC_MAX_DEVICE		1

typedef struct
{
	char* 	memory_name;
	UINT32	gpb_memory_base;
	UINT32 	gpb_memory_size;
} LX_SDEC_MEM_CFG_T;

typedef struct
{
	UINT32	capa_lev:8,			///< capacity level of channel ( full : 1 / simple : 0 ). simple channel has only ts2pes parser for channle browser.
			num_pidf:8,			///< number of pid filter in this channel
			num_secf:8, 			///< number of section filter in this channel
			flt_dept:8; 			///< filter depth
} LX_SDEC_CHA_INFO_T;


typedef struct
{
	UINT32				chip_rev;			///< overall chip revision. use LX_CHIP_REV() macro.
	char*				name;				///< canonical name for debugging.

	LX_SDEC_CHA_INFO_T	*chInfo;			///< channel information
	UINT32				nChannel:8,		///< number of channels
						nVdecOutPort :8,	///< number of vdec output port.
						memCfg :8,			///< memory config index in gMemCfgSDECGPB
						noPesBug :1,		///< pes h/w bug fixed?
						staticGPB :1,		///< GPB is static?
						reserved0 :6;		///< reserved
} LX_SDEC_CFG_T;


/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/
LX_SDEC_CFG_T* SDEC_CFG_GetConfig(void);

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern LX_SDEC_MEM_CFG_T gMemCfgSDECGPB[];
extern LX_SDEC_CFG_T	*gpSdecCfg;

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _SDEC_CFG_H_ */

/** @} */

