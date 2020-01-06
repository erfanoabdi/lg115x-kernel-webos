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
 * author     taewoong.kim@lge.com
 * version    1.0
 * date       2013.08.09
 * note       Additional information.
 *
 * @addtogroup lg115x_vdec
 * @{
 */


/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/
#define LOG_NAME	vdec_if

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <string.h>
#include "vdec_if.h"
#include "vds/de_if_drv.h"
#include "vds/mcu_if.h"
#include "../../mmcu_pipe.h"

#include "log.h"

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/
struct _VDEC_IF_T
{
	BOOLEAN			bValidCurFrame;
	BOOLEAN			bValidNextFrame;
	S_DISPQ_BUF_T	stCurFrame;
	S_DISPQ_BUF_T	stNextFrame;
	DE_IF_DST_E	eDstDe;
	UINT32			u32SyncField;
	UINT32			u32VsyncCh;
};

typedef struct
{
    BOOLEAN         bUse;
    UINT8           ui8SyncCh;
    UINT8           ui8VdecIfCh;
    DE_IF_DST_E   eDeIfDstCh;
} VDISP_CH_TAG_T;

typedef struct
{
	VDISP_CH_TAG_T astSyncChTag[4];
} VSYNC_CH_T;


/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/
extern MMCU_PIPE_T      *pstCmdPipe, *pstReqPipe;
/*----------------------------------------------------------------------------------------
 *   global Functions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/
VDEC_IF_CH_T stVdecIfCh[VDEC_IF_NUM_OF_CHANNEL];
VSYNC_CH_T stVsyncCh[VDEC_IF_NUM_OF_CHANNEL];


void VIF_Init(void)
{
	UINT32 i;
	VDEC_IF_CH_T *pstVdecIfCh;

	for (i=0;i<VDEC_IF_NUM_OF_CHANNEL;i++)
	{
		pstVdecIfCh = &stVdecIfCh[i];

		pstVdecIfCh->bValidCurFrame = FALSE;
		pstVdecIfCh->eDstDe = 0xFF;		// invalid
		pstVdecIfCh->u32VsyncCh = 0xFF;
	}

	return;
}
void _DE_IF_Reset(UINT8 ui8DeIfCh);
VDEC_IF_CH_T* VIF_Open(UINT32 u32VdecIfCh)
{
	VDEC_IF_CH_T		*pstVdecIfCh;

	pstVdecIfCh = &stVdecIfCh[u32VdecIfCh];

	pstVdecIfCh->bValidCurFrame = FALSE;
	pstVdecIfCh->bValidNextFrame = FALSE;

	_DE_IF_Reset(u32VdecIfCh);
	DE_IF_SetPrivateData(u32VdecIfCh, (void *)u32VdecIfCh);

	return pstVdecIfCh;
}

void VIF_SelDe(UINT32 u32VdecIfCh, UINT32 u32DeCh)
{
	VDEC_IF_CH_T *pstVdecIfCh;

	log_noti("VIF %d: sel DE %d\n", u32VdecIfCh, u32DeCh);
	pstVdecIfCh = &stVdecIfCh[u32VdecIfCh];

	pstVdecIfCh->eDstDe = u32DeCh;

	DE_IF_SetTargetDe(u32VdecIfCh, u32DeCh);
	return;
}

void VIF_SelVsync(UINT32 u32VdecIfCh, UINT32 u32VsyncCh)
{
	VSYNC_CH_T			*pstVsyncCh;
	VDEC_IF_CH_T		*pstVdecIfCh;
	UINT32 i;

	log_noti("VIF %d: sel VSYNC %d\n", u32VdecIfCh, u32VsyncCh);

	pstVdecIfCh = &stVdecIfCh[u32VdecIfCh];


	if (pstVdecIfCh->u32VsyncCh != 0xFF)
	{
		pstVsyncCh = &stVsyncCh[pstVdecIfCh->u32VsyncCh];

		for (i=0;i<4;i++)
			if (pstVsyncCh->astSyncChTag[i].bUse == TRUE &&
					pstVsyncCh->astSyncChTag[i].ui8VdecIfCh == u32VdecIfCh)
			{
				log_noti("VIF %d: remove registered VIF from vsync %d\n",
						u32VdecIfCh, pstVdecIfCh->u32VsyncCh);
				pstVsyncCh->astSyncChTag[i].bUse = FALSE;
			}
	}

	pstVdecIfCh->u32VsyncCh = u32VsyncCh;

	if (u32VsyncCh != 0xFF)
	{
		pstVsyncCh = &stVsyncCh[u32VsyncCh];
		for (i=0;i<4;i++)
			if (pstVsyncCh->astSyncChTag[i].bUse == FALSE)
			{
				pstVsyncCh->astSyncChTag[i].bUse = TRUE;
				pstVsyncCh->astSyncChTag[i].ui8VdecIfCh = u32VdecIfCh;
				break;
			}

		if (i==4)
			log_error("error, no empty tag\n");
	}

	return;
}

void MMCU_SendVsyncInfo(UINT32 u32VdecIfCh, VSYNC_INFO_T *pstVsyncInfo)
{
	MMCU_PIPE_ENTRY_T stPipeEntry;

	stPipeEntry.delimiter = 0xABABABAB;
	stPipeEntry.eType = MPIPE_VSYNC_STATE;
	stPipeEntry.u32Param[0] = u32VdecIfCh;
	stPipeEntry.u32Param[1] = MCU_HAL_GetGSTC(); // STC
	stPipeEntry.size = sizeof(VSYNC_INFO_T);
	log_user1("[[%d]] ",stPipeEntry.u32Param[0]  );

	MPIPE_Write(pstReqPipe, (UINT8*)&stPipeEntry, sizeof(MMCU_PIPE_ENTRY_T));
	MPIPE_Write(pstReqPipe, (UINT8*)pstVsyncInfo, sizeof(VSYNC_INFO_T));

	return;
}

UINT32 MMCU_GetCmd(MMCU_PIPE_ENTRY_T *pstPipeEntry)
{
	UINT32 u32Ret;
	UINT8 u8Temp;

	u32Ret = MPIPE_Read(pstCmdPipe, (UINT8*)pstPipeEntry, sizeof(MMCU_PIPE_ENTRY_T));

	if( u32Ret == 0 )
		return 0;

	while( pstPipeEntry->delimiter != 0xABABABAB )
	{
		log_error("wrong delimiter %X\n", pstPipeEntry->delimiter);
		if( MPIPE_Read(pstCmdPipe, &u8Temp, 1) )
		{
			memmove((void*)pstPipeEntry, (void*)(((UINT8*)pstPipeEntry)+1), sizeof(MMCU_PIPE_ENTRY_T)-1);
			memcpy((void*)(((UINT8*)pstPipeEntry) + sizeof(MMCU_PIPE_ENTRY_T)-1), (void*)&u8Temp, 1);
		}
		else
			return 0;
	}

	return u32Ret;
}

void MMCU_SendDispState(UINT32 u32VdispCh, BOOLEAN bValidCur, BOOLEAN bValidNext, 
		UINT32 u32DispPeriod, UINT32 u32FieldCnt)
{
	MMCU_PIPE_ENTRY_T stPipeEntry;
	MCUIF_DISP_STATE	stDispStt;

	stPipeEntry.delimiter = 0xABABABAB;
	stPipeEntry.eType = MPIPE_DISP_STATE;

	stPipeEntry.u32Param[0] = u32VdispCh;
	stPipeEntry.size = sizeof(MCUIF_DISP_STATE);

	stDispStt.bCur = bValidCur;
	stDispStt.bNext = bValidNext;
	stDispStt.u32DispPeriod = u32DispPeriod;
	stDispStt.u32FieldCnt = u32FieldCnt;

	MPIPE_Write(pstReqPipe, (UINT8*)&stPipeEntry, sizeof(MMCU_PIPE_ENTRY_T));
	MPIPE_Write(pstReqPipe, (UINT8*)&stDispStt, sizeof(MCUIF_DISP_STATE));
	return;
}

void MMCU_SendClrFrame(UINT32 u32VdispCh, S_DISPQ_BUF_T *pstClrFrame)
{
	MMCU_PIPE_ENTRY_T stPipeEntry;

	stPipeEntry.delimiter = 0xABABABAB;
	stPipeEntry.eType = MPIPE_CLR_FRAME;

	stPipeEntry.u32Param[0] = u32VdispCh;
	stPipeEntry.u32Param[1] = MCU_HAL_GetGSTC(); // STC
	//printf("[[%d]] ",stPipeEntry.u32Param[0]  );

	stPipeEntry.u32Param[2] = 1; // SyncField
	stPipeEntry.size = sizeof(S_DISPQ_BUF_T);

	MPIPE_Write(pstReqPipe, (UINT8*)&stPipeEntry, sizeof(MMCU_PIPE_ENTRY_T));
	MPIPE_Write(pstReqPipe, (UINT8*)pstClrFrame, sizeof(S_DISPQ_BUF_T));

	//MPIPE_Flush(pstReqPipe);
	return;
}

void VIF_SetNextFrame(UINT32 u32VdecIfCh, S_DISPQ_BUF_T *pstNextFrame)
{
	VDEC_IF_CH_T		*pstVdecIfCh;

	pstVdecIfCh = &stVdecIfCh[u32VdecIfCh];

	if (pstVdecIfCh->bValidNextFrame)
	{
		log_warning("already set!!!!\n");
		MMCU_SendClrFrame(u32VdecIfCh, &pstVdecIfCh->stNextFrame);
	}

	pstVdecIfCh->stNextFrame = *pstNextFrame;
	pstVdecIfCh->bValidNextFrame = TRUE;

	return;
}


void do_pipe_cmd(MMCU_PIPE_ENTRY_T *pstPipeEntry)
{
	UINT32 u32VdecIfCh;

	u32VdecIfCh = pstPipeEntry->u32Param[0];

	switch(pstPipeEntry->eType)
	{
	case MMCU_PIPE_SET_REQPIPE:
	{
		MMCU_PIPE_INFO_T *pstPipeInfo;
		UINT32 u32InfoAddr;

		u32InfoAddr = pstPipeEntry->u32Param[0];
		log_noti("Set Req Pipe %X\n", u32InfoAddr);

		pstPipeInfo = (MMCU_PIPE_INFO_T *)MCU_HAL_TranslateAddrforMCU((UINT32)u32InfoAddr,
				sizeof(MMCU_PIPE_INFO_T));

		MMCU_SetReqPipe(pstPipeInfo);
	}
		break;
	case MMCU_PIPE_SET_NEWFRAME:
	{
		S_DISPQ_BUF_T		stDqFrame;

		MPIPE_Read(pstCmdPipe, (UINT8*)&stDqFrame, pstPipeEntry->size);

		//printf("%d Recv %d\n", *((UINT32*)0xF0001460), stDqFrame.ui32FrameIdx);

		//MPIPE_GetHead();
		VIF_SetNextFrame(u32VdecIfCh, &stDqFrame);
		//MPIPE_Read(pstCmdPipe, NULL, pstPipeEntry->size);
	}
		break;
	case MPIPE_INIT_CH:
		VIF_Open(u32VdecIfCh);
		break;
	case MPIPE_CLOSE_CH:
		//VIF_Close(u32VdecIfCh);
		break;
	case MPIPE_SEL_DE:
		DE_IF_SetTargetDe(u32VdecIfCh,
				(DE_IF_DST_E)pstPipeEntry->u32Param[1]);
		break;
	case MPIPE_SEL_VSYNC:
		VIF_SelVsync(u32VdecIfCh, pstPipeEntry->u32Param[1]);
		break;
	case MPIPE_SET_DUALOUT:
		DE_IF_SetDualOutput((DE_IF_DST_E)pstPipeEntry->u32Param[0],
				(DE_IF_DST_E)pstPipeEntry->u32Param[1]);
		break;
	case MPIPE_SET_REPEAT:
		DE_IF_RepeatFrame(u32VdecIfCh, pstPipeEntry->u32Param[1]);
		break;
	//case MPIPE_SET_LOGLEV:
	//	VIF_SetLogLevel();
	default:
		break;
	}
}

void VIF_PipeCallback(void)
{
	MMCU_PIPE_ENTRY_T stPipeEntry;

	if( pstCmdPipe == NULL )
		return;

	while( MPIPE_GetUsedSize(pstCmdPipe) >= sizeof(MMCU_PIPE_ENTRY_T) )
	{
		MMCU_GetCmd(&stPipeEntry);
		//printf("qsize %d EntSize %d \n", MPIPE_GetUsedSize(pstCmdPipe), stPipeEntry.size );
		do_pipe_cmd(&stPipeEntry);
	}
}

void VIF_VsyncCallback(UINT8 u8VdecIfCh, UINT32 ui32SyncField)
{
	VDEC_IF_CH_T		*pstVdecIfCh;
	VSYNC_INFO_T		stVsyncInfo;
	BOOLEAN				bReqNewFrame;
	BOOLEAN				bSetNew = FALSE;
	BOOLEAN				bEmpty = FALSE;
	BOOLEAN				bValidCur, bValidNext;
	UINT32				u32DispPeriod, u32FieldCnt;

	pstVdecIfCh = &stVdecIfCh[u8VdecIfCh];

	bReqNewFrame = DE_IF_IsNeedNewFrame(u8VdecIfCh);

	if (bReqNewFrame == TRUE)
	{
		if (pstVdecIfCh->bValidNextFrame == TRUE)
		{
			bSetNew = DE_IF_SetNewFrame(u8VdecIfCh, &pstVdecIfCh->stNextFrame, ui32SyncField);
			if (bSetNew == TRUE)
			{
				//printf("Set %d\n", pstVdecIfCh->stNextFrame.ui32FrameIdx);
				pstVdecIfCh->bValidNextFrame = FALSE;
			}
		}
		else
		{
			//printf("empty\n");
			bEmpty = TRUE;
		}
	}

	DE_IF_UpdateDisplay(u8VdecIfCh, ui32SyncField);
	DEIF_GetDispState(u8VdecIfCh, &u32DispPeriod, &u32FieldCnt,
			&bValidCur, &bValidNext);
	MMCU_SendDispState(u8VdecIfCh, bValidCur, bValidNext,
			u32DispPeriod, u32FieldCnt);

	/*
	if( bSetNew )
	{
		if( pstVdecIfCh->bValidCurFrame )
		{
			//printf("Clr %d\n", pstVdecIfCh->stCurFrame.ui32FrameIdx);
			pstVdecIfCh->stCurFrame.bDispResult = TRUE;
			MMCU_SendClrFrame(u8VdecIfCh, &pstVdecIfCh->stCurFrame);
			//bValidCurFrame = FALSE;
		}

		pstVdecIfCh->stCurFrame = pstVdecIfCh->stNextFrame;
		pstVdecIfCh->bValidCurFrame = TRUE;
	}
*/
	if (bSetNew || bEmpty)
	{
		stVsyncInfo.bEmpty = bEmpty;
		if (!bEmpty)
			stVsyncInfo.u32SyncDuration = pstVdecIfCh->stCurFrame.ui32DisplayPeriod;
		else
			stVsyncInfo.u32SyncDuration = 1;

		stVsyncInfo.u32VsyncCh = pstVdecIfCh->u32VsyncCh;
		stVsyncInfo.u32VsyncGstc = MCU_HAL_GetGSTC();

		MMCU_SendVsyncInfo(u8VdecIfCh, &stVsyncInfo);
	}
}

void vdisp_disp_done(void *priv, S_DISPQ_BUF_T *pstClrFrm)
{
	UINT8 u8VdecIfCh = (UINT8)(UINT32)priv;
	pstClrFrm->bDispResult = TRUE;
	MMCU_SendClrFrame(u8VdecIfCh, pstClrFrm);
}

void Vsync_ISR(UINT32 u32VsyncCh)
{
	VSYNC_CH_T		*pstVsyncCh;
	UINT32			i;
	UINT32			ui32SyncField;

	pstVsyncCh = &stVsyncCh[u32VsyncCh];
	ui32SyncField = VSync_HAL_IntField(u32VsyncCh);

	//printf("VSC [%d] %d\n", MCU_HAL_GetGSTC(), ui32SyncField);

	for (i=0;i<4;i++)
	{
		if (pstVsyncCh->astSyncChTag[i].bUse)
		{
			VIF_VsyncCallback(pstVsyncCh->astSyncChTag[i].ui8VdecIfCh, ui32SyncField);
		}
	}

	if (MPIPE_GetUsedSize(pstReqPipe))
		MPIPE_Flush(pstReqPipe);
}
