/* *****************************************************************************
 * SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2013 by LG Electronics Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * ****************************************************************************/

/** @file
 *
 * video decoding unit driver implementation for VDEC device.
 * VDEC device will teach you how to make device driver with lg1154 platform.
 *
 * @author     Youngwoo Jin(youngwoo.jin@lge.com)
 * @version    1.0
 * @date       2013.01.06
 * @note       Additional information.
 *
 * @addtogroup lg1154_vdec
 * @{
 */

/*------------------------------------------------------------------------------
	Control Constants
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	File Inclusions
------------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include "vdu_drv.h"
#include "vdu_dec.h"
#include "vdu_debug.h"
#include "../../log.h"
#include "../../hal/vdec_base.h"
#include "../../hal/vdc_hal_api.h"

/*------------------------------------------------------------------------------
	Constant Definitions
------------------------------------------------------------------------------*/
#define NUMBER_OF_SEARCHING_I				(30)
#define WAIT_TIME_OUT_VALUE					(500)
#define USERDATA_BUFFER_SIZE				(0x0400 * (VDU_DEC_NUM_OF_MAX_DPB))

/*------------------------------------------------------------------------------
	Macro Definitions
------------------------------------------------------------------------------*/
#define _MemAlloc(_size, _align, _name)		({									\
	vdec_alloc((_size), (_align), (_name));										\
})
#define _MemFree(_addr)						({									\
	if( (_addr) != VDU_INVALID_ADDRESS ) {										\
		vdec_free((_addr));														\
		(_addr) = VDU_INVALID_ADDRESS;											\
	}																			\
})
#define _MemMap(_addr, _size)				({									\
	(UINT32)vdec_remap((_addr), (_size));										\
})
#define _MemUnmap(_addr)					({									\
	if( (_addr) != VDU_INVALID_ADDRESS ) {										\
		vdec_unmap((void*)(_addr));												\
		(_addr) = VDU_INVALID_ADDRESS;											\
	}																			\
})
#define _Malloc(_size)						({									\
	kmalloc((_size), (in_interrupt())? GFP_ATOMIC : GFP_KERNEL);				\
})
#define _MFree(_addr)						({									\
	if( (_addr) != NULL ) {														\
		kfree(_addr);															\
		(_addr) = NULL;															\
	}																			\
})
#define _LockInstance(_inst, _flags)		({									\
	spin_lock_irqsave(&((_inst)->stLock), *(_flags));							\
})
#define _UnlockInstance(_inst, _flags)		({									\
	spin_unlock_irqrestore(&((_inst)->stLock), *(_flags));						\
})

/*------------------------------------------------------------------------------
	Type Definitions
------------------------------------------------------------------------------*/
typedef struct tasklet_struct		TASKLET_T;

typedef struct
{
	BOOLEAN				bTopFieldFirst;
	BOOLEAN				bRepeatFirstField;
	VDU_FRM_SCAN_T		eFrameScanType;
} AVC_PIC_STRUCT_T;

typedef enum
{
	CORE_NUM_CNM			= 0,
	CORE_NUM_HEVC			= 1,
	CORE_NUM_GX				= 2,

	CORE_NUM_MIN			= CORE_NUM_CNM,
	CORE_NUM_MAX			= CORE_NUM_GX,
	CORE_NUM_INVALID
} CORE_NUM_T;

typedef struct
{
	const char*				pchName;
	VDU_DEC_OPERATIONS_T*	pstOperations;
} CORE_INFO_T;

typedef enum
{
	CALLBACK_STATE_IDLE		= 0,
	CALLBACK_STATE_RUNNING	= 1,
	CALLBACK_STATE_PENDING	= 2,

	CALLBACK_STATE_MIN		= CALLBACK_STATE_IDLE,
	CALLBACK_STATE_MAX		= CALLBACK_STATE_PENDING,
	CALLBACK_STATE_INVALID
} CALLBACK_STATE_T;

typedef struct CALLBACK_UNIT_T {
	VDU_DEC_RESULT_T		stDecResult;
	struct CALLBACK_UNIT_T*	pstNext;
} CALLBACK_UNIT_T;

typedef struct
{
	spinlock_t				stLock;

	BOOLEAN					bInUsed;
	BOOLEAN					bRunning;
	BOOLEAN					bFlushing;

	VDU_PIC_SCAN_T			ePicScanMode;
	BOOLEAN					bEnableUserData;

	UINT8					aui8ClearFrames[VDU_DEC_NUM_OF_MAX_DPB];

	UINT8					ui8DecRetryCount;

	struct {
		UINT32					ui32ResetAddr;
		UINT32					ui32WriteAddr;
		BOOLEAN					ui32StreamEndAddr;
	}						stFeedInfo;

	struct {
		UINT16					ui16NumberOfSearch;
		SINT8					si8FirstFrameIndex;
		UINT32					ui32GarbageFramesBit;
		UINT16					ui16ValidThreshold;
	}						stValidFrameInfo;

	struct {
		SINT32					si32FramePackArrange;
		UINT8					ui8PicTimingStruct;
	}						stPreDisplayInfo;

	struct {
		UINT8					ui8NumberOfBuffers;
		UINT32					ui32Stride;
		UINT32					ui32Height;
		UINT32					aui32AddressList[VDU_DEC_NUM_OF_MAX_DPB];
	}						stExternBuffer;

	struct {
		TASKLET_T				stTasklet;
		BOOLEAN					bRunnable;
		VDU_DEC_RESULT_T		stDecResult;
		CALLBACK_UNIT_T*		pstList;
	}						stCallbackInfo;

	VDU_RESULT_T			stNotiInfo;

	VDU_DEC_INSTANCE_T*		pstDecInfo;
	VDU_DEC_OPERATIONS_T*	pstOperations;

	CORE_INFO_T*			pstCore;
} INSTANCE_T;

/*------------------------------------------------------------------------------
	External Function Prototype Declarations
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	External Variables
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	global Functions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Static Function Prototypes Declarations
------------------------------------------------------------------------------*/
static INSTANCE_T*			_GetInstance(VDU_HANDLE_T hInstance, ULONG* pulFlags);
static INSTANCE_T*			_WaitDone(VDU_HANDLE_T hInstance, ULONG* pulFlags);

// Checking invalid frames functions
static BOOLEAN				_InitCheckingInvalidFrame(INSTANCE_T* pstInstance, UINT16 ui16Threshold);
static BOOLEAN				_CheckInvalidFrame(INSTANCE_T* pstInstance);
static BOOLEAN				_DidCheckValidIFrame(INSTANCE_T* pstInstance);
static BOOLEAN				_DidFinishCheckInvalidFrame(INSTANCE_T* pstInstance);
static void					_FinishCheckInvalidFrame(INSTANCE_T* pstInstance);

// Callback functions
static void					_RunPostCallback(INSTANCE_T* pstInstance, ULONG* pulFlags);
static void					_CallbackTasklet(ULONG _ulData);
static void					_Callback(VDU_DEC_INSTANCE_T* pstDecInfo, VDU_DEC_RESULT_T* pstDecResult);

// Frame informaiton set functions
static BOOLEAN				_SetDisplayFrameInfo(VDU_FRM_INFO_T* pstFrameInfo, INSTANCE_T* pstInstance);
static BOOLEAN				_SetSizeInfo(VDU_FRM_INFO_T* pstFrameInfo,
											VDU_DEC_FRAME_INFO_T* pstPictureInfo,
											VDU_CODEC_T eCodecType,
											BOOLEAN bInterlacedSequence);
static BOOLEAN				_SetAspectRatio(VDU_FRM_INFO_T* pstFrameInfo, VDU_DEC_FRAME_INFO_T* pstPictureInfo, VDU_CODEC_T eCodecType);
static BOOLEAN				_SetFrameRate(VDU_FRM_INFO_T* pstFrameInfo, VDU_DEC_FRAME_INFO_T* pstPictureInfo, VDU_CODEC_T eCodecType);
static BOOLEAN				_SetScanTypeAndPeriod(VDU_FRM_INFO_T* pstFrameInfo,
													VDU_DEC_FRAME_INFO_T* pstPictureInfo,
													VDU_CODEC_T eCodecType,
													BOOLEAN bInterlacedSequence,
													UINT8* pui8PrePicTimingStruct);
static BOOLEAN				_SetFramePackArrange(VDU_FRM_INFO_T* pstFrameInfo,
													VDU_DEC_FRAME_INFO_T* pstPictureInfo,
													VDU_CODEC_T eCodecType,
													SINT32* psi32PreFramePackArrange);
static BOOLEAN				_SetActiveFormatDescription(VDU_FRM_INFO_T* pstFrameInfo, VDU_DEC_FRAME_INFO_T* pstPictureInfo, VDU_CODEC_T eCodecType);

// Utilities functions
static UINT8				_GetDisplayAspectRatio(UINT32 ui32DisplayWidth, UINT32 ui32DisplayHeight);
static UINT32				_GetGcd(UINT32 ui32Bigger, UINT32 ui32Smaller);

/*------------------------------------------------------------------------------
	global Variables
------------------------------------------------------------------------------*/
logm_define(vdec_vdu, log_level_warning);

/*------------------------------------------------------------------------------
	Static Variables
------------------------------------------------------------------------------*/
static VDU_CALLBACK_FN_T	_gpfnCallback = NULL;
static CORE_INFO_T			_gastCoreInfo[VDEC_NUM_OF_CORES];
static INSTANCE_T			_gastInstancePool[VDU_MAX_NUM_OF_INSTANCES];
static VDU_DEC_BUFFER_T		_gastUserdataBufferPool[VDU_MAX_NUM_OF_INSTANCES];

static int					_giForceLinear = 0;

module_param_named(vdu_force_linear, _giForceLinear, int, 0644);

/*==============================================================================
    Implementation Group
==============================================================================*/
/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T 		VDU_Init(VDU_CALLBACK_FN_T pfnCallback)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	UINT8			ui8Index = 0;

	VDC_HAL_Init();

	_gpfnCallback = pfnCallback;

	for ( ui8Index = 0; ui8Index < VDEC_NUM_OF_CORES; ++ui8Index ) {
			switch (ui8Index)
			{
					case CORE_NUM_CNM:
							_gastCoreInfo[ui8Index].pchName = "cnm";
							break;
					case CORE_NUM_HEVC:
							_gastCoreInfo[ui8Index].pchName = "hevc";
							break;
					case CORE_NUM_GX:
							_gastCoreInfo[ui8Index].pchName = "gx";
							break;
					default:
							logm_error(vdec_vdu,"error? \n");
							break;
			}
			_gastCoreInfo[ui8Index].pstOperations = NULL;
	}

	for ( ui8Index = 0; ui8Index < VDU_MAX_NUM_OF_INSTANCES; ++ui8Index ) {
		spin_lock_init(&_gastInstancePool[ui8Index].stLock);
		_gastInstancePool[ui8Index].bInUsed = FALSE;
		_gastInstancePool[ui8Index].bRunning = FALSE;
		_gastInstancePool[ui8Index].pstOperations = NULL;
		_gastInstancePool[ui8Index].pstCore = NULL;

		_gastUserdataBufferPool[ui8Index].ui32PhysAddr = (UINT32)_MemAlloc(USERDATA_BUFFER_SIZE, 1 << 12, "vpu_userdata");
		if ( _gastUserdataBufferPool[ui8Index].ui32PhysAddr == 0x00 ) {
			goto GOTO_END;
		}

		_gastUserdataBufferPool[ui8Index].ui32VirtAddr = (UINT32)_MemMap(_gastUserdataBufferPool[ui8Index].ui32PhysAddr, USERDATA_BUFFER_SIZE);
		if ( _gastUserdataBufferPool[ui8Index].ui32VirtAddr == 0x00 ) {
			goto GOTO_END;
		}

		_gastUserdataBufferPool[ui8Index].ui32Size = USERDATA_BUFFER_SIZE;
	}

	VDU_DBG_Init();

	eRetVal = VDU_RET_OK;

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_Suspend(void)
{
	VDU_RET_T		eRetVal = VDU_RET_OK;
	UINT8			ui8Core = 0;

	for ( ui8Core = 0; ui8Core < VDEC_NUM_OF_CORES; ++ui8Core ) {
		if ( _gastCoreInfo[ui8Core].pstOperations == NULL ) {
			continue;
		} else if( _gastCoreInfo[ui8Core].pstOperations->pfnSuspend == NULL ) {
			logm_error(vdec_vdu, "There is no function registered for core %d\n", ui8Core);
			continue;
		}

		// TODO: Cpb reset;

		if ( _gastCoreInfo[ui8Core].pstOperations->pfnSuspend() != VDU_RET_OK ) {
			eRetVal = VDU_RET_ERROR;
		}
	}

	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_Resume(void)
{
	VDU_RET_T		eRetVal = VDU_RET_OK;
	UINT8			ui8Core = 0;

	for ( ui8Core = 0; ui8Core < VDEC_NUM_OF_CORES; ++ui8Core ) {
		if ( _gastCoreInfo[ui8Core].pstOperations == NULL ) {
			continue;
		} else if( _gastCoreInfo[ui8Core].pstOperations->pfnResume == NULL ) {
			logm_error(vdec_vdu, "There is no function registered for core %d\n", ui8Core);
			continue;
		}

		// TODO: Cpb reset;

		if ( _gastCoreInfo[ui8Core].pstOperations->pfnResume() != VDU_RET_OK ) {
			eRetVal = VDU_RET_ERROR;
		}
	}

	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_Open(VDU_HANDLE_T *phInstance, VDU_OPEN_PARAM_T *pstOpenParam)
{
	VDU_RET_T			eRetVal = VDU_RET_ERROR;
	INSTANCE_T*			pstInstance = NULL;
	VDU_DEC_INSTANCE_T*	pstDecInfo = NULL;
	VDU_DEC_RUN_PARAM_T	stRunParam = { 0, };
	ULONG				ulFlags = 0x00;

	if ( _giForceLinear != 0 ) {
		pstOpenParam->eFrameMapType = VDU_FRM_MAP_LINEAR;
		pstOpenParam->eExternLinearBufMode = VDU_LINEAR_MODE_NONE;
	}

	for ( *phInstance = 0; *phInstance < VDU_MAX_NUM_OF_INSTANCES; ++*phInstance ) {
		_LockInstance(&_gastInstancePool[(UINT32)*phInstance], &ulFlags);
		if ( _gastInstancePool[(UINT32)*phInstance].bInUsed == FALSE ) {
			pstInstance = &_gastInstancePool[(UINT32)*phInstance];
			break;
		}
		_UnlockInstance(&_gastInstancePool[(UINT32)*phInstance], &ulFlags);
	}

	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	VDU_DBG_Open(*phInstance, pstOpenParam);

	if ( pstOpenParam->eFrameMapType != VDU_FRM_MAP_LINEAR ) {
		pstOpenParam->eExternLinearBufMode = VDU_LINEAR_MODE_NONE;
	}

	pstDecInfo = (VDU_DEC_INSTANCE_T*)_Malloc(sizeof(VDU_DEC_INSTANCE_T));
	if ( pstDecInfo == NULL ) {
		goto GOTO_ERROR;
	}

	pstInstance->bInUsed = TRUE;
	pstInstance->bRunning = FALSE;
	pstInstance->bFlushing = FALSE;
	pstInstance->ePicScanMode = (pstOpenParam->bOneFrameDecoding == TRUE)?
									VDU_PIC_SCAN_I : VDU_PIC_SCAN_ALL;
	pstInstance->bEnableUserData = FALSE;
	memset(pstInstance->aui8ClearFrames, 0, sizeof(pstInstance->aui8ClearFrames));
	pstInstance->ui8DecRetryCount= 0;
	pstInstance->stFeedInfo.ui32ResetAddr = pstOpenParam->ui32CPBAddr;
	pstInstance->stFeedInfo.ui32WriteAddr = pstOpenParam->ui32CPBAddr;
	pstInstance->stFeedInfo.ui32StreamEndAddr = VDU_INVALID_ADDRESS;
	pstInstance->stPreDisplayInfo.si32FramePackArrange = -1;
	pstInstance->stPreDisplayInfo.ui8PicTimingStruct = 9;
	pstInstance->stExternBuffer.ui8NumberOfBuffers = 0;
	pstInstance->stExternBuffer.ui32Stride = 0;
	pstInstance->stExternBuffer.ui32Height = 0;

	pstInstance->pstDecInfo = pstDecInfo;
	pstInstance->pstDecInfo->hVduHandle = *phInstance;
	pstInstance->pstDecInfo->eCodecType = pstOpenParam->eCodecType;
	pstInstance->pstDecInfo->eFrameMapType = pstOpenParam->eFrameMapType;;
	pstInstance->pstDecInfo->ui32CpbBaseAddr = pstOpenParam->ui32CPBAddr;
	pstInstance->pstDecInfo->ui32CpbEndAddr = pstOpenParam->ui32CPBAddr + pstOpenParam->ui32CPBSize;
	pstInstance->pstDecInfo->pstUserdataBuf = &_gastUserdataBufferPool[*phInstance];
	pstInstance->pstDecInfo->bForceUhdEnabled = pstOpenParam->bForceUhdEnable;
	pstInstance->pstDecInfo->bNoDelayMode = pstOpenParam->bNoDelayMode;
	pstInstance->pstDecInfo->bOneFrameDecoding = pstOpenParam->bOneFrameDecoding;
	pstInstance->pstDecInfo->eExternLinearBufMode = pstOpenParam->eExternLinearBufMode;
	pstInstance->pstDecInfo->pPrivateData = NULL;

	tasklet_init(&pstInstance->stCallbackInfo.stTasklet, _CallbackTasklet,	(unsigned long)pstInstance);
	pstInstance->stCallbackInfo.bRunnable = FALSE;
	pstInstance->stCallbackInfo.pstList = NULL;

	_InitCheckingInvalidFrame(pstInstance, pstOpenParam->ui16ValidThreshold);

	switch ( pstOpenParam->eCodecType ) {
	case VDU_CODEC_MPEG2:
	case VDU_CODEC_MPEG4:
	case VDU_CODEC_H263:
	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
	case VDU_CODEC_H264_AVC:
	case VDU_CODEC_H264_MVC:
	case VDU_CODEC_VC1_RCV_V1:
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
	case VDU_CODEC_RVX:
	case VDU_CODEC_AVS:
	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
		pstInstance->pstOperations = _gastCoreInfo[CORE_NUM_CNM].pstOperations;
		pstInstance->pstCore = &_gastCoreInfo[CORE_NUM_CNM];
		break;
	case VDU_CODEC_HEVC:
		if( VDEC_NUM_OF_CORES > CORE_NUM_HEVC )
		{
				pstInstance->pstOperations = _gastCoreInfo[CORE_NUM_HEVC].pstOperations;
				pstInstance->pstCore = &_gastCoreInfo[CORE_NUM_HEVC];
		}
		break;
	case VDU_CODEC_VP9:
		if( VDEC_NUM_OF_CORES > CORE_NUM_GX )
		{
#if 0
				pstInstance->pstOperations = _gastCoreInfo[CORE_NUM_GX].pstOperations;
				pstInstance->pstCore = &_gastCoreInfo[CORE_NUM_GX];
#endif
		}
		break;
	default:
		goto GOTO_ERROR;
		break;
	}


	if ( pstInstance->pstOperations == NULL ) {
		logm_error(vdec_vdu, "[%d]There is no core registered\n", (UINT32)*phInstance);
		goto GOTO_ERROR;
	}
	_UnlockInstance(pstInstance, &ulFlags);

	stRunParam.eCommand = VDU_DEC_CMD_OPEN;
	eRetVal = pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);

	_LockInstance(pstInstance, &ulFlags);

GOTO_ERROR:
	if ( eRetVal != VDU_RET_OK ) {
		pstInstance->bInUsed = FALSE;
		VDU_DBG_Close(*phInstance);
	}

	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	if ( eRetVal != VDU_RET_OK ) {
		_MFree(pstDecInfo);
	}

	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_Close(VDU_HANDLE_T hInstance)
{
	VDU_RET_T			eRetVal = VDU_RET_ERROR;
	INSTANCE_T*			pstInstance = NULL;
	VDU_DEC_INSTANCE_T*	pstDecInfo = NULL;
	VDU_DEC_RUN_PARAM_T	stRunParam = { 0, };
	ULONG				ulFlags = 0x00;

	pstInstance = _WaitDone(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		logm_error(vdec_vdu, "[%d]Fail to wait\n", (UINT32)hInstance);

		pstInstance = _GetInstance(hInstance, &ulFlags);
		if ( pstInstance == NULL ) {
			logm_error(vdec_vdu, "[%d]Invalid instance\n", (UINT32)hInstance);
			goto GOTO_END;
		}

		while ( pstInstance->stCallbackInfo.pstList != NULL ) {
			CALLBACK_UNIT_T*	pstCallbackUnit = pstInstance->stCallbackInfo.pstList;

			pstInstance->stCallbackInfo.pstList = pstCallbackUnit->pstNext;
			_MFree(pstCallbackUnit);
		}
	}
	_UnlockInstance(pstInstance, &ulFlags);

	stRunParam.eCommand = VDU_DEC_CMD_CLOSE;
	eRetVal = pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);

	VDU_DBG_Close(hInstance);

	_LockInstance(pstInstance, &ulFlags);
	pstInstance->bInUsed = FALSE;
	pstDecInfo = pstInstance->pstDecInfo;
	_UnlockInstance(pstInstance, &ulFlags);

	_MFree(pstDecInfo);

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_Flush(VDU_HANDLE_T hInstance)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	ULONG			ulFlags = 0x00;

	pstInstance = _WaitDone(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		logm_error(vdec_vdu, "[%d]Fail to wait\n", (UINT32)hInstance);

		pstInstance = _GetInstance(hInstance, &ulFlags);
		if ( pstInstance == NULL ) {
			logm_error(vdec_vdu, "[%d]Invalid instance\n", (UINT32)hInstance);
			goto GOTO_END;
		}
	}

	pstInstance->stFeedInfo.ui32ResetAddr = pstInstance->stFeedInfo.ui32WriteAddr;
	pstInstance->stFeedInfo.ui32StreamEndAddr = VDU_INVALID_ADDRESS;
	_UnlockInstance(pstInstance, &ulFlags);

	eRetVal = VDU_RET_OK;

	logm_noti(vdec_vdu, "[%d]Flush 0x%08X\n", (UINT32)hInstance, pstInstance->stFeedInfo.ui32ResetAddr);

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_Reset(VDU_HANDLE_T hInstance)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	ULONG			ulFlags = 0x00;
	UINT8			ui8InstIndex = 0;
	UINT8			ui8Index = 0;

	ui8InstIndex = (hInstance == VDU_INVALID_HANDLE)?
					VDU_MAX_NUM_OF_INSTANCES : (UINT8)hInstance;
	ui8Index = (hInstance == VDU_INVALID_HANDLE)?
					0 : (UINT8)hInstance;

	do {
		pstInstance = _GetInstance((VDU_HANDLE_T)ui8Index, &ulFlags);
		if ( pstInstance != NULL ) {
			if ( pstInstance->bRunning == TRUE ) {
				VDU_DEC_RUN_PARAM_T		stRunParam = { 0, };

				pstInstance->bFlushing = TRUE;
				_UnlockInstance(pstInstance, &ulFlags);

				stRunParam.eCommand = VDU_DEC_CMD_RESET;
				eRetVal = pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);

				_LockInstance(pstInstance, &ulFlags);
				pstInstance->bRunning = FALSE;
				pstInstance->bFlushing = FALSE;
				pstInstance->stFeedInfo.ui32ResetAddr = pstInstance->stFeedInfo.ui32WriteAddr;
			}
			_UnlockInstance(pstInstance, &ulFlags);
		}

		++ui8Index;
	} while ( ui8Index < ui8InstIndex);

	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_StartDecoding(VDU_HANDLE_T hInstance, VDU_LINEAR_FRAMES_T* pstLinearFrames)
{
	VDU_RET_T				eRetVal = VDU_RET_ERROR;
	INSTANCE_T*				pstInstance = NULL;
	VDU_DEC_RUN_PARAM_T		stRunParam = { 0, };
	UINT32*					pui32AddressList = NULL;
	ULONG					ulFlags = 0x00;

	pstInstance = _GetInstance(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	if ( pstInstance->bFlushing == TRUE ) {
		logm_info(vdec_vdu, "[%d]Flushed\n", (UINT32)hInstance);
		goto GOTO_ERROR;
	}

	if ( pstInstance->bRunning == TRUE ) {
		logm_noti(vdec_vdu, "[%d]Duplicated to start decoding\n", (UINT32)hInstance);
		goto GOTO_ERROR;
	}

	if ( pstInstance->pstDecInfo->eExternLinearBufMode != VDU_LINEAR_MODE_NONE ) {
		if ( pstLinearFrames != NULL ) {
			if ( (pstInstance->stExternBuffer.ui32Stride != pstLinearFrames->ui32Stride) ||
				 (pstInstance->stExternBuffer.ui32Height != pstLinearFrames->ui32Height) ) {
				pstInstance->stExternBuffer.ui32Stride = pstLinearFrames->ui32Stride;
				pstInstance->stExternBuffer.ui32Height = 1088;//pstLinearFrames->ui32Height;
			}

			if ( pstLinearFrames->aui32AddressList!= NULL ) {
				memcpy(pstInstance->stExternBuffer.aui32AddressList
						+ pstInstance->stExternBuffer.ui8NumberOfBuffers,
					   pstLinearFrames->aui32AddressList,
					   pstLinearFrames->ui8NumberOfFrames * sizeof(UINT32));

				pstInstance->stExternBuffer.ui8NumberOfBuffers += pstLinearFrames->ui8NumberOfFrames;

				logm_trace(vdec_vdu, "[%d]Input %d extern memories\n",
										(UINT32)hInstance,
										pstLinearFrames->ui8NumberOfFrames);
			}
		}

		if ( pstInstance->stExternBuffer.ui8NumberOfBuffers > 0 ) {
			pui32AddressList = _Malloc(pstInstance->stExternBuffer.ui8NumberOfBuffers * sizeof(UINT32));
			if ( pui32AddressList == NULL ) {
				logm_error(vdec_vdu, "[%d]Memory allocation error\n", (UINT32)hInstance);
				goto GOTO_ERROR;
			}

			stRunParam.stLinearFrames.ui8NumberOfFrames = pstInstance->stExternBuffer.ui8NumberOfBuffers;
			stRunParam.stLinearFrames.ui32Stride = pstInstance->stExternBuffer.ui32Stride;
			stRunParam.stLinearFrames.ui32Height = pstInstance->stExternBuffer.ui32Height;
			stRunParam.stLinearFrames.aui32AddressList = pui32AddressList;
			memcpy(pui32AddressList, pstInstance->stExternBuffer.aui32AddressList,
					stRunParam.stLinearFrames.ui8NumberOfFrames * sizeof(UINT32));

			pstInstance->stExternBuffer.ui8NumberOfBuffers = 0;
		}
	} else {
		stRunParam.stLinearFrames.ui8NumberOfFrames = 0;
		stRunParam.stLinearFrames.aui32AddressList = NULL;
	}
	_UnlockInstance(pstInstance, &ulFlags);

	if ( VDU_DBG_WaitStepDecoding(hInstance) == TRUE ) {
		eRetVal = VDU_RET_OK;
		goto GOTO_END;
	}

	VDU_DBG_CheckDecTime(hInstance, VDU_DBG_START_CHECK);

	_LockInstance(pstInstance, &ulFlags);
	stRunParam.bEnableUserData = pstInstance->bEnableUserData;
	memcpy(stRunParam.aui8ClearFrames, pstInstance->aui8ClearFrames, sizeof(pstInstance->aui8ClearFrames));
	memset(pstInstance->aui8ClearFrames, 0, sizeof(pstInstance->aui8ClearFrames));

	stRunParam.ui32ResetAddr = pstInstance->stFeedInfo.ui32ResetAddr;
	if ( pstInstance->stFeedInfo.ui32ResetAddr != VDU_INVALID_ADDRESS ) {
		logm_noti(vdec_vdu, "[%d]Start with reset 0x%08X\n", (UINT32)hInstance, pstInstance->stFeedInfo.ui32ResetAddr);
		pstInstance->stFeedInfo.ui32ResetAddr = VDU_INVALID_ADDRESS;
		_InitCheckingInvalidFrame(pstInstance, pstInstance->stValidFrameInfo.ui16ValidThreshold);
	}

	stRunParam.ui32StreamEndAddr = pstInstance->stFeedInfo.ui32StreamEndAddr;
	if ( pstInstance->stFeedInfo.ui32StreamEndAddr != VDU_INVALID_ADDRESS ) {
		logm_noti(vdec_vdu, "[%d]End of stream 0x%08X\n", (UINT32)hInstance, pstInstance->stFeedInfo.ui32StreamEndAddr);
	}

	stRunParam.ui32WriteAddr = pstInstance->stFeedInfo.ui32WriteAddr;

	if ( (pstInstance->ePicScanMode != VDU_PIC_SCAN_ALL) ||
		 (_DidFinishCheckInvalidFrame(pstInstance) == TRUE) ) {
		stRunParam.ePicScanMode = pstInstance->ePicScanMode;
		_FinishCheckInvalidFrame(pstInstance);
	} else {
		stRunParam.ePicScanMode = _DidCheckValidIFrame(pstInstance)? VDU_PIC_SCAN_IP : VDU_PIC_SCAN_I;	// Skip B frame after I frame
		logm_info(vdec_vdu, "[%d]Decoded only %s-frames\n", (UINT32)hInstance, (stRunParam.ePicScanMode == VDU_PIC_SCAN_I)? "I" : "IP");
	}

	pstInstance->bRunning = TRUE;
	_UnlockInstance(pstInstance, &ulFlags);

	stRunParam.eCommand = VDU_DEC_CMD_DECODE;
	eRetVal = pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);
	if ( eRetVal == VDU_RET_BUSY ) {
		VDU_DEC_RESULT_T	stDecResult = { 0, };

		stDecResult.bDecSuccess = FALSE;
		stDecResult.si8DecodedIndex = VDU_INVALID_FRAME_INDEX;
		stDecResult.si8DisplayIndex = VDU_INVALID_FRAME_INDEX;
		stDecResult.eNotiType = VDU_NOTI_INVALID;

		pstInstance->ui8DecRetryCount++;

		_Callback(pstInstance->pstDecInfo, &stDecResult);

		eRetVal = VDU_RET_OK;
	} else if ( eRetVal != VDU_RET_OK ) {
		VDU_DBG_CheckDecTime(hInstance, VDU_DBG_POSTPONE_CHECK);
	}

	logm_trace(vdec_vdu, "[%d]Start decoding(%d)\n", (UINT32)hInstance, eRetVal);
	_LockInstance(pstInstance, &ulFlags);
	if ( eRetVal != VDU_RET_OK ) {
		pstInstance->bRunning = FALSE;
	} else {
		pstInstance->stCallbackInfo.bRunnable = TRUE;
		pstInstance->stFeedInfo.ui32StreamEndAddr = stRunParam.ui32StreamEndAddr;
	}

GOTO_ERROR:
	_UnlockInstance(pstInstance, &ulFlags);

	if ( pui32AddressList != NULL ) {
		_MFree(pui32AddressList);
	}

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_FeedAu(VDU_HANDLE_T hInstance, VDU_AU_T* pstAu)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	ULONG			ulFlags = 0x00;

	pstInstance = _GetInstance(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	if ( pstInstance->stFeedInfo.ui32StreamEndAddr != VDU_INVALID_ADDRESS ) {
		logm_warning(vdec_vdu, "[%d]Pre-stream data remained\n", (UINT32)hInstance);
		goto GOTO_ERROR;
	}

	if ( (pstAu->ui32StartAddr < pstInstance->pstDecInfo->ui32CpbBaseAddr) ||
		 (pstInstance->pstDecInfo->ui32CpbEndAddr <= pstAu->ui32StartAddr) ||
		 (pstAu->ui32Size == 0) ) {
		if ( pstAu->bEndOfStream == TRUE ) {
			pstInstance->stFeedInfo.ui32StreamEndAddr = pstInstance->stFeedInfo.ui32WriteAddr;
			eRetVal = VDU_RET_OK;
			logm_noti(vdec_vdu, "[%d]Feed end of stream\n", (UINT32)hInstance);
		} else {
			logm_error(vdec_vdu, "[%d]Invalid AU(0x%08X+0x%08X)\n",
								(UINT32)hInstance, pstAu->ui32StartAddr, pstAu->ui32Size);
		}

		goto GOTO_ERROR;
	}

	if ( pstInstance->stFeedInfo.ui32WriteAddr != pstAu->ui32StartAddr ) {
		pstInstance->stFeedInfo.ui32ResetAddr = pstAu->ui32StartAddr;
	}

	pstInstance->stFeedInfo.ui32WriteAddr = pstAu->ui32StartAddr + pstAu->ui32Size;
	if ( pstInstance->stFeedInfo.ui32WriteAddr >= pstInstance->pstDecInfo->ui32CpbEndAddr ) {
		pstInstance->stFeedInfo.ui32WriteAddr = pstInstance->pstDecInfo->ui32CpbBaseAddr
												+ (pstInstance->stFeedInfo.ui32WriteAddr
													- pstInstance->pstDecInfo->ui32CpbEndAddr);
	}

	logm_trace(vdec_vdu, "[%d]Feed 0x%08X/0x%08X\n",
							(UINT32)hInstance, pstAu->ui32StartAddr, pstAu->ui32Size);

	if ( pstAu->bEndOfStream == TRUE ) {
		pstInstance->stFeedInfo.ui32StreamEndAddr = pstInstance->stFeedInfo.ui32WriteAddr;
		logm_noti(vdec_vdu, "[%d]Feed end of stream\n", (UINT32)hInstance);
	}

	eRetVal = VDU_RET_OK;

GOTO_ERROR:
	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	return eRetVal;
}
/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_SetOption(VDU_HANDLE_T hInstance, VDU_OPTION_T* pstOption)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	ULONG			ulFlags = 0x00;

	pstInstance = _GetInstance(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	if( ( pstOption->eUserData > VDU_USERDATA_MAX ) &&
		( pstOption->ePicScanMode > VDU_PIC_SCAN_MAX ) ) {
		logm_error(vdec_vdu, "[%d]Invalid option parameter : User data(%d), Scan mode(%d)\n",
								(UINT32)hInstance, pstOption->eUserData, pstOption->ePicScanMode);
		goto GOTO_ERROR;
	}

	if( pstOption->eUserData <= VDU_USERDATA_MAX ) {
		pstInstance->bEnableUserData = (pstOption->eUserData == VDU_USERDATA_ENABLE)? TRUE : FALSE;
	}

	if( (pstOption->ePicScanMode <= VDU_PIC_SCAN_MAX) &&
		(pstInstance->pstDecInfo->bOneFrameDecoding == FALSE) ) {
		pstInstance->ePicScanMode = pstOption->ePicScanMode;
	}

	eRetVal = VDU_RET_OK;

	logm_info(vdec_vdu, "[%d]Set option : User data(%d), Scan mode(%d)\n",
							(UINT32)hInstance, pstOption->eUserData, pstOption->ePicScanMode);

GOTO_ERROR:
	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_ClearFrame(VDU_HANDLE_T hInstance, SINT8 si8FrameIdx)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	ULONG			ulFlags = 0x00;

	pstInstance = _GetInstance(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	if ( (si8FrameIdx < 0) || (si8FrameIdx >= VDU_DEC_NUM_OF_MAX_DPB) ) {
		logm_error(vdec_vdu, "[%d]Invalid frame index %d\n", (UINT32)hInstance, si8FrameIdx);
		goto GOTO_ERROR;
	}

	pstInstance->aui8ClearFrames[si8FrameIdx]++;

	eRetVal = VDU_RET_OK;
	logm_info(vdec_vdu, "[%d]Frame %2d will be cleared\n", (UINT32)hInstance, si8FrameIdx);

GOTO_ERROR:
	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_GetStatus(VDU_HANDLE_T hInstance, VDU_STATUS_T *peStatus)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	BOOLEAN			bRunning = FALSE;
	ULONG			ulFlags = 0x00;

	pstInstance = _GetInstance(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	if ( pstInstance->pstOperations->pfnGetStatus == NULL ) {
		logm_error(vdec_vdu, "[%d]There is no function registered\n", (UINT32)hInstance);
		goto GOTO_ERROR;
	}

	_UnlockInstance(pstInstance, &ulFlags);
	eRetVal = pstInstance->pstOperations->pfnGetStatus(peStatus,  &bRunning);
	if ( eRetVal != VDU_RET_OK ) {
		goto GOTO_END;
	}
	_LockInstance(pstInstance, &ulFlags);

GOTO_ERROR:
	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_GetLinearFrame(VDU_HANDLE_T hInstance, SINT8 si8FrameIdx, void* pFrameBuf)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	INSTANCE_T*		pstInstance = NULL;
	ULONG			ulFlags = 0x00;

	pstInstance = _GetInstance(hInstance, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	}

	if ( pstInstance->pstOperations->pfnGetLinearFrame == NULL ) {
		logm_error(vdec_vdu, "[%d]There is no function registered\n", (UINT32)hInstance);
		goto GOTO_ERROR;
	}

	_UnlockInstance(pstInstance, &ulFlags);
	eRetVal = pstInstance->pstOperations->pfnGetLinearFrame(pstInstance->pstDecInfo, si8FrameIdx, pFrameBuf);
	if ( eRetVal != VDU_RET_OK ) {
		goto GOTO_END;
	}
	_LockInstance(pstInstance, &ulFlags);

GOTO_ERROR:
	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	return eRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
VDU_RET_T		VDU_DEC_RegisterOperations(char* pchName, VDU_DEC_OPERATIONS_T* pstOperations)
{
	VDU_RET_T		eRetVal = VDU_RET_ERROR;
	UINT8			ui8Index = 0x00;
	CORE_INFO_T*	pstCore = NULL;

	if ( (pchName == NULL) || (pstOperations == NULL) ) {
		logm_error(vdec_vdu, "Null parameter\n");
		goto GOTO_END;
	}

	for ( ui8Index = 0; ui8Index < VDEC_NUM_OF_CORES; ++ui8Index ) {
		if ( strcmp(_gastCoreInfo[ui8Index].pchName, pchName) == 0 ) {
			pstCore = &_gastCoreInfo[ui8Index];
			break;
		}
	}

	if ( pstCore == NULL ) {
		logm_error(vdec_vdu, "Invalid core %s\n", pchName);
		goto GOTO_END;
	}

	pstCore->pstOperations = pstOperations;

	if ( pstCore->pstOperations->pfnInit == NULL ) {
		logm_error(vdec_vdu, "There is no function registered : %s\n", pchName);
		goto GOTO_END;
	}

	eRetVal = pstCore->pstOperations->pfnInit(_Callback);
	if ( eRetVal != VDU_RET_OK ) {
		goto GOTO_END;
	}

GOTO_END:
	return eRetVal;

}
EXPORT_SYMBOL(VDU_DEC_RegisterOperations);

/*==============================================================================
    Static Implementation Group
==============================================================================*/
/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static INSTANCE_T*			_GetInstance(VDU_HANDLE_T hInstance, ULONG* pulFlags)
{
	INSTANCE_T*		pstRetInstance = NULL;

	if ( hInstance < VDU_MAX_NUM_OF_INSTANCES ) {
		pstRetInstance = &_gastInstancePool[(UINT32)hInstance];
		_LockInstance(pstRetInstance, pulFlags);

		if ( pstRetInstance->bInUsed == FALSE ) {
			_UnlockInstance(pstRetInstance, pulFlags);
			pstRetInstance = NULL;
		}
	}

	return pstRetInstance;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static INSTANCE_T*			_WaitDone(VDU_HANDLE_T hInstance, ULONG* pulFlags)
{
	INSTANCE_T*		pstRetInstance = NULL;
	UINT64			ui64TimeOut = get_jiffies_64() + msecs_to_jiffies(WAIT_TIME_OUT_VALUE);

	logm_noti(vdec_vdu, "[%d]Start of Waiting\n", (UINT32)hInstance);

	do {
		pstRetInstance = _GetInstance(hInstance, pulFlags);
		if ( pstRetInstance == NULL ) {
			break;
		}

		pstRetInstance->bFlushing = TRUE;
		pstRetInstance->stCallbackInfo.bRunnable = TRUE;
		if ( (pstRetInstance->bRunning == FALSE) &&
			 (pstRetInstance->stCallbackInfo.pstList == NULL) ) {
			VDU_STATUS_T 	eStatus = VDU_STATUS_BUSY;
			BOOLEAN			bRunning = FALSE;
			_UnlockInstance(pstRetInstance, pulFlags);

			pstRetInstance->pstOperations->pfnGetStatus(&eStatus, &bRunning);
			_LockInstance(pstRetInstance, pulFlags);
			if ( bRunning == FALSE ) {
				pstRetInstance->stCallbackInfo.bRunnable = FALSE;
				pstRetInstance->bFlushing = FALSE;
				break;
			}
		}
		_UnlockInstance(pstRetInstance, pulFlags);
		pstRetInstance = NULL;
	} while( time_before64(get_jiffies_64(), ui64TimeOut) );

	logm_noti(vdec_vdu, "[%d]End of waiting : %s\n",
							(UINT32)hInstance, (pstRetInstance == NULL)? "Fail" : "Success");

	return pstRetInstance;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_InitCheckingInvalidFrame(INSTANCE_T* pstInstance, UINT16 ui16Threshold)
{
	BOOLEAN			bRetVal = FALSE;

	pstInstance->stValidFrameInfo.ui16ValidThreshold = ui16Threshold;
	pstInstance->stValidFrameInfo.si8FirstFrameIndex = VDU_INVALID_FRAME_INDEX;
	pstInstance->stValidFrameInfo.ui16NumberOfSearch = NUMBER_OF_SEARCHING_I
							+ pstInstance->stValidFrameInfo.ui16ValidThreshold;
	pstInstance->stValidFrameInfo.ui32GarbageFramesBit = 0;

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_CheckInvalidFrame(INSTANCE_T* pstInstance)
{
	BOOLEAN				bRetVal = FALSE;
	VDU_DEC_RESULT_T*	pstDecResult = &pstInstance->stCallbackInfo.stDecResult;

	if ( _DidFinishCheckInvalidFrame(pstInstance) == FALSE ) {
		if ( pstDecResult->si8DecodedIndex >= 0 ) {
			switch ( pstDecResult->ePictureType ) {
			case VDU_PICTURE_I:
				if( (pstInstance->stValidFrameInfo.si8FirstFrameIndex != VDU_INVALID_FRAME_INDEX) ||
					(pstInstance->pstDecInfo->eCodecType == VDU_CODEC_HEVC) ) {
					_FinishCheckInvalidFrame(pstInstance);
				} else {
					pstInstance->stValidFrameInfo.si8FirstFrameIndex = pstDecResult->si8DecodedIndex;
					pstInstance->stValidFrameInfo.ui16NumberOfSearch = pstInstance->stValidFrameInfo.ui16ValidThreshold;
				}
				bRetVal = TRUE;
				break;

			case VDU_PICTURE_P:
				if ( pstInstance->stValidFrameInfo.si8FirstFrameIndex == VDU_INVALID_FRAME_INDEX ) {
					pstInstance->stValidFrameInfo.ui32GarbageFramesBit |= (1 << pstDecResult->si8DecodedIndex);
					pstInstance->stValidFrameInfo.ui16NumberOfSearch--;
					pstDecResult->si8DecodedIndex = -2;
				} else {
					_FinishCheckInvalidFrame(pstInstance);
					bRetVal = TRUE;
				}
				break;

			default:
				pstInstance->stValidFrameInfo.ui32GarbageFramesBit |= (1 << pstDecResult->si8DecodedIndex);
				pstInstance->stValidFrameInfo.ui16NumberOfSearch--;
				pstDecResult->si8DecodedIndex = -2;
			}
		} else if ( pstInstance->stValidFrameInfo.ui16NumberOfSearch > 0 ) {
			pstInstance->stValidFrameInfo.ui16NumberOfSearch--;
		}

		if ( (pstInstance->stValidFrameInfo.si8FirstFrameIndex != VDU_INVALID_FRAME_INDEX) &&
			 (pstInstance->stValidFrameInfo.si8FirstFrameIndex == pstDecResult->si8DisplayIndex) ) {
			_FinishCheckInvalidFrame(pstInstance);
		}
	} else {
		bRetVal = TRUE;
	}

	if ( (pstDecResult->si8DisplayIndex >= 0) &&
		 (pstInstance->stValidFrameInfo.ui32GarbageFramesBit & (1 << pstDecResult->si8DisplayIndex)) ) {
		pstInstance->stValidFrameInfo.ui32GarbageFramesBit &= ~(1 << pstDecResult->si8DisplayIndex);

		if ( pstInstance->pstDecInfo->eExternLinearBufMode != VDU_LINEAR_MODE_NONE ) {
			pstInstance->stExternBuffer.aui32AddressList[pstInstance->stExternBuffer.ui8NumberOfBuffers]
				= pstDecResult->stPictureInfo.stAddress.ui32Y;
			pstInstance->stExternBuffer.ui8NumberOfBuffers += 1;
		} else {
			pstInstance->aui8ClearFrames[pstDecResult->si8DisplayIndex]++;
		}

		pstDecResult->si8DisplayIndex = -3;
	}

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_DidCheckValidIFrame(INSTANCE_T* pstInstance)
{
	BOOLEAN			bRetVal = FALSE;

	if ( pstInstance->stValidFrameInfo.ui16NumberOfSearch <=
							pstInstance->stValidFrameInfo.ui16ValidThreshold ) {
		bRetVal = TRUE;
	}

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_DidFinishCheckInvalidFrame(INSTANCE_T* pstInstance)
{
	BOOLEAN			bRetVal = FALSE;

	if ( pstInstance->stValidFrameInfo.ui16NumberOfSearch == 0 ) {
		bRetVal = TRUE;
	}

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static void					_FinishCheckInvalidFrame(INSTANCE_T* pstInstance)
{
	pstInstance->stValidFrameInfo.ui16NumberOfSearch = 0;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static void					_RunPostCallback(INSTANCE_T* pstInstance, ULONG* pulFlags)
{
	VDU_HANDLE_T		hVduHandle = VDU_INVALID_HANDLE;
	VDU_DEC_RESULT_T*	pstDecResult = &pstInstance->stCallbackInfo.stDecResult;
	VDU_RESULT_T*		pstNotiInfo = NULL;
	BOOLEAN				bContinueDecoding = FALSE;
	BOOLEAN				bSkipNotification = FALSE;
	SINT32				si32DecTime = VDU_DBG_POSTPONE_CHECK;

	hVduHandle = pstInstance->pstDecInfo->hVduHandle;

	switch ( pstDecResult->eNotiType ) {
	case VDU_NOTI_SUCCESS:
	case VDU_NOTI_DISPLAY:
		bContinueDecoding = TRUE;
		break;

	case VDU_NOTI_FRAME_SKIP:
		bContinueDecoding = TRUE;

		if ( pstDecResult->bDecSuccess == FALSE ) {
			 if ( pstDecResult->si8DisplayIndex >= 0 ) {
			 	pstDecResult->eNotiType = VDU_NOTI_DISPLAY;
			 } else {
				bSkipNotification = TRUE;
			 }
		}
		break;

	case VDU_NOTI_INVALID:
		if ( (pstInstance->ui8DecRetryCount > 0) &&
			 (pstDecResult->bDecSuccess == FALSE) ) {
			pstInstance->ui8DecRetryCount--;
			bContinueDecoding = TRUE;
			bSkipNotification = TRUE;
		}
		break;

	default:
		break;
	}

	if ( pstDecResult->bDecSuccess == TRUE ) {
		si32DecTime = pstDecResult->ui32DecTime;

		if ( _CheckInvalidFrame(pstInstance) == FALSE ) {
			pstDecResult->eNotiType = VDU_NOTI_FRAME_SKIP;
			bContinueDecoding = TRUE;
			bSkipNotification = FALSE;
		}
	}

	pstNotiInfo = &pstInstance->stNotiInfo;
	pstNotiInfo->eNotiType = pstDecResult->eNotiType;
	pstNotiInfo->si8FrmIndex = pstDecResult->si8DisplayIndex;
	pstNotiInfo->si8DecodedIndex = pstDecResult->si8DecodedIndex;
	pstNotiInfo->bDecSuccess = pstDecResult->bDecSuccess;
	pstNotiInfo->bFieldSuccess = pstDecResult->bFieldSuccess;
	pstNotiInfo->ui32ReadPtr = pstDecResult->ui32ReadPtr;

	pstNotiInfo->stFrameInfo.ui8NumOfFrames = pstInstance->pstDecInfo->stSequence.ui8NumOfFrames;
	if ( (pstNotiInfo->si8FrmIndex >= 0) ||
		 (pstNotiInfo->eNotiType == VDU_NOTI_DPB_EMPTY) ) {
		_SetDisplayFrameInfo(&pstNotiInfo->stFrameInfo, pstInstance);
		// Request by T. Kim(VDISP)
		pstNotiInfo->stFrameInfo.stDispInfo.ui8DisplayPeriod |= ((pstNotiInfo->si8FrmIndex << 3) & 0xF8);
	}

	if ( pstInstance->bFlushing == TRUE ) {
		if ( pstNotiInfo->si8FrmIndex >= 0 ) {
			if ( pstInstance->pstDecInfo->eExternLinearBufMode != VDU_LINEAR_MODE_NONE ) {
				pstInstance->stExternBuffer.aui32AddressList[pstInstance->stExternBuffer.ui8NumberOfBuffers]
					= pstDecResult->stPictureInfo.stAddress.ui32Y;
				pstInstance->stExternBuffer.ui8NumberOfBuffers += 1;
			} else {
				pstInstance->aui8ClearFrames[pstNotiInfo->si8FrmIndex]++;
			}
		}
		pstInstance->bRunning = FALSE;
		_UnlockInstance(pstInstance, pulFlags);

		if ( pstDecResult->eNotiType != VDU_NOTI_INVALID ) {
			VDU_DEC_RUN_PARAM_T	stRunParam = { 0, };

			stRunParam.eCommand = VDU_DEC_CMD_COMPLETE;
			pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);
		}
	} else {
		if ( bSkipNotification == FALSE ) {
			VDU_DBG_PrintResultInfo(hVduHandle, pstNotiInfo);

			if ( pstDecResult->si8DisplayIndex >= 0 ) {
				VDU_DBG_PrintFrameInfo(hVduHandle, &pstNotiInfo->stFrameInfo);
			}
		}

		if ( bContinueDecoding == TRUE ) {
			VDU_RET_T			eRetVal = VDU_RET_ERROR;
			VDU_DEC_RUN_PARAM_T	stRunParam = { 0, };
			UINT32*				pui32AddressList = NULL;

			if ( (pstInstance->ePicScanMode != VDU_PIC_SCAN_ALL) ||
				 (_DidFinishCheckInvalidFrame(pstInstance) == TRUE) ) {
				stRunParam.ePicScanMode = pstInstance->ePicScanMode;
				_FinishCheckInvalidFrame(pstInstance);
			} else {
				stRunParam.ePicScanMode = _DidCheckValidIFrame(pstInstance)? VDU_PIC_SCAN_IP : VDU_PIC_SCAN_I;	// Skip B frame after I frame
				logm_info(vdec_vdu, "[%d]Decoded only %s-frames\n", (UINT32)hVduHandle, (stRunParam.ePicScanMode == VDU_PIC_SCAN_I)? "I" : "IP");
			}

			stRunParam.ui32ResetAddr = VDU_INVALID_ADDRESS;
			stRunParam.ui32WriteAddr = pstInstance->stFeedInfo.ui32WriteAddr;
			stRunParam.ui32StreamEndAddr = pstInstance->stFeedInfo.ui32StreamEndAddr;
			stRunParam.bEnableUserData = pstInstance->bEnableUserData;
			memcpy(stRunParam.aui8ClearFrames, pstInstance->aui8ClearFrames, sizeof(pstInstance->aui8ClearFrames));
			memset(pstInstance->aui8ClearFrames, 0, sizeof(pstInstance->aui8ClearFrames));

			if ( pstInstance->pstDecInfo->eExternLinearBufMode != VDU_LINEAR_MODE_NONE ) {
				if ( pstInstance->stExternBuffer.ui8NumberOfBuffers > 0 ) {
					pui32AddressList = _Malloc(pstInstance->stExternBuffer.ui8NumberOfBuffers * sizeof(UINT32));
					if ( pui32AddressList == NULL ) {
						stRunParam.stLinearFrames.ui8NumberOfFrames = 0;
					} else {
						stRunParam.stLinearFrames.ui8NumberOfFrames = pstInstance->stExternBuffer.ui8NumberOfBuffers;
						stRunParam.stLinearFrames.ui32Stride = pstInstance->stExternBuffer.ui32Stride;
						stRunParam.stLinearFrames.ui32Height = pstInstance->stExternBuffer.ui32Height;
						stRunParam.stLinearFrames.aui32AddressList = pui32AddressList;
						memcpy(pui32AddressList, pstInstance->stExternBuffer.aui32AddressList,
								stRunParam.stLinearFrames.ui8NumberOfFrames * sizeof(UINT32));

						pstInstance->stExternBuffer.ui8NumberOfBuffers = 0;
					}
				}
			} else {
				stRunParam.stLinearFrames.ui8NumberOfFrames = 0;
				stRunParam.stLinearFrames.aui32AddressList = NULL;
			}
			_UnlockInstance(pstInstance, pulFlags);

			if ( pstDecResult->eNotiType != VDU_NOTI_INVALID ) {
				logm_info(vdec_vdu, "[%d]Continue decoding\n", (UINT32)hVduHandle);
				stRunParam.eCommand = VDU_DEC_CMD_CONTINUE;

				VDU_DBG_CheckDecTime(hVduHandle, si32DecTime);
				VDU_DBG_CheckDecTime(hVduHandle, VDU_DBG_START_CHECK);
			} else {
				stRunParam.eCommand = VDU_DEC_CMD_DECODE;
			}

			eRetVal = pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);
			if ( eRetVal != VDU_RET_OK ) {
				if ( eRetVal == VDU_RET_BUSY ) {
					VDU_DEC_RESULT_T	stDecResult = { 0, };

					pstInstance->ui8DecRetryCount++;
					stDecResult.eNotiType = VDU_NOTI_INVALID;
					stDecResult.bDecSuccess = FALSE;
					stDecResult.si8DecodedIndex = VDU_INVALID_FRAME_INDEX;
					stDecResult.si8DisplayIndex = VDU_INVALID_FRAME_INDEX;

					_Callback(pstInstance->pstDecInfo, &stDecResult);
				} else {
					VDU_DBG_CheckDecTime(hVduHandle, VDU_DBG_POSTPONE_CHECK);
					logm_error(vdec_vdu, "Error\n");
				}
			}

			if ( pui32AddressList != NULL ) {
				_MFree(pui32AddressList);
			}

			_LockInstance(pstInstance, pulFlags);
		} else {
			VDU_DEC_RUN_PARAM_T	stRunParam = { 0, };

			pstInstance->bRunning = FALSE;
			pstInstance->stCallbackInfo.bRunnable = FALSE;
			_UnlockInstance(pstInstance, pulFlags);

			stRunParam.eCommand = VDU_DEC_CMD_COMPLETE;
			pstInstance->pstOperations->pfnRun(pstInstance->pstDecInfo, &stRunParam);

			VDU_DBG_CheckDecTime(hVduHandle, si32DecTime);

			_LockInstance(pstInstance, pulFlags);
		}
		_UnlockInstance(pstInstance, pulFlags);

		if ( bSkipNotification == FALSE ) {
			_gpfnCallback(hVduHandle, pstNotiInfo);
		}
	}

	_LockInstance(pstInstance, pulFlags);
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static void					_CallbackTasklet(ULONG _ulData)
{
	INSTANCE_T*			pstInstance = (INSTANCE_T*)_ulData;
	CALLBACK_UNIT_T*	pstCallbackUnit = NULL;
	ULONG				ulFlags = 0x00;

	_LockInstance(pstInstance, &ulFlags);
	pstCallbackUnit = pstInstance->stCallbackInfo.pstList;
	if ( pstCallbackUnit != NULL ) {
		if ( pstInstance->stCallbackInfo.bRunnable == TRUE ) {
			pstInstance->stCallbackInfo.stDecResult = pstCallbackUnit->stDecResult;
			pstInstance->stCallbackInfo.pstList = pstCallbackUnit->pstNext;
			_MFree(pstCallbackUnit);

			_RunPostCallback(pstInstance, &ulFlags);

			if ( pstInstance->stCallbackInfo.pstList != NULL ) {
				tasklet_schedule(&pstInstance->stCallbackInfo.stTasklet);
			}
		} else {
			tasklet_schedule(&pstInstance->stCallbackInfo.stTasklet);
		}
	}
	_UnlockInstance(pstInstance, &ulFlags);

	return;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static void 				_Callback(VDU_DEC_INSTANCE_T* pstDecInfo, VDU_DEC_RESULT_T* pstDecResult)
{
	INSTANCE_T*			pstInstance = NULL;
	ULONG				ulFlags = 0x00;
	CALLBACK_UNIT_T*	pstCallbackUnit = NULL;
	CALLBACK_UNIT_T**	ppstLastPointer = NULL;

	pstInstance = _GetInstance(pstDecInfo->hVduHandle, &ulFlags);
	if ( pstInstance == NULL ) {
		goto GOTO_END;
	} else if( pstInstance->pstDecInfo != pstDecInfo ) {
		_UnlockInstance(pstInstance, &ulFlags);
		goto GOTO_END;
	}

	pstCallbackUnit = (CALLBACK_UNIT_T*)_Malloc(sizeof(CALLBACK_UNIT_T));
	if ( pstCallbackUnit == NULL ) {
		_UnlockInstance(pstInstance, &ulFlags);
		goto GOTO_END;
	}

	pstCallbackUnit->stDecResult = *pstDecResult;
	pstCallbackUnit->pstNext = NULL;

	if ( pstInstance->stCallbackInfo.pstList == NULL ) {
		pstInstance->stCallbackInfo.pstList = pstCallbackUnit;
	} else {
		ppstLastPointer = &(pstInstance->stCallbackInfo.pstList->pstNext);
		while( *ppstLastPointer != NULL ) {
			ppstLastPointer = &((*ppstLastPointer)->pstNext);
		}

		*ppstLastPointer = pstCallbackUnit;
	}

	if ( pstInstance->stCallbackInfo.pstList == pstCallbackUnit ) {
		tasklet_schedule(&pstInstance->stCallbackInfo.stTasklet);
	}
	_UnlockInstance(pstInstance, &ulFlags);

GOTO_END:
	return;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetDisplayFrameInfo(VDU_FRM_INFO_T* pstFrameInfo, INSTANCE_T* pstInstance)
{
	BOOLEAN					bRetVal = FALSE;
	VDU_DEC_INSTANCE_T*		pstDecInfo = pstInstance->pstDecInfo;
	VDU_DEC_FRAME_INFO_T*	pstPictureInfo = &pstInstance->stCallbackInfo.stDecResult.stPictureInfo;
	VDU_CODEC_T				eCodecType = pstDecInfo->eCodecType;
	BOOLEAN					bInterlacedSequence = pstDecInfo->stSequence.bInterlaced;

	_SetSizeInfo(pstFrameInfo, pstPictureInfo, eCodecType, bInterlacedSequence);
	_SetAspectRatio(pstFrameInfo, pstPictureInfo, eCodecType);
	pstFrameInfo->stAddress.ui32TiledBase = pstPictureInfo->stAddress.ui32TiledBase;
	pstFrameInfo->stAddress.ui32Y = pstPictureInfo->stAddress.ui32Y;
	pstFrameInfo->stAddress.ui32Cb = pstPictureInfo->stAddress.ui32Cb;
	pstFrameInfo->stAddress.ui32Cr = pstPictureInfo->stAddress.ui32Cr;
	_SetFrameRate(pstFrameInfo, pstPictureInfo, eCodecType);
	pstFrameInfo->stDispInfo.eFrameMapType = pstPictureInfo->stDispInfo.eFrameMapType;
	_SetScanTypeAndPeriod(pstFrameInfo, pstPictureInfo, eCodecType,
							bInterlacedSequence, &pstInstance->stPreDisplayInfo.ui8PicTimingStruct);
	pstFrameInfo->stDispInfo.bLowDelay = pstDecInfo->stSequence.bLowDelay;
	_SetFramePackArrange(pstFrameInfo, pstPictureInfo, eCodecType,
							&pstInstance->stPreDisplayInfo.si32FramePackArrange);
	pstFrameInfo->stDispInfo.ui8ErrorRate = pstPictureInfo->stDispInfo.ui8ErrorRate;
	pstFrameInfo->stDispInfo.ui8PictureType = pstPictureInfo->stDispInfo.ui8PictureType;
	pstFrameInfo->stDispInfo.bFieldPicture = pstPictureInfo->stDispInfo.bFieldPicture;

	if ( pstDecInfo->eCodecType == VDU_CODEC_MPEG2 ) {
		pstFrameInfo->stDispInfo.si32Mp2DispWidth = pstPictureInfo->stMpeg2Info.si32DispWidth;
		pstFrameInfo->stDispInfo.si32Mp2DispHeight = pstPictureInfo->stMpeg2Info.si32DispHeight;
	} else {
		pstFrameInfo->stDispInfo.si32Mp2DispWidth = 0;
		pstFrameInfo->stDispInfo.si32Mp2DispHeight = 0;
	}

	_SetActiveFormatDescription(pstFrameInfo, pstPictureInfo, eCodecType);
	pstFrameInfo->stUserData.ui32Address = pstPictureInfo->stUserData.ui32Address;
	pstFrameInfo->stUserData.ui32Size = pstPictureInfo->stUserData.ui32Size;

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetSizeInfo(VDU_FRM_INFO_T* pstFrameInfo,
											VDU_DEC_FRAME_INFO_T* pstPictureInfo,
											VDU_CODEC_T eCodecType,
											BOOLEAN bInterlacedSequence)
{
	BOOLEAN		bRetVal = FALSE;

	pstFrameInfo->stRect.ui32Stride = pstPictureInfo->stSizeInfo.ui32Stride;
	pstFrameInfo->stRect.ui32PicWidth = pstPictureInfo->stSizeInfo.ui32Width;
	pstFrameInfo->stRect.ui32PicHeight = pstPictureInfo->stSizeInfo.ui32Height;

	switch ( eCodecType ) {
	case VDU_CODEC_H264_AVC:
	case VDU_CODEC_H264_MVC:
		{
			// Support Only 4:2:0
			UINT16	ui16CropUnitX = 2;
			UINT16	ui16CropUnitY = 4 - ((bInterlacedSequence == FALSE)? 2 : 0);

			pstFrameInfo->stRect.ui16CropLeft = pstPictureInfo->stH264Info.ui16CropLeft;
			pstFrameInfo->stRect.ui16CropRight = pstPictureInfo->stH264Info.ui16CropRight;
			pstFrameInfo->stRect.ui16CropTop = pstPictureInfo->stH264Info.ui16CropTop;
			pstFrameInfo->stRect.ui16CropBottom = pstPictureInfo->stH264Info.ui16CropBottom;

			if( ((pstFrameInfo->stRect.ui16CropRight / ui16CropUnitX) >= 8) ||
				((pstFrameInfo->stRect.ui16CropBottom / ui16CropUnitY) >= 8) )
			{
				logm_noti(vdec_vdu, "Brainfart cropping, cropping disabled(%d/%d/%d/%d)\n",
										pstFrameInfo->stRect.ui16CropLeft, pstFrameInfo->stRect.ui16CropRight,
										pstFrameInfo->stRect.ui16CropTop, pstFrameInfo->stRect.ui16CropBottom);

				pstFrameInfo->stRect.ui16CropLeft = 0;
				pstFrameInfo->stRect.ui16CropRight = 0;
				pstFrameInfo->stRect.ui16CropTop = 0;
				pstFrameInfo->stRect.ui16CropBottom = 0;
			}
		}
		break;

	case VDU_CODEC_MPEG2:
	case VDU_CODEC_MPEG4:
	case VDU_CODEC_H263:
	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
	case VDU_CODEC_VC1_RCV_V1:
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
	case VDU_CODEC_RVX:
	case VDU_CODEC_AVS:
	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
	case VDU_CODEC_HEVC:
	default:
		pstFrameInfo->stRect.ui16CropLeft = 0;
		pstFrameInfo->stRect.ui16CropRight = 0;
		pstFrameInfo->stRect.ui16CropTop = 0;
		pstFrameInfo->stRect.ui16CropBottom = 0;
		break;
	}

	// Check invalid crop size
	if ( pstFrameInfo->stRect.ui16CropLeft > pstFrameInfo->stRect.ui32PicWidth ) {
		pstFrameInfo->stRect.ui16CropLeft = 0;
	}

	if ( pstFrameInfo->stRect.ui16CropRight > pstFrameInfo->stRect.ui32PicWidth ) {
		pstFrameInfo->stRect.ui16CropRight = 0;
	}

	if ( pstFrameInfo->stRect.ui16CropTop > pstFrameInfo->stRect.ui32PicHeight ) {
		pstFrameInfo->stRect.ui16CropTop = 0;
	}

	if ( pstFrameInfo->stRect.ui16CropBottom > pstFrameInfo->stRect.ui32PicHeight ) {
		pstFrameInfo->stRect.ui16CropBottom = 0;
	}

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
* Sample Aspect Ratio(SAR) = Display Aspect Ratio(DAR) * horizontal size / vertical size
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetAspectRatio(VDU_FRM_INFO_T* pstFrameInfo, VDU_DEC_FRAME_INFO_T* pstPictureInfo, VDU_CODEC_T eCodecType)
{
	BOOLEAN			bRetVal = FALSE;
	const UINT8		aui8Dar[ 5][2] = { { 0, 0}, { 1, 1}, { 4, 3}, {16, 9}, {221, 100}	};								// MPEG2
	const UINT8		aui8Sar[17][2] = { { 0, 0}, { 1, 1}, {12,11}, {10,11}, {16,11}, {40,33},							// MPEG4 & VC-1 & H.264
										{24,11}, {20,11}, {32,11},	 {80,33}, {18,11}, {15,11}, {64,33}, {160,99},	// VC-1 & H.264
										{4, 3}, { 3, 2}, { 2, 1} };														// H.264
	UINT16			ui16DispWidth = pstPictureInfo->stSizeInfo.ui32Width;
	UINT16			ui16DispHeight = pstPictureInfo->stSizeInfo.ui32Height;
	UINT8			ui8RetAspectRatio = 0;
	UINT16			ui16RetSarW = 1;
	UINT16			ui16RetSarH = 1;
	UINT8			ui8ArInfo = 0;
	UINT8			ui8ArIdc = 0;

	switch ( eCodecType ) {
	case VDU_CODEC_MPEG2:			// DAR --> SAR
	case VDU_CODEC_AVS:
		if ( eCodecType == VDU_CODEC_MPEG2 ) {
			ui8ArInfo = pstPictureInfo->stMpeg2Info.ui8DisplayAspectRatio & 0x0F;
		} else {
			ui8ArInfo = pstPictureInfo->stAvsInfo.ui8DisplayAspectRatio & 0x0F;
		}

		if ( (1 < ui8ArInfo) && (ui8ArInfo < 5) ) {
			UINT32	ui32SarW, ui32SarH;
			UINT32	ui32Gcd;

			ui16DispHeight = (ui16DispHeight == 1088)? 1080 : ui16DispHeight;
			ui32SarW = aui8Dar[ui8ArInfo][0] * ui16DispHeight;
			ui32SarH = aui8Dar[ui8ArInfo][1] * ui16DispWidth;

			ui32Gcd = (ui32SarW > ui32SarH)? _GetGcd(ui32SarW, ui32SarH) : _GetGcd(ui32SarH, ui32SarW);
			ui32Gcd = (ui32Gcd == 0)? 1 : ui32Gcd;

			ui32SarW = ui32SarW / ui32Gcd;
			ui32SarH = ui32SarH / ui32Gcd;

			if ( (ui32SarW > 0xFFFF) || (ui32SarH > 0xFFFF) ) {
				ui16RetSarW = 100;
				ui16RetSarH = 100 * ui32SarH / ui32SarW;
			} else {
				ui16RetSarW = (UINT16)ui32SarW;
				ui16RetSarH = (UINT16)ui32SarH;
			}

			ui8RetAspectRatio = ui8ArInfo;
		} else	{
			ui8RetAspectRatio = _GetDisplayAspectRatio(ui16DispWidth, ui16DispHeight);
		}
		break;

	case VDU_CODEC_MPEG4:			// (Extendec)SAR--> DAR
	case VDU_CODEC_H263:
	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
		ui8ArInfo = pstPictureInfo->stMpeg4Info.ui8AspectRatioInfo & 0x0F;

		if ( (0 < ui8ArInfo) && (ui8ArInfo < 6) ) {
			ui16RetSarW = aui8Sar[ui8ArInfo][0];
			ui16RetSarH = aui8Sar[ui8ArInfo][1];
		} else if( ui8ArInfo == 0x0F ) {	// extended SAR
			ui16RetSarW = pstPictureInfo->stMpeg4Info.ui8AspectRatioWidth;
			ui16RetSarH = pstPictureInfo->stMpeg4Info.ui8AspectRatioHeight;
		} else {	// Unspecified
			logm_noti(vdec_vdu, "[MPEG4]Wrong Aspect ratio information %d\n", pstPictureInfo->stMpeg4Info.ui8AspectRatioInfo);
		}

		ui8RetAspectRatio = _GetDisplayAspectRatio(ui16DispWidth * ui16RetSarW, ui16DispHeight * ui16RetSarH);
		break;

	case VDU_CODEC_H264_AVC:		// SAR --> DAR
	case VDU_CODEC_H264_MVC:
		ui8ArIdc = pstPictureInfo->stH264Info.ui8AspectRatioIdc;

		if ( (0 < ui8ArIdc) && (ui8ArIdc < 17) ) {
			ui16RetSarW = aui8Sar[ui8ArIdc][0];
			ui16RetSarH = aui8Sar[ui8ArIdc][1];
		} else if( ui8ArIdc == 0xFF ) {	// extended SAR
			ui16RetSarW = pstPictureInfo->stH264Info.ui16AspectRatioWidth;
			ui16RetSarH = pstPictureInfo->stH264Info.ui16AspectRatioHeight;
		} else {	// Unspecified
			logm_noti(vdec_vdu, "[H264]Wrong Aspect ratio information %d\n", pstPictureInfo->stH264Info.ui8AspectRatioIdc);
		}

		ui8RetAspectRatio = _GetDisplayAspectRatio(ui16DispWidth * ui16RetSarW, ui16DispHeight * ui16RetSarH);
		break;

	case VDU_CODEC_VC1_RCV_V1:		// SAR --> DAR
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
		ui8ArInfo = pstPictureInfo->stVc1Info.ui8AspectRatio & 0x0F;

		if( (0 < ui8ArInfo) && (ui8ArInfo < 14) ) {
			ui16RetSarW = aui8Sar[ui8ArInfo][0];
			ui16RetSarH = aui8Sar[ui8ArInfo][1];
		} else if( ui8ArInfo == 0x0F ) {
			ui16RetSarW = pstPictureInfo->stVc1Info.ui8AspectRatioWidth;
			ui16RetSarH = pstPictureInfo->stVc1Info.ui8AspectRatioHeight;
		} else {	// Unspecified
			logm_noti(vdec_vdu, "[VC1]Wrong Aspect ratio information %d\n", pstPictureInfo->stVc1Info.ui8AspectRatio);
		}

		ui8RetAspectRatio = _GetDisplayAspectRatio(ui16DispWidth * ui16RetSarW, ui16DispHeight * ui16RetSarH);
		break;

	case VDU_CODEC_RVX:
		ui8ArInfo = pstPictureInfo->stRvInfo.ui8AspectRatio & 0x0F;

		if ( (0 < ui8ArInfo) && (ui8ArInfo < 6) ) {
			ui16RetSarW = aui8Sar[ui8ArInfo][0];
			ui16RetSarH = aui8Sar[ui8ArInfo][1];
		} else if( ui8ArInfo == 0x0F ) {
			ui16RetSarW = pstPictureInfo->stRvInfo.ui8AspectRatioWidth;
			ui16RetSarH = pstPictureInfo->stRvInfo.ui8AspectRatioHeight;
		} else {	// Unspecified
			logm_noti(vdec_vdu, "[RV]Wrong Aspect ratio information %d\n", pstPictureInfo->stRvInfo.ui8AspectRatio);
		}

		ui8RetAspectRatio = _GetDisplayAspectRatio(ui16DispWidth * ui16RetSarW, ui16DispHeight * ui16RetSarH);
		break;

	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
	case VDU_CODEC_HEVC:
	default:
		ui8RetAspectRatio = _GetDisplayAspectRatio(ui16DispWidth * ui16RetSarW, ui16DispHeight * ui16RetSarH);
		break;
	}

	pstFrameInfo->stAspectRatio.ui16ParW = (ui16RetSarW == 0)? 1 : ui16RetSarW;
	pstFrameInfo->stAspectRatio.ui16ParH = (ui16RetSarH == 0)? 1 : ui16RetSarH;
	pstFrameInfo->stAspectRatio.ui8Mpeg2Dar = ui8RetAspectRatio;

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetFrameRate(VDU_FRM_INFO_T* pstFrameInfo, VDU_DEC_FRAME_INFO_T* pstPictureInfo, VDU_CODEC_T eCodecType)
{
	BOOLEAN		bRetVal = FALSE;

	pstFrameInfo->stFrameRate.ui32Residual = pstPictureInfo->stDispInfo.ui32FrameRateResidual;
	pstFrameInfo->stFrameRate.ui32Divider = pstPictureInfo->stDispInfo.ui32FrameRateDivider;

	switch ( eCodecType ) {
	case VDU_CODEC_H264_AVC:
	case VDU_CODEC_H264_MVC:
		pstFrameInfo->stFrameRate.bVariable = !pstPictureInfo->stH264Info.bFixedFrameRate;
		break;

	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_RVX:
		pstFrameInfo->stFrameRate.ui32Residual = 0;
		pstFrameInfo->stFrameRate.ui32Divider = 0;
		pstFrameInfo->stFrameRate.bVariable = FALSE;
		break;

	case VDU_CODEC_MPEG2:
	case VDU_CODEC_MPEG4:
	case VDU_CODEC_H263:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
	case VDU_CODEC_VC1_RCV_V1:
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
	case VDU_CODEC_AVS:
	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
	case VDU_CODEC_HEVC:
	default:
		pstFrameInfo->stFrameRate.bVariable = FALSE;
		break;
	}

	if ( (pstFrameInfo->stFrameRate.ui32Residual == -1)	||
		 (pstFrameInfo->stFrameRate.ui32Divider == -1)	||
		 (pstFrameInfo->stFrameRate.ui32Residual ==  0)	||
		 (pstFrameInfo->stFrameRate.ui32Divider ==  0) ) {
		pstFrameInfo->stFrameRate.ui32Residual	= 0;
		pstFrameInfo->stFrameRate.ui32Divider	= 0;
	}

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
*	PROG TFF  RFF  PicStruct									Period 1st 2nd 3rd
	1    0    0    3        1 Decoding 1 Display				1T     F   X   X
	1    1    0    3        1 Decoding 1 Display				1T     F   X   X
	1    0    1    3        1 Decoding 2 Display				2T     F   F   X
	1    1    1    3        1 Decoding 3 Display				3T     F   F   F
	0    0    0    3        B -> T (frame picture)				2T     B   T   X
	0    0    0    2        B (field pic.)(Last Decoded Pic.)	2T     T   B   X
	0    1    0    3        T -> B (frame picture)				2T     T   B   X
	0    1    0    1        T (field pic.)(Last Decoded Pic.)	2T     B   T   X
	0    0    0    3        B -> T								2T     B   T   X
	0    1    0    3        T -> B								2T     T   B   X
	0    0    1    3        B -> T -> B							3T     B   T   B
	0    1    1    3        T -> B -> T							3T     T   B   T
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetScanTypeAndPeriod(VDU_FRM_INFO_T* pstFrameInfo,
													VDU_DEC_FRAME_INFO_T* pstPictureInfo,
													VDU_CODEC_T eCodecType,
													BOOLEAN bInterlacedSequence,
													UINT8* pui8PrePicTimingStruct)
{
	BOOLEAN					bRetVal = FALSE;
	const AVC_PIC_STRUCT_T	astAvcPicStruct[9] =	{	{ FALSE, FALSE, VDU_FRM_SCAN_PROGRESSIVE	},	// 0: 1 Frame Display
														{ FALSE, FALSE, VDU_FRM_SCAN_BOTTOM_FIRST	},	// 1: B->T Display (Field Pic)
														{ TRUE , FALSE, VDU_FRM_SCAN_TOP_FIRST		},	// 2: T->B Display (Field Pic)
														{ TRUE , FALSE, VDU_FRM_SCAN_TOP_FIRST		},	// 3: T->B Display
														{ FALSE, FALSE, VDU_FRM_SCAN_BOTTOM_FIRST	},	// 4: B->T Display
														{ TRUE , TRUE , VDU_FRM_SCAN_TOP_FIRST		},	// 5: T->B->T Display
														{ FALSE, TRUE , VDU_FRM_SCAN_BOTTOM_FIRST	},	// 6: B->T->B Display
														{ FALSE, TRUE , VDU_FRM_SCAN_PROGRESSIVE	},	// 7: 2 Frame Display
														{ TRUE , TRUE , VDU_FRM_SCAN_PROGRESSIVE	}	// 8: 3 Frame Display
													};	//	bTopFieldFirst / bRepeatFirstField / eFrameScanType
	BOOLEAN					bTopFieldFirst = FALSE;
	BOOLEAN					bRepeatFirstField = FALSE;
	VDU_FRM_SCAN_T			eFrameScanType = VDU_FRM_SCAN_INVALID;
	UINT8					ui8DisplayPeriod = 1;
	BOOLEAN					bNonPairedField = FALSE;

	switch ( eCodecType ) {
	case VDU_CODEC_MPEG2:
		bTopFieldFirst		= pstPictureInfo->stMpeg2Info.bTopFieldFirst;
		bRepeatFirstField	= pstPictureInfo->stMpeg2Info.bRepeatFirstField;

		if( bInterlacedSequence == TRUE ) {
			switch ( pstPictureInfo->stMpeg2Info.ui8PictureStructure ) {
			case 1:
				eFrameScanType	= VDU_FRM_SCAN_BOTTOM_FIRST;
				break;

			case 2:
				eFrameScanType	= VDU_FRM_SCAN_TOP_FIRST;
				break;

			case 3:
			default:
				if ( bTopFieldFirst == TRUE ) {
					eFrameScanType	= VDU_FRM_SCAN_TOP_FIRST;
				} else {
					eFrameScanType	= VDU_FRM_SCAN_BOTTOM_FIRST;
				}
				break;
			}
		} else {
			eFrameScanType	= VDU_FRM_SCAN_PROGRESSIVE;
		}
		break;

	case VDU_CODEC_MPEG4:
	case VDU_CODEC_H263:
	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
		bTopFieldFirst		= pstPictureInfo->stMpeg4Info.bTopFieldFirst;
		bRepeatFirstField	= FALSE;

		if( bInterlacedSequence == TRUE ) {
			if ( bTopFieldFirst == TRUE ) {
				eFrameScanType = VDU_FRM_SCAN_TOP_FIRST;
			} else {
				eFrameScanType = VDU_FRM_SCAN_BOTTOM_FIRST;
			}
		} else {
			eFrameScanType = VDU_FRM_SCAN_PROGRESSIVE;
		}
		break;

	case VDU_CODEC_H264_AVC:
	case VDU_CODEC_H264_MVC:
		if ( (pstPictureInfo->stH264Info.stSei.si32PicTimingStruct < 0) ||
			 (pstPictureInfo->stH264Info.stSei.si32PicTimingStruct >= 9) ) {
			pstPictureInfo->stH264Info.stSei.si32PicTimingStruct = *pui8PrePicTimingStruct;
		}

		if ( (pstPictureInfo->stH264Info.stSei.si32PicTimingStruct < 0) ||
			 (pstPictureInfo->stH264Info.stSei.si32PicTimingStruct >= 9) ) {
			bTopFieldFirst		= pstPictureInfo->stH264Info.bTopFieldFirst;
			bRepeatFirstField	= FALSE;

			if( bInterlacedSequence == FALSE ) {
				eFrameScanType = VDU_FRM_SCAN_PROGRESSIVE;
			} else if ( bTopFieldFirst == TRUE ) {
				eFrameScanType = VDU_FRM_SCAN_TOP_FIRST;
			} else {
				eFrameScanType = VDU_FRM_SCAN_BOTTOM_FIRST;
			}
		} else {
			const AVC_PIC_STRUCT_T*	pstAvcPicStruct = &astAvcPicStruct[pstPictureInfo->stH264Info.stSei.si32PicTimingStruct];

			bTopFieldFirst     	= pstAvcPicStruct->bTopFieldFirst;
			bRepeatFirstField 	= pstAvcPicStruct->bRepeatFirstField;
			eFrameScanType		= pstAvcPicStruct->eFrameScanType;
		}

		if ( eFrameScanType != VDU_FRM_SCAN_PROGRESSIVE ) {
			if ( pstPictureInfo->stH264Info.eNpfInfo == VDU_DEC_NPF_BOTTOM_ONLY ) {
				eFrameScanType = VDU_FRM_SCAN_BOTTOM_ONLY;
			} else if ( pstPictureInfo->stH264Info.eNpfInfo == VDU_DEC_NPF_TOP_ONLY ) {
				eFrameScanType = VDU_FRM_SCAN_TOP_ONLY;
			}
		}

		*pui8PrePicTimingStruct = pstPictureInfo->stH264Info.stSei.si32PicTimingStruct;

		if ( pstPictureInfo->stH264Info.eNpfInfo != VDU_DEC_NPF_PAIRED) {
			bNonPairedField = TRUE;
		}
		break;

	case VDU_CODEC_VC1_RCV_V1:
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
		bTopFieldFirst		= pstPictureInfo->stVc1Info.bTopFieldFirst;
		bRepeatFirstField	= pstPictureInfo->stVc1Info.bRepeatFirstField;

		if( bInterlacedSequence == TRUE ) {
			if ( bTopFieldFirst == TRUE ) {
				eFrameScanType	= VDU_FRM_SCAN_TOP_FIRST;
			} else {
				eFrameScanType	= VDU_FRM_SCAN_BOTTOM_FIRST;
			}
		} else {
			eFrameScanType	= VDU_FRM_SCAN_PROGRESSIVE;
		}
		break;

	case VDU_CODEC_AVS:
		bTopFieldFirst		= pstPictureInfo->stAvsInfo.bTopFieldFirst;
		bRepeatFirstField	= pstPictureInfo->stAvsInfo.bRepeatFirstField;

		if( bInterlacedSequence == TRUE ) {
			if ( bTopFieldFirst == TRUE ) {
				eFrameScanType	= VDU_FRM_SCAN_TOP_FIRST;
			} else {
				eFrameScanType	= VDU_FRM_SCAN_BOTTOM_FIRST;
			}
		} else {
			eFrameScanType	= VDU_FRM_SCAN_PROGRESSIVE;
		}
		break;

	case VDU_CODEC_RVX:
	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
	case VDU_CODEC_HEVC:
	default:
		bTopFieldFirst		= FALSE;
		bRepeatFirstField	= FALSE;

		if( bInterlacedSequence == TRUE ) {
			eFrameScanType	= VDU_FRM_SCAN_BOTTOM_FIRST;
		} else {
			eFrameScanType	= VDU_FRM_SCAN_PROGRESSIVE;
		}
		break;

	}

	if ( (eFrameScanType == VDU_FRM_SCAN_TOP_FIRST) ||
		 (eFrameScanType == VDU_FRM_SCAN_BOTTOM_FIRST) ) {
		++ui8DisplayPeriod;		// Interlaced
	} else if ( (bTopFieldFirst == TRUE) && (bRepeatFirstField == TRUE) ) {
		++ui8DisplayPeriod;		// Progressive && TopFieldFirst
	}

	if ( bRepeatFirstField == TRUE ) {
		++ui8DisplayPeriod;
	}

	pstFrameInfo->stDispInfo.eFrameScanType		= eFrameScanType;
	pstFrameInfo->stDispInfo.bTopFieldFirst		= bTopFieldFirst;
	pstFrameInfo->stDispInfo.bRepeatFirstField	= bRepeatFirstField;
	pstFrameInfo->stDispInfo.ui8DisplayPeriod	= ui8DisplayPeriod;
	pstFrameInfo->stDispInfo.bNonPairedField	= bNonPairedField;

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* Sample Aspect Ratio(SAR) = Display Aspect Ratio(DAR) * horizontal size / vertical size
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetFramePackArrange(VDU_FRM_INFO_T* pstFrameInfo,
													VDU_DEC_FRAME_INFO_T* pstPictureInfo,
													VDU_CODEC_T eCodecType,
													SINT32* psi32PreFramePackArrange)
{
	BOOLEAN		bRetVal = FALSE;

	switch ( eCodecType ) {
	case VDU_CODEC_H264_AVC:
		if ( pstPictureInfo->stH264Info.stSei.bFramePackingArrangemenExist == TRUE) {
			if ( pstPictureInfo->stH264Info.stSei.bFramePackingArrangementCancelFlag == TRUE) {
				pstFrameInfo->stDispInfo.si8FramePackArrange = -1;
			} else {
				pstFrameInfo->stDispInfo.si8FramePackArrange = pstPictureInfo->stH264Info.stSei.si8FramePackingArrangementType;
			}

			*psi32PreFramePackArrange = pstFrameInfo->stDispInfo.si8FramePackArrange;
		} else {
			pstFrameInfo->stDispInfo.si8FramePackArrange = *psi32PreFramePackArrange;
		}

		if ( pstFrameInfo->stDispInfo.si8FramePackArrange == 5 ) {
			if ( pstPictureInfo->stH264Info.stSei.si8ContentInterpretationType == 0 ) {
				pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_NONE;
			} else if ( pstPictureInfo->stH264Info.stSei.si8ContentInterpretationType ==		// 1 : Frame0 == left, 2 : frame0 == right
						(pstPictureInfo->stH264Info.stSei.si8CurrentFrameIsFrame0Flag + 1) ) {	// Is frame0
				pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_RIGHT; // Right Frame
			} else {
				pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_LEFT; // Left Frame
			}
		} else {
			pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_NONE;
		}
		break;

	case VDU_CODEC_H264_MVC:
		pstFrameInfo->stDispInfo.si8FramePackArrange = 5;

		if ( pstPictureInfo->stH264Info.stSei.si8MvcViewIndexDecoded == 0 ) {
			pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_LEFT; // Base view
		} else {
			pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_RIGHT; // Non base view
		}
		break;

	case VDU_CODEC_HEVC:
		if ( pstPictureInfo->stHevcInfo.stSei.bFramePackingArrangemenExist == TRUE) {
			if ( pstPictureInfo->stHevcInfo.stSei.bFramePackingArrangementCancelFlag == TRUE) {
				pstFrameInfo->stDispInfo.si8FramePackArrange = -1;
			} else {
				pstFrameInfo->stDispInfo.si8FramePackArrange = pstPictureInfo->stHevcInfo.stSei.si8FramePackingArrangementType;
			}

			*psi32PreFramePackArrange = pstFrameInfo->stDispInfo.si8FramePackArrange;
		} else {
			pstFrameInfo->stDispInfo.si8FramePackArrange = *psi32PreFramePackArrange;
		}

		if ( pstFrameInfo->stDispInfo.si8FramePackArrange == 5 ) {
			if ( pstPictureInfo->stHevcInfo.stSei.si8ContentInterpretationType == 0 ) {
				pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_NONE;
			} else if ( pstPictureInfo->stHevcInfo.stSei.si8ContentInterpretationType ==		// 1 : Frame0 == left, 2 : frame0 == right
						(pstPictureInfo->stHevcInfo.stSei.si8CurrentFrameIsFrame0Flag + 1) ) {	// Is frame0
				pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_RIGHT; // Right Frame
			} else {
				pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_LEFT; // Left Frame
			}
		} else {
			pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_NONE;
		}
		break;

	case VDU_CODEC_MPEG2:
	case VDU_CODEC_MPEG4:
	case VDU_CODEC_H263:
	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
	case VDU_CODEC_VC1_RCV_V1:
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
	case VDU_CODEC_RVX:
	case VDU_CODEC_AVS:
	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
	default:
		pstFrameInfo->stDispInfo.si8FramePackArrange = -2;
		pstFrameInfo->stDispInfo.eOrderOf3D = VDU_FRM_3D_NONE;
		break;
	}

	bRetVal = TRUE;

	return bRetVal;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* Sample Aspect Ratio(SAR) = Display Aspect Ratio(DAR) * horizontal size / vertical size
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static BOOLEAN				_SetActiveFormatDescription(VDU_FRM_INFO_T* pstFrameInfo, VDU_DEC_FRAME_INFO_T* pstPictureInfo, VDU_CODEC_T eCodecType)
{
	BOOLEAN		bRetVal = FALSE;

	switch ( eCodecType ) {
	case VDU_CODEC_MPEG2:
		pstFrameInfo->stUserData.ui8ActiveFmtDesc = pstPictureInfo->stMpeg2Info.ui8ActiveFmtDesc;
		break;

	case VDU_CODEC_H264_AVC:
	case VDU_CODEC_H264_MVC:
		pstFrameInfo->stUserData.ui8ActiveFmtDesc = pstPictureInfo->stH264Info.ui8ActiveFmtDesc;
		break;

	case VDU_CODEC_MPEG4:
	case VDU_CODEC_H263:
	case VDU_CODEC_SORENSON_SPARK:
	case VDU_CODEC_XVID:
	case VDU_CODEC_DIVX3:
	case VDU_CODEC_DIVX4:
	case VDU_CODEC_DIVX5:
	case VDU_CODEC_VC1_RCV_V1:
	case VDU_CODEC_VC1_RCV_V2:
	case VDU_CODEC_VC1_ES:
	case VDU_CODEC_RVX:
	case VDU_CODEC_AVS:
	case VDU_CODEC_THEORA:
	case VDU_CODEC_VP3:
	case VDU_CODEC_VP8:
	case VDU_CODEC_HEVC:
	default:
		pstFrameInfo->stUserData.ui8ActiveFmtDesc = 0;
		break;
	}

	bRetVal = TRUE;

	return bRetVal;}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* Sample Aspect Ratio(SAR) = Display Aspect Ratio(DAR) * horizontal size / vertical size
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static UINT8	_GetDisplayAspectRatio(UINT32 ui32DisplayWidth, UINT32 ui32DisplayHeight)
{
	UINT8	ui8RetDisplayAr = 1;
	UINT32	ui32DispAr100;

	ui32DisplayHeight = (ui32DisplayHeight == 0)? 1 : ui32DisplayHeight;
	ui32DispAr100 = 100 * ui32DisplayWidth / ui32DisplayHeight;

	if ( ui32DispAr100 >= 221 * 100 / 100  ) {		// 221 : 100
		ui8RetDisplayAr = 4;
	} else if( ui32DispAr100 >= 16 * 100 / 9  ) {	// 16 : 9
		ui8RetDisplayAr = 3;
	} else if( ui32DispAr100 >= 4 * 100 / 3  ) {	// 4 : 3
		ui8RetDisplayAr = 2;
	}

	return ui8RetDisplayAr;
}

/**
********************************************************************************
* @brief
*
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*
* @return
*
********************************************************************************
*/
static UINT32				_GetGcd(UINT32 ui32Bigger, UINT32 ui32Smaller)
{
	UINT32		ui32RetVal = 0;

	if( ui32Smaller != 0 ) {
		ui32RetVal = ((ui32Bigger % ui32Smaller) == 0)? ui32Smaller : _GetGcd(ui32Smaller, ui32Bigger % ui32Smaller);
	}

	return ui32RetVal;
}

/** @} */
