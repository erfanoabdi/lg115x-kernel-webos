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
 *	@file adec_imc_evt.h
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
#ifndef __ADEC_IMC_EVT_H__
#define __ADEC_IMC_EVT_H__

#include "common/adec_config.h"


#define ADEC_EVT_OVERFLOW		0x0100
#define ADEC_EVT_UNDERFLOW		0x0101

#define ADEC_EVT_DSP_DN_DONE	0x0200
#define ADEC_EVT_DSP_ALIVE		0x0201

#endif //__ADEC_IMC_CMD_H__
/** @} */
