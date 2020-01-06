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


#ifndef __MODULE_CMD_SPDIF_H__
#define __MODULE_CMD_SPDIF_H__

#include "imc/adec_imc_cmd.h"



#define SPDIF_CMD_SET_OUTFORMAT             ADEC_CMD(0x2000, SpdifCmdSetOutmode)
#define SPDIF_CMD_SET_FMT_FOR_SOUNDBAR	    ADEC_CMD(0x2002, SpdifCmdSetFmtForSoundbar)
#define SPDIF_CMD_SET_SCMS                  ADEC_CMD(0x2003, SpdifCmdSetScms)
#define SPDIF_CMD_CLEAR_FMT_FOR_SOUNDBAR	ADEC_CMD_SIMP(0x2004)
#define SPDIF_CMD_SET_LIGHT                 ADEC_CMD(0x2005, SpdifCmdSetLight)
#define SPDIF_CMD_ONOFF_SOUNDBAR            ADEC_CMD(0x2006, SpdifCmdOnoffSoundbar)
#define SPDIF_CMD_SET_CATEGORY_CODE			ADEC_CMD(0x2007, SpdifCmdSetCategoryCode)

#define SPDIF_CMD_SET_AUTOVOLUME_FOR_SOUNDBAR       ADEC_CMD(0x2008, SpdifCmdSetAutovolumeForSoundbar)
#define SPDIF_CMD_SET_WOOFERLEVEL_FOR_SOUNDBAR      ADEC_CMD(0x2009, SpdifCmdSetWooferlevelForSoundbar)
#define SPDIF_CMD_SET_WIRELESSRESET_FOR_SOUNDBAR    ADEC_CMD(0x2010, SpdifCmdSetWirelessresetForSoundbar)

#define SPDIF_CMD_GET_MASTERSYSTEMDELAY		ADEC_CMD(0x2011, SpdifCmdGetMasterSD)

#define SPDIF_CMD_PRINT_MODULE_STATE        ADEC_CMD_SIMP(0x2020)


typedef struct _SpdifCmdSetOutmode
{
	// PCM  = 0,
	// ES = 1,
	unsigned int out_format;
	unsigned int Fs;			// only ES
} SpdifCmdSetOutmode;


//typedef struct _SpdifCmdSetEsMute
//{
//	// Mute Off  = 0,
//	// Mute On   = 1,
//	unsigned int mute;
//} SpdifCmdSetEsMute;


typedef struct _SpdifCmdSetFmtForSoundbar
{
    unsigned int id;
    unsigned int volume;
    unsigned int mute;
}SpdifCmdSetFmtForSoundbar;


typedef struct _SpdifCmdSetScms
{
    unsigned int scms_type ; //copy free : 0, copy once : 2, copy no more : 1, copy never : 3
}SpdifCmdSetScms;



typedef struct _SpdifCmdSetLight
{
    unsigned int onoff ;        // 1 is on, 0 is off
}SpdifCmdSetLight;


typedef struct _SpdifCmdOnoffSoundbar
{
    unsigned int onoff ;        // 1 is on, 0 is off
}SpdifCmdOnoffSoundbar;

typedef struct _SpdifCmdSetCategoryCode
{
	unsigned int category_code;  // 7bit : US : 0x64, EU : 0x0c JP : 0x04
}SpdifCmdSetCategoryCode;

typedef struct _SpdifCmdSetAutovolumeForSoundbar
{
	unsigned int onoff;								// 1bit
}SpdifCmdSetAutovolumeForSoundbar;

typedef struct _SpdifCmdSetWooferlevelForSoundbar
{
	unsigned int level;								// 4bit
}SpdifCmdSetWooferlevelForSoundbar;

typedef struct _SpdifCmdSetWirelessresetForSoundbar
{
	unsigned int reset;								// 1bit
} SpdifCmdSetWirelessresetForSoundbar;

typedef struct _SpdifCmdGetMasterSD
{    
    unsigned int module_id;      // The module ID
}SpdifCmdGetMasterSD;














#endif // #ifndef __MODULE_CMD_SPDIF_H__

