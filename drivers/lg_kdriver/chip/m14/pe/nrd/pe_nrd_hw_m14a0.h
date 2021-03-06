/*
 * SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2013 by LG Electronics Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/** @file pe_nrd_hw_m14a0.h
 *
 *  driver header for picture enhance noise reduction parameters. ( used only within kdriver )
 *	- initial settings
 *	- default settings for each format
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2012.05.12
 *	@see		
 */

#ifndef	_PE_NRD_HW_M14A0_H_
#define	_PE_NRD_HW_M14A0_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "pe_reg.h"

#include "m14a0/pe_nr_l_init_m14a0.h"
#include "m14a0/pe_tnr_l_crg_init_ia_m14a0.h"
#include "m14a0/pe_tnr_l_alphalut_init_ia_m14a0.h"
#include "m14a0/pe_tnr_l_atv_default_m14a0.h"
#include "m14a0/pe_tnr_l_dvr_atv_default_m14a0.h"
#include "m14a0/pe_tnr_l_atv_secam_default_m14a0.h"
#include "m14a0/pe_tnr_l_av_default_m14a0.h"
#include "m14a0/pe_tnr_l_av_secam_default_m14a0.h"
#include "m14a0/pe_tnr_l_sd_default_m14a0.h"
#include "m14a0/pe_tnr_l_sd_mc_off_default_m14a0.h"
#include "m14a0/pe_tnr_l_hd_default_m14a0.h"
#include "m14a0/pe_tnr_l_hd_default_for_udtv_m14a0.h"
#include "m14a0/pe_tnr_l_ud_default_m14a0.h"
#include "m14a0/pe_dnr_l_sd_default_m14a0.h"
#include "m14a0/pe_dnr_l_hd_default_m14a0.h"
#include "m14a0/pe_dnr_l_tp_default_m14a0.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_NRD_HW_M14A0_H_ */
