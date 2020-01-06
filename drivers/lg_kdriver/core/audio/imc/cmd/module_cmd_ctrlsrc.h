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

#ifndef __MODULE_CMD_CTRLSRC_H__
#define __MODULE_CMD_CTRLSRC_H__

#include "imc/adec_imc_cmd.h"

#define CTRLSRC_CMD_SET_SRC_MODULE          ADEC_CMD(0x2000, CtrlsrcCmdSetSrcModule)
#define CTRLSRC_CMD_SET_INTINFO             ADEC_CMD(0x2001, CtrlsrcCmdSetIntInfo)
#define CTRLSRC_CMD_SET_SRCRUNNINGMODE		ADEC_CMD(0x2002, CtrlsrcCmdSetSrcrunningmode)

#define CTRLSRC_CMD_PRINT_MODULE_STATE	    ADEC_CMD_SIMP(0x2020)

typedef struct _CtrlsrcCmdSetSrcModule
{
    unsigned int        src_module_id;
}CtrlsrcCmdSetSrcModule;


typedef struct _CtrlsrcCmdSetIntInfo
{
	unsigned int RefInt;
	unsigned int TarInt;
}CtrlsrcCmdSetIntInfo;

typedef struct _CtrlsrcCmdSetSrcrunningmode
{
    unsigned int	src_running_mode;		// 0 is default, 1 is fastmode ;
}CtrlsrcCmdSetSrcrunningmode;


#endif
