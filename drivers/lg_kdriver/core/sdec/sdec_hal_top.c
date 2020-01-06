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
#include "sdec_reg.h"
#include "h13/sdec_reg_h13a0.h"
#include "m14/sdec_reg_m14a0.h"
#include "m14/sdec_reg_m14b0.h"
#include "h14/sdec_reg_h14a0.h"

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/

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
volatile SDTOP_REG_H13A0_T *stSDEC_TOP_RegH13A0;
volatile SDTOP_REG_H13A0_T stSDEC_TOP_RegShadowH13A0[1];

volatile SDTOP_REG_M14A0_T *stSDEC_TOP_RegM14A0;
volatile SDTOP_REG_M14A0_T stSDEC_TOP_RegShadowM14A0[1];

volatile SDTOP_REG_M14B0_T *stSDEC_TOP_RegM14B0;
volatile SDTOP_REG_M14B0_T stSDEC_TOP_RegShadowM14B0[1];

volatile SDTOP_REG_H14A0_T *stSDEC_TOP_RegH14A0;
volatile SDTOP_REG_H14A0_T stSDEC_TOP_RegShadowH14A0[1];

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/

int SDEC_HAL_TOPInit(void)
{
	int ret = RET_ERROR;

	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
		stSDEC_TOP_RegH14A0	= (SDTOP_REG_H14A0_T *)ioremap(H14_SDEC_TOP_REG_BASE, sizeof(SDTOP_REG_H14A0_T));
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		stSDEC_TOP_RegM14B0	= (SDTOP_REG_M14B0_T *)ioremap(M14_B0_SDEC_TOP_REG_BASE, sizeof(SDTOP_REG_M14B0_T));
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
		stSDEC_TOP_RegM14A0	= (SDTOP_REG_M14A0_T *)ioremap(M14_A0_SDEC_TOP_REG_BASE, sizeof(SDTOP_REG_M14A0_T));
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
		stSDEC_TOP_RegH13A0	= (SDTOP_REG_H13A0_T *)ioremap(H13_SDEC_TOP_REG_BASE, sizeof(SDTOP_REG_H13A0_T));

	ret = RET_OK;

	return ret;
}

int SDEC_HAL_SetVideoOut(UINT8 idx, UINT8 sel)
{
	int ret = RET_ERROR;

	/* idx : VDEC Number, sel : From Which? */
	/* vid0,1 -> 0: SDEC0, 1:SDEC1
	    vid2 -> 0: SDEC0 CDIC2 CHB, 1: PVR Port0, 2: PVR Port1, 3:SDEC1 CDIC2 CHB */

	SDTOP_RdFL(vid_out_sel);

	switch(idx)
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			case 0 : SDTOP_Wr01(vid_out_sel, vid0_sel, sel); break;
			case 1 : SDTOP_Wr01(vid_out_sel, vid1_sel, sel); break;
		}
		case 2 : SDTOP_Wr01(vid_out_sel, vid2_sel, sel); break;
		default : goto exit;
	}

	SDTOP_WrFL(vid_out_sel);
	ret = RET_OK;

exit:
	return ret;
}

int SDEC_HAL_SetPVRSrc(UINT8 idx, UINT8 sel)
{
	int ret = RET_ERROR;

	/* idx : DL Port Number, sel : From Which? 0: SDEC0, 1: SENC, 2: SDEC1*/

	SDTOP_RdFL(dl_sel);

	switch(idx)
	{
		case 0 : SDTOP_Wr01(dl_sel, dl0_sel, sel); break;
		case 1 : SDTOP_Wr01(dl_sel, dl1_sel, sel); break;
		default : goto exit;
	}

	SDTOP_WrFL(dl_sel);
	ret = RET_OK;

exit:
	return ret;
}

int SDEC_HAL_SetAudioOut(UINT8 idx, UINT8 sel)
{
	int ret = RET_ERROR;

	/* idx : ADEC Number, sel : From Which? 0: SDEC0, 1: SDEC1*/
	
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		// SDTOP_RdFL(aud_out_sel);
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
			SD_RdFL_H14A0(SDTOP, aud_out_sel);							
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))		
			SD_RdFL_M14B0(SDTOP, aud_out_sel);

		switch(idx)
		{
			case 0 : //SDTOP_Wr01(aud_out_sel, aud0_sel, sel); break;
				if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
					SD_Wr01_H14A0(SDTOP, aud_out_sel, aud0_sel, sel);
				else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
					SD_Wr01_M14B0(SDTOP, aud_out_sel, aud0_sel, sel);
				break;
			case 1 : //SDTOP_Wr01(aud_out_sel, aud1_sel, sel); break;
				if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
					SD_Wr01_H14A0(SDTOP, aud_out_sel, aud1_sel, sel);
				else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
					SD_Wr01_M14B0(SDTOP, aud_out_sel, aud1_sel, sel);
				break;
			default : goto exit;
		}

		//SDTOP_WrFL(aud_out_sel);
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
			SD_WrFL_H14A0(SDTOP, aud_out_sel);				
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			SD_WrFL_M14B0(SDTOP, aud_out_sel);		
	}
	
	ret = RET_OK;

exit:
	return ret;
}

int SDEC_HAL_SetUploadSrc(UINT8 idx, UINT8 sel)
{
	int ret = RET_ERROR;

	/* idx : Upload Port Number, sel : To where? 0: SDEC0, 1: SDEC1*/
	
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		//SDTOP_RdFL(up_sel);
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
			SD_RdFL_H14A0(SDTOP, up_sel);							
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))		
			SD_RdFL_M14B0(SDTOP, up_sel);
		
		switch(idx)
		{
			case 0 : //SDTOP_Wr01(up_sel, up0_sel, sel); break;
				if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
					SD_Wr01_H14A0(SDTOP, up_sel, up0_sel, sel);
				else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
					SD_Wr01_M14B0(SDTOP, up_sel, up0_sel, sel);
				break;
			case 1 : //SDTOP_Wr01(up_sel, up1_sel, sel); break;
				if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
					SD_Wr01_H14A0(SDTOP, up_sel, up1_sel, sel);
				else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
					SD_Wr01_M14B0(SDTOP, up_sel, up1_sel, sel);
				break;
			default : goto exit;
		}

		//SDTOP_WrFL(up_sel);
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
			SD_WrFL_H14A0(SDTOP, up_sel);				
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			SD_WrFL_M14B0(SDTOP, up_sel);	
	}
	
	ret = RET_OK;

exit:
	return ret;
}

int SDEC_HAL_SetOutputSrcCore(UINT8 idx, UINT8 sel)
{
	int ret = RET_ERROR;

	/* idx : should be 0, sel : From Which CDOC? 0: SDEC0, 1: SDEC1*/
	
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		//SDTOP_RdFL(stpo_sel);
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
			SD_RdFL_H14A0(SDTOP, stpo_sel);							
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))		
			SD_RdFL_M14B0(SDTOP, stpo_sel);

		 //SDTOP_Wr01(stpo_sel, stpo_sel, sel); break;
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			
			SD_Wr01_H14A0(SDTOP, stpo_sel, stpo_sel, sel);
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			SD_Wr01_M14B0(SDTOP, stpo_sel, stpo_sel, sel);

		//SDTOP_WrFL(stpo_sel);
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
			SD_WrFL_H14A0(SDTOP, stpo_sel);				
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			SD_WrFL_M14B0(SDTOP, stpo_sel);	
	}
	
	ret = RET_OK;

	return ret;
}



