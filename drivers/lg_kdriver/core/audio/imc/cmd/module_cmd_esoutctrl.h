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

#include "imc/adec_imc_cmd.h"

#define ESOUTCTRL_CMD_SET_ALL			ADEC_CMD(0x2000, EsoutctrlCmdSetAll)
#define ESOUTCTRL_CMD_SET_MUTE		ADEC_CMD(0x2003, EsoutctrlCmdSetMute)
#define ESOUTCTRL_CMD_SET_DELAY		ADEC_CMD(0x2004, EsoutctrlCmdSetDelay)

#define ESOUTCTRL_CMD_PRINT_MODULE_STATE  ADEC_CMD_SIMP(0x2020)



typedef struct _EsoutctrlCmdSetAll
{
	int Delay;              // ?
	int Fs;
	int Mute;				// 1 : Mute on, 0: Mute off
}EsoutctrlCmdSetAll;

typedef struct _EsoutctrlCmdSetDelay
{
	int Delay;              // ?
	int Fs;
}EsoutctrlCmdSetDelay;

typedef struct _EsoutctrlCmdSetMute
{
	int Mute;              // 1 : Mute on, 0: Mute off
}EsoutctrlCmdSetMute;


