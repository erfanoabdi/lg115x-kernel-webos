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
 * video decoding unit decoder api for vdec device.
 * VDEC device will teach you how to make device driver with lg1154 platform.
 *
 * author     Youngwoo Jin(youngwoo.jin@lge.com)
 * version    1.0
 * date       2013.08.12
 * note       Additional information.
 *
 */

#ifndef _VDU_DEC_H_
#define _VDU_DEC_H_

/*------------------------------------------------------------------------------
	Control Constants
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    File Inclusions
------------------------------------------------------------------------------*/
#include "vdu_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
	Constant Definitions
------------------------------------------------------------------------------*/
#define VDU_DEC_NUM_OF_MAX_DPB			(24)

/*------------------------------------------------------------------------------
	Macro Definitions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Type Definitions
------------------------------------------------------------------------------*/
typedef enum {
	VDU_DEC_CMD_OPEN		= 0,
	VDU_DEC_CMD_CLOSE		= 1,
	VDU_DEC_CMD_RESET		= 2,
	VDU_DEC_CMD_DECODE		= 3,
	VDU_DEC_CMD_CONTINUE	= 4,
	VDU_DEC_CMD_COMPLETE	= 6,

	VDU_DEC_CMD_MIN			= VDU_DEC_CMD_OPEN,
	VDU_DEC_CMD_MAX			= VDU_DEC_CMD_COMPLETE,
	VDU_DEC_CMD_INVALID,
} VDU_DEC_CMD_T;

typedef enum {
	VDU_DEC_NPF_PAIRED		= 0,
	VDU_DEC_NPF_BOTTOM_ONLY	= 1,
	VDU_DEC_NPF_TOP_ONLY	= 2,
	VDU_DEC_NPF_NONE		= 3,

	VDU_DEC_NPF_MIN			= VDU_DEC_NPF_PAIRED,
	VUD_DEC_NPF_MAX			= VDU_DEC_NPF_NONE,
	VUD_DEC_NPF_INVALID,
} VDU_DEC_NPF_T;

typedef struct {
	UINT32					ui32PhysAddr;
	UINT32					ui32VirtAddr;
	UINT32					ui32Size;
} VDU_DEC_BUFFER_T;

typedef struct {
	UINT8					ui8NumOfFrames;
	BOOLEAN					bInterlaced;
	BOOLEAN					bLowDelay;
} VDU_DEC_SEQUENCE_T;

typedef struct {
	VDU_DEC_CMD_T			eCommand;
	UINT32					ui32ResetAddr;
	UINT32					ui32WriteAddr;
	UINT32					ui32StreamEndAddr;
	VDU_PIC_SCAN_T			ePicScanMode;
	UINT8					aui8ClearFrames[VDU_DEC_NUM_OF_MAX_DPB];
	BOOLEAN					bEnableUserData;
	VDU_LINEAR_FRAMES_T		stLinearFrames;
} VDU_DEC_RUN_PARAM_T;

typedef struct {
	UINT32					ui32Stride;
	UINT32					ui32Width;
	UINT32					ui32Height;
} VDU_DEC_FRAME_SIZE_T;

typedef struct {
	UINT32					ui32TiledBase;
	UINT32					ui32Y;
	UINT32					ui32Cb;
	UINT32					ui32Cr;
} VDU_DEC_FRAME_ADDR_T;

typedef struct {
	VDU_FRM_MAP_T			eFrameMapType;
	UINT8					ui8ErrorRate;
	UINT8					ui8PictureType;
	BOOLEAN					bFieldPicture;
	UINT32					ui32FrameRateResidual;
	UINT32					ui32FrameRateDivider;
} VDU_DEC_DISPLAY_INFO_T;

typedef struct
{
	SINT32					si32PicTimingStruct;
	BOOLEAN					bFramePackingArrangemenExist;
	BOOLEAN					bFramePackingArrangementCancelFlag;
	SINT8					si8FramePackingArrangementType;
	SINT8					si8ContentInterpretationType;
	BOOLEAN					si8CurrentFrameIsFrame0Flag;
	SINT8					si8MvcViewIndexDecoded;
} VDU_DEC_SEI_T;

typedef struct
{
	UINT8					ui8DisplayAspectRatio;
	UINT16					ui16BarLeft;
	UINT16					ui16BarRight;
	UINT16					ui16BarTop;
	UINT16					ui16BarBottom;
	UINT8					ui8PictureStructure;
	BOOLEAN					bTopFieldFirst;
	BOOLEAN					bRepeatFirstField;
	SINT32					si32DispWidth;
	SINT32					si32DispHeight;
	UINT8					ui8ActiveFmtDesc;
} VDU_DEC_MPEG2_T;

typedef struct
{
	UINT8					ui8AspectRatioInfo;
	UINT8					ui8AspectRatioWidth;
	UINT8					ui8AspectRatioHeight;
	BOOLEAN					bTopFieldFirst;
} VDU_DEC_MPEG4_T;

typedef struct
{
	UINT8					ui8AspectRatioIdc;
	UINT16					ui16AspectRatioWidth;
	UINT16					ui16AspectRatioHeight;
	UINT16					ui16CropLeft;
	UINT16					ui16CropRight;
	UINT16					ui16CropTop;
	UINT16					ui16CropBottom;
	BOOLEAN					bTopFieldFirst;
	BOOLEAN					bFixedFrameRate;
	UINT8					ui8ActiveFmtDesc;
	VDU_DEC_NPF_T			eNpfInfo;
	VDU_DEC_SEI_T			stSei;
} VDU_DEC_H264_T;

typedef struct
{
	UINT8					ui8AspectRatio;
	UINT8					ui8AspectRatioWidth;
	UINT8					ui8AspectRatioHeight;
	BOOLEAN					bTopFieldFirst;
	BOOLEAN					bRepeatFirstField;
} VDU_DEC_VC1_T;

typedef struct
{
	UINT8					ui8AspectRatio;
	UINT8					ui8AspectRatioWidth;
	UINT8					ui8AspectRatioHeight;
} VDU_DEC_RV_T;

typedef struct
{
	UINT8					ui8DisplayAspectRatio;
	BOOLEAN					bTopFieldFirst;
	BOOLEAN					bRepeatFirstField;
} VDU_DEC_AVS_T;

typedef struct
{
	VDU_DEC_SEI_T			stSei;
} VDU_DEC_HEVC_T;

typedef struct {
	UINT32					ui32Address;
	UINT32					ui32Size;
} VDU_DEC_USERDATA_T;

typedef struct
{
	VDU_DEC_FRAME_SIZE_T	stSizeInfo;
	VDU_DEC_FRAME_ADDR_T	stAddress;
	VDU_DEC_DISPLAY_INFO_T	stDispInfo;
	VDU_DEC_USERDATA_T		stUserData;

	union {
	VDU_DEC_MPEG2_T			stMpeg2Info;
	VDU_DEC_MPEG4_T			stMpeg4Info;
	VDU_DEC_H264_T			stH264Info;
	VDU_DEC_VC1_T			stVc1Info;
	VDU_DEC_RV_T			stRvInfo;
	VDU_DEC_AVS_T			stAvsInfo;
	VDU_DEC_HEVC_T			stHevcInfo;
	};
} VDU_DEC_FRAME_INFO_T;

typedef struct {
	BOOLEAN					bDecSuccess;
	VDU_NOTI_T				eNotiType;
	SINT8					si8DisplayIndex;
	SINT8					si8DecodedIndex;
	VDU_PICTURE_T			ePictureType;
	UINT32					ui32DecTime;
	BOOLEAN					bFieldSuccess;
	UINT32					ui32ReadPtr;
	VDU_DEC_FRAME_INFO_T	stPictureInfo;
} VDU_DEC_RESULT_T;

typedef struct {
	VDU_HANDLE_T			hVduHandle;
	VDU_CODEC_T				eCodecType;
	VDU_FRM_MAP_T			eFrameMapType;
	UINT32					ui32CpbBaseAddr;
	UINT32					ui32CpbEndAddr;
	BOOLEAN					bForceUhdEnabled;
	BOOLEAN					bNoDelayMode;
	BOOLEAN					bOneFrameDecoding;
	VDU_LINEAR_MODE_T		eExternLinearBufMode;
	VDU_DEC_BUFFER_T*		pstUserdataBuf;

	VDU_DEC_SEQUENCE_T		stSequence;
	void*					pPrivateData;
} VDU_DEC_INSTANCE_T;

typedef void		(*VDU_DEC_CALLBACK_FN_T)(VDU_DEC_INSTANCE_T* pstInstance, VDU_DEC_RESULT_T* pstDecResult);

typedef VDU_RET_T	(*VDU_DEC_INIT_FN_T)(VDU_DEC_CALLBACK_FN_T pfnCallback);
typedef VDU_RET_T	(*VDU_DEC_SUSPEND_FN_T)(void);
typedef VDU_RET_T	(*VDU_DEC_RESUME_FN_T)(void);
typedef VDU_RET_T	(*VDU_DEC_GET_STATUS_FN_T)(VDU_STATUS_T *peStatus, BOOLEAN* pbRunning);
typedef VDU_RET_T	(*VDU_DEC_RUN_FN_T)(VDU_DEC_INSTANCE_T* pstInstance, VDU_DEC_RUN_PARAM_T* pstRunParam);
typedef VDU_RET_T	(*VDU_DEC_GET_LINEAR_FRAME_FN_T)(VDU_DEC_INSTANCE_T* pstInstance, SINT8 si8FrameIdx, void* pFrameBuf);

typedef struct
{
	VDU_DEC_INIT_FN_T				pfnInit;
	VDU_DEC_SUSPEND_FN_T			pfnSuspend;
	VDU_DEC_RESUME_FN_T				pfnResume;
	VDU_DEC_GET_STATUS_FN_T			pfnGetStatus;
	VDU_DEC_RUN_FN_T				pfnRun;
	VDU_DEC_GET_LINEAR_FRAME_FN_T	pfnGetLinearFrame;
} VDU_DEC_OPERATIONS_T;

/*------------------------------------------------------------------------------
	Extern Function Prototype Declaration
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Extern Variables
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Generic Usage Functions
------------------------------------------------------------------------------*/
VDU_RET_T		VDU_DEC_RegisterOperations(char* pchName, VDU_DEC_OPERATIONS_T* pstOperations);
VDU_RET_T		VDU_DEC_UnregisterOperations(char* pchName);

#ifdef __cplusplus
}
#endif

#endif /* _VDU_DEC_H_ */

