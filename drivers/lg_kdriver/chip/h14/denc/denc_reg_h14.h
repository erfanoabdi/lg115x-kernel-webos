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
 * author     jaemo.kim (jaemo.kim@lge.com), justine.jeong@lge.com
 * version    1.0
 * date       2012.03.14
 * note       Additional information.
 *
 * @addtogroup lg1152_de
 * @{
 */

#ifndef  DENC_REG_H14_INC
#define  DENC_REG_H14_INC

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/

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
int DENC_REG_H14_Create(void);
int DENC_REG_H14_Destory(void);
int DENC_REG_H14_Init(void);
int DENC_REG_H14_OnOff ( BOOLEAN *pOnOff );
int DENC_REG_H14_SetOutPutVideoStd(LX_DENC_VIDEO_SYSTEM_T *pstParams);
int DENC_REG_H14_SetSource(LX_DENC_VIDEO_SOURCE_T *pstParams);
int DENC_REG_H14_TtxEnable(BOOLEAN *pstParams);
int DENC_REG_H14_WssEnable(BOOLEAN *pstParams);
int DENC_REG_H14_VpsEnable(BOOLEAN *pstParams);
int DENC_REG_H14_SetTtxData(LX_DENC_TTX_DATA_T *pstParams);
int DENC_REG_H14_SetWssData(LX_DENC_WSS_DATA_T *pstParams);
int DENC_REG_H14_SetVpsData(LX_DENC_VPS_DATA_T *pstParams);
int DENC_REG_H14_ColorBarEnable(BOOLEAN *pstParams);
int DENC_REG_H14_VdacPowerControl(BOOLEAN *pstParams);

int DENC_REG_H14_NTSC_Init(void);

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/

#endif   /* ----- #ifndef DENC_REG_H14_INC  ----- */
/**  @} */
