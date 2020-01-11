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
 *  hal api for venc device.
 *	venc device will teach you how to make device driver with new platform.
 *
 *  @author		jaeseop.so (jaeseop.so@lge.com)
 *  version		1.0
 *  date		2012.07.23
 *  note		Additional information.
 *
 *  @addtogroup lg1154_venc
 *	@{
 */

#ifndef	_VENC_HAL_H_
#define	_VENC_HAL_H_

/*-----------------------------------------------------------------------------
	Control Constants
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    File Inclusions
-----------------------------------------------------------------------------*/
#include <asm/irq.h>

#include "os_util.h"
#include "venc_kapi.h"
#include "venc_cfg.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*-----------------------------------------------------------------------------
	Constant Definitions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Macro Definitions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    Type Definitions
-----------------------------------------------------------------------------*/
typedef struct {
	int version;

// BEGIN of public APIs
	int (*DeviceInitialize) ( void );
	int (*DeviceFinalize) 	( void );
	int (*Initialize)		( void );

	int (*Suspend)			( void );
	int (*Resume)			( void );

	int (*Open)				( int );
	int (*Close)			( int );
	int (*GetBufferInfo)	( LX_VENC_BUFFER_T *pstParams );

	// Recording
	// Setter functions
	int (*SetEncodeType)	( LX_VENC_ENCODE_TYPE_T eParam );
	int (*SetInputConfig)	( LX_VENC_RECORD_INPUT_T *pstParams );
	int (*SetCommand)		( LX_VENC_RECORD_COMMAND_T eParam );
	int (*SetGOP)			( UINT32 ui32Param );
	int (*SetRateControl)	( LX_VENC_RC_TYPE_T eType, UINT32 value );
	int (*GetEncodeInfo)	( LX_VENC_RECORD_INFO_T *pstParams );
	int (*GetFrameImage)	( LX_VENC_RECORD_FRAME_IMAGE_T *pstParams );
	int (*GetOutputBuffer)	( LX_VENC_RECORD_OUTPUT_T *pstParams );
	int (*CheckIPCRegister)	( void );
// END of public APIs

#ifdef SUPPORT_VENC_DEVICE_ENC_API
	// To support API for encoding
	int (*ENC_Create)		( LX_VENC_ENC_CREATE_T *	pstParams );
	int (*ENC_Destroy)		( LX_VENC_ENC_DESTROY_T *	pstParams );
	int (*ENC_Encode)	 	( LX_VENC_ENC_ENCODE_T *	pstParams );
#endif

	int (*PROC_ReadStatus)	( char *buffer );

// BEGIN of ISRs
	irqreturn_t (*ISRHandler)( void );
// END of ISRs

} VENC_HAL_API_T;

/*-----------------------------------------------------------------------------
	External Variables
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Generic Usage Functions
-----------------------------------------------------------------------------*/

int VENC_HAL_DeviceInitialize ( void );
int VENC_HAL_DeviceFinalize ( void );
int VENC_HAL_Initialize( void );

int VENC_HAL_Suspend( void );
int VENC_HAL_Resume( void );

int VENC_HAL_Open( int );
int VENC_HAL_Close( int );
int VENC_HAL_GetBufferInfo ( LX_VENC_BUFFER_T *pstParams );

// FOR Recoding
int VENC_HAL_SetEncodeType( LX_VENC_ENCODE_TYPE_T eParam );
int VENC_HAL_SetInputConfig ( LX_VENC_RECORD_INPUT_T *pstParams );
int VENC_HAL_SetCommand ( LX_VENC_RECORD_COMMAND_T eParam );
int VENC_HAL_SetGOP ( UINT32 ui32Param );
int VENC_HAL_SetRateControl( LX_VENC_RECORD_RC_T *pstParams );
int VENC_HAL_GetEncodeInfo( LX_VENC_RECORD_INFO_T *pstParams );
int VENC_HAL_GetFrameImage( LX_VENC_RECORD_FRAME_IMAGE_T *pstParams );
int VENC_HAL_GetOutputBuffer( LX_VENC_RECORD_OUTPUT_T *pstParams );
int VENC_HAL_CheckIPCRegister( void );

irqreturn_t VENC_HAL_ISRHandler( void );

#ifdef SUPPORT_VENC_DEVICE_ENC_API
int VENC_HAL_ENC_Create	( LX_VENC_ENC_CREATE_T *	pstParams );
int VENC_HAL_ENC_Destroy( LX_VENC_ENC_DESTROY_T *	pstParams );
int VENC_HAL_ENC_Encode	( LX_VENC_ENC_ENCODE_T *	pstParams );
#endif

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _VENC_HAL_H_ */

/** @} */

