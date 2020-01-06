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

/** @file
 *
 *  clock gating implementation for venc device
 *	venc device will teach you how to make device driver with new platform.
 *
 *  author		jaeseop.so (jaeseop.so@lge.com)
 *  version		1.0
 *  date		2013.06.27
 *  note		Additional information.
 *
 *  @addtogroup lg115x_venc
 *	@{
 */

/*-----------------------------------------------------------------------------
	Control Constants
-----------------------------------------------------------------------------*/
//#define VENC_ENABLE_CLOCK_GATE

/*-----------------------------------------------------------------------------
	File Inclusions
-----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>

#include "os_util.h"
#include "base_types.h"
#include "venc_drv.h"

#ifdef VENC_ENABLE_CLOCK_GATE
// INCLUDE_H13_CHIP_KDRV
#include "../chip/h13/sys/cpu_top_reg_h13.h"
#include "../chip/h13/sys/ctop_ctrl_reg_h13.h"

// INCLUDE_M14_CHIP_KDRV
#include "../chip/m14/sys/cpu_top_reg_m14.h"
#include "../chip/m14/sys/ctop_ctrl_reg_m14.h"

// INCLUDE_M14_CHIP_KDRV
#include "../chip/h14/sys/cpu_top_reg_h14.h"
#include "../chip/h14/sys/ctop_ctrl_reg_h14.h"

#endif

/*-----------------------------------------------------------------------------
	Constant Definitions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Macro Definitions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Type Definitions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	External Function Prototype Declarations
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	External Variables
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	global Functions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Static Function Prototypes Declarations
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Static Variables
-----------------------------------------------------------------------------*/

/*========================================================================================
  Implementation Group
========================================================================================*/

#ifdef VENC_ENABLE_CLOCK_GATE
static inline void VENC_FlushEnable( BOOLEAN en )
{
// INCLUDE_H14_CHIP_KDRV
	if      ( lx_chip_rev() >= LX_CHIP_REV(H14,A0) )
	{
		CPU_TOP_H14A0_RdFL( flush_req );
		CPU_TOP_H14A0_Wr01( flush_req, venc_flush_en, en ? 1 : 0 );
		CPU_TOP_H14A0_WrFL( flush_req );
	} else

// INCLUDE_M14_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV(M14,B0) )
	{
		CPU_TOP_M14B0_RdFL( flush_req );
		CPU_TOP_M14B0_Wr01( flush_req, venc_flush_en, en ? 1 : 0 );
		CPU_TOP_M14B0_WrFL( flush_req );
	}
	else if ( lx_chip_rev() >= LX_CHIP_REV(M14,A0) )
	{
		CPU_TOP_M14A0_RdFL( flush_req );
		CPU_TOP_M14A0_Wr01( flush_req, venc_flush_en, en ? 1 : 0 );
		CPU_TOP_M14A0_WrFL( flush_req );
	} else

// INCLUDE_H13_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV(H13,B0) )
	{
		CPU_TOP_H13B0_RdFL( flush_req );
		CPU_TOP_H13B0_Wr01( flush_req, venc_flush_en, en ? 1 : 0 );
		CPU_TOP_H13B0_WrFL( flush_req );
	} else
	{
		// Unkown chip revision
	}

#if 0
	UINT32 flush_req;
	UINT32 venc_flush_done = 0;

	// 2. check flush done
	while( TRUE )
	{
		CPU_TOP_RdFL( flush_done_status );
		CPU_TOP_Rd01( flush_done_status, venc_flush_done, venc_flush_done );

		if ( venc_flush_done )
		{
			printk("Check venc_flush_done\n");
			break;
		}
	}
#endif

	return;
}

static inline void VENC_SoftwareResetEnable( BOOLEAN en )
{
// INCLUDE_H14_CHIP_KDRV
	if      ( lx_chip_rev() >= LX_CHIP_REV(H14,A0) )
	{
		CTOP_CTRL_H14A0_RdFL( ctr04 );
		CTOP_CTRL_H14A0_Wr01( ctr04, swrst_ve, en ? 1 : 0 );		// CTR04[31]
		CTOP_CTRL_H14A0_Wr01( ctr04, swrst_te_ve, en ? 1 : 0 );	// CTR04[29]
		CTOP_CTRL_H14A0_Wr01( ctr04, reg_swrst_se, en ? 1 : 0 );	// CTR04[15]
		CTOP_CTRL_H14A0_WrFL( ctr04 );

	} else 

// INCLUDE_M14_CHIP_KDRV
	if      ( lx_chip_rev() >= LX_CHIP_REV(M14,B0) )
	{
		CTOP_CTRL_M14B0_RdFL( VENC, ctr01 );
		CTOP_CTRL_M14B0_Wr01( VENC, ctr01, swrst_ve, en ? 1 : 0 );	// CTR04[31]
		CTOP_CTRL_M14B0_Wr01( VENC, ctr01, swrst_te, en ? 1 : 0 );	// CTR04[29]
		CTOP_CTRL_M14B0_Wr01( VENC, ctr01, swrst_se, en ? 1 : 0 );	// CTR04[15]
		CTOP_CTRL_M14B0_WrFL( VENC, ctr01 );
	}
	else if ( lx_chip_rev() >= LX_CHIP_REV(M14,A0) )
	{
		CTOP_CTRL_M14A0_RdFL( ctr04 );
		CTOP_CTRL_M14A0_Wr01( ctr04, swrst_ve, en ? 1 : 0 );		// CTR04[31]
		CTOP_CTRL_M14A0_Wr01( ctr04, swrst_te_ve, en ? 1 : 0 );	// CTR04[29]
		CTOP_CTRL_M14A0_Wr01( ctr04, reg_swrst_se, en ? 1 : 0 );	// CTR04[15]
		CTOP_CTRL_M14A0_WrFL( ctr04 );
	} else 

// INCLUDE_H13_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV(H13,B0) )
	{
		CTOP_CTRL_H13B0_RdFL( ctr04 );
		CTOP_CTRL_H13B0_Wr01( ctr04, swrst_ve, en ? 1 : 0 );		// CTR04[31]
		CTOP_CTRL_H13B0_Wr01( ctr04, swrst_te_ve, en ? 1 : 0 );	// CTR04[29]
		CTOP_CTRL_H13B0_Wr01( ctr04, reg_swrst_se, en ? 1 : 0 );	// CTR04[15]
		CTOP_CTRL_H13B0_WrFL( ctr04 );
	} else
	{
		// Unkown chip revision
	}

	return;
}

static inline void VENC_ClockEnable( BOOLEAN en )
{
	UINT8 clock_sel = ( en ) ? 0x0: 0x3 ;

// INCLUDE_H14_CHIP_KDRV
	if      ( lx_chip_rev() >= LX_CHIP_REV(H14,A0) )
	{
		CTOP_CTRL_H14A0_RdFL( ctr06 );
		CTOP_CTRL_H14A0_Wr01( ctr06, veclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_H14A0_Wr01( ctr06, te_teclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_H14A0_WrFL( ctr06 );
	} else

// INCLUDE_M14_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV(M14,B0) )
	{
		CTOP_CTRL_M14B0_RdFL( VENC, ctr00 );
		CTOP_CTRL_M14B0_Wr01( VENC, ctr00, veclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_M14B0_Wr01( VENC, ctr00, teclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_M14B0_WrFL( VENC, ctr00 );
	}
	else if ( lx_chip_rev() >= LX_CHIP_REV(M14,A0) )
	{
		CTOP_CTRL_M14A0_RdFL( ctr06 );
		CTOP_CTRL_M14A0_Wr01( ctr06, veclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_M14A0_Wr01( ctr06, te_teclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_M14A0_WrFL( ctr06 );
	} else 

// INCLUDE_H13_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV(H13,B0) )
	{
		CTOP_CTRL_H13B0_RdFL( ctr06 );
		CTOP_CTRL_H13B0_Wr01( ctr06, veclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_H13B0_Wr01( ctr06, te_teclk_sel, clock_sel );	// 0:original, 1:half, 2:quater, 4/5/6/7:disable
		CTOP_CTRL_H13B0_WrFL( ctr06 );
	} else
	
	{
		// Unkown chip revison
	}

	return;
}
#endif

void ClockGateOff( void )
{
#ifdef VENC_ENABLE_CLOCK_GATE
	VENC_FlushEnable( TRUE );
	VENC_SoftwareResetEnable( TRUE );
	VENC_ClockEnable( FALSE );

	VENC_NOTI("%s :: Clock gating: OFF\n", __FUNCTION__);
#endif

	return;
}

void ClockGateOn( void )
{
#ifdef	VENC_ENABLE_CLOCK_GATE
	VENC_FlushEnable( FALSE );
	VENC_SoftwareResetEnable( FALSE );
	VENC_ClockEnable( TRUE );

	VENC_NOTI("%s :: Clock gating: ON\n", __FUNCTION__);
#endif

	return;
}

