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
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#include "os_util.h"
//#include "l9/base_addr_sw_l9.h"

#include "sdec_kapi.h"
#include "sdec_reg.h"
#include "sdec_hal.h"
#include "sdec_drv.h"
#include "sdec_io.h"

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

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/


/**
********************************************************************************
* @brief
*   register setting
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/

#ifdef USE_QEMU_SYSTEM
SDTOP_REG_T  pSDEC_TOP_Reg;
SDIO_REG_T pSDEC_IO_Reg;
MPG_REG_T pSDEC_MPG_Reg0;
MPG_REG_T pSDEC_MPG_Reg1;
#endif
#if 0
DTV_STATUS_T SDEC_RegInit(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = OK;
	LX_ADDR_SW_CFG_T addr_sw_cfg_sdec;

	SDEC_DTV_SOC_Message( NORMAL, "<--SDEC_RegInit");

	if (stpSdecParam == NULL)
	{
		SDEC_DEBUG_Print("Invalid parameters");
		eRet = INVALID_PARAMS;

		goto error;
	}

	if (lx_chip_rev() >= LX_CHIP_REV(L9, A0))
	{
		stpSdecParam->stSDEC_TOP_Reg	= (SDTOP_REG_T *)ioremap(L9_SDEC_TOP_REG_BASE, 0x20);
		stpSdecParam->stSDEC_IO_Reg		= (SDIO_REG_T *)ioremap(L9_SDEC_IO_REG_BASE, 0x200);
		stpSdecParam->stSDEC_MPG_Reg[0]	= (MPG_REG_T *)ioremap(L9_SDEC_MPG_REG_BASE0, 0x800);
		stpSdecParam->stSDEC_MPG_Reg[1]	= (MPG_REG_T *)ioremap(L9_SDEC_MPG_REG_BASE1, 0x800);

		//add address sw setting for L9 logical memory map
		BASE_L9_GetAddrSwCfg( ADDR_SW_CFG_TE_SDEC , &addr_sw_cfg_sdec );
		stpSdecParam->stSDEC_TOP_Reg->addr_sw0 = addr_sw_cfg_sdec.de_sav; //0x34030150;
		stpSdecParam->stSDEC_TOP_Reg->addr_sw1 = addr_sw_cfg_sdec.cpu_gpu; //0x000200D0;
		stpSdecParam->stSDEC_TOP_Reg->addr_sw2 = addr_sw_cfg_sdec.cpu_shadow; //0x0C010140;
	}
	else
	{
		stpSdecParam->stSDEC_TOP_Reg	= (SDTOP_REG_T *)(L8_SDEC_TOP_REG_BASE);
		stpSdecParam->stSDEC_IO_Reg		= (SDIO_REG_T *)(L8_SDEC_IO_REG_BASE);
		stpSdecParam->stSDEC_MPG_Reg[0]	= (MPG_REG_T *)(L8_SDEC_MPG_REG_BASE0);
		stpSdecParam->stSDEC_MPG_Reg[1]	= (MPG_REG_T *)(L8_SDEC_MPG_REG_BASE1);
	}

	SDEC_DTV_SOC_Message( NORMAL, "-->SDEC_RegInit");

	return (eRet);

error:
	return (eRet);
}

#endif
