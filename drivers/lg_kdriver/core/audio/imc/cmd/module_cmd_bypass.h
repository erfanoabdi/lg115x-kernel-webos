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

#ifndef __MODULE_CMD_BYPASS_H__
#define __MODULE_CMD_BYPASS_H__

#include "imc/adec_imc_cmd.h"

#define BYPASS_CMD_SET_MODE         ADEC_CMD(0xB000, BypassCmdSetMode)

#define BYPASS_CMD_DEBUG_STATUS     ADEC_CMD_SIMP(0x2020)

typedef struct _BypassCmdSetMode
{
	// 0 - Byte Based Mode
	// 1 - AU Based Mode
	unsigned int		mode;

	// 0 - No Overflow Protection
	// 1 - Enable Overflow Protection
	unsigned int		over_protect;
}BypassCmdSetMode;


#endif
