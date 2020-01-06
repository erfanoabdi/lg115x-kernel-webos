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

#ifndef __MODULE_CMD_INPUTCTRL_H__
#define __MODULE_CMD_INPUTCTRL_H__

#include "imc/adec_imc_cmd.h"

#define INPUTCTRL_CMD_SET_LIP_ONOFF			ADEC_CMD(0x2000, InputctrlCmdSetLipOnoff)
#define INPUTCTRL_CMD_SET_LIP_BOUND			ADEC_CMD(0x2001, InputctrlCmdSetLipBound)
#define INPUTCTRL_CMD_SET_LIP_CLOCKTYPE		ADEC_CMD(0x2002, InputctrlCmdSetLipclocktype)
#define INPUTCTRL_CMD_SET_LIP_BASE			ADEC_CMD(0x2003, InputctrlCmdSetLipBase)
#define INPUTCTRL_CMD_SET_LIP_FS			ADEC_CMD(0x2004, InputctrlCmdSetLipFs)
#define INPUTCTRL_CMD_SET_LIP_DEBUGPRINT	ADEC_CMD(0x2005, InputctrlCmdSetLipDebugprint)
#define INPUTCTRL_CMD_SET_LIP_NODELAY		ADEC_CMD(0x2006, InputctrlCmdSetLipNodelay)

#define INPUTCTRL_CMD_SET_DELAY				ADEC_CMD(0x2007, InputctrlCmdSetDelay)
#define INPUTCTRL_CMD_SET_GAIN				ADEC_CMD(0x2008, InputctrlCmdSetGain)
#define	INPUTCTRL_CMD_SET_MUTE				ADEC_CMD(0x2009, InputctrlCmdSetMute)
#define INPUTCTRL_CMD_SET_CH_GAIN			ADEC_CMD(0x200a, InputctrlCmdSetChGain)

#define INPUTCTRL_CMD_GET_SYSTEMDELAY		ADEC_CMD_SIMP(0x2010)

#define INPUTCTRL_CMD_PRINT_MODULE_STATE	ADEC_CMD_SIMP(0x2020)

typedef struct _InputctrlCmdSetLipOnoff{
	// 0 : off ,  1 : on
	unsigned int onoff;
}InputctrlCmdSetLipOnoff;

typedef struct _InputctrlCmdSetLipBound{
	unsigned int lbound;
	unsigned int ubound;
	unsigned int offset;
	unsigned int freerunlbound;
	unsigned int freerunubound;
}InputctrlCmdSetLipBound;

typedef struct _InputctrlCmdSetLipclocktype{
	// 0x01 : pcrM,  0x02 : pcrA, 0x11 : None pcr, 0x100 : gstc
	unsigned int clocktype;
}InputctrlCmdSetLipclocktype;

typedef struct _InputctrlCmdSetLipBase{
	unsigned int clockbase;
	unsigned int streambase;
}InputctrlCmdSetLipBase;


typedef struct _InputctrlCmdSetLipFs{
	unsigned int Fs;
}InputctrlCmdSetLipFs;

typedef struct _InputctrlCmdSetLipDebugprint{
	unsigned int interval;		// ms, 0 is off,
}InputctrlCmdSetLipDebugprint;

typedef struct _InputctrlCmdSetLipNodelay{
	unsigned int onoff;
	unsigned int upperthreshold;		//msec
	unsigned int lowerthreshold;		//msec
	unsigned int prebyteper1sec;		// bytes(per sample) * channel * Hz (ex 4byte * 2ch * 48000 Hz)
	unsigned int posbyteper1sec;		// maybe 384000 in H13 (4byte * 2ch * 48000 Hz)
}InputctrlCmdSetLipNodelay;

typedef struct _InputctrlCmdSetDelay{
	unsigned int delay;
}InputctrlCmdSetDelay;

typedef struct _InputctrlCmdSetGain{
	unsigned int Gain;              // ?
	unsigned int GainEnable;         //
}InputctrlCmdSetGain;

typedef struct _InputctrlCmdSetMute{
	unsigned int Mute;				// 0 is mute off, 1 is mute on
}InputctrlCmdSetMute;

typedef struct _InputctrlCmdSetChGain{
	unsigned int LeftGain;              //
	unsigned int RightGain; 	       //
}InputctrlCmdSetChGain;

#endif
