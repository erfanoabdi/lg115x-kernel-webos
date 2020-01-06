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



/**
 *	@file adec_imc_cmd.h
 *
 *
 *	@author		Jung, Kyung Soon (kyungsoon.jung@leg.com)
 *	@version	0.0.1
 *	@date		2012-03-28
 *	@note
 *	@see		http://www.lge.com
 *	@addtogroup ADEC
 *	@{
 */


/******************************************************************************
  Header File Guarder
 ******************************************************************************/
#ifndef __ADEC_IMC_CMD_H__
#define __ADEC_IMC_CMD_H__

#include "common/adec_config.h"

#define ADEC_CMD(_ID, _PARAM_TYPE)	((sizeof(_PARAM_TYPE) & 0xFFFF) | ((_ID&0xFFFF)<<16))
#define ADEC_CMD_SIMP(_ID)			((_ID&0xFFFF)<<16)
#define ADEC_CMD_PARAM_LEN(_CMD)	((_CMD&0xFFFF))
#define ADEC_CMD_TAG(_CMD)			((_CMD>>16) & 0xFFFF)
/*
 CMD Format
                8               16              24             32
 +--------------+---------------+---------------+--------------+
 |    CMD_TAG                   |                 PARAM_LENGTH |
 +--------------+---------------+---------------+--------------+

 PARAM_LENGTH : Length of the parameter.
 CMD_TAG : CMD type indicator.

 */

#define ADEC_CMD_INIT			ADEC_CMD_SIMP(0x1000)
#define ADEC_CMD_START			ADEC_CMD_SIMP(0x1001)
#define ADEC_CMD_STOP			ADEC_CMD_SIMP(0x1002)
#define ADEC_CMD_FLUSH			ADEC_CMD(0x1003, ImcCmdFlushParam)
#define ADEC_CMD_GET_DECINFO	ADEC_CMD_SIMP(0x1004) // For DEC module only.
#define ADEC_CMD_SET_OUTMODE	ADEC_CMD(0x1005, ImcCmdSetOutputMode) // For DEC module only.
#define ADEC_CMD_SET_DEBUGCNT   ADEC_CMD(0x1006, ImcCmdSetDebugCount)
#define ADEC_CMD_MULTI			ADEC_CMD(0x1010, ImcCmdMultiParam)

typedef struct _ImcCmdFlushParam
{
	unsigned int	num_of_port;
	unsigned int	port_list[ADEC_CNST_IMC_MAX_PORT_COUNT];
}ImcCmdFlushParam;

typedef struct _ImcCmdSetOutputMode
{
	unsigned int output_mode;	// LR : 0, LL : 1, RR : 2, L+R : 3
}ImcCmdSetOutputMode;

typedef struct _ImcCmdSetDebugCount
{
    unsigned int debug_cnt;     // 0 : Every 1 sec, N : Every N frame.
}ImcCmdSetDebugCount;


#endif //__ADEC_IMC_CMD_H__
/** @} */
