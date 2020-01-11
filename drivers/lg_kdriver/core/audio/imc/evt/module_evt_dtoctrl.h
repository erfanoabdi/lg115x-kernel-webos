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
 *	@file module_evt_dtoctrl.h
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
#ifndef __MODULE_EVT_DTO_H__
#define __MODULE_EVT_DTO_H__

#include "common/adec_media_type.h"
#include "imc/adec_imc_evt.h"


#define DTO_EVT_CHANGE_DTO_RATE           0x0C00

/**
  * DTO Rate Change Event ó���� ���� ����ü
  */
typedef struct _DtoEvtChangeDtoRate
{
   unsigned int         dto_rate;       /**< DTO Rate */
}DtoEvtChangeDtoRate;


#endif //__MODULE_EVT_DTO_H__

