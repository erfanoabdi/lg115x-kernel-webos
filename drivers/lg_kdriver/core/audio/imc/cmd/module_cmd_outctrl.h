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

#define OUTCTRL_CMD_SET_ALL			ADEC_CMD(0x2000, OutctrlCmdSetAll)
#define OUTCTRL_CMD_SET_GAIN		ADEC_CMD(0x2001, OutctrlCmdSetGain)
#define OUTCTRL_CMD_SET_BALANCED	ADEC_CMD(0x2002, OutctrlCmdSetBalanced)
#define OUTCTRL_CMD_SET_MUTE		ADEC_CMD(0x2003, OutctrlCmdSetMute)
#define OUTCTRL_CMD_SET_DELAY		ADEC_CMD(0x2004, OutctrlCmdSetDelay)
#define OUTCTRL_CMD_SET_PCMOUTMODE	ADEC_CMD(0x2005, OutctrlCmdSetPcmoutmode)
#define OUTCTRL_CMD_SET_CHANNEL		ADEC_CMD(0x2006, OutctrlCmdSetChannel)

#define OUTCTRL_CMD_PRINT_MODULE_STATE  ADEC_CMD_SIMP(0x2020)



typedef struct _OutctrlCmdSetAll
{
	int Delay;              // ?
	int Gain;				//
	int GainEnable ;		// 1 : Gain Enable, 0: Gain disable
    int Balanced;           // -50~ 50
	int Mute;				// 1 : Mute on, 0: Mute off
}OutctrlCmdSetAll;


typedef struct _OutctrlCmdSetGain
{
	int Gain;              // ?
	int GainEnable;         //
}OutctrlCmdSetGain;


typedef struct _OutctrlCmdSetBalanced
{
	int Balanced;              // ?
}OutctrlCmdSetBalanced;


typedef struct _OutctrlCmdSetDelay
{
	int Delay;              // ?
}OutctrlCmdSetDelay;


typedef struct _OutctrlCmdSetMute
{
	int Mute;              // 1 : Mute on, 0: Mute off
}OutctrlCmdSetMute;


typedef struct _OutctrlCmdSetPcmoutmode
{
	int OutMode; 		// 0 : LR , 1 : LL, 2: RR, 3: L+R
}OutctrlCmdSetPcmoutmode;


typedef struct _OutctrlCmdSetChannel
{
	unsigned int Ch;		// must be even 2, 4 ...
}OutctrlCmdSetChannel;


