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


#ifndef __MODULE_CMD_BYPASS_ES_H__
#define __MODULE_CMD_BYPASS_ES_H__

#include "imc/adec_imc_cmd.h"

#define BYPASSES_CMD_SET_MODE		ADEC_CMD(0xB000, BypassESCmdSetMode)

typedef struct _BypassESCmdSetMode
{
	// 0 - Byte Based Mode
	// 1 - AU Based Mode
	unsigned int		mode;

	// 0 - No Overflow Protection
	// 1 - Enable Overflow Protection
	unsigned int		over_protect;

	// 0 - AAC
	// 1 - HE-AAC v1
	// 2 - HE-AAC v2
	unsigned int		version;
}BypassESCmdSetMode;


#endif // #ifndef __MODULE_CMD_BYPASS_ES_H__

