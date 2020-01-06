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

#ifndef __MODULE_CMD_MIXER_H__
#define __MODULE_CMD_MIXER_H__

#include "imc/adec_imc_cmd.h"


#define MIX_CMD_PRINT_MODULE_STATE  ADEC_CMD_SIMP(0x2020)

////////////////////////////////////////////////////////////////////////////////
// Enable or disable the mixer port
////////////////////////////////////////////////////////////////////////////////
#define MIX_CMD_ENABLE_PORT				ADEC_CMD(0x4000, MixCmdEnablePort)
typedef struct _MixCmdEnablePort
{
	unsigned int		port;
	unsigned int		enable; 	// if 0, disable. otherwise, enable.
}MixCmdEnablePort;

////////////////////////////////////////////////////////////////////////////////
// Configurate mixer port
////////////////////////////////////////////////////////////////////////////////
#define MIX_CMD_SET_CONFIG				ADEC_CMD(0x4001, MixCmdSetConfig)
typedef struct _MixCmdSetConfig
{
	unsigned int		port;
	unsigned int		num_of_channel;	// currently mono or stereo is only supported.
	unsigned int		bit_per_sample;	// 16 Bit, 32 Bit
	unsigned int		gain;			// 1024 is 0dB
}MixCmdSetConfig;

////////////////////////////////////////////////////////////////////////////////
// Configurate extra mixer property
////////////////////////////////////////////////////////////////////////////////
#define MIX_CMD_SET_EXT					ADEC_CMD(0x400f, MixCmdSetExt)
typedef struct _MixCmdSetExt
{
	unsigned int		port;
	unsigned int		wait_length;
	unsigned int		initial_mute_length;
	unsigned int		enable_auto_fade_in;
	unsigned int		enable_auto_fade_out;
	unsigned int		fade_length_in_bit;	// if 9, the fade length will be 512samples
	unsigned int		init_delay_length;	// num of samples. if 48, the delay length is 1ms.
}MixCmdSetExt;


////////////////////////////////////////////////////////////////////////////////
// Partial configurable command
////////////////////////////////////////////////////////////////////////////////
#define MIX_CMD_SET_FORMAT				ADEC_CMD(0x4010, MixCmdSetFormat)
typedef struct _MixCmdSetFormat
{
	unsigned int		port;
	unsigned int		num_of_channel;	// currently mono or stereo is only supported.
	unsigned int		bit_per_sample;	// 16 Bit, 32 Bit
}MixCmdSetFormat;

#define MIX_CMD_SET_GAIN				ADEC_CMD(0x4011, MixCmdSetGain)
typedef struct _MixCmdSetGain
{
	unsigned int		port;
	unsigned int		gain;			// 1024 is 0dB
}MixCmdSetGain;

#define MIX_CMD_SET_WAIT_LENGTH			ADEC_CMD(0x4012, MixCmdSetWaitLength)
typedef struct _MixCmdSetWaitLength
{
	unsigned int		port;
	unsigned int		wait_length;
}MixCmdSetWaitLength;

#define MIX_CMD_SET_MUTE_LENGTH			ADEC_CMD(0x4013, MixCmdSetMuteLength)
typedef struct _MixCmdSetMuteLength
{
	unsigned int		port;
	unsigned int		mute_length;
}MixCmdSetMuteLength;

#define MIX_CMD_SET_FADE_CFG			ADEC_CMD(0x4014, MixCmdSetFadeCfg)
typedef struct _MixCmdSetFadeCfg
{
	unsigned int		port;
	unsigned int		enable_auto_fade_in;
	unsigned int		enable_auto_fade_out;
	unsigned int		fade_length_in_bit;
}MixCmdSetFadeCfg;

#define MIX_CMD_SET_INIT_DELAY			ADEC_CMD(0x4015, MixCmdSetInitDelay)
typedef struct _MixCmdSetInitDelay
{
	unsigned int		port;
	unsigned int		init_delay_length;
}MixCmdSetInitDelay;

#endif
