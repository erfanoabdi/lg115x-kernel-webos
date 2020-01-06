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


/** @file
 *
 *  sdec driver
 *
 *  @author	Jihoon Lee ( gaius.lee@lge.com)
 *  @author	Jinhwan Bae ( jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010-03-30
 *  @note		Additional information.
 */

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <asm/io.h>

#include "os_util.h"

#include "sdec_hal.h"

#include "rc0_reg.h"
#include "rc1_reg.h"

#include "sdec_reg_pdec.h"

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/
#ifdef LX_SDEC_LG1152_VDEC_BASE
#undef LX_SDEC_LG1152_VDEC_BASE
#endif

#ifdef __XTENSA__
#define 	LX_SDEC_LG1152_VDEC_BASE					(0xF0000000)
#else
//daeseok.youn moving vma mapping for increasing size of vma
//#if defined(CONFIG_LG_INCREASE_VMA_SIZE)
//#define 	LX_SDEC_LG1152_VDEC_BASE					(0xDE000000 + 0x003000)
//#else
#define 	LX_SDEC_LG1152_VDEC_BASE					(0xC0000000 + 0x003000)
//#endif
#endif

#define LX_SDEC_VDEC_RC0_BASE					(LX_SDEC_LG1152_VDEC_BASE + 0x1400)
#define LX_SDEC_L9_PDEC_BASE(ch)					(LX_SDEC_LG1152_VDEC_BASE + 0x1800 + (ch*0x100))

#define	LX_SDEC_PDEC_CHANNEL						2
#define	LX_SDEC_SDEC_CHANNEL						2

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Functions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/
static 	volatile RC0_REG_T			*stpRC0_Reg = NULL;
static	volatile PDEC_REG_T			*stpPdecReg;

void SDEC_VDECHAL_PDECInit(void)
{
	REG_PDEC_CONF		reg_pdec_conf;

	/* init VDEC TOP Register */
	stpRC0_Reg		= (volatile RC0_REG_T *) ioremap ((LX_SDEC_VDEC_RC0_BASE) , sizeof(RC0_REG_T));

	stpPdecReg = (volatile PDEC_REG_T *) ioremap ((LX_SDEC_L9_PDEC_BASE(LX_SDEC_PDEC_CHANNEL)) , sizeof(PDEC_REG_T));
	stpPdecReg->reg_aux_mode  = 0x40000010;

	reg_pdec_conf = stpPdecReg->reg_pdec_conf;
	reg_pdec_conf.reg_scd_en = 0;
	reg_pdec_conf.reg_pdec_en = 0;
	reg_pdec_conf.reg_aub_intr_chk_en = 0;
	reg_pdec_conf.reg_video_standard = 0;
	reg_pdec_conf.reg_pdec_endian = 0;
	reg_pdec_conf.reg_srst = 0;
	stpPdecReg->reg_pdec_conf = reg_pdec_conf;
}

/*
 * Control VDEC TOP register
 *
 */
void SDEC_VDECHAL_TOPSetPdecInputSelection(UINT8 ui8TeCh)
{
	//RC0_Wr01(pdec_in_conf, reg_pdec2_in_sel, ui8TECh&0x3);
	stpRC0_Reg->pdec_in_conf.reg_pdec2_in_sel = ui8TeCh & 0x3;
	//RC0_Wr01(pdec_in_conf, reg_vid2_rdy_dis, 0);
	stpRC0_Reg->pdec_in_conf.reg_vid2_rdy_dis = 0;
}

UINT8 SDEC_VDECHAL_TOPGetPdecInputSelection(void)
{
	UINT8 ui8TECh;

	//RC0_Rd01(pdec_in_conf, reg_pdec2_in_sel, ui8TECh);
	ui8TECh = stpRC0_Reg->pdec_in_conf.reg_pdec2_in_sel;

	return ui8TECh;
}

void SDEC_VDECHAL_TOPEnablePdecInput(BOOLEAN bEnable)
{
	stpRC0_Reg->pdec_in_conf.reg_pdec2_in_dis = (bEnable) ? 0 : 1;
}

void SDEC_VDECHAL_TOPEnableExtIntr(UINT32 ui32IntrSrc)
{
	stpRC0_Reg->intr_e_en |= (1 << ui32IntrSrc);
}

void SDEC_VDECHAL_TOPDisableExtIntr(UINT32 ui32IntrSrc)
{
	stpRC0_Reg->intr_e_en &= ~(1 << ui32IntrSrc);
}

void SDEC_VDECHAL_TOPClearExtIntr(UINT32 ui32IntrSrc)
{
	stpRC0_Reg->intr_e_cl &= ~(1 << ui32IntrSrc);
}

void SDEC_VDECHAL_TOPClearExtIntrMsk(UINT32 ui32IntrMsk)
{
	stpRC0_Reg->intr_e_cl = ui32IntrMsk;
}

void SDEC_VDECHAL_TOPDisableExtIntrAll(void)
{
	stpRC0_Reg->intr_e_en = 0;
	stpRC0_Reg->intr_e_cl = 0xFFFFFFFF;
}

/*
 * Control VDEC PDEC register
 *
 */

void SDEC_VDECHAL_PDECReset(void)
{
	volatile REG_PDEC_CONF         reg_pdec_conf;

	// PDEC sw reset
	reg_pdec_conf = stpPdecReg->reg_pdec_conf;
	reg_pdec_conf.reg_srst = 0x7;
	stpPdecReg->reg_pdec_conf = reg_pdec_conf;

	reg_pdec_conf = stpPdecReg->reg_pdec_conf;
	while( reg_pdec_conf.reg_srst )
	{ // wait sw reset ack
		reg_pdec_conf = stpPdecReg->reg_pdec_conf;
	}
}
void SDEC_VDECHAL_PDECEnable(void)
{
	volatile REG_PDEC_CONF         reg_pdec_conf;

	reg_pdec_conf = stpPdecReg->reg_pdec_conf;

	reg_pdec_conf.reg_scd_en = 1;
	reg_pdec_conf.reg_pdec_en = 1;

	stpPdecReg->reg_pdec_conf = reg_pdec_conf;
}

void SDEC_VDECHAL_PDECDisable(void)
{
	volatile REG_PDEC_CONF         reg_pdec_conf;

	reg_pdec_conf = stpPdecReg->reg_pdec_conf;

	reg_pdec_conf.reg_scd_en = 0;
	reg_pdec_conf.reg_pdec_en = 0;

	stpPdecReg->reg_pdec_conf = reg_pdec_conf;
}

void SDEC_VDECHAL_PDECSetVideoStandard(UINT8 ui8Vcodec)
{
	volatile REG_PDEC_CONF         reg_pdec_conf;

	reg_pdec_conf = stpPdecReg->reg_pdec_conf;

	reg_pdec_conf.reg_video_standard = ui8Vcodec;

	stpPdecReg->reg_pdec_conf = reg_pdec_conf;
}

void SDEC_VDECHAL_PDECCPB_Init(UINT32 ui32CpbBase, UINT32 ui32CpbSize)
{
	stpPdecReg->reg_cpb_base = ui32CpbBase;
	stpPdecReg->reg_cpb_size = ui32CpbSize;
	stpPdecReg->reg_cpb_wptr = ui32CpbBase;
	stpPdecReg->reg_cpb_rptr = ui32CpbBase;
}

UINT32 SDEC_VDECHAL_PDECGetCPBBase(void)
{
	return stpPdecReg->reg_cpb_base;
}

UINT32 SDEC_VDECHAL_PDECGetCPBSize(void)
{
	return stpPdecReg->reg_cpb_size;
}

UINT32 SDEC_VDECHAL_PDECCPB_GetWrPtr(void)
{
	return stpPdecReg->reg_cpb_wptr;
}

void SDEC_VDECHAL_PDECCPB_SetWrPtr(UINT32 ui32CpbRdPtr)
{
	stpPdecReg->reg_cpb_wptr = ui32CpbRdPtr;
}

UINT32 SDEC_VDECHAL_PDECCPB_GetRdPtr(void)
{
	return stpPdecReg->reg_cpb_rptr;
}

void SDEC_VDECHAL_PDECCPB_SetRdPtr(UINT32 ui32CpbRdPtr)
{
	stpPdecReg->reg_cpb_rptr = ui32CpbRdPtr;
}

void SDEC_VDECHAL_PDECCPB_Reset(void)
{
	UINT32	reg_cpb_base;
	reg_cpb_base	= stpPdecReg->reg_cpb_base;

	stpPdecReg->reg_cpb_rptr = reg_cpb_base;
	stpPdecReg->reg_cpb_wptr = reg_cpb_base;
}

void SDEC_VDECHAL_PDECSetBypass(void)
{
	volatile UINT32 reg_trick_mode;

	reg_trick_mode = stpPdecReg->reg_trick_mode;

	reg_trick_mode |= (1 << 2);

	stpPdecReg->reg_trick_mode = reg_trick_mode;
}

void SDEC_VDECHAL_PDECEnableUserDefinedSCD(UINT8 ui8ScdIdx)
{
	stpPdecReg->reg_aux_mode  |=  1 << (ui8ScdIdx + 22) ;
}


void SDEC_VDECHAL_PDECSetUserDefinedSCD(UINT32 ui32Val)
{
	stpPdecReg->reg_udef_scd0.reg_udef_scd0		= ui32Val;
	stpPdecReg->reg_udef_scd_num.reg_udef_scd0 	= 3;
}

void SDEC_VDECHAL_PDECBDRC_Enable(BOOLEAN bEnable)
{
	volatile REG_BDRC_CONF reg_bdrc_conf;

	reg_bdrc_conf = stpPdecReg->reg_bdrc_conf;

	if( bEnable )
	{
		reg_bdrc_conf.reg_bdrc_dtype	= 0;
		reg_bdrc_conf.reg_bdrc_endian	= 0;
		reg_bdrc_conf.reg_bdrc_rst		= 1;
		reg_bdrc_conf.reg_bdrc_en		= 1;
	}
	else
	{
		reg_bdrc_conf.reg_bdrc_en		= 0;
	}

	stpPdecReg->reg_bdrc_conf  =  reg_bdrc_conf;
}

void SDEC_VDECHAL_PDECBDRC_Init(UINT32 bufBase, UINT32 bufSize)
{
	stpPdecReg->reg_bdrc_buf_base.reg_bdrc_base	= bufBase;
	stpPdecReg->reg_bdrc_buf_size.reg_bdrc_size	= bufSize;
	stpPdecReg->reg_bdrc_buf_wptr.reg_bdrc_wptr	= bufBase;
	stpPdecReg->reg_bdrc_buf_rptr.reg_bdrc_rptr	= bufBase;
}

void SDEC_VDECHAL_PDECBDRC_SetWPtr(UINT32 wptr)
{
	stpPdecReg->reg_bdrc_buf_wptr.reg_bdrc_wptr	= wptr;
}

void SDEC_VDECHAL_PDECBDRC_Update(void)
{
	stpPdecReg->reg_bdrc_conf.reg_wptr_upd = 1;
}

