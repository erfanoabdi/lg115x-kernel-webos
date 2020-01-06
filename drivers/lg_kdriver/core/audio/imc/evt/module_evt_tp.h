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
 *	@file module_evt_tp.h
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
#ifndef __MODULE_EVT_TP_H__
#define __MODULE_EVT_TP_H__

#include "common/adec_media_type.h"
#include "imc/adec_imc_evt.h"


#define TP_EVT_PDEC_ERROR       0x0600
#define TP_EVT_PCR_ERROR        0x0601
#define TP_EVT_ES_EXIST_INFO    0x0602


// PDEC Error
typedef struct _TpEvtPdecErrorParam
{
	unsigned int		media_type;			// ADEC_MEDIA_TYPE
	unsigned int		error_type;			// 0 : non-fatal, 1 : fatal error
}TpEvtPdecErrorParam;

// PCR Error
typedef struct _TpEvtPcrErrorParam
{
	unsigned int		pcr_error;			// 0 : normal, 1 : free run
}TpEvtPcrErrorParam;

// ES Exist
typedef struct _TpEvtESExistInfoParam
{
	unsigned int		es_exist;
}TpEvtESExistInfoParam;


#endif // #ifndef __MODULE_EVT_TP_H__
/** @} */

