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


#ifndef __MODULE_CMD_MP3_H__
#define __MODULE_CMD_MP3_H__

#include "imc/adec_imc_cmd.h"

#define MP3_CMD_SET_PARAM       ADEC_CMD(0x2000, Mp3CmdSetParam)

#define MP3_CMD_DEBUG_STATUS    ADEC_CMD_SIMP(0x2020)

typedef struct _Mp3CmdSetParam
{
    unsigned int     drc_mode;         // DRC mode               : 0 = Line mode(default),  1 = RF mode
    unsigned int     isSubDec;         // Sub Decoding           : 0 = Main Dec, 1 = Sub Dec
    unsigned int     isADMode;         // AD mode                : 0 = Off(default), 1 = On (DEC0, DEC1 must set)
} Mp3CmdSetParam;

#endif // #ifndef __MODULE_CMD_MP3_H__

