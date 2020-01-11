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

#ifndef __MODULE_CMD_RTS_H__
#define __MODULE_CMD_RTS_H__

#include "imc/adec_imc_cmd.h"

////////////////////////////////////////////////////////////////////////////////
// Set threshold
////////////////////////////////////////////////////////////////////////////////
#define RTS_CMD_SET_THRESHOLD                   ADEC_CMD(0x8200, RtsCmdSetThreashold)
typedef struct _RtsCmdSetThreshold
{
    unsigned int        upper_th;
    unsigned int        lower_th;
    unsigned int        recover_upper;
    unsigned int        recover_lower;
}RtsCmdSetThreashold;


////////////////////////////////////////////////////////////////////////////////
// Set src module
////////////////////////////////////////////////////////////////////////////////
#define  RTS_CMD_SET_SRC_MODULE                 ADEC_CMD(0x8201, RtsCmdSetSrcModule)
typedef struct _RtsCmdSetSrcModule
{
    unsigned int        src_module_id;
}RtsCmdSetSrcModule;


////////////////////////////////////////////////////////////////////////////////
// Set output frequency
////////////////////////////////////////////////////////////////////////////////
#define RTS_CMD_SET_OUT_FREQ                    ADEC_CMD(0x8202, RtsCmdSetOutFreq)
typedef struct _RtsCmdSetOutFreq
{
    unsigned int        fast_freq;      // Default 47750    (192/191)
    unsigned int        normal_freq;    // Default 48000
    unsigned int        slow_freq;      // Default 48250    (192/193)
}RtsCmdSetOutFreq;

// fast freq < normal freq < slow freq


////////////////////////////////////////////////////////////////////////////////
// Set port rational multiplier
////////////////////////////////////////////////////////////////////////////////
#define RTS_CMD_SET_PORT_MULTPLIER              ADEC_CMD(0x8203, RtsCmdSetPortMultiplier)
typedef struct _RtsCmdSetPortMultiplier
{
    unsigned int        ref_port;
    unsigned int        numerator;
    unsigned int        denominator;
}RtsCmdSetPortMultiplier;

// Optional command to estimate the buffering time more exactly.
// [estimated buf time] = [buf size] * [numerator] / [denominator];
// ex1) the input source is 16bit mono 24khz pcm
// numerator = 2 * 2 * 48, denominator = 24  => num = 8, denom = 1
// ex2) the input soruce is 44.1khz 32bit stereo pcm
// numerator = 48, denominator = 44.1 => num = 160, denom = 147


////////////////////////////////////////////////////////////////////////////////
// Set operation mode
////////////////////////////////////////////////////////////////////////////////
#define RTS_CMD_SET_OPERATION_MODE              ADEC_CMD(0x8210, RtsCmdSetOperationMode)
typedef struct _RtsCmdSetOperationMode
{
    unsigned int        op_mode;
    // op_mode : 0 => Continuous mode (Control SRC). Default.
    // op_mode : 1 => Skipping mode (Flush Buffers).
    // op_mode : 2 => Hybrid mode
}RtsCmdSetOperationMode;


////////////////////////////////////////////////////////////////////////////////
// Set operation mode
////////////////////////////////////////////////////////////////////////////////
#define RTS_CMD_SET_SKIPPING_CONFIG          ADEC_CMD(0x8211, RtsCmdSetSkippingConfig)
typedef struct _RtsCmdSetSkippingConfig
{
    unsigned int        skipping_threshold;
    unsigned int        skipping_recovery;
    ADEC_MODULE_ID      mixer_module_id;        // Currently not used. Reserved for preventing pop-up noise.
    int                 mixer_port;             // Currently not used. Reserved for preventing pop-up noise.
}RtsCmdSetSkippingConfig;


#endif
