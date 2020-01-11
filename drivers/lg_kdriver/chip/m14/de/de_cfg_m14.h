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
 * main driver implementation for de device.
 * de device will teach you how to make device driver with new platform.
 *
 * author     jaemo.kim (jaemo.kim@lge.com)
 * version    1.0
 * date       2011.04.06
 * note       Additional information.
 *
 * @addtogroup lg1152_de
 * @{
 */

#ifndef  DE_CFG_M14_INC
#define  DE_CFG_M14_INC

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/
//#define USE_PREW_MEM_ON_LBUS_M14B0
#define USE_SEPERATED_BE_MEM_M14B0
//#define USE_VIDEO_UART2_FOR_MCU

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include "de_model.h"
#include "de_ver_def.h"
#include "de_cfg_def_m14.h"

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/
#define VIDEO_M14_FRAME_DDR_LW_SCR          783
#define VIDEO_M14_FRAME_DDR_LW_CAP          348
#define VIDEO_M14_FRAME_DDR_LW_TNR_IPC      870
#define VIDEO_M14_FRAME_DDR_RW_SCR          783
#define VIDEO_M14_FRAME_DDR_RW_CAP          348
#define VIDEO_M14_FRAME_DDR_RW_TNR_IPC      696
#define VIDEO_M14_FRAME_DDR_SW_VCR          228
#define VIDEO_M14_FRAME_DDR_SW_MDI          138
#define VIDEO_M14_FRAME_DDR_LW_TNM          69
#define VIDEO_M14_FRAME_DDR_LW_CSC          416

#define VIDEO_M14_CODEC_DDR_BASE            (0x70000000)
#define VIDEOM14_LOWER_DDR_BASE             (0x80000000)
#define VIDEO_M14_FRAME_DDR_BASE            (0x90000000)
#define VIDEO_M14_FRAME_FIRMWARE_OFFSET     (0x00000000)

#define VIDEO_M14_CODEC_DDR_OFFSET          (0x2000000)

#define VIDEO_M14_ROW2PHY(_r)               (CONV_MEM_ROW2BYTE(_r) | VIDEO_M14_FRAME_DDR_BASE)

#define VIDEO_M14_FIRMWARE_ROW_SIZE_OF_DE   (VIDEO_MEM_FIRMWARE_ROW_SIZE*2)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_OF_LED  (VIDEO_MEM_FIRMWARE_ROW_SIZE)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_OF_IPC  (VIDEO_MEM_FIRMWARE_ROW_SIZE)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_IPC_MC	(1)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_DDR_UP	(1)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_OF_TTX	(1)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_WEB_OS	(1)
#define VIDEO_M14_FIRMWARE_ROW_SIZE_REG_TR	(1)

#ifdef USE_DE_FIRMWARE_RUN_IN_PAK_M14
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_LED	(VIDEO_M14_FRAME_FIRMWARE_OFFSET/VIDEO_ROW_STRIDE)
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_LED + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_LED)
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_DE	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_IPC)
#define VIDEO_M14_START_OF_FRAME_MEMORY		(VIDEO_M14_FIRMWARE_ROW_BASE_OF_DE  + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_DE)
#else
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_DE	(VIDEO_M14_FRAME_FIRMWARE_OFFSET/VIDEO_ROW_STRIDE)
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_LED	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_DE  + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_DE)
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_LED + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_LED)
#define VIDEO_M14_START_OF_FRAME_MEMORY		(VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_IPC)
#endif

#define VIDEO_M14_ROW_SIZE_OF_FRAME_MEMORY  (VIDEO_M14_FRAME_DDR_LW_SCR + VIDEO_M14_FRAME_DDR_LW_TNR_IPC + VIDEO_M14_FRAME_DDR_RW_SCR + VIDEO_M14_FRAME_DDR_RW_TNR_IPC + VIDEO_M14_FRAME_DDR_SW_VCR + VIDEO_M14_FRAME_DDR_LW_TNM)
#define VIDEO_M14_ROW_SIZE_OF_FRAME_PREW    (VIDEO_M14_FRAME_DDR_LW_CAP + VIDEO_M14_FRAME_DDR_RW_CAP + 32)
#define VIDEO_M14_ROW_SIZE_OF_FRAME_GRAP    (VIDEO_M14_FRAME_DDR_SW_MDI)
#define VIDEO_M14_ROW_SIZE_OF_FRAME_GCSC    (VIDEO_M14_FRAME_DDR_LW_CSC)

#define VIDEO_M14_FIRMWARE_ROW_BASE_IPC_MC	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC                                     )
#define VIDEO_M14_FIRMWARE_ROW_BASE_DDR_UP	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC + VIDEO_M14_FIRMWARE_ROW_SIZE_IPC_MC)
#define VIDEO_M14_FIRMWARE_ROW_BASE_OF_TTX	(VIDEO_M14_FIRMWARE_ROW_BASE_DDR_UP + VIDEO_M14_FIRMWARE_ROW_SIZE_DDR_UP)
#define VIDEO_M14_FIRMWARE_ROW_BASE_WEB_OS	(VIDEO_M14_FIRMWARE_ROW_BASE_OF_TTX + VIDEO_M14_FIRMWARE_ROW_SIZE_WEB_OS)
#define VIDEO_M14_FIRMWARE_ROW_BASE_REG_TR	(VIDEO_M14_FIRMWARE_ROW_BASE_WEB_OS + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_TTX)

#define VIDEO_M14_FIRMWARE_MEM_BASE_OF_DE   VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_OF_DE)
#define VIDEO_M14_FIRMWARE_MEM_BASE_OF_LED  VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_OF_LED)
#define VIDEO_M14_FIRMWARE_MEM_BASE_OF_IPC  VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_OF_IPC)
#define VIDEO_M14_FIRMWARE_MEM_BASE_DDR_UP	VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_DDR_UP)
#define VIDEO_M14_FIRMWARE_MEM_BASE_OF_TTX	VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_OF_TTX)
#define VIDEO_M14_FIRMWARE_MEM_BASE_WEB_OS	VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_WEB_OS)
#define VIDEO_M14_FIRMWARE_MEM_BASE_REG_TR	VIDEO_M14_ROW2PHY(VIDEO_M14_FIRMWARE_ROW_BASE_REG_TR)
#define VIDEO_M14_MBASE_OF_FRAME_MEMORY     VIDEO_M14_ROW2PHY(VIDEO_M14_START_OF_FRAME_MEMORY)

#define VIDEO_M14_FIRMWARE_MEM_SIZE_OF_DE	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_OF_DE)
#define VIDEO_M14_FIRMWARE_MEM_SIZE_OF_LED	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_OF_LED)
#define VIDEO_M14_FIRMWARE_MEM_SIZE_OF_IPC	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_OF_IPC)
#define VIDEO_M14_FIRMWARE_MEM_SIZE_DDR_UP	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_DDR_UP)
#define VIDEO_M14_FIRMWARE_MEM_SIZE_OF_TTX	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_OF_TTX)
#define VIDEO_M14_FIRMWARE_MEM_SIZE_WEB_OS	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_WEB_OS)
#define VIDEO_M14_FIRMWARE_MEM_SIZE_REG_TR	CONV_MEM_ROW2BYTE(VIDEO_M14_FIRMWARE_ROW_SIZE_REG_TR)
#define VIDEO_M14_MEM_SIZE_OF_FRAME_MEMORY  CONV_MEM_ROW2BYTE(VIDEO_M14_ROW_SIZE_OF_FRAME_MEMORY)
#define VIDEO_M14_MEM_SIZE_OF_FRAME_PREW    CONV_MEM_ROW2BYTE(VIDEO_M14_ROW_SIZE_OF_FRAME_PREW)
#define VIDEO_M14_MEM_SIZE_OF_FRAME_GRAP    CONV_MEM_ROW2BYTE(VIDEO_M14_ROW_SIZE_OF_FRAME_GRAP)
#define VIDEO_M14_MEM_SIZE_OF_FRAME_GCSC    CONV_MEM_ROW2BYTE(VIDEO_M14_ROW_SIZE_OF_FRAME_GCSC)

#define VIDEO_M14_FIRMWARE_ROW_OFST_IPC_MC	(0)
#define VIDEO_M14_FIRMWARE_ROW_OFST_DDR_UP	(VIDEO_M14_FIRMWARE_ROW_OFST_IPC_MC + VIDEO_M14_FIRMWARE_ROW_SIZE_IPC_MC)
#define VIDEO_M14_FIRMWARE_ROW_OFST_OF_TTX	(VIDEO_M14_FIRMWARE_ROW_OFST_DDR_UP + VIDEO_M14_FIRMWARE_ROW_SIZE_DDR_UP)
#define VIDEO_M14_FIRMWARE_ROW_OFST_WEB_OS	(VIDEO_M14_FIRMWARE_ROW_OFST_OF_TTX + VIDEO_M14_FIRMWARE_ROW_SIZE_WEB_OS)
#define VIDEO_M14_FIRMWARE_ROW_OFST_REG_TR	(VIDEO_M14_FIRMWARE_ROW_OFST_WEB_OS + VIDEO_M14_FIRMWARE_ROW_SIZE_OF_TTX)

#ifdef USE_PREW_MEM_ON_LBUS_M14B0
#define VIDEO_M14B_MEM_SIZE_OF_FRAME_PREW   ( 24 * 1024 * 1024 )
#else
#define VIDEO_M14B_MEM_SIZE_OF_FRAME_PREW   ( 0 )
#endif
#define VIDEO_M14B_MEM_SIZE_OF_FRAME_GRAP   ( 0 )
#define VIDEO_M14B_MEM_SIZE_OF_FRAME_DE     ((90 * 1024 * 1024) - VIDEO_M14B_MEM_SIZE_OF_FRAME_PREW)

#ifdef USE_SEPERATED_BE_MEM_M14B0
#define VIDEO_M14B_MEM_SIZE_OF_FRAME_BE     ( 0 )
#else
#define VIDEO_M14B_MEM_SIZE_OF_FRAME_BE     (82 * 1024 * 1024)
#endif
#define VIDEO_M14B_MEM_SIZE_OF_DE_BE        (VIDEO_M14B_MEM_SIZE_OF_FRAME_DE + VIDEO_M14B_MEM_SIZE_OF_FRAME_BE)

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

#endif   /* ----- #ifndef DE_CFG_M14_INC  ----- */
/**  @} */
