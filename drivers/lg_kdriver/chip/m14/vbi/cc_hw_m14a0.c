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
 *	CC module control Drivers linux module
 *
 *  author      won.hur (won.hur@lge.com)
 *  version		1.0
 *  date
 *  note		Additional information.
 *
 *  @addtogroup lg1150_CC
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>	/**< printk() */
#include <linux/slab.h> 	/**< kmalloc() */
#include <linux/fs.h> 		/**< everything\ldots{} */
#include <linux/types.h> 	/**< size_t */
#include <linux/fcntl.h>	/**< O_ACCMODE */
#include <asm/uaccess.h>
#include <linux/ioport.h>	/**< For request_region, check_region etc */
#include <asm/io.h>			/**< For ioremap_nocache */

#include "vbi_kapi.h"

#include "vbi_module.h"
#include "vbi_hw.h"
#include "vbi_drv.h"
#include "os_util.h"
#include "vbi_data.h"

#include "sys_regs.h"
#include "../afe/de_cvd_reg_m14ax.h"
#include "../../../core/afe/cvd_module.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define DEBUG_PRINT_INCOMING_CC_PACKET 0

#define NEW_CC_KERNEL_DRIVER	1
#define PROBLEM_LATCH_UP_WHEN_THIS_PART_IS_ON	0

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define	VBI_OFF	0
#define	VBI_ON	1

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/
extern volatile DE_CVD_REG_M14Ax_T __iomem *gpRegCVD_M14Ax;
extern volatile DE_CVD_REG_M14Ax_T __iomem *gpRealRegCVD_M14Ax;


/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
static volatile unsigned long gCC_addr;

static unsigned int	gEnableDebugPrintCCData = 0;
static unsigned int gPrevVline625 = 0xffffffff;

/*========================================================================================
	Implementation Group
========================================================================================*/
//cvd register
int CC_M14Ax_HWInitialize(void)
{
	int ret = 0;

	CC_PRINT("[%s:%d] Attempting IOREMAPPING address of [0x%08x] with size of [0x%08x]\n", __F__, __L__, gstRegCfgVBI->vbi_register_base, gstRegCfgVBI->vbi_register_size);

	gCC_addr = (volatile unsigned long)ioremap(gstRegCfgVBI->vbi_register_base , gstRegCfgVBI->vbi_register_size);

	if((void *)gCC_addr == NULL){
		CC_PRINT("[%s:%d] Critical Error! Failed IOREMAPPING VBI HW memory IO.\n",__F__, __L__); 
		return -1;
	}
	CC_PRINT("[%s:%d] Success IOREMAPPING address of [0x%08x] with size of [0x%08x] to [0x%08x]\n", __F__, __L__, gstRegCfgVBI->vbi_register_base, gstRegCfgVBI->vbi_register_size, (unsigned int)gCC_addr);


	CVD_M14Ax_RdFL(vbi_ctrl_001);
	CVD_M14Ax_Wr01(vbi_ctrl_001, reg_vbi_little_endian, 1);
	CVD_M14Ax_WrFL(vbi_ctrl_001);

	//111128 wonsik.do : enabled one bit error ignore from M14Ax revision.
	if( lx_chip_rev( ) >= LX_CHIP_REV(L9,B0) )
	{
		CVD_M14Ax_RdFL(vbi_004);
		CVD_M14Ax_Wr01(vbi_004, reg_vbi_st_err_ignored, 1);
		CVD_M14Ax_WrFL(vbi_004);
	}

	/* 2013.06.27 won.hur : Added code for reg_swrst_vbi set to 0 
	 * There will be no operation in vbi module if this value is not set to 0.
	 * Please be aware of this CTOP register */
	CTOP_CTRL_M14A0_RdFL(ctr25);
	CTOP_CTRL_M14A0_Wr01(ctr25, reg_swrst_vbi, 0x0);
	CTOP_CTRL_M14A0_WrFL(ctr25);


	CC_PRINT("%s done\n", __func__);

	return ret;
}

int CC_M14Ax_HWUninitialize(void)
{
	int ret = 0;

	iounmap((void *)gCC_addr);

	return ret;
}

int CC_M14Ax_SetInputFormat(eVBIDataType type)
{
	int ret = 0;

	CC_PRINT("%s input : %d\n" , __func__ , type);
	
	switch(type)
	{
		case eDisable:
			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_OFF);
			CVD_M14Ax_WrFL(vbi_004);
		break;
		case eClosedCaptionUS:
		case eClosedCaptionEURO:
			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_RdFL(iris_040);

			/* Added for M14Ax on improving performance & bugs... */
			/* 2012.10.22 won.hur */
			CVD_M14Ax_RdFL(vbi_023);
			CVD_M14Ax_RdFL(vbi_024);
			CVD_M14Ax_RdFL(vbi_025);
			CVD_M14Ax_RdFL(vbi_026);
			CVD_M14Ax_RdFL(vbi_027);
			CVD_M14Ax_RdFL(vbi_028);
			CVD_M14Ax_RdFL(vbi_029);
			CVD_M14Ax_RdFL(vbi_030);
			CVD_M14Ax_RdFL(vbi_031);
			CVD_M14Ax_RdFL(vbi_032);
			CVD_M14Ax_RdFL(vbi_033);
			CVD_M14Ax_RdFL(vbi_034);
			CVD_M14Ax_RdFL(vbi_035);
			CVD_M14Ax_RdFL(vbi_036);
			CVD_M14Ax_RdFL(vbi_037);
			CVD_M14Ax_RdFL(vbi_038);


			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);

			//111130 wonsik.do : disable vbi muxout to prevent vbi data violate video data.
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_muxout, 0);
			CVD_M14Ax_Wr(vbi_006, 0xE728);
			CVD_M14Ax_Wr(vbi_007, 0);
			CVD_M14Ax_Wr(vbi_008, 0);
			CVD_M14Ax_Wr(vbi_009, 0);
			//110929 : dws add Line 18, 22 for CC Slicing
			//111014 : dws add Line 24, 25 for CC Slicing(PAL line 21,22 can be inserted on line 24, 25 on NTSC system)
			//111014 : dws removed Line 24, 25 (this is no use)
			CVD_M14Ax_Wr(vbi_010, 0x1100DD11);
			//CVD_M14Ax_Wr(vbi_011, 0x11001111);
			CVD_M14Ax_Wr(vbi_011, 0x11000000);
			//CVD_M14Ax_Wr(vbi_010, 0x11);
			//CVD_M14Ax_Wr(vbi_011, 0);
			CVD_M14Ax_Wr(vbi_014, 0x35A012DB);
			CVD_M14Ax_Wr(vbi_015, 0x1E2B159A);	// 120531 : won.hur Changed DTO value for CGMS slicing
			//CVD_M14Ax_Wr(vbi_015, 0x2066159A);
			//CVD_M14Ax_Wr(vbi_020, 0x54548282);
			CVD_M14Ax_Wr(vbi_020, 0x5A548282);    // 2011.12.10 won.hur : Changed CC find start h-sync value
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x25);

			/* Added for M14Ax on improving performance & bugs... */
			/* 2012.10.22 won.hur */
			CVD_M14Ax_Wr(vbi_023, 0x5F1B6110);
			CVD_M14Ax_Wr(vbi_024, 0x32E0222D);
			CVD_M14Ax_Wr(vbi_025, 0x00000000);
			CVD_M14Ax_Wr(vbi_026, 0x00000000);
			CVD_M14Ax_Wr(vbi_027, 0x400006A1);
			CVD_M14Ax_Wr(vbi_028, 0x400006A1);
			CVD_M14Ax_Wr(vbi_029, 0x00000000);
			CVD_M14Ax_Wr(vbi_030, 0x00000000);
			CVD_M14Ax_Wr(vbi_031, 0x00000000);
			CVD_M14Ax_Wr(vbi_032, 0x00000000);
			CVD_M14Ax_Wr(vbi_033, 0x00000000);
			CVD_M14Ax_Wr(vbi_034, 0x00000000);
			CVD_M14Ax_Wr(vbi_035, 0x11523001);
			CVD_M14Ax_Wr(vbi_036, 0x01181113);
			CVD_M14Ax_Wr(vbi_037, 0x00000001);
			CVD_M14Ax_Wr(vbi_038, 0x00000111);

			CVD_M14Ax_WrFL(vbi_023);
			CVD_M14Ax_WrFL(vbi_024);
			CVD_M14Ax_WrFL(vbi_025);
			CVD_M14Ax_WrFL(vbi_026);
			CVD_M14Ax_WrFL(vbi_027);
			CVD_M14Ax_WrFL(vbi_028);
			CVD_M14Ax_WrFL(vbi_029);
			CVD_M14Ax_WrFL(vbi_030);
			CVD_M14Ax_WrFL(vbi_031);
			CVD_M14Ax_WrFL(vbi_032);
			CVD_M14Ax_WrFL(vbi_033);
			CVD_M14Ax_WrFL(vbi_034);
			CVD_M14Ax_WrFL(vbi_035);
			CVD_M14Ax_WrFL(vbi_036);
			CVD_M14Ax_WrFL(vbi_037);
			CVD_M14Ax_WrFL(vbi_038);


			CVD_M14Ax_WrFL(vbi_004);
			CVD_M14Ax_WrFL(vbi_007);
			CVD_M14Ax_WrFL(vbi_008);
			CVD_M14Ax_WrFL(vbi_009);
			CVD_M14Ax_WrFL(vbi_010);
			CVD_M14Ax_WrFL(vbi_011);
			CVD_M14Ax_WrFL(vbi_006);
			CVD_M14Ax_WrFL(vbi_020);
			CVD_M14Ax_WrFL(vbi_014);
			CVD_M14Ax_WrFL(vbi_015);
			CVD_M14Ax_WrFL(vbi_016);
			CVD_M14Ax_WrFL(iris_040);

		break;
		case eVPS:
			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_Wr(vbi_007, 0);
			CVD_M14Ax_WrFL(vbi_007);

			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_Wr(vbi_008, 0);
			CVD_M14Ax_WrFL(vbi_008);

			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_Wr(vbi_009, 0);
			CVD_M14Ax_WrFL(vbi_009);

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_Wr(vbi_010, 0);
			CVD_M14Ax_WrFL(vbi_010);

			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_Wr(vbi_011, 0);
			CVD_M14Ax_WrFL(vbi_011);

			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_Wr(vbi_006, 0x2736);
			CVD_M14Ax_WrFL(vbi_006);

			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_Wr(vbi_020, 0x54545A82);
			CVD_M14Ax_WrFL(vbi_020);

			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_Wr01(vbi_014, reg_teletext_dto, 0x12DB);
			CVD_M14Ax_WrFL(vbi_014);

			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_Wr01(vbi_015, reg_wss625_dto, 0x2066);
			CVD_M14Ax_WrFL(vbi_015);

			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_WrFL(vbi_016);

			CVD_M14Ax_RdFL(iris_040);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x25);
			CVD_M14Ax_WrFL(iris_040);

			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);
			CVD_M14Ax_WrFL(vbi_004);
		break;

		case eTTX625A:
			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_Wr(vbi_007, 0x55555555);
			CVD_M14Ax_WrFL(vbi_007);

			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_Wr(vbi_008, 0x55555555);
			CVD_M14Ax_WrFL(vbi_008);

			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_Wr(vbi_009, 0x55555555);
			CVD_M14Ax_WrFL(vbi_009);

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_Wr(vbi_010, 0x55555555);
			CVD_M14Ax_WrFL(vbi_010);

			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_Wr(vbi_011, 0);
			CVD_M14Ax_WrFL(vbi_011);

			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_Wr(vbi_006, 0x2736);
			CVD_M14Ax_WrFL(vbi_006);

			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_Wr(vbi_020, 0x54545A82);
			CVD_M14Ax_WrFL(vbi_020);

			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_Wr01(vbi_014, reg_teletext_dto, 0x12DB);
			CVD_M14Ax_WrFL(vbi_014);

			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_Wr01(vbi_015, reg_wss625_dto, 0x2066);
			CVD_M14Ax_WrFL(vbi_015);

			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_WrFL(vbi_016);

			CVD_M14Ax_RdFL(iris_040);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x25);
			CVD_M14Ax_WrFL(iris_040);

			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);
			CVD_M14Ax_WrFL(vbi_004);
		break;
		case eTTX625B:


		break;

		case eTTX625C:

		break;
		case eTTX625D:

		break;
		case eTTX525B:
			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);
			CVD_M14Ax_WrFL(vbi_004);

			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_Wr(vbi_007, 0x99999999);
			CVD_M14Ax_WrFL(vbi_007);

			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_Wr(vbi_008, 0x99999999);
			CVD_M14Ax_WrFL(vbi_008);

			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_Wr(vbi_009, 0x99999999);
			CVD_M14Ax_WrFL(vbi_009);

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_Wr(vbi_010, 0x99999999);
			CVD_M14Ax_WrFL(vbi_010);

			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_Wr(vbi_011, 0);
			CVD_M14Ax_WrFL(vbi_011);

			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_Wr(vbi_006, 0x2736);
			CVD_M14Ax_WrFL(vbi_006);

			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_Wr(vbi_020, 0x54545A82);
			CVD_M14Ax_WrFL(vbi_020);

			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_Wr01(vbi_014, reg_teletext_dto, 0x12DB);
			CVD_M14Ax_WrFL(vbi_014);

			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_Wr01(vbi_015, reg_wss625_dto, 0x2066);
			CVD_M14Ax_WrFL(vbi_015);

			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_WrFL(vbi_016);

			CVD_M14Ax_RdFL(iris_040);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x25);
			CVD_M14Ax_WrFL(iris_040);

		break;

		case eTTX525C:
			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);
			CVD_M14Ax_WrFL(vbi_004);

			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_Wr(vbi_007, 0xAAAAAAAA);
			CVD_M14Ax_WrFL(vbi_007);

			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_Wr(vbi_008, 0xAAAAAAAA);
			CVD_M14Ax_WrFL(vbi_008);

			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_Wr(vbi_009, 0xAAAAAAAA);
			CVD_M14Ax_WrFL(vbi_009);

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_Wr(vbi_010, 0xAAAAAAAA);
			CVD_M14Ax_WrFL(vbi_010);

			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_Wr(vbi_011, 0);
			CVD_M14Ax_WrFL(vbi_011);

			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_Wr(vbi_006, 0xE736);
			CVD_M14Ax_WrFL(vbi_006);

			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_Wr(vbi_020, 0x54546482);
			CVD_M14Ax_WrFL(vbi_020);

			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_Wr01(vbi_014, reg_teletext_dto, 0x12DB);
			CVD_M14Ax_WrFL(vbi_014);

			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_Wr01(vbi_015, reg_wss625_dto, 0x2066);
			CVD_M14Ax_WrFL(vbi_015);

			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_WrFL(vbi_016);

			CVD_M14Ax_RdFL(iris_040);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x23);
			CVD_M14Ax_WrFL(iris_040);
		break;
		case eTTX525D:
			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);
			CVD_M14Ax_WrFL(vbi_004);

			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_Wr(vbi_007, 0xBBBBBBBB);
			CVD_M14Ax_WrFL(vbi_007);

			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_Wr(vbi_008, 0xBBBBBBBB);
			CVD_M14Ax_WrFL(vbi_008);

			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_Wr(vbi_009, 0xBBBBBBBB);
			CVD_M14Ax_WrFL(vbi_009);

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_Wr(vbi_010, 0xBBBBBBBB);
			CVD_M14Ax_WrFL(vbi_010);

			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_Wr(vbi_011, 0);
			CVD_M14Ax_WrFL(vbi_011);

			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_Wr(vbi_006, 0xA736);
			CVD_M14Ax_WrFL(vbi_006);

			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_Wr(vbi_020, 0x54545A82);
			CVD_M14Ax_WrFL(vbi_020);

			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_Wr01(vbi_014, reg_teletext_dto, 0x12DB);
			CVD_M14Ax_WrFL(vbi_014);

			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_Wr01(vbi_015, reg_wss625_dto, 0x2066);
			CVD_M14Ax_WrFL(vbi_015);

			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_WrFL(vbi_016);

			CVD_M14Ax_RdFL(iris_040);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x23);
			CVD_M14Ax_WrFL(iris_040);

		break;

		case eWSS625	:
			CVD_M14Ax_RdFL(vbi_004);
			CVD_M14Ax_Wr01(vbi_004, reg_vbi_en, VBI_ON);
			CVD_M14Ax_WrFL(vbi_004);

			CVD_M14Ax_RdFL(vbi_007);
			CVD_M14Ax_Wr(vbi_007, 0);
			CVD_M14Ax_WrFL(vbi_007);

			CVD_M14Ax_RdFL(vbi_008);
			CVD_M14Ax_Wr(vbi_008, 0);
			CVD_M14Ax_WrFL(vbi_008);

			CVD_M14Ax_RdFL(vbi_009);
			CVD_M14Ax_Wr(vbi_009, 0);
			CVD_M14Ax_WrFL(vbi_009);

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_Wr(vbi_010, 0);
			CVD_M14Ax_WrFL(vbi_010);

			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_Wr(vbi_011, 0x00CC0000);
			CVD_M14Ax_WrFL(vbi_011);

			CVD_M14Ax_RdFL(vbi_006);
			CVD_M14Ax_Wr(vbi_006, 0xE736);
			CVD_M14Ax_WrFL(vbi_006);

			CVD_M14Ax_RdFL(vbi_020);
			CVD_M14Ax_Wr(vbi_020, 0x54828282);
			CVD_M14Ax_WrFL(vbi_020);

			CVD_M14Ax_RdFL(vbi_014);
			CVD_M14Ax_Wr01(vbi_014, reg_teletext_dto, 0x12DB);
			CVD_M14Ax_WrFL(vbi_014);

			CVD_M14Ax_RdFL(vbi_015);
			CVD_M14Ax_Wr01(vbi_015, reg_wss625_dto, 0x1599);
			CVD_M14Ax_WrFL(vbi_015);

			CVD_M14Ax_RdFL(vbi_016);
			CVD_M14Ax_Wr01(vbi_016, reg_wss625_frame_start, 0x2);
			CVD_M14Ax_WrFL(vbi_016);

			CVD_M14Ax_RdFL(iris_040);
			CVD_M14Ax_Wr01(iris_040, reg_vsync_vbi_max, 0x23);
			CVD_M14Ax_WrFL(iris_040);
		break;
		case eWSSJ	:

		break;
		default:

		break;
	}

	CC_PRINT("%s done\n", __func__);
	return ret;
}


int CC_M14Ax_SetInterruptClear(void)
{
	int ret = 0;
	UINT32 intr_vp1_en;

	CVD_M14Ax_RdFL(cvd_intr1_clear0);
	intr_vp1_en = CVD_M14Ax_Rd(cvd_intr1_clear0);
	CVD_M14Ax_Wr(cvd_intr1_clear0, (intr_vp1_en |= (1<<15)));
	CVD_M14Ax_WrFL(cvd_intr1_clear0);

	CVD_M14Ax_RdFL(cvd_intr1_clear0);
	intr_vp1_en = CVD_M14Ax_Rd(cvd_intr1_clear0);
	CVD_M14Ax_Wr(cvd_intr1_clear0, (intr_vp1_en &= ~(1<<15)));
	CVD_M14Ax_WrFL(cvd_intr1_clear0);

	return ret;
}


int CC_M14Ax_SetInterrupt(int on_off)
{
	int ret = 0;
	UINT32 intr_vp1_en;

	if(on_off)
	{

		/* 2013.06.27 won.hur : Added code for reg_swrst_vbi set to 0 
		 * There will be no operation in vbi module if this value is not set to 0.
		 * Please be aware of this CTOP register */
		CTOP_CTRL_M14A0_RdFL(ctr25);
		CTOP_CTRL_M14A0_Wr01(ctr25, reg_swrst_vbi, 0x0);
		CTOP_CTRL_M14A0_WrFL(ctr25);

		CVD_M14Ax_RdFL(cvd_intr1_enable1);
		intr_vp1_en = CVD_M14Ax_Rd(cvd_intr1_enable1);
		CVD_M14Ax_Wr(cvd_intr1_enable1, (intr_vp1_en |= L9_VBI_INTERRUPT_MASK));
		CVD_M14Ax_WrFL(cvd_intr1_enable1);
	}
	else{
		CVD_M14Ax_RdFL(cvd_intr1_enable1);
		intr_vp1_en = CVD_M14Ax_Rd(cvd_intr1_enable1);
		CVD_M14Ax_Wr(cvd_intr1_enable1, (intr_vp1_en &= ~L9_VBI_INTERRUPT_MASK));
		CVD_M14Ax_WrFL(cvd_intr1_enable1);

		VBI_CC_SetInterruptClear();
	}

	return ret;

}


int CC_M14Ax_ParsingVBIData(unsigned int *packet_array)
{
	int ret = 0;
	unsigned int start, end, count;
	sCloseCaption data[6];	// 5->6 Added to prevent segmentation fault
	unsigned int valid_packet_count, buffer_number;
	unsigned int start_inc, first_bytes, second_bytes;
	int sent_data_count;
	int i;
	unsigned long addr;
#if NEW_CC_KERNEL_DRIVER
	int data_pointer = 0;
	sCGMS cgms_data;
	unsigned char vbi_data_type = 0x00;
#else
	unsigned int 	packet_count = count/9;
	int	valid_packet_count = 0;
	int	valid_data_flag = 0;
	sCloseCaption temp_data;
	int found_preamble;
	int found_preamble_pointer;
	int loop_count;
#endif

	start = 0;
	end = 0;
	count = 0;

	CVD_M14Ax_RdFL(vbi_ctrl_002);
	CVD_M14Ax_Rd01(vbi_ctrl_002, reg_vbi_buffer_number, buffer_number);

	switch(buffer_number)
	{
		case 0:
			CVD_M14Ax_RdFL(vbi_ctrl_002);
			CVD_M14Ax_RdFL(vbi_ctrl_003);

			CVD_M14Ax_Rd01(vbi_ctrl_002, reg_vbi0_start_addr, start);
			CVD_M14Ax_Rd01(vbi_ctrl_003, reg_vbi0_end_addr, end);
			CVD_M14Ax_Rd01(vbi_ctrl_002, reg_vbi0_data_cnt, count);

			break;
		case 1:
			CVD_M14Ax_RdFL(vbi_ctrl_004);
			CVD_M14Ax_RdFL(vbi_ctrl_005);

			CVD_M14Ax_Rd01(vbi_ctrl_004, reg_vbi1_start_addr, start);
			CVD_M14Ax_Rd01(vbi_ctrl_005, reg_vbi1_end_addr, end);
			CVD_M14Ax_Rd01(vbi_ctrl_004, reg_vbi1_data_cnt, count);

			break;
		case 2:
			CVD_M14Ax_RdFL(vbi_ctrl_006);
			CVD_M14Ax_RdFL(vbi_ctrl_007);

			CVD_M14Ax_Rd01(vbi_ctrl_006, reg_vbi2_start_addr, start);
			CVD_M14Ax_Rd01(vbi_ctrl_007, reg_vbi2_end_addr, end);
			CVD_M14Ax_Rd01(vbi_ctrl_006, reg_vbi2_data_cnt, count);

			break;
		case 3:
			CVD_M14Ax_RdFL(vbi_ctrl_008);
			CVD_M14Ax_RdFL(vbi_ctrl_009);

			CVD_M14Ax_Rd01(vbi_ctrl_008, reg_vbi3_start_addr, start);
			CVD_M14Ax_Rd01(vbi_ctrl_009, reg_vbi3_end_addr, end);
			CVD_M14Ax_Rd01(vbi_ctrl_008, reg_vbi3_data_cnt, count);

			break;
		default:
			ret = -1;
			return ret;
			break;
	}

#if NEW_CC_KERNEL_DRIVER

	if(count > 0x800){
		if(gEnableDebugPrintCCData>0) CC_PRINT("ERROR! HW BUG! CVD VBI sliced data count is over 0x800!\n");
		return -1;
	}
	else
	{
		valid_packet_count = 0;
		start_inc = start * 4;

		if(count > 0) data_pointer = count;
		else data_pointer = 0;

		while(data_pointer >= 1)
		{
			if( ((gstRegCfgVBI->vbi_register_size>>2)*4 - start_inc) < 9)
			{
				first_bytes = ((gstRegCfgVBI->vbi_register_size>>2)*4 - start_inc);

				if(first_bytes >= 4) vbi_data_type = (unsigned char)(*(unsigned int *)(gCC_addr + start_inc + 3));
				else vbi_data_type = (unsigned char)(*(unsigned int *)(gCC_addr + 3 - first_bytes));

				/* For Closed Caption Data */
				if((vbi_data_type == 0x00) || (vbi_data_type == 0x01))
				{
					second_bytes = 9 - first_bytes;

					addr = (unsigned long)&data[valid_packet_count];
					valid_packet_count++;

					memcpy((void *)addr , (void *)(gCC_addr + start_inc ) , first_bytes );
					memcpy((void *)(addr + first_bytes), (void *)(gCC_addr) ,second_bytes );

					if(gEnableDebugPrintCCData>0)
					{
						printk("CC_MESSAGE_A= %4d %4d %4d", start, end, count);
						for(i=0; i<first_bytes; i++)printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						for(i=0; i<second_bytes; i++) printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + i)));
						printk("\n");
					}

					start_inc = second_bytes;
					data_pointer -= 9;
				}
				/* For CGMS data on 20 line */
				else if((vbi_data_type == 0x0A) || (vbi_data_type == 0x0B))
				{
					second_bytes = 10 - first_bytes;
#if 0
					memcpy((void *)&cgms_data, (void *)(gCC_addr + start_inc ) , first_bytes );
					memcpy((void *)(&cgms_data + first_bytes), (void *)(gCC_addr) ,second_bytes );

					if(gEnableDebugPrintCCData>0)
					{
						printk("CC_MESSAGE_B= %4d %4d %4d", start, end, count);
						for(i=0; i<first_bytes; i++)printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						for(i=0; i<second_bytes; i++) printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + i)));
						printk("\n");
					}

					VBI_PutCGMSData(&cgms_data);
#endif
					start_inc = second_bytes;
					data_pointer -= 10;
				}
				/* Exception case for WSS when 625i video input to CVD module */
				else if((vbi_data_type == 0x08) || (vbi_data_type == 0x09))
				{
					second_bytes = 9 - first_bytes;
#if 0
					memcpy((void *)&cgms_data, (void *)(gCC_addr + start_inc ) , first_bytes );
					memcpy((void *)(&cgms_data + first_bytes), (void *)(gCC_addr) ,second_bytes );

					if(gEnableDebugPrintCCData>0)
					{
						printk("CC_MESSAGE_C= %4d %4d %4d", start, end, count);
						for(i=0; i<first_bytes; i++)printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						for(i=0; i<second_bytes; i++) printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + i)));
						printk("\n");
					}

					VBI_PutCGMSData(&cgms_data);
#endif
					start_inc = second_bytes;
					data_pointer -= 9;
				}
				else
				{
					//CC_PRINT("VBI CC : WARNING UNKNOWN VBI DATA CAME!! VBI DATA TYPE IS [0x%2x]!\n", vbi_data_type);
					data_pointer = 0;	// Get out of while loop.
				}
			}
			else
			{

				vbi_data_type = (unsigned char)(*(unsigned int *)(gCC_addr + start_inc + 3));

				if((vbi_data_type == 0x00) || (vbi_data_type == 0x01))
				{
					memcpy((void *)&data[valid_packet_count], (void *)(gCC_addr + start_inc ) , 9);

					if(gEnableDebugPrintCCData>0)
					{
						printk("CC_MESSAGE_A= %4d %4d %4d", start, end, count);
						for(i=0; i<9; i++) printk(" %2x",(unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						printk("\n");
					}

					valid_packet_count++;

					start_inc += 9;
					data_pointer -= 9;
				}
				else if((vbi_data_type == 0x0A) || (vbi_data_type == 0x0B))
				{
					/* 예외 처리 */
					if(((gstRegCfgVBI->vbi_register_size>>2)*4 - start_inc) == 9)
					{
						memcpy((void *)&cgms_data, (void *)(gCC_addr + start_inc ) , 9);
						memcpy((void *)&cgms_data, (void *)(gCC_addr), 1);

						if(gEnableDebugPrintCCData>0)
						{
							printk("CC_MESSAGE_B= %4d %4d %4d", start, end, count);
							for(i=0; i< 9; i++) printk(" %2x",(unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
							printk(" %2x",(unsigned char )(*(unsigned int *)(gCC_addr)));
							printk("\n");
							}
					}
					else
					{
						memcpy((void *)&cgms_data, (void *)(gCC_addr + start_inc ) , 10 );
						if(gEnableDebugPrintCCData>0)
						{
							printk("CC_MESSAGE_B= %4d %4d %4d", start, end, count);
							for(i=0; i< 10; i++) printk(" %2x",(unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
							printk("\n");
						}
					}
					VBI_PutCGMSData(&cgms_data);

					start_inc += 10;
					data_pointer -= 10;
				}
				else if((vbi_data_type == 0x08) || (vbi_data_type == 0x09))
				{
					memcpy((void *)&cgms_data, (void *)(gCC_addr + start_inc ) , 9 );
					VBI_PutCGMSData(&cgms_data);

					if(gEnableDebugPrintCCData>0)
					{
						printk("CC_MESSAGE_C= %4d %4d %4d", start, end, count);
						for(i=0; i< 9; i++) printk(" %2x",(unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						printk("\n");
					}

					start_inc += 9;
					data_pointer -= 9;
				}
				else
				{
				//CC_PRINT("VBI CC : WARNING UNKNOWN VBI DATA CAME!! VBI DATA TYPE IS [0x%2x]!\n", vbi_data_type);
				data_pointer = 0; // Get out of while loop
				}

			}
		}

		sent_data_count = 0;

		for(i=0; i<valid_packet_count; i++)
		{
			packet_array[sent_data_count] = ((data[i].LineCount & 0x3FF)<<22)|((data[i].Status & 0x0F)<<16)|(data[i].data1 << 8)|data[i].data0;
			sent_data_count++;
		}

		ret = sent_data_count;
	}
#else
	packet_count = count/9;
	valid_data_flag = 0;
	valid_packet_count = 0;
	buffer_number = 0;
	found_preamble = 0;
	found_preamble_pointer =0;
	sent_data_count = -1;

	if(count%9)
		packet_count++;

	if(packet_count <= 5)
	{
		start_inc = start * 4;

		for(loop_count = 0;loop_count < packet_count;loop_count ++)
		{
			if( (start < gstRegCfgVBI->vbi_register_size) && (end < gstRegCfgVBI->vbi_register_size) /*&& (count <= 4*5)*/ )
			{
				valid_data_flag = 1;
				if( ((gstRegCfgVBI->vbi_register_size>>2)*4 - start_inc) < 9)
				{

					first_bytes = ((gstRegCfgVBI->vbi_register_size>>2)*4 - start_inc);
					second_bytes = 9 - first_bytes;

					addr = (unsigned long)&data[valid_packet_count];

					memcpy((void *)addr , (void *)(gCC_addr + start_inc ) , first_bytes );
					memcpy((void *)(addr + first_bytes), (void *)(gCC_addr) ,second_bytes );
					if(gEnableDebugPrintCCData)
					{
						printk("CC_MESSAGE_A= %4d %4d %4d", start, end, count);
						for(i=0; i<first_bytes; i++)printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						for(i=0; i<second_bytes; i++) printk(" %2x", (unsigned char )(*(unsigned int *)(gCC_addr + i)));
						printk("\n");
					}
					valid_packet_count++;
					start_inc = second_bytes;
				}
				else
				{
					memcpy((void *)&data[valid_packet_count], (void *)(gCC_addr + start_inc ) , 9);
					if(gEnableDebugPrintCCData)
					{
						printk("CC_MESSAGE_A= %4d %4d %4d", start, end, count);
						for(i=0; i<9; i++) printk(" %2x",(unsigned char )(*(unsigned int *)(gCC_addr + start_inc + i)));
						printk("\n");
					}
					valid_packet_count++;
					start_inc += 9;
				}
			}
			else
			{
				CC_PRINT("ERROR!!! start[%x], end[%x], \n" ,start, end);
				valid_data_flag = 0;
			}
		}

		for(loop_count = 0;loop_count < valid_packet_count; loop_count++)
		{
			if((count%9)&&(count >9))
			{
				memcpy((void *)&temp_data, (void *)&data[loop_count], 9);
				if(found_preamble == 0)
				{
					for(i=0; i<9; i++)

					{
						if(found_preamble == 0)
						{
				 			if((*((unsigned char *)&temp_data+i)== CC_VBI_PREAMBLE_0)&&\
				 			(*((unsigned char *)&temp_data+i+1)== CC_VBI_PREAMBLE_1)&&\
			       			(*((unsigned char *)&temp_data+i+2)== CC_VBI_PREAMBLE_2)) found_preamble++;
						}
						else if(found_preamble == 1)
						{
				 			if((*((unsigned char *)&temp_data+i)== CC_VBI_PREAMBLE_0)&&\
				 			(*((unsigned char *)&temp_data+i+1)== CC_VBI_PREAMBLE_1)&&\
			       			(*((unsigned char *)&temp_data+i+2)== CC_VBI_PREAMBLE_2))
							{
								found_preamble++;
								found_preamble_pointer = i;
							}
							else if(i == 7)
							{
								if((*((unsigned char *)&temp_data+i)== CC_VBI_PREAMBLE_0)&&\
				 				(*((unsigned char *)&temp_data+i+1)== CC_VBI_PREAMBLE_1)) found_preamble = 3;

							}
							else if(i == 8)
							{
								if(*((unsigned char *)&temp_data+i)== CC_VBI_PREAMBLE_0) found_preamble = 4;
							}
						}
					}

					if(found_preamble == 2)
					{
						memcpy((unsigned char *)&data[loop_count], ((unsigned char *)&temp_data + found_preamble_pointer), (9-found_preamble_pointer));
						memcpy(((unsigned char *)&data[loop_count]+9-found_preamble_pointer), (unsigned char *)(&data[loop_count+1]), found_preamble_pointer);

						if( ((*((unsigned char *)&data[loop_count])== CC_VBI_PREAMBLE_0)&&\
		    		 	(*((unsigned char *)&data[loop_count]+1)== CC_VBI_PREAMBLE_1)&&\
		      	     	(*((unsigned char *)&data[loop_count]+2)== CC_VBI_PREAMBLE_2))&&(valid_data_flag >0))
						{
							sent_data_count++;
							packet_array[sent_data_count] = ((data[loop_count].LineCount & 0x3FF)<<22)|((data[loop_count].Status & 0x0F)<<16)|(data[loop_count].data1 << 8)|data[loop_count].data0;
						}
					}
				}
				else if(found_preamble == 2)
				{
					memcpy((unsigned char *)&data[loop_count], ((unsigned char *)&temp_data + found_preamble_pointer), (9-found_preamble_pointer));
					memcpy(((unsigned char *)&data[loop_count]+9-found_preamble_pointer), (unsigned char *)(&data[loop_count+1]), found_preamble_pointer);
					if( ((*((unsigned char *)&data[loop_count])== CC_VBI_PREAMBLE_0)&&\
		    	 	(*((unsigned char *)&data[loop_count]+1)== CC_VBI_PREAMBLE_1)&&\
		          	(*((unsigned char *)&data[loop_count]+2)== CC_VBI_PREAMBLE_2))&&(valid_data_flag >0))
					{
						sent_data_count++;
						packet_array[sent_data_count] = ((data[loop_count].LineCount & 0x3FF)<<22)|((data[loop_count].Status & 0x0F)<<16)|(data[loop_count].data1 << 8)|data[loop_count].data0;
					}
				}
				else if((found_preamble == 3)||(found_preamble == 4))
				{
					if(found_preamble == 3)
					{
						if(*((unsigned char *)&temp_data)== CC_VBI_PREAMBLE_2)
						{
							found_preamble_pointer = 2;
							found_preamble = 2;

							*((unsigned char *)&data[loop_count]) = CC_VBI_PREAMBLE_0;
							*((unsigned char *)&data[loop_count]+1) = CC_VBI_PREAMBLE_1;

							memcpy((unsigned char *)&data[loop_count]+found_preamble_pointer, ((unsigned char *)&temp_data), (9-found_preamble_pointer));

							if( ((*((unsigned char *)&data[loop_count])== CC_VBI_PREAMBLE_0)&&\
		    	 			(*((unsigned char *)&data[loop_count]+1)== CC_VBI_PREAMBLE_1)&&\
		         	 		(*((unsigned char *)&data[loop_count]+2)== CC_VBI_PREAMBLE_2))&&(valid_data_flag >0))
							{
								sent_data_count++;
								packet_array[sent_data_count] = ((data[loop_count].LineCount & 0x3FF)<<22)|((data[loop_count].Status & 0x0F)<<16)|(data[loop_count].data1 << 8)|data[loop_count].data0;
							}
						}
					}
					else
					{
						if((*((unsigned char *)&temp_data)== CC_VBI_PREAMBLE_1)&&\
						   (*((unsigned char *)&temp_data+1) == CC_VBI_PREAMBLE_2))
						{
							found_preamble_pointer = 1;
							found_preamble = 2;

							*((unsigned char *)&data[loop_count]) = CC_VBI_PREAMBLE_0;

							memcpy((unsigned char *)&data[loop_count]+found_preamble_pointer, ((unsigned char *)&temp_data), (9-found_preamble_pointer));

							if( ((*((unsigned char *)&data[loop_count])== CC_VBI_PREAMBLE_0)&&\
		    	 			(*((unsigned char *)&data[loop_count]+1)== CC_VBI_PREAMBLE_1)&&\
		         	 		(*((unsigned char *)&data[loop_count]+2)== CC_VBI_PREAMBLE_2))&&(valid_data_flag >0))
							{
								sent_data_count++;
								packet_array[sent_data_count] = ((data[loop_count].LineCount & 0x3FF)<<22)|((data[loop_count].Status & 0x0F)<<16)|(data[loop_count].data1 << 8)|data[loop_count].data0;
							}
						}
					}
					if(found_preamble != 2) found_preamble=1;
				}
			}
			else
			{
				if( ((*((unsigned char *)&data[loop_count])== CC_VBI_PREAMBLE_0)&&\
		    		 (*((unsigned char *)&data[loop_count]+1)== CC_VBI_PREAMBLE_1)&&\
		      	     (*((unsigned char *)&data[loop_count]+2)== CC_VBI_PREAMBLE_2))&&(valid_data_flag >0))
				{
					if(valid_packet_count <= 0)
					{
						CC_PRINT("FATAL ERROR!!! Data captured but valid data flag did not raise up!!\n");
						break;
					}
					else
					{
						sent_data_count++;
						packet_array[sent_data_count] = ((data[loop_count].LineCount & 0x3FF)<<22)|((data[loop_count].Status & 0x0F)<<16)|(data[loop_count].data1 << 8)|data[loop_count].data0;
					}
				}
				else
					CC_PRINT("Data came, but preamble was wrong! \n");
			}
		}
		ret = sent_data_count + 1;
	}
	else
	{
		CC_PRINT("Data came. But count value has exceeded max value. Count[%x], Start[%x], End[%x], \n" ,count, start, end);
		ret = -1;
	}
#endif

	return ret;
}

int CC_M14Ax_DebugPrintMode(int mode)
{
	int ret = 0;

	if(mode) gEnableDebugPrintCCData = 1;
	else gEnableDebugPrintCCData = 0;

	return ret;
}

int CC_M14Ax_CheckVline625(void)
{
	int ret = RET_OK;
	unsigned int isVline625;

	return RET_OK;

	CVD_M14Ax_RdFL(iris_013);
	CVD_M14Ax_Rd01(iris_013, reg_vline_625, isVline625);

	if(isVline625 != 0)
	{
		if(isVline625 != gPrevVline625)
		{
			gPrevVline625 = isVline625;

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_RdFL(vbi_015);

			CVD_M14Ax_Wr(vbi_010, 0x0000DD00);
			CVD_M14Ax_Wr(vbi_011, 0x000C0000);
			CVD_M14Ax_Wr(vbi_015, 0x2066159A);

			CVD_M14Ax_WrFL(vbi_010);
			CVD_M14Ax_WrFL(vbi_011);
			CVD_M14Ax_WrFL(vbi_015);

		}
	}
	else
	{
		if(isVline625 != gPrevVline625)
		{
			gPrevVline625 = isVline625;

			CVD_M14Ax_RdFL(vbi_010);
			CVD_M14Ax_RdFL(vbi_011);
			CVD_M14Ax_RdFL(vbi_015);

			CVD_M14Ax_Wr(vbi_010, 0x1100DD11);
			CVD_M14Ax_Wr(vbi_011, 0x11000000);
			CVD_M14Ax_Wr(vbi_015, 0x1E2B159A);

			CVD_M14Ax_WrFL(vbi_010);
			CVD_M14Ax_WrFL(vbi_011);
			CVD_M14Ax_WrFL(vbi_015);
		}
	}

	return ret;
}

int CC_M14Ax_CheckAnalogCP(void)
{
	LX_VBI_CP_TYPE_T ret = RET_ERROR;	// NULL value for init value.
	unsigned int analog_cp_psync;
	unsigned int analog_cp_cstripe;
	unsigned int cur_hlock, cur_vlock;


	/* Get Lock Info */
	CVD_M14Ax_RdFL(iris_001);
	CVD_M14Ax_Rd01(iris_001, reg_hlock, cur_hlock);
	CVD_M14Ax_RdFL(iris_001);
	CVD_M14Ax_Rd01(iris_001, reg_vlock, cur_vlock);

	/* Get CP Info */
	CVD_M14Ax_RdFL(iris_001);
	CVD_M14Ax_Rd01(iris_001, reg_mv_vbi_detected, analog_cp_psync);
	CVD_M14Ax_Rd01(iris_001, reg_mv_colourstripes, analog_cp_cstripe);

	do{
		ret = 0; 	// Initiate return value

		if((analog_cp_psync == 0)||(cur_hlock == 0)||(cur_vlock == 0)) 
		{
			ret = 0; 
			break;
		}
		else {
			ret = (ret)|(LX_VBI_CP_TYPE_ANALOG_1);
		}

		/* DO NOT USE CSTRIPE INFO!
		 * CVD CP CSTRIPE HW DOES NOT WORK PROPERLY!! */
		//if(analog_cp_cstripe) ret = (ret)|(LX_VBI_CP_TYPE_ANALOG_2);
	}while(0);

	if(gEnableDebugPrintCCData){
		printk("[%s] ret[0x%x] PSYNC[0x%x],CS[0x%x], Hlock[%d] Vlock[%d]\n", __F__, ret, analog_cp_psync, analog_cp_cstripe, cur_hlock, cur_vlock);
	}

	return (int)ret;
}

#ifdef	KDRV_CONFIG_PM

int CC_M14Ax_RunSuspend(void)
{
	// param will be saved automatically....
	return 0;
}

int CC_M14Ax_RunResume(void)
{
	return 0;
}

#endif

/** @} */
