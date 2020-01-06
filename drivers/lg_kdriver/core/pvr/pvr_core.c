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
 *  main driver implementation for pvr device.
 *	pvr device will teach you how to make device driver with new platform.
 *
 *  author		murugan.d (murugan.d@lge.com)
 *  author		modified by ki beom kim (kibeom.kim@lge.com)
 *  version		1.0
 *  date		2010.02.05
 *  note		Additional information.
 *
 *  @addtogroup lg1150_pvr
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "os_util.h"
#include "pvr_drv.h"
#include "pvr_dev.h"
#include "pvr_reg.h"
#include "pvr_cfg.h"
#include <pvr_core_api.h>
#include <asm/io.h>			/**< ioremap() */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/* The value is modified when download buf size is modified */
UINT32 ui32DvrDnMaxPktCount = 0x3000;
UINT32 ui32DvrDnMaxBufLimit = 6;	//GP DDI uses 6 X 384K buffer currently

/* Define for reading index buffer per every 384k download buffer */
UINT32 ui32DvrDnMinBufLimit = 1; 		// DDI 384 x 1 buffer. 384K buffer , 1 EA
UINT32 ui32DvrDnMinPktCount = 0x800;	// 0x800 * 192 = 384K, so packet count is up to 0x800

#define LX_TP_PARSING_BUFFER_LIMIT 	PVR_DOWN_TS_PACKET_SIZE / 2

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *	function prototypes
 *----------------------------------------------------------------------------------------*/

void DVR_DefaultMemoryMap ( LX_PVR_CH_T	ch );


/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Global variables
----------------------------------------------------------------------------------------*/

DVR_MOD_T astDvrControl[LX_PVR_CH_NUM];
DVR_PIE_DATA_T astDvrPieData[LX_PVR_CH_NUM];
DVR_STATE_T	astDvrState[LX_PVR_CH_NUM];
DVR_BUFFER_STATE_T	astDvrBufferState[LX_PVR_CH_NUM];
extern DVR_ErrorStatus_T astDvrErrStat[];

UINT8	gTsBuffer[PVR_DOWN_TS_PACKET_SIZE*2];
PVR_SPS_T	gSPS;
LX_PVR_STREAMINFO_T gStreamInfo;

BOOLEAN		gbValidSPS = FALSE;

SINT32	gPictureNum = 0;
UINT32	gRefTimeStamp = 0;

#if 0//(SYS_ATSC)

#if 0 // support 50HZ for AR (20091124_2340_CH24_AR_Blocking.trp field stream)
static UINT32	_gH264RefFrameRate[8] = {2700, 3300, 2700, 3300, 5700, 7000, 5700, 7000 };
#define	PVR_H264_FR_INDEX_ADJUST	2
#else
static UINT32	gH264RefFrameRate[12] = {1700, 2700, 2700, 3300, 2700, 3300, 3750, 5700, 5700, 7000, 5700, 7000 };
#define	LX_PVR_H264_FR_INDEX_ADJUST	3
#endif // end support 50HZ for AR

#define	LX_PVR_H264_FR_CRITERION		(1)

#else //(SYS_DVB)

UINT32	gH264RefFrameRate[16] = {1700, 2750, 1700, 2750, 1700, 2750, 2750, 3750, 2750, 3750, 3750, 5750, 5750, 7750, 5750, 7750 };
#define	LX_PVR_H264_FR_INDEX_ADJUST		1
#define	LX_PVR_H264_FR_CRITERION		(2)
#endif


/*----------------------------------------------------------------------------------------
 *	Driver functions for PVR IOCTL calls
 *----------------------------------------------------------------------------------------*/


/**
 * PVR_DD_Init
 * note:
 * @def PVR_IOW_INIT
 * Initialize pvr module.
 * Set PVR to default status.
 */
UINT32 PVR_DD_Init (LX_PVR_CH_T	 ch)
{
	/* Initialize the registers with default value */
	DVR_UP_ResetBlock (ch);
	DVR_UP_EnableReg(ch, FALSE, 192);
	DVR_DN_ResetBlock (ch);
	DVR_DN_EnableReg(ch, FALSE);
	DVR_PIE_ResetBlock (ch);

	//The interrupt level configuration is used for PIE buffer number calculation
	//DVR_DN_MIN_BUF_CNT = 1, DVR_DN_MIN_PKT_CNT = 0x800
	//Now 384K 1 Buffer, 192byte packet 0x800 = 384K
	DVR_DN_ConfigureIntrLevel(ch, DVR_DN_MIN_BUF_CNT, DVR_DN_MIN_PKT_CNT);

	// Set PIE_MODE Register
	// 1 : Interrupt only when index table is full (32 entries, PIE Bufer Size is 32)
	// 2 : Interrupt when index table full or packet limit --> Now Use This Mode!
	DVR_PIE_SetModeReg(ch, 2/* 1 */);

	/* Initialize default buffer base addresses */
	DVR_DefaultMemoryMap(ch);

	astDvrState[ch].eUpState = LX_PVR_UP_STATE_IDLE;

	return PVR_OK;
}

/**
 * PVR_DD_Terminate
 * note:
 * @def PVR_IOW_TERMINATE
 * Terminate pvr module.
 */

UINT32	PVR_DD_Terminate ( void )
{
	/**/
	return PVR_OK;
}



/**
 * PVR_DD_StartDownload
 * note:
 * @def PVR_IOW_DN_START
 * Start PVR download for specified PVR channel
 */

UINT32	PVR_DD_StartDownload ( LX_PVR_CH_T	ch)
{
	UINT32 rtrn = PVR_OK;

	if ( astDvrState[ch].eDnState != LX_PVR_DN_STATE_IDLE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_DN> One recording already active in [%d] channel, State[%d]\n",
			ch,
			(UINT32)astDvrState[ch].eDnState );
		/* Stop the previous download and restart again, but the problem has to be fixed in middleware !!!*/
		PVR_DD_StopDownload(ch);
		/*return -1;*/
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_DN> Restarting download[%d]channel !!!!\n",
			ch );
	}
	//Reset download block
	rtrn |= DVR_DN_ResetBlock(ch);

	/* Reset the error variables */
	astDvrErrStat[ch].ui32DnUnitBuf = 0;
	astDvrErrStat[ch].ui32DnOverFlowErr = 0;

	//Configure base address & write pointer
	rtrn |= DVR_DN_SetBufBoundReg(ch,
		astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase,
		astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd );
	astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr = astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase;
	astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr = astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase;

	//Configure write pointer
	rtrn |= DVR_DN_SetWritePtrReg(ch,
		astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase );

    rtrn |= DVR_DN_EnableINTReg(ch, TRUE);
    rtrn |= DVR_DN_PauseReg(ch, FALSE);
    rtrn |= DVR_DN_EnableReg(ch, TRUE);

	astDvrState[ch].eDnState = LX_PVR_DN_STATE_RECORD;

	gStreamInfo.bFoundSeqSPS = FALSE;
	gStreamInfo.bitRate = 0;
	gStreamInfo.frRate = LX_PVR_FR_UNKNOWN;
	gPictureNum = 0;
	gRefTimeStamp = 0;
	gbValidSPS = FALSE;

	return rtrn;
}

/**
 * PVR_DD_StopDownload
 * note:
 * @def PVR_IOW_DN_STOP
 * Stop PVR download for specified PVR channel
 */

UINT32	PVR_DD_StopDownload ( LX_PVR_CH_T	ch)
{
	UINT32 rtrn = PVR_OK;

	if ( astDvrState[ch].eDnState == LX_PVR_DN_STATE_IDLE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_DN> Recording not active in [%d]channel, State[%d]\n",
			ch,
			(UINT32)astDvrState[ch].eDnState );
		//rtrn = -1;
		/* return -1; */
		/* Still go down and clear all the registers and state */
	}

    rtrn |= DVR_DN_EnableReg(ch, FALSE);
    rtrn |= DVR_DN_EnableINTReg(ch, FALSE);

	//Disable PIE interrupt also
	rtrn |= DVR_PIE_EnableINTReg(ch, FALSE);

	//Also clear the PIE pid register
	DVR_DN_SetPIDReg(ch, 0x1FFF);

	astDvrState[ch].eDnState = LX_PVR_DN_STATE_IDLE;
	astDvrState[ch].ePieState = LX_PVR_PIE_STATE_IDLE;

	return rtrn;
}

/**
 * PVR_DD_StopDownloadSDT
 * note:
 * @def PVR_IOW_DN_STOP
 * Stop PVR download for specified PVR channel
 * jinhwan.bae 20140124, to support H13 UHD DVB download and SDT
 */

UINT32	PVR_DD_StopDownloadSDT ( LX_PVR_CH_T	ch)
{
	UINT32 rtrn = PVR_OK;

	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		/* just return, not available from M14B0 */
		return rtrn;
	}

	if ( astDvrState[ch].eDnState == LX_PVR_DN_STATE_IDLE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_DN> Recording not active in [%d]channel, State[%d]\n",
			ch,
			(UINT32)astDvrState[ch].eDnState );
		//rtrn = -1;
		/* return -1; */
		/* Still go down and clear all the registers and state */
	}

    rtrn |= DVR_DN_EnableReg(ch, FALSE);
    rtrn |= DVR_DN_EnableINTReg(ch, FALSE);

#if 0 // jinhwan.bae here is differenct points from normal stop download - index related one
	//Disable PIE interrupt also
	rtrn |= DVR_PIE_EnableINTReg(ch, FALSE);

	//Also clear the PIE pid register
	DVR_DN_SetPIDReg(ch, 0x1FFF);
#endif

	// jinhwan.bae PIE block reset needed
	// if not, PIE module is not working after SDT cancel 20140124
	DVR_PIE_ResetBlock(ch);

	astDvrState[ch].eDnState = LX_PVR_DN_STATE_IDLE;
	astDvrState[ch].ePieState = LX_PVR_PIE_STATE_IDLE;

	return rtrn;
}


/**
 * PVR_DD_SetDownloadbuffer
 * note:
 * @def PVR_IOW_DN_SET_BUF
 * Set the buffer base, end, read/write pointers for download buffer
 */
UINT32	PVR_DD_SetDownloadbuffer (LX_PVR_CH_T ch, UINT32 uiBufAddr, UINT32 uiBufSize)
{
	UINT32 rtrn = PVR_OK;
	UINT32 ptrBase;
    UINT32 ptrEnd;

	ptrBase = uiBufAddr;
	ptrEnd = uiBufAddr + uiBufSize;

	/* Set DN_BUF_SPTR, EPTR, WPTR, not RPTR, actually no RPTR in Register Map */
	DVR_DN_SetBufBoundReg(ch, ptrBase, ptrEnd);
	DVR_DN_SetWritePtrReg(ch, ptrBase );

	/* Set Buffer DB with Physical Address */
	astDvrMemMap[ch].stDvrDnBuff.ui32BufferBase = ptrBase;
	astDvrMemMap[ch].stDvrDnBuff.ui32BufferEnd 	= ptrEnd;
	astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr 	= ptrBase;
	astDvrMemMap[ch].stDvrDnBuff.ui32WritePtr 	= ptrBase;

#if 1 // Check it jinhwan.bae M14_TBD
	/*
	 * The GP DDI expects the packet index to wrap around along
	 * with the size of the download buffer, so set the packet limit
	 * same as the buffer size. The download buffer size has to be multiple of 192
	 */
	 ui32DvrDnMaxPktCount = uiBufSize/192;	//Currently this variable is not used
#else
	ui32DvrDnMaxBufLimit = 6;
	ui32DvrDnMaxPktCount = (uiBufSize/DVR_DN_MAX_BUF_CNT)/192;
#endif /* #if 0 */

	return rtrn;
}

/**
 * PVR_DD_SetPiebuffer
 * note:
 * @def PVR_IOW_PIE_SET_BUF
 * Set the buffer base, end, read/write pointers for PIE buffer
 * The buffer is only a user virtual address and managed by the PVR kdriver
 */

UINT32	PVR_DD_SetPiebuffer ( LX_PVR_CH_T	ch, UINT32 uiBufAddr, UINT32 uiBufSize)
{
	UINT32 rtrn = PVR_OK;
	UINT32 ptrBase;
    UINT32 ptrEnd;

	ptrBase = uiBufAddr;
	ptrEnd = uiBufAddr + uiBufSize;

	/* Memorize the buffer base, end and read/write pointers */
	/* 2012. 12. 22 jinhwan.bae Change from mmaped virtual to physical. Pie Buffer Setting in Kdriver should be ioremapped address, deleted mmaped address operation */
#if 0
	astDvrMemMap[ch].stPieUserVirtBuff.ui32BufferBase = ptrBase;
	astDvrMemMap[ch].stPieUserVirtBuff.ui32BufferEnd 	= ptrEnd;
	astDvrMemMap[ch].stPieUserVirtBuff.ui32ReadPtr 	= ptrBase;
	astDvrMemMap[ch].stPieUserVirtBuff.ui32WritePtr 	= ptrBase;
#else
	if( (astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase != uiBufAddr)
		|| ((astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd - astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase) != uiBufSize) )
	{
		PVR_KDRV_LOG ( PVR_WARNING, "[WARN]PIE Buffer Setting! ch[%d] Prev[0x%x]Size[%d] Now[0x%x]Size[%d]\n", ch,
			astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase, (astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd - astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase),
			uiBufAddr, uiBufSize);
		astDvrMemMap[ch].stPiePhyBuff.ui32BufferBase = ptrBase;
		astDvrMemMap[ch].stPiePhyBuff.ui32BufferEnd  = ptrEnd;
		astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr 	 = ptrBase;
		astDvrMemMap[ch].stPiePhyBuff.ui32WritePtr 	 = ptrBase;

		PVR_KDRV_LOG ( PVR_WARNING, "[WARN]iounmapped previous ioremap pointer \n");
		if(astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase != 0)
		{
			iounmap((void*)astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase);
		}

		PVR_KDRV_LOG ( PVR_WARNING, "[WARN]ioremap with new address ch[%d] \n", ch);
		astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase = (UINT32) ioremap(uiBufAddr, uiBufSize);
		astDvrMemMap[ch].stPieMappedBuff.ui32BufferEnd  = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase  + uiBufSize;
		astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
		astDvrMemMap[ch].stPieMappedBuff.ui32WritePtr	= astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase;
		astDvrMemMap[ch].si32PieVirtOffset = astDvrMemMap[ch].stPieMappedBuff.ui32BufferBase - uiBufAddr;
	}
#endif

	return rtrn;
}


/**
 * PVR_DD_GetDownloadWriteAddr
 * note:
 * @def PVR_IOR_DN_GET_WRITE_ADD
 * Get the buffer write pointer for download buffer
 */

UINT32	PVR_DD_GetDownloadWriteAddr ( LX_PVR_CH_T	ch, UINT32 *pui32WritePtr)
{
	UINT32 rtrn = PVR_OK;

	//Get the write pointer
	rtrn = DVR_DN_GetWritePtrReg(ch, pui32WritePtr );

	return rtrn;
}

/**
 * PVR_DD_SetDownloadReadAddr
 * note:
 * @def PVR_IOW_DN_SET_CONFIG
 * Set the buffer read pointer for download buffer. This is used for overflow detection
 */

UINT32	PVR_DD_SetDownloadReadAddr( LX_PVR_CH_T	ch, UINT32 ui32ReadPtr)
{
	UINT32 rtrn = PVR_OK;

	astDvrMemMap[ch].stDvrDnBuff.ui32ReadPtr = ui32ReadPtr;

	return rtrn;
}

/**
 * PVR_DD_SetPieReadAddr
 * note:
 * @def PVR_IOW_DN_SET_CONFIG
 * Set the buffer read pointer for pie buffer. This is used for overflow detection
 */

UINT32	PVR_DD_SetPieReadAddr( LX_PVR_CH_T	ch, UINT32 ui32ReadPtr)
{
	UINT32 rtrn = PVR_OK;

	/* 2012. 12. 22 jinhwan.bae Change from mmaped virtual to physical. Pie Buffer Setting in Kdriver should be ioremapped address, deleted mmaped address operation */
	/* Input Should Be Physical Address */
#if 0
	astDvrMemMap[ch].stPieUserVirtBuff.ui32ReadPtr = ui32ReadPtr;
#else
	astDvrMemMap[ch].stPiePhyBuff.ui32ReadPtr = ui32ReadPtr;
	astDvrMemMap[ch].stPieMappedBuff.ui32ReadPtr = ui32ReadPtr + astDvrMemMap[ch].si32PieVirtOffset;
#endif

	return rtrn;
}


/**
 * PVR_DD_GetUploadReadAddr
 * note:
 * @def PVR_IOR_UP_GET_READ_ADD
 * Get the buffer write pointer for upload buffer
 */

UINT32	PVR_DD_GetUploadReadAddr ( LX_PVR_CH_T	ch, UINT32 *pui32WritePtr, UINT32 *pui32ReadPtr)
{
	UINT32 rtrn = PVR_OK;

	DVR_UP_GetPointersReg(
						ch,
						pui32WritePtr,
						pui32ReadPtr );

	return rtrn;
}



/**
 * PVR_DD_SetUploadbuffer
 * note:
 * @def PVR_IOW_UP_SET_BUF
 * Set the buffer base, end, read/write pointers for upload buffer
 */

UINT32	PVR_DD_SetUploadbuffer ( LX_PVR_CH_T	ch, UINT32 uiBufAddr, UINT32 uiBufSize)
{
	UINT32 rtrn = PVR_OK;
	UINT32 ptrBase;
    UINT32 ptrEnd;

	ptrBase = uiBufAddr;
	ptrEnd = uiBufAddr + uiBufSize;

	//Set buffer bound based on start and end
	DVR_UP_SetBufBoundReg(ch, ptrBase, ptrEnd);

	//set the write pointer to the start of buffer
	DVR_UP_SetWritePtrReg(ch, ptrBase);

	//Set the read pointer to the start of buffer
	DVR_UP_SetReadPtrReg(ch, ptrBase);

	DVR_UP_ResetBlock(ch);	//Reset the upload block for the new bound to take effect

	/* Memorize the buffer base, end and read/write pointers */
	astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase	= ptrBase;
	astDvrMemMap[ch].stDvrUpBuff.ui32BufferEnd	= ptrEnd;
	astDvrMemMap[ch].stDvrUpBuff.ui32ReadPtr	= ptrBase;
	astDvrMemMap[ch].stDvrUpBuff.ui32WritePtr	= ptrBase;

	return rtrn;
}

/**
 * PVR_DD_RestartUpload
 * note:
 * @def PVR_IOW_UP_RESTART
 * Set the buffer base, end, read/write pointers for upload buffer
 */

UINT32	PVR_DD_RestartUpload ( LX_PVR_CH_T	ch, UINT32 uiBufAddr, UINT32 uiBufSize)
{
	UINT32 rtrn = PVR_OK;

	/* Disable the Upload, so that the readptr doesn't move */
	DVR_UP_EnableReg(ch, FALSE, astDvrControl[ch].ui8PacketLen);

	//To reset the buffer read and write pointers
	PVR_DD_SetUploadbuffer ( ch, uiBufAddr, uiBufSize);

	//After resetting this pointers, enable the upload during the next data transfer
	astDvrControl[ch].ui8UploadFirstTime = TRUE;

	return rtrn;
}
/*
 * PVR_DD_ReSetUpload
 * note:
 * @def PVR_IOW_UP_RESET
 * Reset the buffer base, end, read/write pointers for upload buffer
 */

UINT32	PVR_DD_ReSetUpload ( LX_PVR_CH_T	ch )
{
	UINT32 rtrn = PVR_OK;
	UINT32 ptrBase;

	ptrBase = astDvrMemMap[ch].stDvrUpBuff.ui32BufferBase;

	/* Murugan-18/11/2010 - Added the reset block code to reset the read and write pointers to the base */
	DVR_UP_ResetBlock(ch);	//Reset the upload block for the new bound to take effect

	/* Disable the Upload, so that the readptr doesn't move */
	DVR_UP_EnableReg(ch, FALSE, astDvrControl[ch].ui8PacketLen);

	//set the write pointer to the start of buffer
	DVR_UP_SetWritePtrReg(ch, ptrBase);

	//Set the read pointer to the start of buffer
	DVR_UP_SetReadPtrReg(ch, ptrBase);

	//After resetting this pointers, enable the upload during the next data transfer
	astDvrControl[ch].ui8UploadFirstTime = TRUE;

	return rtrn;
}

/**
 * PVR_DD_UpUploadbuffer
 * note:
 * @def PVR_IOW_UP_UPLOAD_BUFFER
 * Set the new write pointers for upload buffer
 */

UINT32	PVR_DD_UpUploadbuffer ( LX_PVR_CH_T	ch, UINT32 ui32WritePtr)
{
	//set the write pointer to the start of buffer
	DVR_UP_SetWritePtrReg(ch, ui32WritePtr);

	/* Clear the IACK register to acknowledge the overlap error due to under flow */
	if (DVR_UP_GetErrorStat(ch))
	{
		DVR_UP_ClearIACKReg(ch, 4);	//Clear only err stat
		/* Maintain the underflow status for the middleware to know */
		astDvrBufferState[ch].eUpBufState = LX_PVR_BUF_STAT_Empty;
	}
	else
	{
		//Clear the state back to normal if no error
		astDvrBufferState[ch].eUpBufState = LX_PVR_BUF_STAT_Ready;
	}

	if ( astDvrControl[ch].ui8UploadFirstTime )
	{
		DVR_UP_EnableReg(ch, TRUE, astDvrControl[ch].ui8PacketLen);	//Memorized earlier
		astDvrControl[ch].ui8UploadFirstTime = FALSE;
	}

	return PVR_OK;
}

/**
 * PVR_DD_UpMode
 * note:
 * @def PVR_IOW_UP_MODE
 * Set the new upload mode
 */

UINT32	PVR_DD_UpMode( LX_PVR_CH_T ch, LX_PVR_UP_MODE_T eMode)
{
	if (ch > LX_PVR_CH_MAX)
	{
		PVR_KDRV_LOG ( PVR_ERROR, "ioctl: Bad parameter for trickmode!!\n");
		return -EFAULT;
	}
#if 1
    if ( 1 )	// TSC Disable method
	{
		switch (eMode)
		{
			case LX_PVR_UPMODE_NORMAL :

				//DVR_EnableWaitCycle(ch,0x0);
                DVR_EnableWaitCycle(ch,0x2000);
				/* Clear the TSC Disable register for normal play */
				DVR_UP_TimeStampCheckDisableReg (ch, FALSE);
				break;
			case LX_PVR_UPMODE_TRICK_MODE:

				DVR_EnableWaitCycle(ch,0x2000);
				/* Enable the TSC Disable register for trick mode play */
				DVR_UP_TimeStampCheckDisableReg (ch, TRUE);
				break;
		}
	}
#else
	if ( 1 )	// TSC Disable method
	{
		switch (eMode)
		{
			case LX_PVR_UPMODE_NORMAL :

				//DVR_EnableWaitCycle(0x0);
                DVR_EnableWaitCycle(0x7000);
				/* Clear the TSC Disable register for normal play */
				DVR_UP_TimeStampCheckDisableReg (ch, FALSE);
				break;
			case LX_PVR_UPMODE_TRICK_MODE:

				DVR_EnableWaitCycle(0x2000);
				/* Enable the TSC Disable register for trick mode play */
				DVR_UP_TimeStampCheckDisableReg (ch, TRUE);
				break;
		}
	}
#endif	
	else if ( 0 )		//PLAY_MOD = 100 Trickmode method
	{
		switch (eMode)
		{
			case LX_PVR_UPMODE_NORMAL :
				/* Upload playmode is set to normal playmode value 000 */
				DVR_UP_ChangePlaymode(ch, LX_PVR_UPMODE_NORMAL);
				break;
			case LX_PVR_UPMODE_TRICK_MODE:
				/* Change playmode to trick playmode 100 (4) */
				//Need to check if default AL_JITTER value has to be changed or not !!!
				DVR_UP_ChangePlaymode(ch, 4);
				break;
		}
	}
	else if ( 0 )		//PLAY_MOD variable according to different speed
	{
		switch (eMode)
		{
			case LX_PVR_UPMODE_NORMAL :
			case LX_PVR_UPMODE_TRICK_MODE:
				/*
				 * Change playmode to the trick mode value passed, the enum value is matching the
				  * register field definition
				  */
				DVR_UP_ChangePlaymode(ch, (UINT8) eMode);
				break;
		}
	}

	return PVR_OK;
}


/**
 * PVR_DD_StartUpload
 * note:
 * @def PVR_IOW_UP_START
 * Stop PVR upload for specified PVR channel
 */

UINT32	PVR_DD_StartUpload ( LX_PVR_CH_T	ch, UINT8 ui8PacketLen)
{
	UINT32 retrn = 0;

	if ( astDvrState[ch].eUpState != LX_PVR_UP_STATE_IDLE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_UP> One playback already active in [%d]channel, State[%d]\n",
			ch,
			(UINT32)astDvrState[ch].eUpState );
		/* return -1; */
		/* Previous upload seem to be active, so close it and restart again */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_UP> Restarting playback[%d]channel !!!!\n",
			ch );
		PVR_DD_StopUpload(ch);
	}

	/* Reset the error variables */
	astDvrErrStat[ch].ui32UpAlmostEmpty = 0;
	astDvrErrStat[ch].ui32UpOverlapErr = 0;

	PVR_KDRV_LOG ( PVR_UPLOAD, "> Start Upload [%c] setting...\n", ch ? 'B' : 'A' );
    retrn |= DVR_UP_PauseReg(ch, FALSE);
	/*
	 * Disable Almost empty and almost full interrupt
	 * handling by passing argument as FALSE
	 */
	retrn |= DVR_UP_EnableEmptyLevelReg(ch, FALSE);
	/*
	 * The packet length is memorized, and used when the upload
	 * is started after copying first block of data
	 */
	astDvrControl[ch].ui8PacketLen = ui8PacketLen;
//    retrn |= DVR_UP_EnableReg(ch, TRUE, ui8PacketLen);
	//This flag is for enabling the upload after data copy for first time
	astDvrControl[ch].ui8UploadFirstTime = TRUE;

	astDvrState[ch].eUpState = LX_PVR_UP_STATE_PLAY;

	return retrn;
}

/**
 * PVR_DD_StopUpload
 * note:
 * @def PVR_IOW_UP_STOP
 * Stop PVR upload for specified PVR channel
 */

UINT32	PVR_DD_StopUpload ( LX_PVR_CH_T	ch)
{
	UINT32 retrn;

	if ( astDvrState[ch].eUpState == LX_PVR_UP_STATE_IDLE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_UP> Playback not active in [%d]channel, State[%d]\n",
			ch,
			(UINT32)astDvrState[ch].eUpState );
		return -1;
	}

	retrn  = DVR_UP_EnableEmptyLevelReg(ch, FALSE);	//Disable the interrupt
	retrn |= DVR_UP_EnableReg(ch, FALSE, 188);	//Packet len doesn't matter
	/* Clear the Pause state just in case */
	DVR_UP_PauseReg ( ch, FALSE );

	astDvrState[ch].eUpState = LX_PVR_UP_STATE_IDLE;
	return retrn;
}

/**
 * PVR_DD_PauseUpload
 * note:
 * @def PVR_IOW_UP_PAUSE
 * Pause PVR upload for specified PVR channel
 */

UINT32	PVR_DD_PauseUpload ( LX_PVR_CH_T ch )
{
	UINT32 retrn;

	if ( astDvrState[ch].eUpState == LX_PVR_UP_STATE_IDLE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_UP> Playback not active in [%d]channel, State[%d]\n",
			ch,
			(UINT32)astDvrState[ch].eUpState );
		return -1;
	}

	/* Change to Pause state */
	retrn  = DVR_UP_PauseReg ( ch, TRUE );

	astDvrState[ch].eUpState = LX_PVR_UP_STATE_PAUSE;
	return retrn;
}

/**
 * PVR_DD_ResumeUpload
 * note:
 * @def PVR_IOW_UP_PAUSE
 * Resume PVR upload for specified PVR channel
 */

UINT32	PVR_DD_ResumeUpload ( LX_PVR_CH_T ch )
{
	UINT32 retrn;

	if ( astDvrState[ch].eUpState != LX_PVR_UP_STATE_PAUSE )
	{
		/* One recording already going on */
		PVR_KDRV_LOG ( PVR_WARNING, "DVR_UP> Playback not in Pause state-[%d]channel, State[%d] but Set TCP\n",
			ch,
			(UINT32)astDvrState[ch].eUpState );
		/* Change to Pause state */
		DVR_UP_PauseReg ( ch, FALSE );
		return -1;
	}

	/* Change to Pause state */
	retrn  = DVR_UP_PauseReg ( ch, FALSE );

	astDvrState[ch].eUpState = LX_PVR_UP_STATE_PLAY;
	return retrn;
}

UINT32 PVR_DD_EnableTimeStampCopy(UINT8 ch, BOOLEAN bEnable)
{
	UINT32 rtrn = PVR_OK;

	switch(ch)
	{
		case 0:
			if ( astDvrState[LX_PVR_CH_A].eUpState == LX_PVR_UP_STATE_PLAY )
			{
				DVR_UP_TimeStampCopyReg(LX_PVR_CH_A, bEnable);
			}
			break;
		case 1:
			if ( astDvrState[LX_PVR_CH_B].eUpState == LX_PVR_UP_STATE_PLAY )
			{
				DVR_UP_TimeStampCopyReg(LX_PVR_CH_B, bEnable);
			}
			break;
		default :
			rtrn = PVR_FAILURE;
	}
	return rtrn;
}

UINT32 DVR_SetDownloadPID(LX_PVR_CH_T ch, UINT32 ui32Pid, LX_PVR_PIE_TYPE_T ePieType)
{
	UINT32 rtrn = PVR_OK;

	DVR_PIE_ResetBlock(ch);

	//The interrupt level configuration is used for PIE buffer number calculation
	// Set the index INT level to 384K x 1 download buffer.
	DVR_DN_ConfigureIntrLevel(ch, DVR_DN_MIN_BUF_CNT, DVR_DN_MIN_PKT_CNT);

	/*
	 * PIE Mode:1 -> Interrupt only when index table is full (32 entries)
	 * PIE Mode:2 -> Interrupt when index table full and packet limit
	 */
	DVR_PIE_SetModeReg(ch, 2/* 1 */);

	switch ( ePieType )
	{
		case LX_PVR_PIE_TYPE_MPEG2TS:
			/* Disable GSCD, Legacy mode MP2 PIE enabled */
			DVR_PIE_EnableSCDReg(ch, FALSE);
			break;
		case LX_PVR_PIE_TYPE_H264TS:
		case LX_PVR_PIE_TYPE_OTHERS:
			/* Not supported in middleware & IOCTL layer as of now */
			/* Check Point M14_TBD, H14_TBD, jinhwan.bae - not implemented yet??? */
			PVR_KDRV_LOG ( PVR_PIE,  "PIE for H.264 not implemented yet !!!!!\n\n\n" );
			DVR_PIE_EnableSCDReg(ch, TRUE);
			/* MPEG-4/H.264 */
			DVR_PIE_ConfigureSCD(ch, 0, 0xFFFFFF1F, 0x00000107, TRUE ); //SPS
			DVR_PIE_ConfigureSCD(ch, 1, 0xFFFFFF1F, 0x00000105, FALSE ); //IDR picture
			DVR_PIE_ConfigureSCD(ch, 2, 0xFFFFFF1F, 0x00000109, TRUE ); //AU Delimeter
			DVR_PIE_ConfigureSCD(ch, 3, 0xFFFFFF1F, 0x00000106, FALSE ); //SEI
			break;
		default:
			break;
	}

    rtrn |= DVR_DN_SetPIDReg(ch, ui32Pid);
	rtrn |= DVR_PIE_EnableINTReg (ch, TRUE);
	return rtrn;
}


/**
 * PVR_DD_GetUploadReadAddr
 * note:
 * @def PVR_IOW_MM_Init
 * Get the buffer write pointer for upload buffer
 */

UINT32	PVR_DD_MemoryInit (LX_PVR_GPB_INFO_T *stpLXPvrGPBInfo)
{
	UINT32 rtrn = PVR_OK;

	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
        stpLXPvrGPBInfo->uiDnBase = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
		stpLXPvrGPBInfo->uiDnSize = gMemCfgDvr[2][LX_PVR_MEM_DN].size;

		stpLXPvrGPBInfo->uiUpBase = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
		stpLXPvrGPBInfo->uiUpSize = gMemCfgDvr[2][LX_PVR_MEM_UP].size;

		stpLXPvrGPBInfo->uiUpBufBase = gMemCfgDvr[2][LX_PVR_MEM_UPBUF].base;
		stpLXPvrGPBInfo->uiUpBufSize = gMemCfgDvr[2][LX_PVR_MEM_UPBUF].size;

		stpLXPvrGPBInfo->uiPieBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
		stpLXPvrGPBInfo->uiPieSize = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;

		stpLXPvrGPBInfo->uiDnBase1 = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
		stpLXPvrGPBInfo->uiDnSize1 = gMemCfgDvr[2][LX_PVR_MEM_DN1].size;

		stpLXPvrGPBInfo->uiUpBase1 = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
		stpLXPvrGPBInfo->uiUpSize1 = gMemCfgDvr[2][LX_PVR_MEM_UP1].size;

		stpLXPvrGPBInfo->uiUpBufBase1 = gMemCfgDvr[2][LX_PVR_MEM_UPBUF1].base;
		stpLXPvrGPBInfo->uiUpBufSize1 = gMemCfgDvr[2][LX_PVR_MEM_UPBUF1].size;

		stpLXPvrGPBInfo->uiPieBase1 = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
		stpLXPvrGPBInfo->uiPieSize1 = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
        stpLXPvrGPBInfo->uiDnBase = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
		stpLXPvrGPBInfo->uiDnSize = gMemCfgDvr[1][LX_PVR_MEM_DN].size;

		stpLXPvrGPBInfo->uiUpBase = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
		stpLXPvrGPBInfo->uiUpSize = gMemCfgDvr[1][LX_PVR_MEM_UP].size;

		stpLXPvrGPBInfo->uiUpBufBase = gMemCfgDvr[1][LX_PVR_MEM_UPBUF].base;
		stpLXPvrGPBInfo->uiUpBufSize = gMemCfgDvr[1][LX_PVR_MEM_UPBUF].size;

		stpLXPvrGPBInfo->uiPieBase = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
		stpLXPvrGPBInfo->uiPieSize = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size;

		stpLXPvrGPBInfo->uiDnBase1 = gMemCfgDvr[1][LX_PVR_MEM_DN].base;
		stpLXPvrGPBInfo->uiDnSize1 = gMemCfgDvr[1][LX_PVR_MEM_DN].size;

		stpLXPvrGPBInfo->uiUpBase1 = gMemCfgDvr[1][LX_PVR_MEM_UP].base;
		stpLXPvrGPBInfo->uiUpSize1 = gMemCfgDvr[1][LX_PVR_MEM_UP].size;

		stpLXPvrGPBInfo->uiUpBufBase1 = gMemCfgDvr[1][LX_PVR_MEM_UPBUF].base;
		stpLXPvrGPBInfo->uiUpBufSize1 = gMemCfgDvr[1][LX_PVR_MEM_UPBUF].size;

		stpLXPvrGPBInfo->uiPieBase1 = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].base;
		stpLXPvrGPBInfo->uiPieSize1 = gMemCfgDvr[1][LX_PVR_MEM_PIEBUF].size;
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
        stpLXPvrGPBInfo->uiDnBase = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
		stpLXPvrGPBInfo->uiDnSize = gMemCfgDvr[2][LX_PVR_MEM_DN].size;

		stpLXPvrGPBInfo->uiUpBase = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
		stpLXPvrGPBInfo->uiUpSize = gMemCfgDvr[2][LX_PVR_MEM_UP].size;

		stpLXPvrGPBInfo->uiUpBufBase = gMemCfgDvr[2][LX_PVR_MEM_UPBUF].base;
		stpLXPvrGPBInfo->uiUpBufSize = gMemCfgDvr[2][LX_PVR_MEM_UPBUF].size;

		stpLXPvrGPBInfo->uiPieBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
		stpLXPvrGPBInfo->uiPieSize = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;

		stpLXPvrGPBInfo->uiDnBase1 = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
		stpLXPvrGPBInfo->uiDnSize1 = gMemCfgDvr[2][LX_PVR_MEM_DN1].size;

		stpLXPvrGPBInfo->uiUpBase1 = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
		stpLXPvrGPBInfo->uiUpSize1 = gMemCfgDvr[2][LX_PVR_MEM_UP1].size;

		stpLXPvrGPBInfo->uiUpBufBase1 = gMemCfgDvr[2][LX_PVR_MEM_UPBUF1].base;
		stpLXPvrGPBInfo->uiUpBufSize1 = gMemCfgDvr[2][LX_PVR_MEM_UPBUF1].size;

		stpLXPvrGPBInfo->uiPieBase1 = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
		stpLXPvrGPBInfo->uiPieSize1 = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
        stpLXPvrGPBInfo->uiDnBase = gMemCfgDvr[2][LX_PVR_MEM_DN].base;
		stpLXPvrGPBInfo->uiDnSize = gMemCfgDvr[2][LX_PVR_MEM_DN].size;

		stpLXPvrGPBInfo->uiUpBase = gMemCfgDvr[2][LX_PVR_MEM_UP].base;
		stpLXPvrGPBInfo->uiUpSize = gMemCfgDvr[2][LX_PVR_MEM_UP].size;

		stpLXPvrGPBInfo->uiUpBufBase = gMemCfgDvr[2][LX_PVR_MEM_UPBUF].base;
		stpLXPvrGPBInfo->uiUpBufSize = gMemCfgDvr[2][LX_PVR_MEM_UPBUF].size;

		stpLXPvrGPBInfo->uiPieBase = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].base;
		stpLXPvrGPBInfo->uiPieSize = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF].size;

		stpLXPvrGPBInfo->uiDnBase1 = gMemCfgDvr[2][LX_PVR_MEM_DN1].base;
		stpLXPvrGPBInfo->uiDnSize1 = gMemCfgDvr[2][LX_PVR_MEM_DN1].size;

		stpLXPvrGPBInfo->uiUpBase1 = gMemCfgDvr[2][LX_PVR_MEM_UP1].base;
		stpLXPvrGPBInfo->uiUpSize1 = gMemCfgDvr[2][LX_PVR_MEM_UP1].size;

		stpLXPvrGPBInfo->uiUpBufBase1 = gMemCfgDvr[2][LX_PVR_MEM_UPBUF1].base;
		stpLXPvrGPBInfo->uiUpBufSize1 = gMemCfgDvr[2][LX_PVR_MEM_UPBUF1].size;

		stpLXPvrGPBInfo->uiPieBase1 = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].base;
		stpLXPvrGPBInfo->uiPieSize1 = gMemCfgDvr[2][LX_PVR_MEM_PIEBUF1].size;
	}

	return rtrn;
}

UINT32 PVR_DD_GetTPHeader(UINT8 *pSingleTP, PVR_TP_HEADER_INFO_T *pTPHeader)
{
	UINT32	adaptationSize = 0;
	UINT32	offset = 0;

	UINT8	*pStart;

	if (pSingleTP == NULL || pTPHeader == NULL)
	{
		PVR_KDRV_LOG ( PVR_ERROR, "Invalid argument!\n");
		return PVR_FAILURE;
	}

	pStart = pSingleTP;

	if (*(pSingleTP + 4) == PVR_TP_SYNC_BYTE)
	{
		pSingleTP += 4;
		offset = PVR_DOWN_PACKET_SIZE;
	}
	else
	{
		PVR_KDRV_LOG ( PVR_ERROR, "Sync. byte error\n");
		return PVR_FAILURE;
	}

	pTPHeader->sync_byte = *(pSingleTP++);
	pTPHeader->transport_error = GET_TRANSPORT_ERROR(pSingleTP);
	pTPHeader->payload_unit_start = GET_PAYLOAD_UNIT_START(pSingleTP);
	pTPHeader->transport_priority = GET_TRANSPORT_PRIORITY(pSingleTP);
	pTPHeader->pid = ((*(pSingleTP++) & PID_HIGH_BYTE_MASK) << PID_HIGH_BYTE_SHIFT);
	pTPHeader->pid |= ((*(pSingleTP++) & PID_LOW_BYTE_MASK) >> PID_LOW_BYTE_SHIFT);
	pTPHeader->scrambling_control = GET_SCRAMBLING_CONTROL(pSingleTP);
	pTPHeader->adaptation_field_control = GET_ADAPTATION_FIELD_CONTROL(pSingleTP);
	pTPHeader->continuity_counter = GET_CONTINUITY_COUNTER(pSingleTP);

	pSingleTP++;

	#ifdef DEBUG_H264_HEADER_PASER
	PVR_KDRV_LOG( PVR_INFO, "Sync. Byte [0x%x]\n", pTPHeader->sync_byte);
	PVR_KDRV_LOG( PVR_INFO, "Transport error [%1u]  Payload unit start [%1u] Priority [%1u]\n",
			pTPHeader->transport_error, pTPHeader->payload_unit_start, pTPHeader->transport_priority);
	PVR_KDRV_LOG( PVR_INFO, "PID [0x%x]  Scrambling [%2u]  Adaptation [%2u]  CC [%u]\n",
			pTPHeader->pid, pTPHeader->scrambling_control, pTPHeader->adaptation_field_control, pTPHeader->continuity_counter);
	#endif

	if ((pTPHeader->adaptation_field_control == NO_ADAPTATION_FIELD) || (pTPHeader->adaptation_field_control == NO_ADAPTATION_FIELD_RSRV))
	{
		offset = (UINT32)(pSingleTP - pStart);
	}
	else if (pTPHeader->adaptation_field_control == ADAPTATION_FIELD_PAYLOAD)
	{
		adaptationSize = (UINT32)*pSingleTP;
		offset = (UINT32)(++pSingleTP - pStart) + adaptationSize;
	}

	#ifdef DEBUG_H264_HEADER_PASER
	PVR_KDRV_LOG( PVR_INFO,"Start address [%p]  Current address [%p] Adaptation field length [%u]\n",
			pStart, pSingleTP, adaptationSize);
	PVR_KDRV_LOG( PVR_INFO,"Payload offset is [%u]\n", offset);
	#endif

	pTPHeader->byteOffsetPayload = offset;

	return PVR_OK;
}


UINT32 PVR_DD_RemoveEP3Bytes(UINT8 *pPayload, UINT32 size)
{
	UINT32	matched_cnt;
	UINT32	loopI;

	BOOLEAN	bFound;

	matched_cnt = 0;
	bFound = FALSE;

	if (pPayload != NULL)
	{
		for( loopI = 0; loopI < size; loopI++ )
		{
			switch (*(pPayload+loopI))
			{
				case 0x00 :
					if (matched_cnt != 2) matched_cnt++;
					break;
				case 0x03 :
					if (matched_cnt == 2) bFound = TRUE;
					else matched_cnt = 0;
					break;
				default :
					matched_cnt = 0;
					break;
			}

			if (bFound)
			{
				memcpy(pPayload + loopI, pPayload + loopI +1, size - loopI - 1);
				memset(pPayload + size - 1, 0xFF, 1);

				bFound = FALSE;
				matched_cnt = 0;
			}
		}
	}

	return PVR_OK;
}

UINT32 PVR_DD_ReadNBits(PVR_BIT_STREAM_T *pBitStreamData, UINT32 nBits)
{
	UINT32	result = 0;
	UINT32	validBitSize;
	UINT32	startBitPos;

	if (pBitStreamData != NULL && pBitStreamData->pData != NULL)
	{
		if (nBits > PVR_BYTE_SIZE*4)
		{
			PVR_KDRV_LOG( PVR_ERROR ,"Can't read more than 32 bits!!\n");
			return result;
		}

		if (nBits == 0)
		{
			return result;
		}

		if (pBitStreamData->dataSize <= 0)
		{
			PVR_KDRV_LOG( PVR_ERROR ,"No data!!\n");
			return result;
		}

		validBitSize = PVR_BYTE_SIZE*3 + pBitStreamData->startBitPos + 1;

		PVR_GETWORD(pBitStreamData->pData, result);

		startBitPos = PVR_BYTE_SIZE*3 + pBitStreamData->startBitPos;

		if (nBits <= validBitSize)
		{
			result = PVR_READ_N_BITS(result, startBitPos, nBits);
			PVR_UPDATE_BITSTREAM_BUFFER(pBitStreamData->pData, pBitStreamData->startBitPos, nBits, pBitStreamData->dataSize);
		}
		else
		{
			result = PVR_READ_N_BITS(result, startBitPos, validBitSize);
			PVR_UPDATE_BITSTREAM_BUFFER(pBitStreamData->pData, pBitStreamData->startBitPos, validBitSize, pBitStreamData->dataSize);

			validBitSize = nBits - validBitSize;

			result = result << validBitSize;

			result |= PVR_DD_ReadNBits(pBitStreamData, validBitSize);
		}

		pBitStreamData->bitSize = nBits;
	}

	return result;
}

static UINT32 PVR_DD_ParseExpGolombCode(PVR_BIT_STREAM_T	*pBitStream, PVR_EXP_GOLOMB_MODE_T mode)
{
	SINT32	leadingZeroBits;

	UINT32	codeNum = 0;
	UINT32	result = 0;
	UINT32	bitsVal;
	UINT32	bitSize = 0;

	/* Check arguments */
	if (pBitStream && pBitStream->pData)
	{
		if (pBitStream->dataSize <= 0)
		{
			PVR_KDRV_LOG( PVR_ERROR ,"No data!!\n");
			return result;
		}

		leadingZeroBits = -1;
		bitSize = 0;

		for (bitsVal = 0; !bitsVal; leadingZeroBits++)
		{
			bitsVal = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;
		}

		codeNum = (0x01 << leadingZeroBits) - 1 + PVR_DD_ReadNBits(pBitStream, leadingZeroBits);
		bitSize += pBitStream->bitSize;

		pBitStream->bitSize = bitSize;

		switch (mode)
		{
			case EXP_GOLOMB_SE :
				result = codeNum / 2;
				if ( codeNum % 2 )
				{
					result++;
				}
				else
				{
					result = (UINT32)( -1 * (SINT32)result );
				}
				break;

			case EXP_GOLOMB_ME :
				// TODO: Improve me for me(v)
			case EXP_GOLOMB_TE :
				// TODO: Improve me for te(v)
			case EXP_GOLOMB_UE :
			default :
				result = codeNum;
				break;
		}
	}

	return result;
}

UINT32 PVR_DD_ParseHRD(PVR_BIT_STREAM_T *pBitStream, PVR_HRD_T *pHRD)
{
	UINT32	loopVar;
	UINT32	bitSize = 0;

	if (pBitStream != NULL && pBitStream->pData != NULL && pHRD != NULL)
	{
		memset(pHRD, 0x00, sizeof(*pHRD));

		pHRD->ccm1 =PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		pHRD->brs = PVR_DD_ReadNBits(pBitStream, 4);
		bitSize += pBitStream->bitSize;

		pHRD->css = PVR_DD_ReadNBits(pBitStream, 4);
		bitSize += pBitStream->bitSize;

		for (loopVar = 0; loopVar <= pHRD->ccm1; loopVar++)
		{
			pHRD->brvm1[loopVar] = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pHRD->csvm1[loopVar] = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pHRD->cFlag[loopVar] = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;
		}

		pHRD->icrdlm1 = PVR_DD_ReadNBits(pBitStream, 5);
		pHRD->crdlm1 = PVR_DD_ReadNBits(pBitStream, 5);
		pHRD->dodlm1 = PVR_DD_ReadNBits(pBitStream, 5);
		pHRD->tol = PVR_DD_ReadNBits(pBitStream, 5);

		pBitStream->bitSize = bitSize + 20;
	}

	return PVR_OK;
}

UINT32 PVR_DD_ParseScalingList(PVR_BIT_STREAM_T *pBitStream, UINT32 slSize, SINT32 slArray[])
{
	UINT32	loopVar;
	UINT32	bitSize;

	SINT8	deltaScale;
	SINT8	lastScale = 8;
	SINT8	nextScale = 8;

	if (pBitStream != NULL && pBitStream->pData != NULL && slArray != NULL)
	{
		bitSize = 0;

		for (loopVar = 0; loopVar < slSize; loopVar++)
		{
			if (nextScale != 0)
			{
				deltaScale = (SINT8)PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_SE);
				nextScale = (lastScale + deltaScale + 256) % 256;
			}

			slArray[loopVar] = (nextScale == 0) ? lastScale : nextScale;

			lastScale = (SINT8)slArray[loopVar];

			bitSize += pBitStream->bitSize;
		}

		pBitStream->bitSize = bitSize;
	}

	return PVR_OK;
}

UINT32 PVR_DD_ParsePICTiming(PVR_BIT_STREAM_T *pBitStream, PVR_PIC_TIMING_T *pPICTIME)
{
	UINT32	bitSize;
	UINT32	loopVar;

	if (pBitStream != NULL && pBitStream->pData != NULL && pPICTIME != NULL)
	{
		bitSize = 0;

		if (pPICTIME->cddpFlag != 0)
		{
			pPICTIME->crd = PVR_DD_ReadNBits(pBitStream, pPICTIME->crdlm1+1);
			bitSize += pBitStream->bitSize;

			pPICTIME->dod= PVR_DD_ReadNBits(pBitStream, pPICTIME->dodlm1+1);
			bitSize += pBitStream->bitSize;
		}

		if (pPICTIME->pspf != 0)
		{
			pPICTIME->ps = PVR_DD_ReadNBits(pBitStream, 4);
			bitSize += pBitStream->bitSize;

			switch (pPICTIME->ps)
			{
				case 0 :
				case 1 :
				case 2 :
					pPICTIME->numClockTS = 1;
					break;

				case 3 :
				case 4 :
				case 7 :
					pPICTIME->numClockTS = 2;
					break;

				case 5 :
				case 6 :
				case 8 :
					pPICTIME->numClockTS = 3;
					break;

				default :
					pPICTIME->numClockTS = 0;
					break;
			}

			for (loopVar = 0; loopVar < pPICTIME->numClockTS; loopVar++)
			{
				pPICTIME->ctFlag[loopVar] = PVR_DD_ReadNBits(pBitStream, 1);
				bitSize += pBitStream->bitSize;

				if (pPICTIME->ctFlag[loopVar] != 0)
				{
					pPICTIME->ct = PVR_DD_ReadNBits(pBitStream, 2);
					bitSize += pBitStream->bitSize;

					pPICTIME->nfbFlag = PVR_DD_ReadNBits(pBitStream, 1);
					bitSize += pBitStream->bitSize;

					pPICTIME->cntt = PVR_DD_ReadNBits(pBitStream, 5);
					bitSize += pBitStream->bitSize;

					pPICTIME->ftFlag = PVR_DD_ReadNBits(pBitStream, 1);
					bitSize += pBitStream->bitSize;

					pPICTIME->dFlag = PVR_DD_ReadNBits(pBitStream, 1);
					bitSize += pBitStream->bitSize;

					pPICTIME->cdFlag = PVR_DD_ReadNBits(pBitStream, 1);
					bitSize += pBitStream->bitSize;

					pPICTIME->nf = PVR_DD_ReadNBits(pBitStream, 8);
					bitSize += pBitStream->bitSize;

					if (pPICTIME->ftFlag != 0)
					{
						pPICTIME->sv = PVR_DD_ReadNBits(pBitStream, 6);
						bitSize += pBitStream->bitSize;

						pPICTIME->mv = PVR_DD_ReadNBits(pBitStream, 6);
						bitSize += pBitStream->bitSize;

						pPICTIME->hv = PVR_DD_ReadNBits(pBitStream, 5);
						bitSize += pBitStream->bitSize;
					}
					else
					{
						pPICTIME->sFlag = PVR_DD_ReadNBits(pBitStream, 1);
						bitSize += pBitStream->bitSize;

						if (pPICTIME->sFlag != 0)
						{
							pPICTIME->sv1 = PVR_DD_ReadNBits(pBitStream, 6);
							bitSize += pBitStream->bitSize;

							pPICTIME->mFlag = PVR_DD_ReadNBits(pBitStream, 1);
							bitSize += pBitStream->bitSize;

							if (pPICTIME->mFlag != 0)
							{
								pPICTIME->mv1 = PVR_DD_ReadNBits(pBitStream, 6);
								bitSize += pBitStream->bitSize;

								pPICTIME->hFlag = PVR_DD_ReadNBits(pBitStream, 1);
								bitSize += pBitStream->bitSize;

								if (pPICTIME->hFlag != 0)
								{
									pPICTIME->hv1 = PVR_DD_ReadNBits(pBitStream, 5);
									bitSize += pBitStream->bitSize;
								}
							}
						}
					}

					if (pPICTIME->tol > 0)
					{
						pPICTIME->to = PVR_DD_ReadNBits(pBitStream, pPICTIME->tol);
						bitSize += pBitStream->bitSize;
					}
				}
			}
		}

		pBitStream->bitSize = bitSize;
	}

	return PVR_OK;
}

UINT32 PVR_DD_ParseVUI(PVR_BIT_STREAM_T *pBitStream, PVR_VUI_T *pVUI)
{
	UINT32	bitSize;

	if (pBitStream != NULL && pBitStream->pData != NULL && pVUI != NULL)
	{
		memset(pVUI, 0x00, sizeof(*pVUI));
		bitSize = 0;

		pVUI->aripf = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pVUI->aripf != 0)
		{
			pVUI->arIdc = PVR_DD_ReadNBits(pBitStream, 8);
			bitSize += pBitStream->bitSize;

			if (pVUI->arIdc == 0xff)	/* Extended_SAR */
			{
				pVUI->sw = PVR_DD_ReadNBits(pBitStream, 16);
				bitSize += pBitStream->bitSize;

				pVUI->sh = PVR_DD_ReadNBits(pBitStream, 16);
				bitSize += pBitStream->bitSize;
			}
		}

		pVUI->oipFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pVUI->oipFlag != 0)
		{
			pVUI->oaFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;
		}

		pVUI->vstpFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pVUI->vstpFlag != 0)
		{
			pVUI->vf = PVR_DD_ReadNBits(pBitStream, 3);
			bitSize += pBitStream->bitSize;

			pVUI->vfrFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;

			pVUI->cdpFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;

			if (pVUI->cdpFlag != 0)
			{
				pVUI->cp = PVR_DD_ReadNBits(pBitStream, 8);
				bitSize += pBitStream->bitSize;

				pVUI->tc = PVR_DD_ReadNBits(pBitStream, 8);
				bitSize += pBitStream->bitSize;

				pVUI->mc = PVR_DD_ReadNBits(pBitStream, 8);
				bitSize += pBitStream->bitSize;
			}
		}

		pVUI->clipFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pVUI->clipFlag != 0)
		{
			pVUI->cslttf = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pVUI->vdltbf = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;
		}

		pVUI->tipFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;
		PVR_KDRV_LOG( PVR_INFO ,"^R^pVUI->tipFlag [ %u ] bitSize [%d]\n", pVUI->tipFlag, bitSize);

		if (pVUI->tipFlag != 0)
		{
			pVUI->nuit = PVR_DD_ReadNBits(pBitStream, 32);
			bitSize += pBitStream->bitSize;

			pVUI->ts = PVR_DD_ReadNBits(pBitStream, 32);
			bitSize += pBitStream->bitSize;

			pVUI->ffrFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;
		}

		pVUI->nhppFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;
		PVR_KDRV_LOG( PVR_INFO ,"^R^pVUI->nhppFlag [ %u ] bitSize [%d]\n", pVUI->nhppFlag, bitSize);

		if (pVUI->nhppFlag != 0)
		{
			PVR_DD_ParseHRD(pBitStream, &pVUI->nhrdp);
			bitSize += pBitStream->bitSize;
		}

		pVUI->vhppFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;
		PVR_KDRV_LOG( PVR_INFO ,"^R^pVUI->vhppFlag [ %u ] bitSize [%d]\n", pVUI->vhppFlag, bitSize);

		if (pVUI->vhppFlag != 0)
		{
			PVR_DD_ParseHRD(pBitStream, &pVUI->vhrdp);
			bitSize += pBitStream->bitSize;
		}

		if (pVUI->nhppFlag != 0 || pVUI->vhppFlag != 0)
		{
			pVUI->ldhFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;
		}

		pVUI->pspFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;
		PVR_KDRV_LOG( PVR_INFO ,"^R^pVUI->pspFlag [ %u ] bitSize [%d]\n", pVUI->pspFlag, bitSize);

		pVUI->brFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;
		PVR_KDRV_LOG( PVR_INFO ,"^R^pVUI->brFlag [ %u ] bitSize [%d]\n", pVUI->brFlag, bitSize);

		if (pVUI->brFlag != 0)
		{
			pVUI->mvopbFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;

			pVUI->mbppd = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pVUI->mbpmd = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pVUI->lmmlh = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pVUI->lmmlv = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pVUI->nrf = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pVUI->mdfb = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;
		}

		pBitStream->bitSize = bitSize;
	}

	return PVR_OK;
}

UINT32 PVR_DD_GetSEIHeader(PVR_BIT_STREAM_T *pBitStream, PVR_SEI_HEADER_T *pSEIHeader)
{
	UINT32	payloadSize;
	UINT32	bitSize;

	UINT8	next8Bits = PVR_H264_SEI_INVALID;

	bitSize = 0;

	if (pSEIHeader != NULL && pBitStream != NULL)
	{
		while((pBitStream->dataSize > 0) && ((next8Bits = PVR_DD_ReadNBits(pBitStream, 8)) == 0xFF))
		{
			pSEIHeader->payloadType += next8Bits;
			bitSize += pBitStream->bitSize;
		}

		bitSize += pBitStream->bitSize;

		pSEIHeader->payloadType += next8Bits;

		next8Bits = 0;

		while((pBitStream->dataSize > 0) && ((next8Bits = PVR_DD_ReadNBits(pBitStream, 8)) == 0xFF))
		{
			pSEIHeader->payloadSize += next8Bits;
			bitSize += pBitStream->bitSize;
		}

		bitSize += pBitStream->bitSize;

		pSEIHeader->payloadSize = next8Bits;

		payloadSize = 0;

		memcpy(pSEIHeader->payload, pBitStream->pData, pSEIHeader->payloadSize);

		pBitStream->pData += pSEIHeader->payloadSize;
		pBitStream->dataSize -= pSEIHeader->payloadSize;

		bitSize += pSEIHeader->payloadSize * PVR_BYTE_SIZE;

		pBitStream->bitSize = bitSize;
	}

	return PVR_OK;
}

UINT32 PVR_DD_ParseSPS(PVR_BIT_STREAM_T *pBitStream, PVR_SPS_T *pSPS)
{
	UINT32	bitSize;
	UINT32	loopVar;

	if (pBitStream != NULL && pBitStream->pData != NULL && pSPS != NULL)
	{
		memset(pSPS, 0x00, sizeof(*pSPS));
		bitSize = 0;

		pSPS->profileIdc = PVR_DD_ReadNBits(pBitStream, 8);
		bitSize += pBitStream->bitSize;

		pSPS->cs0flag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->cs1flag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->cs2flag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->cs3flag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->cs4flag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->rsv1 = PVR_DD_ReadNBits(pBitStream, 3);
		bitSize += pBitStream->bitSize;

		pSPS->levelIdc = PVR_DD_ReadNBits(pBitStream, 8);
		bitSize += pBitStream->bitSize;

		pSPS->spsID = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		if (pSPS->profileIdc == 100 || pSPS->profileIdc == 110 ||
			pSPS->profileIdc == 122 || pSPS->profileIdc == 244 || pSPS->profileIdc == 44 ||
			pSPS->profileIdc == 83 || pSPS->profileIdc == 86 || pSPS->profileIdc == 118)
		{

			pSPS->cfIDC = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			if (pSPS->cfIDC == 3)
			{
				pSPS->scpFlag = PVR_DD_ReadNBits(pBitStream, 1);
				bitSize += pBitStream->bitSize;
			}

			pSPS->bdlm8 = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pSPS->bdcm8 = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pSPS->qyztbFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;

			pSPS->ssmpFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;

			if (pSPS->ssmpFlag != 0)
			{
				for (loopVar = 0; loopVar < ((pSPS->cfIDC != 3) ? 8 : 12); loopVar++)
				{
					pSPS->sslpFlag[loopVar] = PVR_DD_ReadNBits(pBitStream, 1);
					bitSize += pBitStream->bitSize;

					if (pSPS->sslpFlag[loopVar] != 0)
					{
						if (loopVar < 6)
						{
							PVR_DD_ParseScalingList(pBitStream, 16, pSPS->sclist4x4[loopVar]);
							bitSize += pBitStream->bitSize;
						}
						else
						{
							PVR_DD_ParseScalingList(pBitStream, 64, pSPS->sclist8x8[loopVar-6]);
							bitSize += pBitStream->bitSize;
						}
					}
				}
			}
		}

		pSPS->lmfnm4 = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		pSPS->poct = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		if (pSPS->poct == 0)
		{
			pSPS->lmpoclm4 = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;
		}
		else if (pSPS->poct == 1)
		{
			pSPS->dpoazFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;

			pSPS->ofnrp = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_SE);
			bitSize += pBitStream->bitSize;

			pSPS->ofttbf = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_SE);
			bitSize += pBitStream->bitSize;

			pSPS->nrfipocc = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			for (loopVar = 0; loopVar < pSPS->nrfipocc; loopVar++)
			{
				pSPS->ofrf[loopVar] = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_SE);
				bitSize += pBitStream->bitSize;
			}
		}

		pSPS->mnrf = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		pSPS->gifnvaFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->pwimm1 = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		pSPS->phimum1 = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
		bitSize += pBitStream->bitSize;

		pSPS->fmoFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pSPS->fmoFlag == 0)
		{
			pSPS->maffFlag = PVR_DD_ReadNBits(pBitStream, 1);
			bitSize += pBitStream->bitSize;
		}

		pSPS->d8iFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		pSPS->fcFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pSPS->fcFlag != 0)
		{
			pSPS->fclo = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pSPS->fcro = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pSPS->fcto = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;

			pSPS->fcbo = PVR_DD_ParseExpGolombCode(pBitStream, EXP_GOLOMB_UE);
			bitSize += pBitStream->bitSize;
		}

		pSPS->vuippFlag = PVR_DD_ReadNBits(pBitStream, 1);
		bitSize += pBitStream->bitSize;

		if (pSPS->vuippFlag != 0)
		{
			PVR_DD_ParseVUI(pBitStream, &pSPS->vuip);
			bitSize += pBitStream->bitSize;
		}

		pBitStream->bitSize = bitSize;

		//PVR_DN_PRINTF("[%s] profileIdc = %d, levelIdc = %d, spsId = %d, refFrmCnt = %d, vuippFlag = %d\n\n",
		//				__FUNCTION__, pSPS->profileIdc, pSPS->levelIdc, pSPS->spsID, pSPS->mnrf, pSPS->vuippFlag);
	}

	return PVR_OK;
}

void PVR_DD_DumpHRD(PVR_HRD_T *pHRD)
{
	if (0)
	{
		PVR_KDRV_LOG( PVR_INFO ,"cpb_cnt_minus1 [ %u ]\n",pHRD->ccm1);			/* cpb_cnt_minus1 */
		PVR_KDRV_LOG( PVR_INFO ,"bit_rate_scale [ %u ]\n",pHRD->brs);		/* bit_rate_scale */
		PVR_KDRV_LOG( PVR_INFO ,"cpb_size_scale [ %u ]\n",pHRD->css);		/* cpb_size_scale */
		PVR_KDRV_LOG( PVR_INFO ,"initial_cpb_removal_delay_length_minus1 [ %u ]\n",pHRD->icrdlm1);	/* initial_cpb_removal_delay_length_minus1 */
		PVR_KDRV_LOG( PVR_INFO ,"cpb_removal_delay_length_minus1 [ %u ]\n",pHRD->crdlm1);		/* cpb_removal_delay_length_minus1 */
		PVR_KDRV_LOG( PVR_INFO ,"dpb_output_delay_length_minus1 [ %u ]\n",pHRD->dodlm1);		/* dpb_output_delay_length_minus1 */
		PVR_KDRV_LOG( PVR_INFO ,"time_offset_length [ %u ]\n",pHRD->tol);		/* time_offset_length */
	}
}

void PVR_DD_DumpVUI(PVR_VUI_T *pVUI)
{
	if (0)
	{
		PVR_KDRV_LOG( PVR_INFO ,"aspect_ratio_info_present_flag [ %u ]\n", pVUI->aripf);			/* aspect_ratio_info_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"aspect_ratio_idc [ %u ]\n", pVUI->arIdc);			/* aspect_ratio_idc */
		PVR_KDRV_LOG( PVR_INFO ,"sar_width [ %u ]\n", pVUI->sw);				/* sar_width */
		PVR_KDRV_LOG( PVR_INFO ,"sar_height [ %u ]\n", pVUI->sh);				/* sar_height */

		PVR_KDRV_LOG( PVR_INFO ,"overscan_info_present_flag [ %u ]\n", pVUI->oipFlag);	/* overscan_info_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"overscan_appropriate_flag [ %u ]\n", pVUI->oaFlag);		/* overscan_appropriate_flag */
		PVR_KDRV_LOG( PVR_INFO ,"video_signal_type_present_flag [ %u ]\n", pVUI->vstpFlag);	/* video_signal_type_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"video_format [ %u ]\n", pVUI->vf);			/* video_format */
		PVR_KDRV_LOG( PVR_INFO ,"video_full_range_flag [ %u ]\n", pVUI->vfrFlag);	/* video_full_range_flag */
		PVR_KDRV_LOG( PVR_INFO ,"colour_description_present_flag [ %u ]\n", pVUI->cdpFlag);	/* colour_description_present_flag */

		PVR_KDRV_LOG( PVR_INFO ,"colour_primaries [ %u ]\n", pVUI->cp);				/* colour_primaries */
		PVR_KDRV_LOG( PVR_INFO ,"transfer_characteristics [ %u ]\n", pVUI->tc);				/* transfer_characteristics */
		PVR_KDRV_LOG( PVR_INFO ,"matrix_coefficients [ %u ]\n", pVUI->mc);				/* matrix_coefficients */

		PVR_KDRV_LOG( PVR_INFO ,"chroma_loc_info_present_flag [ %u ]\n", pVUI->clipFlag);		/* chroma_loc_info_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"chroma_sample_loc_type_top_field [ %u ]\n", pVUI->cslttf);			/* chroma_sample_loc_type_top_field */
		PVR_KDRV_LOG( PVR_INFO ,"chroma_sample_loc_type_bottom_field [ %u ]\n", pVUI->vdltbf);			/* chroma_sample_loc_type_bottom_field */

		PVR_KDRV_LOG( PVR_INFO ,"timing_info_present_flag [ %u ]\n", pVUI->tipFlag);		/* timing_info_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"num_units_in_tick [ %u ]\n", pVUI->nuit);			/* num_units_in_tick */
		PVR_KDRV_LOG( PVR_INFO ,"^B^time_scale [ %u ]\n", pVUI->ts);				/* time_scale */
		PVR_KDRV_LOG( PVR_INFO ,"fixed_frame_rate_flag [ %u ]\n", pVUI->ffrFlag);		/* fixed_frame_rate_flag */

		PVR_KDRV_LOG( PVR_INFO ,"nal_hrd_parameters_present_flag [ %u ]\n", pVUI->nhppFlag);		/* nal_hrd_parameters_present_flag */
		PVR_DD_DumpHRD(& pVUI->nhrdp);		/* nal hrd_parameters */

		PVR_KDRV_LOG( PVR_INFO ,"vcl_hrd_parameters_present_flag [ %u ]\n", pVUI->vhppFlag);		/* vcl_hrd_parameters_present_flag */
		PVR_DD_DumpHRD(& pVUI->vhrdp);		/* vcl hrd_parameters */
		PVR_KDRV_LOG( PVR_INFO ,"low_delay_hrd_flag [ %u ]\n", pVUI->ldhFlag);		/* low_delay_hrd_flag */
		PVR_KDRV_LOG( PVR_INFO ,"pic_struct_present_flag [ %u ]\n", pVUI->pspFlag);		/* pic_struct_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"bitstream_restriction_flag [ %u ]\n", pVUI->brFlag);			/* bitstream_restriction_flag */

		PVR_KDRV_LOG( PVR_INFO ,"motion_vectors_over_pic_bondaries_flag [ %u ]\n", pVUI->mvopbFlag);		/* motion_vectors_over_pic_bondaries_flag */
		PVR_KDRV_LOG( PVR_INFO ,"max_bytes_per_pic_denom [ %u ]\n", pVUI->mbppd);			/* max_bytes_per_pic_denom */
		PVR_KDRV_LOG( PVR_INFO ,"max_bits_per_mb_denom [ %u ]\n", pVUI->mbpmd);			/* max_bits_per_mb_denom */
		PVR_KDRV_LOG( PVR_INFO ,"log2_max_mv_length_horizontal [ %u ]\n", pVUI->lmmlh);			/* log2_max_mv_length_horizontal */
		PVR_KDRV_LOG( PVR_INFO ,"log2_max_mv_length_vertical [ %u ]\n", pVUI->lmmlv);			/* log2_max_mv_length_vertical */
		PVR_KDRV_LOG( PVR_INFO ,"num_reorder_frames [ %u ]\n", pVUI->nrf);			/* num_reorder_frames */
		PVR_KDRV_LOG( PVR_INFO ,"max_dec_frame_buffering [ %u ]\n", pVUI->mdfb);			/* max_dec_frame_buffering */
	}
}

void PVR_DD_DumpSPS(PVR_SPS_T *pSPS)
{
	UINT32 loopVar;

	if (0)
	{
		PVR_KDRV_LOG( PVR_INFO ,"^R^profileIdc [ %u ]\n", pSPS->profileIdc);
		PVR_KDRV_LOG( PVR_INFO ,"constraint_set0_flag [ %u ]\n", pSPS->cs0flag);
		PVR_KDRV_LOG( PVR_INFO ,"constraint_set1_flag [ %u ]\n", pSPS->cs1flag);
		PVR_KDRV_LOG( PVR_INFO ,"constraint_set2_flag [ %u ]\n", pSPS->cs2flag);
		PVR_KDRV_LOG( PVR_INFO ,"constraint_set3_flag [ %u ]\n", pSPS->cs3flag);
		PVR_KDRV_LOG( PVR_INFO ,"constraint_set4_flag [ %u ]\n", pSPS->cs4flag);

		PVR_KDRV_LOG( PVR_INFO ,"levelIdc [ %u ]\n", pSPS->levelIdc);
		PVR_KDRV_LOG( PVR_INFO ,"seq_parameter_set_id [ %u ]\n", pSPS->spsID);
		PVR_KDRV_LOG( PVR_INFO ,"chroma_format_idc [ %u ]\n", pSPS->cfIDC);
		PVR_KDRV_LOG( PVR_INFO ,"separate_colour_plane_flag [ %u ]\n", pSPS->scpFlag);
		PVR_KDRV_LOG( PVR_INFO ,"bit_depth_luma_minus8 [ %u ]\n", pSPS->bdlm8);
		PVR_KDRV_LOG( PVR_INFO ,"bit_depth_chroma_minus8 [ %u ]\n", pSPS->bdcm8);

		PVR_KDRV_LOG( PVR_INFO ,"qpprime_y_zero_transform_bypass_flag [ %u ]\n", pSPS->qyztbFlag);
		PVR_KDRV_LOG( PVR_INFO ,"seq_scaling_matrix_present_flag [ %u ]\n", pSPS->ssmpFlag);

		for (loopVar = 0; loopVar < ((pSPS->cfIDC != 3) ? 8 : 12); loopVar++)
		{
			PVR_KDRV_LOG( PVR_INFO ,"seq_scaling_list_present_flag%d [ %u ]\n", loopVar,  pSPS->sslpFlag[loopVar]);
		}
		PVR_KDRV_LOG( PVR_INFO ,"log2_max_frame_num_minus4 [ %u ]\n", pSPS->lmfnm4);
		PVR_KDRV_LOG( PVR_INFO ,"pic_order_cnt_type [ %u ]\n", pSPS->poct);
		PVR_KDRV_LOG( PVR_INFO ,"log2_max_pic_order_cnt_lsb_minus4 [ %u ]\n", pSPS->lmpoclm4);
		PVR_KDRV_LOG( PVR_INFO ,"delta_pic_order_always_zero_flag [ %u ]\n", pSPS->dpoazFlag);
		PVR_KDRV_LOG( PVR_INFO ,"offset_for_non_ref_pic [ %u ]\n", pSPS->ofnrp);
		PVR_KDRV_LOG( PVR_INFO ,"offset_for_top_to_bottom_field [ %u ]\n", pSPS->ofttbf);
		PVR_KDRV_LOG( PVR_INFO ,"num_ref_frames_in_pic_order_cnt_cycle [ %u ]\n", pSPS->nrfipocc);

		PVR_KDRV_LOG( PVR_INFO ,"max_num_ref_frames [ %u ]\n", pSPS->mnrf);
		PVR_KDRV_LOG( PVR_INFO ,"gaps_in_frame_num_value_allowed_flag [ %u ]\n", pSPS->gifnvaFlag);
		PVR_KDRV_LOG( PVR_INFO ,"pic_width_in_mbs_minus1 [ %u ]\n", pSPS->pwimm1);
		PVR_KDRV_LOG( PVR_INFO ,"pic_height_in_map_units_minus1 [ %u ]\n", pSPS->phimum1);
		PVR_KDRV_LOG( PVR_INFO ,"frame_mbs_only_flag [ %u ]\n", pSPS->fmoFlag);
		PVR_KDRV_LOG( PVR_INFO ,"mb_adaptive_frame_field_flag [ %u ]\n", pSPS->maffFlag);
		PVR_KDRV_LOG( PVR_INFO ,"direct_8x8_inference_flag [ %u ]\n", pSPS->d8iFlag);
		PVR_KDRV_LOG( PVR_INFO ,"frame_cropping_flag [ %u ]\n", pSPS->fcFlag);

		PVR_KDRV_LOG( PVR_INFO ,"frame_crop_left_offset [ %u ]\n", pSPS->fclo);
		PVR_KDRV_LOG( PVR_INFO ,"frame_crop_right_offset [ %u ]\n", pSPS->fcro);
		PVR_KDRV_LOG( PVR_INFO ,"frame_crop_top_offset [ %u ]\n", pSPS->fcto);
		PVR_KDRV_LOG( PVR_INFO ,"frame_crop_bottom_offset [ %u ]\n", pSPS->fcbo);

		PVR_KDRV_LOG( PVR_INFO ,"vui_parameters_present_flag [ %u ]\n", pSPS->vuippFlag);
		PVR_DD_DumpVUI(&pSPS->vuip);
	}
}

void PVR_DD_DumpPICTIme(PVR_PIC_TIMING_T *pPICTime)
{
	if (0)
	{
		PVR_KDRV_LOG( PVR_INFO ,"cpb_dpb_delay_present_flag [ %u ]\n", pPICTime->cddpFlag);		/* cpb_dpb_delay_present_flag */
		PVR_KDRV_LOG( PVR_INFO ,"cpb_removal_delay_length_minus1 [ %u ]\n", pPICTime->crdlm1);			/* cpb_removal_delay_length_minus1 */
		PVR_KDRV_LOG( PVR_INFO ,"dpb_output_delay_length_minus1 [ %u ]\n", pPICTime->dodlm1);			/* dpb_output_delay_length_minus1 */
		PVR_KDRV_LOG( PVR_INFO ,"cpb_removal_delay [ %u ]\n", pPICTime->crd);			/* cpb_removal_delay */
		PVR_KDRV_LOG( PVR_INFO ,"dpb_output_delay [ %u ]\n", pPICTime->dod);			/* dpb_output_delay */

		PVR_KDRV_LOG( PVR_INFO ,"pic_struct_present_flag [ %u ]\n", pPICTime->pspf);			/* pic_struct_present_flag */

		PVR_KDRV_LOG( PVR_INFO ,"^B^pic_struct [ %u ]\n", pPICTime->ps);				/* pic_struct */

		PVR_KDRV_LOG( PVR_INFO ,"number of clock TS  [ %u ]\n", pPICTime->numClockTS);		/* number of clock TS */
		PVR_KDRV_LOG( PVR_INFO ,"ct_type [ %u ]\n", pPICTime->ct);				/* ct_type */
		PVR_KDRV_LOG( PVR_INFO ,"nuit_field_based_flag [ %u ]\n", pPICTime->nfbFlag);		/* nuit_field_based_flag */
		PVR_KDRV_LOG( PVR_INFO ,"counting_type [ %u ]\n", pPICTime->cntt);			/* counting_type */
		PVR_KDRV_LOG( PVR_INFO ,"full_timestamp_flag [ %u ]\n", pPICTime->ftFlag);			/* full_timestamp_flag */
		PVR_KDRV_LOG( PVR_INFO ,"discontinuity_flag [ %u ]\n", pPICTime->dFlag);			/* discontinuity_flag */
		PVR_KDRV_LOG( PVR_INFO ,"cnt_dropped_flag [ %u ]\n", pPICTime->cdFlag);			/* cnt_dropped_flag */
		PVR_KDRV_LOG( PVR_INFO ,"n_frames [ %u ]\n", pPICTime->nf);				/* n_frames */
		PVR_KDRV_LOG( PVR_INFO ,"seconds_value [ %u ]\n", pPICTime->sv);				/* seconds_value */
		PVR_KDRV_LOG( PVR_INFO ,"minutes_value [ %u ]\n", pPICTime->mv);				/* minutes_value */
		PVR_KDRV_LOG( PVR_INFO ,"hours_value [ %u ]\n", pPICTime->hv);				/* hours_value */

		PVR_KDRV_LOG( PVR_INFO ,"seconds_flag [ %u ]\n", pPICTime->sFlag);			/* seconds_flag */
		PVR_KDRV_LOG( PVR_INFO ,"seconds_value [ %u ]\n", pPICTime->sv1);			/* seconds_value */
		PVR_KDRV_LOG( PVR_INFO ,"minutes_flag [ %u ]\n", pPICTime->mFlag);			/* minutes_flag */
		PVR_KDRV_LOG( PVR_INFO ,"minutes_value [ %u ]\n", pPICTime->mv1);			/* minutes_value */
		PVR_KDRV_LOG( PVR_INFO ,"hours_flag [ %u ]\n", pPICTime->hFlag);			/* hours_flag */
		PVR_KDRV_LOG( PVR_INFO ,"hours_value [ %u ]\n", pPICTime->hv1);			/* hours_value */

		PVR_KDRV_LOG( PVR_INFO ,"time_offset_length [ %u ]\n", pPICTime->tol);			/* time_offset_length */
		PVR_KDRV_LOG( PVR_INFO ,"time_offset [ %u ]\n", pPICTime->to);				/* time_offset */
	}
}

void PVR_DD_GetSPS(UINT8 *tpstream)
{
	PVR_TP_HEADER_INFO_T	tpHeader;
	PVR_BIT_STREAM_T		bitStream;
	UINT8		*pCurrAddr = NULL;
	UINT32		loopI;
	UINT32		bitrate;
	UINT8		startOffest = 0, stopcheck = 0, startbitposition = 0;

	bitStream.pData = gTsBuffer;
	bitStream.startBitPos = 7;
	bitStream.dataSize = 0;

	pCurrAddr = (UINT8 *)tpstream;

	PVR_DD_GetTPHeader(pCurrAddr, &tpHeader);
	pCurrAddr += tpHeader.byteOffsetPayload;

	if(tpHeader.byteOffsetPayload >= (LX_TP_PARSING_BUFFER_LIMIT))
		return;

	do{
		if( (*(volatile UINT8 *)(pCurrAddr - 3) == 0x00) && (*(volatile UINT8 *)(pCurrAddr - 2) == 0x00) &&	(*(volatile UINT8 *)(pCurrAddr - 1) == 0x01)
			&& ((*(volatile UINT8 *)(pCurrAddr) & 0x1F) == PVR_H264_NAL_SPS) )
		{
			stopcheck = 1;
		}

		startOffest++;
		pCurrAddr++;
	}while( (startOffest < (LX_TP_PARSING_BUFFER_LIMIT)) && (stopcheck == 0));

	if(startOffest >= (LX_TP_PARSING_BUFFER_LIMIT))
		return;

	startbitposition = PVR_DOWN_TS_PACKET_SIZE - startOffest - tpHeader.byteOffsetPayload;

	if(startbitposition <= 0)
		return;

	memcpy(bitStream.pData, pCurrAddr, startbitposition);

	bitStream.dataSize += startbitposition;

	PVR_DD_RemoveEP3Bytes(bitStream.pData, bitStream.dataSize);

	if (PVR_DD_ParseSPS(&bitStream, &gSPS) == PVR_OK)
	{
		gbValidSPS = TRUE;

		if(gSPS.vuip.nhppFlag)
		{
			bitrate = (gSPS.vuip.nhrdp.brvm1[0] + 1);
			for(loopI=0; loopI<gSPS.vuip.nhrdp.brs; loopI++)
				bitrate *= 2;
			bitrate *= 64;  //2^6;
			bitrate /= 1024;	// DASY wants to get kbps unit
			PVR_KDRV_LOG( PVR_PIE ,"^G^bitrate [ 0x%x ]\n",bitrate);
		}
	}
	else
	{
		gbValidSPS = FALSE;
	}

	PVR_DD_DumpSPS(&gSPS);
	return;
}

PVR_H264_PIC_STRUCT_T PVR_DD_GetPicStruct(UINT8 *tpstream)
{

	PVR_TP_HEADER_INFO_T	tpHeader;
	PVR_BIT_STREAM_T		bitStream;
	PVR_SEI_HEADER_T		sei;
	PVR_PIC_TIMING_T		pictime;
	PVR_H264_PIC_STRUCT_T	picStruct = H264_RESERVED;

	UINT8		*pCurrAddr;
	UINT8		startOffest = 0, stopcheck = 0, startbitposition = 0;

	if ( gbValidSPS && (gSPS.vuip.pspFlag != 0 || gSPS.vuip.vhppFlag != 0 || gSPS.vuip.nhppFlag != 0))
	{
		bitStream.pData = gTsBuffer;
		bitStream.startBitPos = 7;
		bitStream.dataSize = 0;

		pCurrAddr = (UINT8 *)tpstream;

		PVR_DD_GetTPHeader(pCurrAddr, &tpHeader);
		pCurrAddr += tpHeader.byteOffsetPayload;

		if(tpHeader.byteOffsetPayload >= LX_TP_PARSING_BUFFER_LIMIT)
			return picStruct;

		do{
			if( (*(volatile UINT8 *)(pCurrAddr - 3) == 0x00) && (*(volatile UINT8 *)(pCurrAddr - 2) == 0x00) && (*(volatile UINT8 *)(pCurrAddr - 1) == 0x01)
				&& ((*(volatile UINT8 *)(pCurrAddr) & 0x1F) == PVR_H264_NAL_SEI) )
			{
				stopcheck = 1;
			}

			startOffest++;
			pCurrAddr++;
		}while( (startOffest < (LX_TP_PARSING_BUFFER_LIMIT)) && (stopcheck == 0));

		if(startOffest >= LX_TP_PARSING_BUFFER_LIMIT)
			return picStruct;

		startbitposition = PVR_DOWN_TS_PACKET_SIZE - startOffest - tpHeader.byteOffsetPayload;

		if(startbitposition <= 0)
			return picStruct;

		memcpy(bitStream.pData, pCurrAddr, startbitposition);
		bitStream.dataSize += startbitposition;

		PVR_DD_RemoveEP3Bytes(bitStream.pData, bitStream.dataSize);

		memset(&sei, 0x00, sizeof(sei));

		PVR_DD_GetSEIHeader(&bitStream, &sei);

		while ((bitStream.dataSize > 0) && (sei.payloadType != PVR_H264_SEI_PT) && (*bitStream.pData != 0x80))
		{
			memset(&sei, 0x00, sizeof(sei));

			PVR_DD_GetSEIHeader(&bitStream, &sei);
		}

		if (sei.payloadType == PVR_H264_SEI_PT)
		{
			bitStream.pData = sei.payload;
			bitStream.dataSize = sei.payloadSize;
			bitStream.startBitPos = 7;
			bitStream.bitSize = 0;

			memset(&pictime, 0x00, sizeof(pictime));

			if (gSPS.vuip.vhppFlag != 0 || gSPS.vuip.nhppFlag != 0)
			{
				pictime.cddpFlag = 1;
			}

			if (gSPS.vuip.vhppFlag != 0)
			{
				pictime.crdlm1 = gSPS.vuip.vhrdp.crdlm1;
				pictime.dodlm1 = gSPS.vuip.vhrdp.dodlm1;
				pictime.tol = gSPS.vuip.vhrdp.tol;
			}
			else if (gSPS.vuip.nhppFlag != 0)
			{
				pictime.crdlm1 = gSPS.vuip.nhrdp.crdlm1;
				pictime.dodlm1 = gSPS.vuip.nhrdp.dodlm1;
				pictime.tol = gSPS.vuip.nhrdp.tol;
			}

			pictime.pspf = gSPS.vuip.pspFlag;

			PVR_DD_ParsePICTiming(&bitStream, &pictime);

			PVR_DD_DumpPICTIme(&pictime);

			picStruct = pictime.ps;
		}
	}
	return picStruct;
}

UINT32 PVR_DD_AdjustH264StreamFrameRate(UINT16 *pFrameRateIndex)
{
	UINT16		oldFrameIndex;
	UINT16		newFrameIndex;

	oldFrameIndex = *pFrameRateIndex;
	newFrameIndex = oldFrameIndex;

	switch((LX_PVR_FRAMERATE_T)oldFrameIndex)
	{
		case LX_PVR_FR_23_976HZ:
		case LX_PVR_FR_24HZ:
		case LX_PVR_FR_25HZ:
			if(gStreamInfo.bFoundSeqSPS && gSPS.vuippFlag && gSPS.vuip.tipFlag)
			{
				/* Progressive Signal :  Frame rate.
				 * Picture Count is around 1700 ~ 2750
				 * So, It is right to assume that this stream is progressive.
				 * 23.97(P), 24(P), 25(P) */

				if((gSPS.vuip.ts == 48000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_23_976HZ;
				else if((gSPS.vuip.ts == 48) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_24HZ;
				else if((gSPS.vuip.ts == 50) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_25HZ;
				else if((gSPS.vuip.ts == 60000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_29_97HZ;
				else if((gSPS.vuip.ts == 60) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_30HZ;

				// Progressive Signal :  Wrong Encoded Stream to Frame rate.
				// Theoretically, frame rate = (ts/2)/nuit. But Some streams have
				// wrong parameters with regard to frame rate. The wrong streams
				// treat frame rate as (ts/nuit). So, if real picture count is
				// around 1700 ~ 2750, then it is right to assume that this stream
				// is encoded with wrong parameters.
				if((gSPS.vuip.ts == 24000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_23_976HZ;
				else if((gSPS.vuip.ts == 24) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_24HZ;
				else if((gSPS.vuip.ts == 25) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_25HZ;
				else if((gSPS.vuip.ts == 30000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_29_97HZ;
				else if((gSPS.vuip.ts == 30) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_30HZ;
			}
			else
			{
				#if 0//(SYS_ATSC)
				newFrameIndex = LX_PVR_FR_24HZ;
				#else
				newFrameIndex = LX_PVR_FR_25HZ;
				#endif
			}
			break;
		case LX_PVR_FR_29_97HZ:
		case LX_PVR_FR_30HZ:
			if(gStreamInfo.bFoundSeqSPS && gSPS.vuippFlag && gSPS.vuip.tipFlag)
			{
				/* Progressive Signal :  Frame rate.
				 * Picture Count is around 2750 ~ 3750
				 * So, It is right to assume that this stream is progressive.
				 * 29.97(P), 30(P) */

				if((gSPS.vuip.ts == 50) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_25HZ;
				else if((gSPS.vuip.ts == 60000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_29_97HZ;
				else if((gSPS.vuip.ts == 60) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_30HZ;

				// Progressive Signal :  Wrong Encoded Stream to Frame rate.
				// Theoretically, frame rate = (ts/2)/nuit. But Some streams have
				// wrong parameters with regard to frame rate. The wrong streams
				// treats frame rate as (ts/nuit). So, if real picture count is
				// around 2750 ~ 3750, then it is right to assume that this stream
				// is encoded with wrong parameters.
				if((gSPS.vuip.ts == 25) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_25HZ;
				else if((gSPS.vuip.ts == 30000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_29_97HZ;
				else if((gSPS.vuip.ts == 30) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_30HZ;
			}
			break;
		case LX_PVR_FR_50HZ:
		case LX_PVR_FR_59_94HZ:
		case LX_PVR_FR_60HZ:
			if(gStreamInfo.bFoundSeqSPS && gSPS.vuippFlag && gSPS.vuip.tipFlag)
			{
				/* Progressive Signal :  Frame rate.
				 * Picture Count is around 3750 ~ 5750 or  5750 ~ 7750
				 * In case of progressive signal, frame count can be those values.
				 * 50(P), 59.94(P), 60(P) */
				if((gSPS.vuip.ts == 100) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_50HZ;
				else if((gSPS.vuip.ts == 120000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_59_94HZ;
				else if((gSPS.vuip.ts == 120) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_60HZ;

				/* 1) Progressive Signal :  Wrong Stream Encoding to Frame rate.
				 *    Theoretically, frame rate = (ts/2)/nuit. But Some streams have
				 *    wrong parameters with regard to frame rate. The wrong streams
				 *    treats frame rate as (ts/nuit). So, if real picture count is
				 *    around 3750 ~ 7750, then it is right to assume that this stream
				 *    is encoded with wrong parameters.
				 * 2) Interlaced Signal :  Field rate.
				 *    Picture Count is around 3750 ~ 5750 or  5750 ~ 7750.
				 *    In case of interlaced signal, field count can be those values.
				 *    25(I), 29.97(I), 30(I) */
				if((gSPS.vuip.ts == 50) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_50HZ;
				else if((gSPS.vuip.ts == 60000) && (gSPS.vuip.nuit == 1001))
					newFrameIndex = LX_PVR_FR_59_94HZ;
				else if((gSPS.vuip.ts == 60) && (gSPS.vuip.nuit == 1))
					newFrameIndex = LX_PVR_FR_60HZ;
			}
			break;
		default:
			break;
	}

	*pFrameRateIndex = newFrameIndex;

	PVR_KDRV_LOG( PVR_PIE ,"[%s] bFoundSeqSPS = %d: vuippFlag = %d, tipFlag = %d, gSPS.vuip.ts = %d, gSPS.vuip.nuit=%d, oldFrameIndex = %d, newFrameIndex = %d\n",
						__FUNCTION__, gStreamInfo.bFoundSeqSPS, gSPS.vuippFlag, gSPS.vuip.tipFlag, gSPS.vuip.ts, gSPS.vuip.nuit, oldFrameIndex, newFrameIndex);

	return PVR_FAILURE;
}

UINT32 PVR_DD_UpdateH264StreamInfo(BOOLEAN bFoundSPS, UINT8 *tpstream, UINT32 packetOffset)
{
	UINT16					loopI;
	UINT8					*pPacketData = NULL;

	UINT32					frRate;
	UINT32					curTS;
	UINT32					tsDiff;

	BOOLEAN					bSetFrRate;

	pPacketData = (UINT8 *)tpstream;

	//PVR_KDRV_LOG( PVR_PIE ,"Update H264 Strem Info. : Packet Offset [%d], SPS [%s]]\n", packetOffset, bFoundSPS ? "TRUE" : "FALSE");

	if (bFoundSPS)
	{
		gStreamInfo.bFoundSeqSPS = TRUE;
		return PVR_OK;
	}

	pPacketData = (UINT8 *)pPacketData + packetOffset;
	curTS = 0;
	curTS = (*pPacketData << 24);
	curTS |= (*(pPacketData + 1) << 16);
	curTS |= (*(pPacketData + 2) << 8);
	curTS |= (*(pPacketData + 3));

	pPacketData += 4;

	if (*pPacketData != PVR_TP_SYNC_BYTE)
	{
		PVR_KDRV_LOG( PVR_WARNING ,"Warning : Paket alignment is broken!\n");
		return PVR_FAILURE;
	}

	/* Assuming modulo 300 is used */
	//curTS = (curTS & 0x1FF) + (((curTS & 0x3FFFFE00) >> 9) * 300);

	if (!curTS) curTS = 1;

	if (!gRefTimeStamp) gRefTimeStamp = curTS;
	else
	{
		//if (curTS < gRefTimeStamp) tsDiff = curTS + (0x3FFFFE00 >> 9) * 300 + 0x1FF - gRefTimeStamp;
		//else if (curTS == gRefTimeStamp) tsDiff = (0x3FFFFE00 >> 9) * 300 + 0x1FF;
		//else tsDiff = curTS - gRefTimeStamp;

		if (curTS < gRefTimeStamp) tsDiff = curTS  - gRefTimeStamp;
		else if (curTS == gRefTimeStamp) tsDiff = 0;
		else tsDiff = curTS - gRefTimeStamp;

		PVR_KDRV_LOG( PVR_INFO ,"^Y^tsDiff %d gRefTimeStamp %d gPictureNum %d\n", tsDiff, gRefTimeStamp, gPictureNum);

		if (tsDiff >= PVR_TIME_STAMP_CLOCK)
		{
			// Be Careful !!. Actually it is picture rate, not frame rate.
			// Progressive Signal : frRate = frame rate.
			// Interlaced Signal : frRate = field rate.
			frRate = (gPictureNum-1) * 100000 / (tsDiff / 27000);

			PVR_KDRV_LOG( PVR_INFO ,"^G^Cal FR RATE = %d (NR Pic [%d] Prev TS [%d] Cur TS [%d] TS Diff [%d])\n", frRate,	gPictureNum, gRefTimeStamp, curTS, tsDiff);
			gPictureNum = 0;
			gRefTimeStamp = 0;

			if (tsDiff < PVR_TIME_STAMP_MAX_DIFF)
			{
				bSetFrRate = FALSE;

				for (loopI = 0; loopI < sizeof(gH264RefFrameRate)/sizeof(gH264RefFrameRate[0]); loopI += 2)
				{
					if ( !loopI && frRate < gH264RefFrameRate[loopI] )
					{
						bSetFrRate = TRUE;
						break;
					}

					if( frRate >= gH264RefFrameRate[loopI] && frRate <= gH264RefFrameRate[loopI+1])
					{
						bSetFrRate = TRUE;
						break;
					}
				}

				if (bSetFrRate)
				{
					loopI /=2;
					loopI += LX_PVR_H264_FR_INDEX_ADJUST;

					PVR_DD_AdjustH264StreamFrameRate(&loopI);

					PVR_KDRV_LOG( PVR_INFO ,"^Y^Frame Rate is %d\n", loopI);
					gStreamInfo.frRate = (LX_PVR_FRAMERATE_T)loopI;
				}
			}
		}
	}

	return PVR_OK;
}

UINT32 PVR_DD_UpdateMPEG2StreamInfo(UINT8 *tpstream)
{
	PVR_TP_HEADER_INFO_T	tpHeader;
	UINT8		*pCurrAddr = NULL;
	UINT8		startOffest = 0, stopcheck = 0;
	UINT32		regdata = 0;

	pCurrAddr = (UINT8 *)tpstream;

	PVR_DD_GetTPHeader(pCurrAddr, &tpHeader);
	pCurrAddr += tpHeader.byteOffsetPayload;

	if(tpHeader.byteOffsetPayload >= (LX_TP_PARSING_BUFFER_LIMIT))
		return PVR_OK;

	do{
		if( (*(volatile UINT8 *)(pCurrAddr - 3) == 0x00) && (*(volatile UINT8 *)(pCurrAddr - 2) == 0x00) &&	(*(volatile UINT8 *)(pCurrAddr - 1) == 0x01)
			&& ((*(volatile UINT8 *)(pCurrAddr) ) == PVR_INDEX_MPGE2_SEQ_START) )
		{
			stopcheck = 1;
		}

		startOffest++;
		pCurrAddr++;
	}while( (startOffest < (LX_TP_PARSING_BUFFER_LIMIT)) && (stopcheck == 0));

	if(startOffest >= LX_TP_PARSING_BUFFER_LIMIT)
		return PVR_OK;

	gStreamInfo.bFoundSeqSPS = TRUE;

	regdata = (*(pCurrAddr+3) << 24)|(*(pCurrAddr+2)<<16)|(*(pCurrAddr+1)<<8)|(*(pCurrAddr));
	pCurrAddr += 4;

	gStreamInfo.frRate = (regdata & 0x0F000000) >> 24;

	if (gStreamInfo.frRate > LX_PVR_FR_UNKNOWN) gStreamInfo.frRate = LX_PVR_FR_UNKNOWN;

	regdata = (*(pCurrAddr+3) << 24)|(*(pCurrAddr+2)<<16)|(*(pCurrAddr+1)<<8)|(*(pCurrAddr));
	pCurrAddr += 4;

	gStreamInfo.bitRate = (regdata & 0xFF) << 10;
	gStreamInfo.bitRate |= ((regdata & 0xFF00) >> 8) << 2;
	gStreamInfo.bitRate |= ((regdata & 0xc00000) >> 20);
	gStreamInfo.bitRate *= 400;	// 400bps unit
	gStreamInfo.bitRate /= 1024;	// DASY wants to get kbps unit

	PVR_KDRV_LOG( PVR_INFO ,"FR [%d], Bit [%d]\n",gStreamInfo.frRate, gStreamInfo.bitRate);

	return PVR_OK;
}

