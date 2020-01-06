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
 *  core driver interface header for ci device. ( used only within kdriver )
 *
 *  @author		Srinivasan Shanmugam	(srinivasan.shanmugam@lge.com)
 *  @author		Hwajeong Lee (hwajeong.lee@lge.com)
 *  @author		Jinhwan Bae (jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010.2.22
 *
 *  @addtogroup lg1150_ci
 *	@{
 */

#ifndef	_CI_COREDRV_H_
#define	_CI_COREDRV_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "ci_kapi.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/
typedef enum
{
	PCMCIA_BUS_SPEED_MIN = 0x0,
	PCMCIA_BUS_SPEED_LOW,
	PCMCIA_BUS_SPEED_HIGH,
	PCMCIA_BUS_SPEED_MAX
} CI_BUS_SPEED_T;

typedef enum
{
	ACCESS_1BYTE_MODE = 0,
	ACCESS_2BYTE_MODE = 1,
	ACCESS_4BYTE_MODE = 2
}CI_ACCESS_MODE_T;


/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
void CI_InitSMC(void);
void CI_UnmapSMC(void);
void CI_DefaultInit(void);

SINT32 CI_Initialize( void );

SINT32 CI_UnInitialize( void );

SINT32 CI_ResetCI( void );

SINT32 CI_CAMInit( void );

SINT32 CI_CAMPowerOff( void );

SINT32 CI_CAMPowerOnCompleted( void );

SINT32 CI_CheckCIS( void );

SINT32 CI_WriteCOR( void );

SINT32 CI_CAMDetectStatus( UINT32 *o_puwIsCardDetected );

SINT32 CI_ReadData ( UINT8 *o_pbyData, UINT16 *io_pwDataBufSize );

SINT32 CI_NegoBuff( UINT32 *o_puwBufSize );

SINT32 CI_ReadDAStatus( UINT32 *o_puwIsDataAvailable );

SINT32 CI_WriteData ( UINT8 *i_pbyData, UINT32 i_wDataBufSize );

SINT32 CI_ResetPhysicalIntrf( void );

SINT32 CI_SetRS(void );

SINT32 CI_ReadIIRStatus( UINT32 *o_puwIIRStatus );

SINT32 CI_CheckCAMType( SINT8 *o_pRtnValue, UINT8 *o_puwCheckCAMType );

SINT32 CI_GetCIPlusSupportVersion( SINT8 *o_pRtnValue, UINT32 *o_puwVersion );

SINT32 CI_GetCIPlusOperatorProfile( SINT8 *o_pRtnValue, UINT32 *o_puwProfile);

SINT32 CI_RegPrint( void );

SINT32 CI_RegWrite( UINT32 ui32Offset, UINT32 ui32Value );

SINT32 CI_CAMSetDelay( CI_DELAY_TYPE_T eDelayType, UINT32 uiDelayValue );

SINT32 CI_CAMPrintDelayValues( void );

SINT32 CI_SetPCCardBusTiming( CI_BUS_SPEED_T speed );

SINT32 CI_ChangeAccessMode( CI_ACCESS_MODE_T mode );

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _CI_COREDRV_H_ */

/** @} */
