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

#ifndef __MODULE_CMD_LIPSYNC_H__
#define __MODULE_CMD_LIPSYNC_H__

#include "imc/adec_imc_cmd.h"


#define LIPSYNC_CMD_SET_ONOFF			ADEC_CMD(0x2000, LipsyncCmdSetOnoff)
#define LIPSYNC_CMD_SET_BOUND			ADEC_CMD(0x2001, LipsyncCmdSetBound)
#define LIPSYNC_CMD_SET_CLOCKTYPE		ADEC_CMD(0x2002, LipsyncCmdSetclocktype)
#define LIPSYNC_CMD_SET_BASE			ADEC_CMD(0x2003, LipsyncCmdSetBase)
#define LIPSYNC_CMD_SET_FS              ADEC_CMD(0x2004, LipsyncCmdSetFs)
#define LIPSYNC_CMD_SET_DEBUGPRINT      ADEC_CMD(0x2005, LipsyncCmdSetDebugprint)
#define LIPSYNC_CMD_SET_NODELAY			ADEC_CMD(0x2006, LipsyncCmdSetNodelay)
#define LIPSYNC_CMD_SET_RATE			ADEC_CMD(0x2007, LipsyncCmdSetRate)
#define LIPSYNC_CMD_SET_DATATYPE		ADEC_CMD(0x2008, LipsyncCmdSetDatatype)
#define LIPSYNC_CMD_SET_BUFAFTERLIP		ADEC_CMD(0x2009, LipsyncCmdSetBufAfterLip)

#define LIPSYNC_CMD_PRINT_MODULE_STATE	ADEC_CMD_SIMP(0x2020)

typedef struct _LipsyncCmdSetOnoff{
	// 0 : off ,  1 : on
	unsigned int onoff;
}LipsyncCmdSetOnoff;

typedef struct _LipsyncCmdSetBound{
	unsigned int lbound;
	unsigned int ubound;
	unsigned int offset;
	unsigned int freerunlbound;
	unsigned int freerunubound;
}LipsyncCmdSetBound;

typedef struct _LipsyncCmdSetclocktype{
	// 0x01 : pcrM,  0x02 : pcrA, 0x11 : None pcr, 0x100 : gstc
	unsigned int clocktype;
}LipsyncCmdSetclocktype;

typedef struct _LipsyncCmdSetBase{
	unsigned int clockbase;
	unsigned int streambase;
}LipsyncCmdSetBase;

typedef struct _LipsyncCmdSetFs{
	unsigned int Fs;
}LipsyncCmdSetFs;

typedef struct _LipsyncCmdSetDebugprint{
	unsigned int interval;		// ms, 0 is off,
}LipsyncCmdSetDebugprint;

typedef struct _LipsyncCmdSetNodelay{
	unsigned int onoff;
	unsigned int upperthreshold;		//msec
	unsigned int lowerthreshold;		//msec
	unsigned int prebyteper1sec;		// bytes(per sample) * channel * Hz (ex 4byte * 2ch * 48000 Hz)
	unsigned int posbyteper1sec;		// maybe 384000 in H13 (4byte * 2ch * 48000 Hz)
}LipsyncCmdSetNodelay;

typedef struct _LipsyncCmdSetRate{
	unsigned int in; 				// ex) x1 : 100 , x2 : 200 , x0.5 : 50
	unsigned int out;				// alway 100
}LipsyncCmdSetRate;

typedef enum _eLIPDATATYPE{
	LIPDATATYPE_PCM=0,
	LIPDATATYPE_ES
}eLIPDATATYPE;

typedef struct _LipsyncCmdSetDatatype{
	unsigned int datatype;						// eLIPDATATYPE, 0 is pcm, 1 is ES
}LipsyncCmdSetDatatype;


typedef struct _LipsyncCmdSetBufAfterLip
{
    unsigned int		num_of_modules;     // Num of modules connected in series
    struct {
        unsigned int	module_id;      // The module ID
        unsigned int	module_port;    // Port of the module. Recommended : Using input port only
    }module_list[6];
}LipsyncCmdSetBufAfterLip;


#endif
