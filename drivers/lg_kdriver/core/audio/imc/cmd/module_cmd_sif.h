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

#ifndef __MODULE_CMD_SIF_H__
#define __MODULE_CMD_SIF_H__

#include "imc/adec_imc_cmd.h"

#define SIF_CMD_SET_GAIN		ADEC_CMD(0x2000, SifCmdSetGain)
#define SIF_CMD_SET_FMT			ADEC_CMD(0x2001, SifCmdSetFmt)

#define SIF_CMD_DEBUG_STATUS	ADEC_CMD_SIMP(0x2020)


typedef struct _SifCmdSetGain
{
	unsigned int gain;
}SifCmdSetGain;

typedef struct _SifCmdSetFmt
{
	// SAI_PCM_FMT_I2S = 0,
	// SAI_PCM_FMT_LJ = 1,
	// SAI_PCM_FMT_RJ = 2,
	unsigned int format;

	// SAI_PCM_RES_16BIT = 0,
	// SAI_PCM_RES_18BIT = 1,
	// SAI_PCM_RES_20BIT = 2,
	// SAI_PCM_RES_24BIT = 3,
	unsigned int resolution;

	//0 : normal
	//1 : invert
	unsigned int sck_polarity_control;

	//0 : left channel data on LRCK = '0'
	//1 : left channel data on LRCK = '1'
	unsigned int lrck_polarity_control;
}SifCmdSetFmt;

#endif
