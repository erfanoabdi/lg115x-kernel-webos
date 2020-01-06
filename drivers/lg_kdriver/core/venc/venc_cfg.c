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
 *  main configuration file for venc device
 *	venc device will teach you how to make device driver with new platform.
 *
 *  author		youngwoo.jin (youngwoo.jin@lge.com)
 *  version		1.0
 *  date		2011.05.19
 *  note		Additional information.
 *
 *  @addtogroup lg1152_venc
 *	@{
 */

/*-----------------------------------------------------------------------------
	Control Constants
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	File Inclusions
-----------------------------------------------------------------------------*/
#include "os_util.h"
#include "venc_cfg.h"
#include "venc_drv.h"

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
	global Variables
-----------------------------------------------------------------------------*/
LX_VENC_MEM_CFG_S_T	*gpstVencMemConfig;
LX_VENC_CFG_T		*gpstVencConfig;

/*-----------------------------------------------------------------------------
	Static Function Prototypes Declarations
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Static Variables
-----------------------------------------------------------------------------*/
LX_VENC_MEM_CFG_T	gMemCfgVenc[] =
{
	// For L9 Platform
	[0] = { 
		.pcMEMName = "VENC MEM",
		.uiMEMBase = 0x0,
		.uiMEMSize = 0x02300000,
	},

	// For H13 Platform ( LGEncoder or H1Encoder )
	[1] = {
		.pcMEMName = "VENC MEM (FHD)",
		.uiMEMBase = 0x0,
		.uiMEMSize = 0x02300000,	// 35Mb
	},

	// For M14 Platform ( H1Encoder )
	[2] = {
		.pcMEMName = "VENC MEM (HD)",
		.uiMEMBase = 0x0,
		.uiMEMSize = 0x01420000,	// 20.11Mb
	},
};

LX_VENC_MEM_CFG_S_T gMemCfgVencInternal[VENC_MAX_MEM_CONFIG] =
{

	// For H13B0 (H1Encode) (Total = 0x02300000, 35Mb) (FHD)
	{
		.pcDPBName		= "VencDPB",		// Output buffer for h1encoder
		.uiDPBBase		= 0,
		.uiDPBSize		= 0x00200000,

		.pcESBufName	= "VencESBuf",		// Output buffer for User (4MB) 
		.uiESBufBase	= 0,
		.uiESBufSize	= 0x003FE000,		

		.pcAUIBufName	= "VencAUIBuf",
		.uiAUIBufBase	= 0,
		.uiAUIBufSize	= 0x00002000,		// 16bytes AUI * 512

		.pcInBufName	= "VencBuf",		// for DE Frame buffer (FHD 3 Frames)
		.uiInBufBase	= 0,
		.uiInBufSize	= 0x00CC0000,

		// Not use in H13
		// 2012/06/14 TEST for Thumbnail.
		.pcScalerName	= "VencScaler",
		.uiScalerBase	= 0x0,
		.uiScalerSize	= 0x00440000,		// 2048x1088x2 (4:2:2)

		.pcH1EncBufName = "H1Encoder",		// for H1Encoder SW/HW Shared Memory
		.uiH1EncBufBase = 0,
		.uiH1EncBufSize = 0x00C00000,		// 12MB
	},

	// For M14 (H1Encode) (Total = 0x01440000, 23.81Mb->20.25) (HD) 
	{
		.pcDPBName		= "VencDPB",		// Output buffer for h1encoder
		.uiDPBBase		= 0,
		.uiDPBSize		= 0x00100000,

		.pcESBufName	= "VencESBuf",		// Output buffer for User (2MB)
		.uiESBufBase	= 0,
		.uiESBufSize	= 0x001FF000,		

		.pcAUIBufName	= "VencAUIBuf",
		.uiAUIBufBase	= 0,
		.uiAUIBufSize	= 0x00001000,		// 16bytes AUI * 512

		.pcInBufName	= "VencBuf",		// for DE Frame buffer ( 1280(2048) x 720 x 16bits x 3 frames )
		.uiInBufBase	= 0,
		.uiInBufSize	= 0x00C00000,		// 2048*1024*6 (12MB)

		.pcScalerName	= "VencScaler",
		.uiScalerBase	= 0x0,
		//.uiScalerSize	= 0x002D0000,		// 1280x720x2   (4:2:2)
		.uiScalerSize	= 0x0021C000,		// 1280x720x1.5 (4:2:0)

		.pcH1EncBufName = "H1Encoder",		// for H1Encoder SW/HW Shared Memory
		.uiH1EncBufBase = 0,
//		.uiH1EncBufSize = 0x00600000,		// 6MB
		.uiH1EncBufSize = 0x00304000,		// 3MB (ref frame count 1)
	},
};

LX_VENC_CFG_T	 gVencCfgTable[] = {
	[0]	= {	.chip = LX_CHIP_REV( H13, A0 ),
			.venc_reg_base = H13_VENC_BASE,
			.irq_num = H13_IRQ_VENC0,
			.num_device = 1,
			.hw_clock = 300000000,	// 300Mhz
			.hw_clock_mhz = 300,
	},

	[1]	= {	.chip = LX_CHIP_REV( M14, A0 ),
			.venc_reg_base = M14_A0_VENC_BASE,
			.irq_num = M14_A0_IRQ_VENC0,
			.num_device = 1,
			.hw_clock = 300000000,	// 300Mhz
			.hw_clock_mhz = 300,
	},

	[2]	= {	.chip = LX_CHIP_REV( M14, B0 ),
			.venc_reg_base = M14_B0_VENC_BASE,
			.irq_num = M14_B0_IRQ_VENC0,
			.num_device = 1,
			.hw_clock = 320000000,	// 320Mhz
			.hw_clock_mhz = 320,
	},

	[3]	= {	.chip = LX_CHIP_REV( H14, A0 ),
			.venc_reg_base = H14_VENC_BASE,
			.irq_num = H14_IRQ_VENC0,
			.num_device = 1,
			.hw_clock = 320000000,	// 320Mhz
			.hw_clock_mhz = 320,
	},
};

/*========================================================================================
	Implementation Group
========================================================================================*/
static int _VENC_CFG_InternalMmoryMap( LX_VENC_MEM_CFG_T *pstMemCFG, LX_VENC_MEM_CFG_S_T	*pstVencInternalMemCFG )
{

	if ( pstMemCFG == NULL )
	{
		return RET_ERROR;
	}
	else
	{
		UINT32	StartAddr = 0;
		UINT32	EndAddr = 0;
		UINT32	TotalSize = 0, AllocatedSize = 0;
		
		LX_MEMCFG_T*	pM = (LX_MEMCFG_T*)pstVencInternalMemCFG;
		int 			nM = sizeof(*pstVencInternalMemCFG)/sizeof(LX_MEMCFG_T);

		StartAddr =  pstMemCFG->uiMEMBase;
		EndAddr = pstMemCFG->uiMEMBase;
		AllocatedSize = pstMemCFG->uiMEMSize;

		for ( ; nM > 0; nM--, pM++)
		{

			pM->base = EndAddr;
			EndAddr += pM->size;

			VENC_PRINT("[MEM_CFG] %12s: 0x%08X ~ 0x%08X [0x%08X]\n", pM->name, pM->base, pM->base + pM->size, pM->size);
		}

		TotalSize = EndAddr - StartAddr;

		VENC_PRINT("[MEM_CFG] -----------------------------------------------------------\n");
		VENC_PRINT("[MEM_CFG] %12s: 0x%08X ~ 0x%08X [0x%08X]\n", "Total", StartAddr, EndAddr, TotalSize);

		if ( TotalSize > AllocatedSize )
		{
			VENC_ERROR("Not enough allocated memory. (Allocated: 0x%08X, Required: 0x%08X)\n",
				AllocatedSize, TotalSize);

			return RET_ERROR;
		}
	}

	return RET_OK;
}

//void VENC_CFG_Init( LX_VENC_CFG_T *venc_cfg )
int VENC_CFG_Init( void )
{
	int ret = RET_ERROR;
	int i, flag = 0;
	unsigned int chip_rev = lx_chip_rev();

	//printk("PNG_Config\n" );

	//printk("chip_rev : 0x%X\n", chip_rev);

	for(i = 0; i < NELEMENTS(gVencCfgTable); i++)
	{
		//printk("g_png_cfg_table chip_rev : 0x%X\n", g_png_cfg_table[i].chip);
		if(chip_rev == gVencCfgTable[i].chip)
		{
			flag = 1;
			//memcpy(venc_cfg, &gVencCfgTable[i], sizeof(LX_VENC_CFG_T));
			gpstVencConfig = &gVencCfgTable[i];

			VENC_NOTI("found chip rev 0x%x\n", chip_rev);
			ret = RET_OK;			
			break;
		}
	}
	
	if ( !flag )
	{
		chip_rev = chip_rev >> 4;
		//printk("chip_rev : 0x%X\n", chip_rev);

		for(i = 0; i < NELEMENTS(gVencCfgTable); i++)
		{
			//printk("g_png_cfg_table chip_rev : 0x%X\n", g_png_cfg_table[i].chip >> 4);
			if(chip_rev == gVencCfgTable[i].chip >> 4)
			{
				//memcpy(venc_cfg, &gVencCfgTable[i], sizeof(LX_VENC_CFG_T));
				gpstVencConfig = &gVencCfgTable[i];

				flag = 1;
				break;
			}
			else if(chip_rev < gVencCfgTable[i].chip >> 4)
			{
				//memcpy(venc_cfg, &gVencCfgTable[i-1], sizeof(LX_VENC_CFG_T));
				gpstVencConfig = &gVencCfgTable[i-1];

				flag = 1;
				break;
			}
		}
		
		if(flag)
		{
			VENC_NOTI("matched chip rev 0x%0X to 0x%X\n", lx_chip_rev(), gVencCfgTable[i].chip);
			ret = RET_OK;
		}
	}

	return ret;
}

void VENC_CFG_MemoryMap ( void )
{
	LX_VENC_MEM_CFG_T *pstMemCFG;

	// FHD
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0) )
	{
		// H14
		pstMemCFG = &gMemCfgVenc[1];
		gpstVencMemConfig = &gMemCfgVencInternal[0];
	} else

	if (lx_chip_rev() >= LX_CHIP_REV(M14, A0) )
	{
		// M14
		if ( gMemCfgVenc[2].uiMEMBase == 0 )
		{
			pstMemCFG = &gMemCfgVenc[1];
			gpstVencMemConfig = &gMemCfgVencInternal[0];
		}
		else
		{
			pstMemCFG = &gMemCfgVenc[2];
			gpstVencMemConfig = &gMemCfgVencInternal[1];
		}
	} else

	{
		// H13B0
		pstMemCFG = &gMemCfgVenc[1];
		gpstVencMemConfig = &gMemCfgVencInternal[0];
	}
	
	_VENC_CFG_InternalMmoryMap( pstMemCFG, gpstVencMemConfig );
}


/** @} */

