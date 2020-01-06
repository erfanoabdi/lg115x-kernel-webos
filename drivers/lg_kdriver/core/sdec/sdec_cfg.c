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
 *  main configuration file for sdec device
 *  sdec device will teach you how to make device driver with new platform.
 *
 *  @author	Jihoon Lee ( gaius.lee@lge.com)
 *  @author	Jinhwan Bae ( jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010-03-30
 *  @note		Additional information.
 */

#include "sdec_cfg.h"
#include "os_util.h"

/*------------
----------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
LX_SDEC_MEM_CFG_T gMemCfgSDECGPB[] =
{
	[0] = // for LG1150
	{
		.memory_name				= "sdec_gpb",
		.gpb_memory_base			= 0x00000000,
		.gpb_memory_size			= 0x00300000
	},
	[1] = // for LG1152 , LG1154, LG1131
	{
		.memory_name				= "sdec_gpb",
		.gpb_memory_base			= 0x00000000, //0x7B4E4000,
#if (KDRV_PLATFORM == KDRV_COSMO_PLATFORM)	// GCD
		.gpb_memory_size			= 0x00800000
#else		// NetCast
		.gpb_memory_size			= 0x00900000
#endif
	},
	[2] = // for LG1156
	{
		.memory_name				= "sdec_gpb",
		.gpb_memory_base			= 0x00000000,
#if (KDRV_PLATFORM == KDRV_COSMO_PLATFORM)	// GCD
		.gpb_memory_size			= 0x01000000
#else		// NetCast
		.gpb_memory_size			= 0x01200000
#endif
	},
	[3] = // for LG1154 H13 + Dx UD Platform
	{
		.memory_name				= "sdec_gpb",
		.gpb_memory_base			= 0x00000000,
		.gpb_memory_size			= 0x01100000
	},
	[4] = // for LG1156 + Dx UD Platform
	{
		.memory_name				= "sdec_gpb",
		.gpb_memory_base			= 0x00000000,
		.gpb_memory_size			= 0x01900000
	}
};

/* L8 B0 ~ L9 A1 */
LX_SDEC_CHA_INFO_T gSdecChannelCfg_0[] =
{
	// A
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64, 	//* # of pid filter */
		.num_secf	= 64, 	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// B
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64,	//* # of pid filter */
		.num_secf	= 64,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// C
	{
		.capa_lev	= 0,	//* simple channel, just for channel browser */
		.num_pidf	= 1,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
};

/* L9 B0 ~ */
LX_SDEC_CHA_INFO_T gSdecChannelCfg_1[] =
{
	// A
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64, 	//* # of pid filter */
		.num_secf	= 64, 	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// B
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64,	//* # of pid filter */
		.num_secf	= 64,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// C
	{
		.capa_lev	= 0,	//* simple channel, just for channel browser */
		.num_pidf	= 1,	//* # of pid filter */
		.num_secf	= 1,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// D
	{
		.capa_lev	= 0,	//* simple channel, just for all pass recording */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
};

/* H13 A0 ~ M14 A0 */
LX_SDEC_CHA_INFO_T gSdecChannelCfg_H13_A0[] =
{
	// A
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64, 	//* # of pid filter */
		.num_secf	= 64, 	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// B
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64,	//* # of pid filter */
		.num_secf	= 64,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// C
	{
		.capa_lev	= 0,	//* simple channel, just for SDT Parsing*/
		.num_pidf	= 4,	//* # of pid filter */
		.num_secf	= 4,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// D
	{
		.capa_lev	= 0,	//* simple channel, just for all pass recording */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
};

LX_SDEC_CHA_INFO_T gSdecChannelCfg_DualCore[] =
{
	// A
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64, 	//* # of pid filter */
		.num_secf	= 64, 	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// B
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64,	//* # of pid filter */
		.num_secf	= 64,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// C
	{
		.capa_lev	= 0,	//* simple channel, just for SDT Parsing*/
		.num_pidf	= 4,	//* # of pid filter */
		.num_secf	= 4,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// D
	{
		.capa_lev	= 0,	//* simple channel, just for all pass recording */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
	// E
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64, 	//* # of pid filter */
		.num_secf	= 64, 	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// F
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64,	//* # of pid filter */
		.num_secf	= 64,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// G
	{
		.capa_lev	= 0,	//* simple channel, just for SDT Parsing*/
		.num_pidf	= 4,	//* # of pid filter */
		.num_secf	= 4,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// H
	{
		.capa_lev	= 0,	//* simple channel, just for all pass recording */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
};

LX_SDEC_CHA_INFO_T gSdecChannelCfg_M14Bx[] =
{
	// A
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64, 	//* # of pid filter */
		.num_secf	= 64, 	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// B
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 64,	//* # of pid filter */
		.num_secf	= 64,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// C
	{
		.capa_lev	= 0,	//* simple channel, just for SDT Parsing*/
		.num_pidf	= 4,	//* # of pid filter */
		.num_secf	= 4,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// D
	{
		.capa_lev	= 0,	//* simple channel, just for all pass recording */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
	// E
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 1, 	//* # of pid filter */
		.num_secf	= 2, 	//* # of section filter, including #0 for driver consistency 20131007 jinhwan.bae */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// F
	{
		.capa_lev	= 1,	//* Full feature channel */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// G
	{
		.capa_lev	= 0,	//* simple channel, just for SDT Parsing*/
		.num_pidf	= 4,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 8,	//* depth of section filter mask */
	},
	// H
	{
		.capa_lev	= 0,	//* simple channel, just for all pass recording */
		.num_pidf	= 0,	//* # of pid filter */
		.num_secf	= 0,	//* # of section filter */
		.flt_dept	= 0,	//* depth of section filter mask */
	},
};




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

LX_SDEC_CFG_T	_gSdecCfgs[] =
{
#if (KDRV_PLATFORM == KDRV_COSMO_PLATFORM)	// GCD
	{ .chip_rev = LX_CHIP_REV(H14,A0),	.name = "SDEC H14 A0", 	.nChannel = 8, .nVdecOutPort = 3, 	.memCfg = 2,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_DualCore }, // H14 A0
	{ .chip_rev = LX_CHIP_REV(M14,B1),	.name = "SDEC M14 B1", 	.nChannel = 8, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_M14Bx  }, // M14 B1
	{ .chip_rev = LX_CHIP_REV(M14,B0),	.name = "SDEC M14 B0", 	.nChannel = 8, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_M14Bx  }, // M14 B0
	{ .chip_rev = LX_CHIP_REV(M14,A1),	.name = "SDEC M14 A1", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // M14 A1
	{ .chip_rev = LX_CHIP_REV(M14,A0),	.name = "SDEC M14 A0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // M14 A0
	{ .chip_rev = LX_CHIP_REV(H13,B0),	.name = "SDEC H13 B0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 B0~
	{ .chip_rev = LX_CHIP_REV(H13,A1),	.name = "SDEC H13 A1", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 A1
	{ .chip_rev = LX_CHIP_REV(H13,A0),	.name = "SDEC H13 A0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 A0
	{ .chip_rev = LX_CHIP_REV(L9,B2),	.name = "SDEC L9 B2", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 0,	.chInfo = gSdecChannelCfg_1 }, // L9	B0
	{ .chip_rev = LX_CHIP_REV(L9,B1),	.name = "SDEC L9 B1", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 0,	.chInfo = gSdecChannelCfg_1 }, // L9	B0
	{ .chip_rev = LX_CHIP_REV(L9,B0),	.name = "SDEC L9 B0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 0,	.staticGPB = 0,	.chInfo = gSdecChannelCfg_1 }, // L9	B0
	{ .chip_rev = LX_CHIP_REV(L9,A1),	.name = "SDEC L9 A1",	.nChannel = 3, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 0,	.staticGPB = 0, .chInfo = gSdecChannelCfg_0 }, // L9	A1
	{ .chip_rev = LX_CHIP_REV(L9,A0),	.name = "SDEC L9 A0",	.nChannel = 3, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 0,	.staticGPB = 0, .chInfo = gSdecChannelCfg_0 }, // L9	A0
#else	// NetCast and WebOS
	/* jinhwan.bae 20131024 set H14 default memCfg as UD configuration to support FHD/UD at the same time.
	    if runtime branch possible, divide it as each ( FHD/UD ) Case */
	{ .chip_rev = LX_CHIP_REV(H14,A0),	.name = "SDEC H14 A0", 	.nChannel = 8, .nVdecOutPort = 3, 	.memCfg = 4,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_DualCore }, // H14 A0
	{ .chip_rev = LX_CHIP_REV(M14,B1),	.name = "SDEC M14 B1", 	.nChannel = 8, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_M14Bx  }, // M14 B1
	{ .chip_rev = LX_CHIP_REV(M14,B0),	.name = "SDEC M14 B0", 	.nChannel = 8, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_M14Bx  }, // M14 B0
	{ .chip_rev = LX_CHIP_REV(M14,A1),	.name = "SDEC M14 A1", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // M14 A1
	{ .chip_rev = LX_CHIP_REV(M14,A0),	.name = "SDEC M14 A0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // M14 A0
	{ .chip_rev = LX_CHIP_REV(H13,B0),	.name = "SDEC H13 B0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 B0~
	{ .chip_rev = LX_CHIP_REV(H13,A1),	.name = "SDEC H13 A1", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 A1
	{ .chip_rev = LX_CHIP_REV(H13,A0),	.name = "SDEC H13 A0", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 A0
	{ .chip_rev = LX_CHIP_REV(L9,B2),	.name = "SDEC L9 B2",	.nChannel = 4, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1, .chInfo = gSdecChannelCfg_1 }, // L9	B0
	{ .chip_rev = LX_CHIP_REV(L9,B1),	.name = "SDEC L9 B1",	.nChannel = 4, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 1,	.staticGPB = 1, .chInfo = gSdecChannelCfg_1 }, // L9	B0
	{ .chip_rev = LX_CHIP_REV(L9,B0),	.name = "SDEC L9 B0",	.nChannel = 4, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 0,	.staticGPB = 1, .chInfo = gSdecChannelCfg_1 }, // L9	B0
	{ .chip_rev = LX_CHIP_REV(L9,A1),	.name = "SDEC L9 A1",	.nChannel = 3, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 0,	.staticGPB = 1, .chInfo = gSdecChannelCfg_0 }, // L9	A1
	{ .chip_rev = LX_CHIP_REV(L9,A0),	.name = "SDEC L9 A0",	.nChannel = 3, .nVdecOutPort = 3,	.memCfg = 1,	.noPesBug = 0,	.staticGPB = 1, .chInfo = gSdecChannelCfg_0 }, // L9	A0
#endif
	{ .chip_rev = LX_CHIP_REV(L8,B0),	.name = "SDEC L8 B0", 	.nChannel = 3, .nVdecOutPort = 2, 	.memCfg = 0,	.noPesBug = 0,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_0}, // L8	B0
};

LX_SDEC_CFG_T	_gSdecCfgs_H13UD[] = 
{
	{ .chip_rev = LX_CHIP_REV(H13,B0),	.name = "SDEC H13 B0 UD", 	.nChannel = 4, .nVdecOutPort = 3, 	.memCfg = 3,	.noPesBug = 1,	.staticGPB = 1,	.chInfo = gSdecChannelCfg_H13_A0 }, // H13 UD	
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

/**
********************************************************************************
* @brief
*	Get conf which is fit to chip revision
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*  LX_SDEC_CFG_T* : pointer of config structure
********************************************************************************
*/
LX_SDEC_CFG_T* SDEC_CFG_GetConfig(void)
{
	int i, num;

	/* For runtime branch between H13 FHD and H13 UD */
	if(lx_chip_rev() < LX_CHIP_REV(M14, A0))
	{
		if(lx_chip_plt()!= LX_CHIP_PLT_FHD)
		{
			return &_gSdecCfgs_H13UD[0];
		}
	}

	/* number of configs */
	num = sizeof(_gSdecCfgs) / sizeof(LX_SDEC_CFG_T);

	for ( i = 0 ; i < num ; i++ )
	{
		if(_gSdecCfgs[i].chip_rev == lx_chip_rev())
			return &_gSdecCfgs[i];
	}

	return &_gSdecCfgs[0];
}


/** @} */

