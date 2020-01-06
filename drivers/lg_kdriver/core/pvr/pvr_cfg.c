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
 *  main configuration file for pvr device
 *	pvr device will teach you how to make device driver with new platform.
 *
 *  author		kyungbin.pak
 *  author		modified by ki beom kim (kibeom.kim@lge.com)
 *  version		1.0 
 *  date		2010.02.05
 *  note		Additional information. 
 *
 *  @addtogroup lg1150_pvr 
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <asm/io.h>			/**< For ioremap_nocache */
#include "os_util.h"

#include "pvr_drv.h"
#include "pvr_dev.h"
#include "pvr_reg.h"
#include "pvr_cfg.h"

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
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/

DVR_MEM_CFG	astDvrMemMap[PVR_MAX_DEVICE];

LX_MEMCFG_T	gMemCfgDvr[][LX_PVR_MEM_NUM] = 
{
	[0] = // for LG1150
	{
		{ .name = "dvr/dn", 	.base = 0x00000000, 	.size = 0x00240000},	/* dn buffer should be multuple of 384KB */
		{ .name = "dvr/up", 	.base = 0x00240000,		.size = 0x00240000},	/* up buffer should be multuple of 384KB */
		{ .name = "dvr/upbuf",	.base = 0x00480000,		.size = 0x00060000},	/* up slot buffer should be 384KB */
		{ .name = "dvr/piebuf",	.base = 0x004E0000, 	.size = 0x00001000}		/* picture index buffer */
	},
	[1] = // for LG1152
	{
		{ .name = "dvr/dn", 	.base = 0x0, /* 0x7B000000, */	.size = 0x006C0000},	/* dn buffer should be multuple of 384KB */
		{ .name = "dvr/up", 	.base = 0x0, /* 0x7B240000,	*/	.size = 0x00240000},	/* up buffer should be multuple of 384KB */
		{ .name = "dvr/upbuf",	.base = 0x0, /* 0x7B480000,	*/	.size = 0x00060000},	/* up slot buffer should be 384KB */
		{ .name = "dvr/piebuf",	.base = 0x0, /* 0x7B4E0000, */	.size = 0x00001000}		/* picture index buffer */	
	}
    ,
    [2] = // for LG1154
	{
		{ .name = "dvr/dn", 	.base = 0x0, /* 0x7B000000, */	.size = 0x006C0000},	/* dn buffer should be multuple of 384KB */
		{ .name = "dvr/up", 	.base = 0x0, /* 0x7B240000,	*/	.size = 0x00240000},	/* up buffer should be multuple of 384KB */
		{ .name = "dvr/upbuf",	.base = 0x0, /* 0x7B480000,	*/	.size = 0x00060000},	/* up slot buffer should be 384KB */
		{ .name = "dvr/piebuf",	.base = 0x0, /* 0x7B4E0000, */	.size = 0x00001000},	/* picture index buffer */	
        { .name = "dvr/dn1", 	.base = 0x0, /* 0x7B000000, */	.size = 0x006C0000},	/* dn_1 buffer should be multuple of 384KB */
		{ .name = "dvr/up1", 	.base = 0x0, /* 0x7B240000,	*/	.size = 0x00240000},	/* up_1 buffer should be multuple of 384KB */
		{ .name = "dvr/upbuf1",	.base = 0x0, /* 0x7B480000,	*/	.size = 0x00060000},	/* up_1 slot buffer should be 384KB */
		{ .name = "dvr/piebuf1",.base = 0x0, /* 0x7B4E0000, */	.size = 0x00001000}		/* picture index buffer_1 */ 
    }
};
/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/


/*========================================================================================
	Implementation Group
========================================================================================*/

void DVR_DefaultMemoryMap (LX_PVR_CH_T ch)
{
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
       switch(ch)
       {
       	 case LX_PVR_CH_A:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_DN].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_UP].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
		 	break;
			
		 case LX_PVR_CH_B:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
		 	break;
			
	     default:
		 	break;
       }	
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
       switch(ch)
       {
       	 case LX_PVR_CH_A:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[1][LX_PVR_MEM_DN].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[1][LX_PVR_MEM_UP].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base + gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
		 	break;
			
		 case LX_PVR_CH_B:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[1][LX_PVR_MEM_DN].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[1][LX_PVR_MEM_UP].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base + gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
		 	break;
			
	     default:
		 	break;
       }	
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
       switch(ch)
       {
       	 case LX_PVR_CH_A:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_DN].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_UP].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
		 	break;
			
		 case LX_PVR_CH_B:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
		 	break;
			
	     default:
		 	break;
       }	
	}
    else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
       switch(ch)
       {
       	 case LX_PVR_CH_A:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_DN].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_UP].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
		 	break;
		 case LX_PVR_CH_B:
		 	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
			astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;

			astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
			astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;

			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
			astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr    = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
			astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr   = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
		 	break;
	     default:
		 	break;
       }
	}

	/* UP Buffer base */
	DVR_UP_SetBufBoundReg(ch, astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase, astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd );

	/* DN Buffer base */
	DVR_DN_SetBufBoundReg(ch, astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase, astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd );

	/* PIE Buffer */
	/* 2012. 12. 22 Jinhwan.bae for netcast 4.0 Down Channel B (PVR_DN_CH_A) PIE Buffer in Kdriver */
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		switch(ch)
		{
			 case LX_PVR_CH_A:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base, gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
				break;
			 case LX_PVR_CH_B:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base, gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
				break;
			 default:
				break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		switch(ch)
		{
			 case LX_PVR_CH_A:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base, gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
				break;
			 case LX_PVR_CH_B:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base, gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
				break;
			 default:
				break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		switch(ch)
		{
			 case LX_PVR_CH_A:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base, gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
				break;
			 case LX_PVR_CH_B:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base, gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
				break;
			 default:
				break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		switch(ch)
		{
			 case LX_PVR_CH_A:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base, gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
				break;
			 case LX_PVR_CH_B:
			 	astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base, gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size);
				astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
				astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
				astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
				break;
			 default:
				break;
		}
	}
}


/** @} */

