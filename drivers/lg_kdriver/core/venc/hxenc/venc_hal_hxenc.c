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
 *  hal api implementation for venc device.
 *	venc device will teach you how to make device driver with new platform.
 *
 *  author		jaeseop.so (jaeseop.so@lge.com)
 *  version		1.0
 *  date		2012.07.23
 *  note		Additional information.
 *
 *  @addtogroup lg1154_venc
 *	@{
 */

/*-----------------------------------------------------------------------------
	Control Constants
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	File Inclusions
-----------------------------------------------------------------------------*/
#include <asm/io.h>
#include "os_util.h"
#include "venc_hal.h"
#include "venc_drv.h"
//#include "venc_kapi.h"
#include "h1encode.h"

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
	global Variables
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Static Function Prototypes Declarations
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Static Variables
-----------------------------------------------------------------------------*/
static VENC_HAL_API_T _VencHalApiH13Bx;

/*========================================================================================
	Implementation Group
========================================================================================*/
VENC_HAL_API_T *VENC_HAL_InitializeH13Bx( void )
{
	return &_VencHalApiH13Bx;
}

static int _VENC_DeviceInitialize ( void )
{
	H1EncodeInitialize();

	return RET_OK;
}

static int _VENC_DeviceFinalize ( void )
{
	H1EncodeFinalize();

	return RET_OK;
}

static int _VENC_Initialize( void )
{
	return RET_OK;
}

static int _VENC_Open( int channel )
{
	return H1EncodeOpen( channel );	
}

static int _VENC_Close( int channel )
{
	return H1EncodeClose( channel );
}

static int _VENC_SetInputConfig ( LX_VENC_RECORD_INPUT_T *pstParams )
{
	int ret = RET_ERROR;
	
	if ( pstParams == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	// if ui32FrameRate is zero, it means framerate auto-dectected mode.
	//if ( pstParams->ui32FrameRate > 0 )
	{
		ret = H1EncodeSetFramerate( pstParams->ui32FrameRate );
		if ( ret != RET_OK )	return ret;
	}

	if ( pstParams->eInputSource > 0 )
	{
		ret = H1EncodeSetInputSource( pstParams->eInputSource );
		if ( ret != RET_OK )	return ret;
	}

	if ( pstParams->ui16Width > 0 && pstParams->ui16Height > 0 )
	{
		ret = H1EncodeSetResolution( pstParams->ui16Width, pstParams->ui16Height );
		if ( ret != RET_OK )	return ret;
	}

	if ( pstParams->nSARWidth > 0 && pstParams->nSARHeight > 0 )
	{
		ret = H1EncodeSetSAR( pstParams->nSARWidth, pstParams->nSARHeight );
		if ( ret != RET_OK )	return ret;
	}
	
	return ret;
}

static int _VENC_SetCommand ( LX_VENC_RECORD_COMMAND_T eParam )
{
	return H1EncodeCommand( eParam );
}

static int _VENC_SetGOP ( UINT32 u32Param )
{
	return H1EncodeSetGOP( u32Param );
}

static int _VENC_SetRateControl( LX_VENC_RC_TYPE_T eType, UINT32 value )
{
	int ret = RET_ERROR;
	int targetBitrate = 0;
	
	switch( eType )
	{
		case LX_VENC_RC_TYPE_VBR:
			targetBitrate = value * 1000;
			
			ret = H1EncodeSetBitrate( targetBitrate, FALSE );
		break;
		case LX_VENC_RC_TYPE_CBR:
			targetBitrate = value * 1000;
			
			ret = H1EncodeSetBitrate( targetBitrate, TRUE );
		break;

		case LX_VENC_RC_TYPE_FIXED_QP:
			ret = H1EncodeSetQPValue( value );
		break;
	}

	return ret;
}

static int _VENC_SetEncodeType( LX_VENC_ENCODE_TYPE_T eParam )
{
	if ( eParam > LX_VENC_ENCODE_TYPE_MAX )
	{
		return RET_INVALID_PARAMS;
	}

	H1EncodeSetEncodeType( eParam );
	
	return RET_OK;
}

static int _VENC_GetEncodeInfo( LX_VENC_RECORD_INFO_T *pstParams )
{
	u16 width, height;
	u32 nSarWidth, nSarHeight;

	LX_VENC_RECORD_FRAME_RATE_T eFramerate = LX_VENC_FRAME_RATE_NONE;
	LX_VENC_SOURCE_TYPE_T eSourceType = LX_VENC_SOURCE_TYPE_FIELD;
	UINT32	nFramerate = 0;

	if ( pstParams == NULL )
	{
		return RET_INVALID_PARAMS;
	}	

	if ( H1EncodeGetFramerate( &eFramerate, &nFramerate, &eSourceType ) == RET_OK )
	{
		pstParams->ui32FrameRate	= eFramerate;
		pstParams->nFrameRate		= nFramerate;
		pstParams->eSourceType		= eSourceType;
	}

	// Kadp�� ��ȯ�� ���� kbits/s ����
	pstParams->ui32BitRate		= H1EncodeGetBitrate() / 1000;
	
	pstParams->ui8QPLevel		= H1EncodeGetQPValue();
	pstParams->ui8GopN			= H1EncodeGetGOP();
	pstParams->ui8GopM			= 0;
	pstParams->ui8GopIdr		= 0;

	if ( H1EncodeGetResolution( &width, &height ) == RET_OK )
	{
		pstParams->ui16Width = width;
		pstParams->ui16Height = height;
	}

	if ( H1EncodeGetSAR( &nSarWidth, &nSarHeight ) == RET_OK )
	{
		pstParams->nSarWidth = nSarWidth;
		pstParams->nSarHeight = nSarHeight;
	}
	
	return RET_OK;
}

static int _VENC_GetFrameImage( LX_VENC_RECORD_FRAME_IMAGE_T *pstParams )
{
	if ( pstParams == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	H1EncodeGetFrameImage( pstParams );

	return RET_OK;
}

static int _VENC_GetOutputBuffer( LX_VENC_RECORD_OUTPUT_T *pstParams )
{
	if ( pstParams == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	return H1EncodeGetOutputBuffer( pstParams );
}

static int _VENC_CheckIPCRegister( void )
{
	return H1EncodeCheckIPCRegister();
}

static int _VENC_PROC_ReadStatus( char * buffer )
{
	if ( buffer == NULL )
	{
		return 0;
	}

	return H1EncodeReadStatus( buffer );
}

static irqreturn_t _VENC_ISRHandler( void )
{
	return H1EncodeISRHandler();
}

#ifdef SUPPORT_VENC_DEVICE_ENC_API
static int _VENC_ENC_Create(LX_VENC_ENC_CREATE_T *pstParams)
{
	return ENC_Create(pstParams);
}

static int _VENC_ENC_Destroy(LX_VENC_ENC_DESTROY_T *pstParams)
{
	return ENC_Destroy(pstParams);
}

static int _VENC_ENC_Encode(LX_VENC_ENC_ENCODE_T *pstParams)
{
	return ENC_Encode(pstParams);
}
#endif

/*-----------------------------------------------------------------------------
	Static Function Definition
-----------------------------------------------------------------------------*/
static VENC_HAL_API_T _VencHalApiH13Bx = 
{
	.version			= LX_CHIP_REV(H13,B0),

	// Initialize/Finalize functions
	.DeviceInitialize 	= _VENC_DeviceInitialize,
	.DeviceFinalize 	= _VENC_DeviceFinalize,
	.Initialize			= _VENC_Initialize,

	.Open				= _VENC_Open,
	.Close				= _VENC_Close,
	
	// Getter functions
	.GetOutputBuffer	= _VENC_GetOutputBuffer,
	.GetEncodeInfo		= _VENC_GetEncodeInfo,
	.GetFrameImage		= _VENC_GetFrameImage,

	// Setter functions
	.SetEncodeType 		= _VENC_SetEncodeType,
	.SetInputConfig 	= _VENC_SetInputConfig,
	.SetCommand 		= _VENC_SetCommand,
	.SetGOP				= _VENC_SetGOP,
	.SetRateControl		= _VENC_SetRateControl,

	.CheckIPCRegister	= _VENC_CheckIPCRegister,

	.PROC_ReadStatus	= _VENC_PROC_ReadStatus,

	.ENC_Create			= _VENC_ENC_Create,
	.ENC_Destroy		= _VENC_ENC_Destroy,
	.ENC_Encode 		= _VENC_ENC_Encode,

	// ISR Handler
	.ISRHandler			= _VENC_ISRHandler,
};

/** @} */


