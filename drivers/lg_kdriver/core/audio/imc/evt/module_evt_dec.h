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



/**
 *	@file module_evt_dec.h
 *
 *
 *	@author		Jung, Kyung Soon (kyungsoon.jung@leg.com)
 *	@version	0.0.1
 *	@date		2012-07-20
 *	@note
 *	@see		http://www.lge.com
 *	@addtogroup ADEC
 *	@{
 */


/******************************************************************************
  Header File Guarder
 ******************************************************************************/
#ifndef __MODULE_EVT_DEC_H__
#define __MODULE_EVT_DEC_H__

#include "common/adec_media_type.h"
#include "imc/adec_imc_evt.h"


#define DEC_EVT_DEC_ERROR           0x0A00
#define DEC_EVT_ES_DEC_INFO         0x0A01
#define DEC_EVT_ES_ENC_INFO         0x0A02
#define DEC_EVT_ES_EXIST_INFO       0x0A03
#define DEC_EVT_DECODED_INDEX       0x0A04
#define DEC_EVT_DECODED_END         0x0A05
#define DEC_EVT_REQ_SPDIF_TYPE      0x0A06

/* PCM Types of channel modes */
typedef enum _PCMDEC_CHANNEL_MODE
{
    PCMDEC_CHANNELMODE_UNDEFINED = 0,
    PCMDEC_CHANNELMODE_1CH,
    PCMDEC_CHANNELMODE_2CH,
    PCMDEC_CHANNELMODE_3CH,
    PCMDEC_CHANNELMODE_4CH,
    PCMDEC_CHANNELMODE_5CH,
    PCMDEC_CHANNELMODE_6CH
} PCMDEC_CHANNEL_MODE;

/* MS10DDC Types of channel modes */
typedef enum _MS10DDC_CHANNEL_MODE
{
    MS10DDC_CHANNELMODE_DUAL_MONO = 0,
    MS10DDC_CHANNELMODE_MONO_CENTER,
    MS10DDC_CHANNELMODE_STEREO,
    MS10DDC_CHANNELMODE_3_0_CHANNEL,
    MS10DDC_CHANNELMODE_2_1_CHANNEL,
    MS10DDC_CHANNELMODE_3_1_CHANNEL,
    MS10DDC_CHANNELMODE_2_2_CHANNEL,
    MS10DDC_CHANNELMODE_3_2_CHANNEL
} MS10DDC_CHANNEL_MODE;

/* MS10DDT Types of channel modes */
typedef enum _MS10DDT_CHANNEL_MODE
{
    MS10DDT_CHANNELMODE_UNDEFINED = 0,
    MS10DDT_CHANNELMODE_MONO,
    MS10DDT_CHANNELMODE_PARAMETRIC_STEREO,
    MS10DDT_CHANNELMODE_DUAL_CHANNEL,
    MS10DDT_CHANNELMODE_STEREO,
    MS10DDT_CHANNELMODE_3_CHANNEL_FRONT,
    MS10DDT_CHANNELMODE_3_CHANNEL_SURR,
    MS10DDT_CHANNELMODE_4_CHANNEL_2SURR,
    MS10DDT_CHANNELMODE_4_CHANNEL_1SURR,
    MS10DDT_CHANNELMODE_5_CHANNEL,
    MS10DDT_CHANNELMODE_6_CHANNEL,              /* Not supported */
    MS10DDT_CHANNELMODE_7_CHANNEL,              /* Not supported */
    MS10DDT_CHANNELMODE_2_1_STEREO,
    MS10DDT_CHANNELMODE_3_1_CHANNEL_FRONT,
    MS10DDT_CHANNELMODE_3_1_CHANNEL_SURR,
    MS10DDT_CHANNELMODE_4_1_CHANNEL_2SURR,
    MS10DDT_CHANNELMODE_4_1_CHANNEL_1SURR,
    MS10DDT_CHANNELMODE_5_1_CHANNEL,
    MS10DDT_CHANNELMODE_6_1_CHANNEL,             /* Not supported */
    MS10DDT_CHANNELMODE_7_1_CHANNEL              /* Not supported */
} MS10DDT_CHANNEL_MODE;

/* MP3DEC Types of channel modes */
typedef enum _MP3DEC_CHANNEL_MODE
{
    MP3DEC_CHANNELMODE_STEREO = 0,
    MP3DEC_CHANNELMODE_JOINT_STEREO,
    MP3DEC_CHANNELMODE_DUAL_MONO,
    MP3DEC_CHANNELMODE_MONO
} MP3DEC_CHANNEL_MODE;

/* WMADEC Types of channel modes */
typedef enum _WMADEC_CHANNEL_MODE
{
    WMADEC_CHANNELMODE_STEREO = 0x3,
    WMADEC_CHANNELMODE_MONO   = 0x4,
    WMADEC_CHANNELMODE_THREE  = 0x7,
    WMADEC_CHANNELMODE_BACK   = 0x30,
    WMADEC_CHANNELMODE_QUAD   = 0x33,
    WMADEC_CHANNELMODE_5      = 0x37,
    WMADEC_CHANNELMODE_5DOT1  = 0x3F,
    WMADEC_CHANNELMODE_6DOT1  = 0x13F,
    WMADEC_CHANNELMODE_7DOT1  = 0xFF
} WMADEC_CHANNEL_MODE;

/* DTSDEC Types of channel modes */
typedef enum _DTSDEC_CHANNEL_MODE
{
    DTSDEC_CHANNELMODE_UNDEFINED = 0,
    DTSDEC_CHANNELMODE_1CH,
    DTSDEC_CHANNELMODE_2CH,
    DTSDEC_CHANNELMODE_3CH,
    DTSDEC_CHANNELMODE_4CH,
    DTSDEC_CHANNELMODE_5CH,
    DTSDEC_CHANNELMODE_6CH
} DTSDEC_CHANNEL_MODE;

/* OGGDEC Types of channel modes */
typedef enum _OGGDEC_CHANNEL_MODE
{
    OGGDEC_CHANNELMODE_UNDEFINED = 0,
    OGGDEC_CHANNELMODE_1CH,
    OGGDEC_CHANNELMODE_2CH
} OGGDEC_CHANNEL_MODE;

/* AMR_WB DEC Types of channel modes */
typedef enum _AMRWBDEC_CHANNEL_MODE
{
    AMRWBDEC_CHANNELMODE_UNDEFINED = 0,
    AMRWBDEC_CHANNELMODE_MONO,
} AMRWBDEC_CHANNEL_MODE;

/* AMR_NB DEC Types of channel modes */
typedef enum _AMRNBDEC_CHANNEL_MODE
{
    AMRNBDEC_CHANNELMODE_UNDEFINED = 0,
    AMRNBDEC_CHANNELMODE_MONO,
} AMRNBDEC_CHANNEL_MODE;

/* RA8DEC Types of channel modes */
typedef enum _RA8DEC_CHANNEL_MODE
{
    RA8DEC_CHANNELMODE_UNDEFINED = 0,
    RA8DEC_CHANNELMODE_1CH,
    RA8DEC_CHANNELMODE_2CH
} RA8DEC_CHANNEL_MODE;

/* FLACDEC Types of channel modes */
typedef enum _FLACDEC_CHANNEL_MODE
{
    FLACDEC_CHANNELMODE_UNDEFINED = 0,
    FLACDEC_CHANNELMODE_1CH,
    FLACDEC_CHANNELMODE_2CH,
    FLACDEC_CHANNELMODE_3CH,
    FLACDEC_CHANNELMODE_4CH,
    FLACDEC_CHANNELMODE_5CH,
    FLACDEC_CHANNELMODE_6CH,
    FLACDEC_CHANNELMODE_7CH,
    FLACDEC_CHANNELMODE_8CH
} FLACDEC_CHANNEL_MODE;

// AC3 ES Info
typedef struct _DecAC3EsInfo
{
	unsigned int		EAC3;				// AC3 0x0, EAC3 0x1
}DecAC3EsInfo;


// MPEG ES Info
typedef struct _DecMPEGEsInfo
{
	unsigned int		layer;
}DecMPEGEsInfo;


// HE-AAC ES Info
typedef struct _DecHEAACEsInfo
{
	unsigned int		version;			// AAC = 0x0 , HE-AACv1 = 0x1, HE-AACv2 = 0x2
	unsigned int		transmissionformat;	// LOAS/LATM = 0x0 , ADTS = 0x1
}DecHEAACEsInfo;


typedef struct _DecEvtESDecInfoParam
{
	unsigned int		media_type;		// ADEC_MEDIA_TYPE
	unsigned int		avg_bit_rate;
	unsigned int		sample_rate;
	unsigned int		num_of_channel;
	unsigned int		bit_per_sample;
	unsigned int		channel_mode;
	unsigned int		input_endian;
	unsigned int		input_signed;
	unsigned int		es_info[3];
}DecEvtESDecInfoParam;

typedef struct _DecEvtESExistInfoParam
{
	unsigned int		es_exist;
}DecEvtESExistInfoParam;

typedef struct _DecEvtDecErrorParam
{
	unsigned int		media_type;			// ADEC_MEDIA_TYPE
	unsigned int		error_type;			// 0 : non-fatal, 1 : fatal error, 2 : sampling rate changed
}DecEvtDecErrorParam;

typedef struct _DecEvtReqSpdifType
{
	unsigned int		media_type;			// ADEC_MEDIA_TYPE
	unsigned int		spdif_type;			// 0 : PCM, 1 : ES
}DecEvtReqSpdifType;

typedef struct _DecEvtDecIndexParam
{
	unsigned int		index;				// Decoded Index
	unsigned int		timestamp;			// PTS
}DecEvtDecIndexParam;


#endif //__ADEC_IMC_CMD_H__
/** @} */
