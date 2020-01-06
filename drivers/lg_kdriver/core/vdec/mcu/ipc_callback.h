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
 * main driver implementation for de device.
 * de device will teach you how to make device driver with new platform.
 *
 * author     youngki.lyu@lge.com
 * version    1.0
 * date       2011.07.30
 * note       Additional information.
 *
 */

#ifndef _IPC_NOTI_H_
#define _IPC_NOTI_H_


#include "vdec_type_defs.h"


#ifdef __cplusplus
extern "C" {
#endif



typedef struct
{
	void	*priv;
	UINT32	u32CodecType_Config;

	enum
	{
		MSVC_UPDATE_CMD_HDR_SEQINFO = 0,
		MSVC_UPDATE_CMD_HDR_PICINFO,
		MSVC_UPDATE_CMD_HDR_32bits = 0x20110714
	} eCmdHdr;

	union
	{
		struct
		{
			UINT32	u32IsSuccess;
//			UINT32	u32PicWidth;
//			UINT32	u32PicHeight;
//			UINT32	u32FrmCnt;
//			UINT32	u32MinFrameBufferCount;
//			UINT32	u32FrameBufDelay;
//			UINT32	u32AspectRatio;
//			UINT32	u32DecSeqInfo;
//			UINT32	u32NextDecodedIdxNum;
			UINT32	u32SeqHeadInfo;
//			UINT32	u32CurRdPtr;

//			UINT32	u32FrameRateRes;
//			UINT32	u32FrameRateDiv;

//			UINT32	u32VideoStandard;
		} seqInfo;

		struct
		{
			UINT32	u32DecodingSuccess;
			UINT32	u32NumOfErrMBs;
//			UINT32	u32IndexFrameDecoded;
//			UINT32	u32IndexFrameDisplay;
			UINT32	u32PicType;
			UINT32	u32LowDelay;
//			UINT32	u32PicStruct;
			UINT32	u32Aspectratio;
			UINT32	u32Stride;
			UINT32	u32PicWidth;
			UINT32	u32PicHeight;
			UINT32	u32HOffset;
			UINT32	u32VOffset;
			UINT32	u32ActiveFMT;
//			UINT32	u32DecodedFrameNum;
			UINT32	u32DecodedPTS;
//			UINT32	u32CurRdPtr;
			UINT32	u32AddrY;
			UINT32	u32AddrCb;
			UINT32	u32AddrCr;
			UINT32	ui32FrameRateRes;
			UINT32	ui32FrameRateDiv;
			UINT32	u32ProgSeq;
			UINT32	u32ProgFrame;
			SINT32	si32FrmPackArrSei;
//			SINT32	u32bPackedMode;

			struct
			{
				UINT32 	u32PicType;
				UINT32	u32Rpt_ff;
				UINT32	u32Top_ff;
				UINT32	u32BuffAddr;
				UINT32	u32Size;
				UINT32	u32Frm_idx;
			} usrData;
		} picInfo;
	} u;

	UINT32	u32uID;
	UINT64	u64Timestamp;
} S_IPC_CALLBACK_BODY_DECINFO_T;

typedef struct
{
	void	*priv;
	enum
	{
		CPB_STATUS_ALMOST_EMPTH = 0,
		CPB_STATUS_NORMAL,
		CPB_STATUS_ALMOST_FULL,
		CPB_STATUS_32bits = 0x20110921
	} eBufStatus;
} S_IPC_CALLBACK_BODY_CPBSTATUS_T;

typedef struct
{
	void	*priv;
	UINT8	bReset;
	UINT32	ui32Addr;
	UINT32	ui32Size;
} S_IPC_CALLBACK_BODY_REQUEST_CMD_T;


typedef void (*IPC_CALLBACK_DECINFO_CB_T)(S_IPC_CALLBACK_BODY_DECINFO_T *);
typedef void (*IPC_CALLBACK_CPBSTATUS_CB_T)(S_IPC_CALLBACK_BODY_CPBSTATUS_T *);
typedef void (*IPC_CALLBACK_REQUESTCMD_CB_T)(S_IPC_CALLBACK_BODY_REQUEST_CMD_T *);



BOOLEAN IPC_CALLBACK_Register_DecInfo(IPC_CALLBACK_DECINFO_CB_T fpCallback);
BOOLEAN IPC_CALLBACK_Register_CpbStatus(IPC_CALLBACK_CPBSTATUS_CB_T fpCallback);
BOOLEAN IPC_CALLBACK_Register_ReqReset(IPC_CALLBACK_REQUESTCMD_CB_T fpCallback);

void IPC_CALLBACK_Init(void);
BOOLEAN IPC_CALLBACK_Noti_DecInfo(S_IPC_CALLBACK_BODY_DECINFO_T *pDecInfo);
BOOLEAN IPC_CALLBACK_Report_CpbStatus(S_IPC_CALLBACK_BODY_CPBSTATUS_T *pCpbStatus);
BOOLEAN IPC_CALLBACK_RequestCmd_Reset(S_IPC_CALLBACK_BODY_REQUEST_CMD_T *pReqCmd);



#ifdef __cplusplus
}
#endif

#endif /* _IPC_NOTI_H_ */

