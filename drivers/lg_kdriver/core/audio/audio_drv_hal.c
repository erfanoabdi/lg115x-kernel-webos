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
 *  Linux proc interface for audio device.
 *  audio device will teach you how to make device driver with new platform.
 *
 *  author	wonchang.shin (wonchang.shin@lge.com)
 *  version	0.1
 *  date		2012.05.03
 *  note		Additional information.
 *
 *  @addtogroup lg1150_audio
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
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>	/**< For request_region, check_region etc */
#include <linux/rmap.h>
#include <linux/file.h>		/**< For Audio Memory debug */
#include <linux/syscalls.h>
#include <linux/fcntl.h>

#include <asm/io.h>			/**< For ioremap_nocache */
#include <asm/uaccess.h>
#include "proc_util.h"
#include "debug_util.h"

#include "base_types.h"
#include "base_device.h"

#include "os_util.h"
#include "base_drv.h"		/**< av lipsync */

#include "audio_cfg.h"
#include "audio_drv.h"

#include "audio_buffer.h"
#include "audio_drv_hal.h"
#include "audio_imc_func.h"
#include "common/adec_common_version.h"

#include "audio_drv_decoder.h"
#include "audio_drv_renderer.h"
#include "audio_drv_master.h"
#include "audio_drv_encoder.h"
#include "audio_drv_capturer.h"
#include "audio_kapi_master.h"

#ifdef INCLUDE_H14_CHIP_KDRV
#include "../../chip/h14/audio/audio_drv_hal_h14_a0.h"
#endif	//#ifdef INCLUDE_H14_CHIP_KDRV

#ifdef INCLUDE_M14_CHIP_KDRV
#include "../../chip/m14/audio/audio_drv_hal_m14_a0.h"
#endif	//#ifdef INCLUDE_M14_CHIP_KDRV

#ifdef INCLUDE_H13_CHIP_KDRV
#include "../../chip/h13/audio/audio_drv_hal_h13.h"
#endif	//#ifdef INCLUDE_H13_CHIP_KDRV


/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
//define DSP Ready timeout value
#define WAIT_DSP0_READY_TIMEOUT		300		// Wait 300 ms for DSP0 to Ready
#define WAIT_DSP1_READY_TIMEOUT		300		// Wait 300 ms for DSP1 to Ready
#define WAIT_MONOTOR_TASK_TIMEOUT	200		// Wait 200 ms for Monitor Task
#define MONOTOR_TASK_PRINT_TIME		((1000/WAIT_MONOTOR_TASK_TIMEOUT)+1)	// 1.xx seconds
#define DSP_AUTO_RECOVERY_COUNT		  3		// 3 : 200 * 3 = 600 ms for ESD recovery, 2011.11.02 after reset, audio outputs 200ms.
												//QE Spec. is 1 seconds.
#define DSP_MAX_RESET_COUNT			  5

//VDEC H/W Reg. Address for A/V lipsync basetime
#define H14_VDEC_LIPSYNC_REG_MEMBASE	(0xC00048F0)
#define M14_VDEC_LIPSYNC_REG_MEMBASE	(0xC00048F0)
#define H13_VDEC_LIPSYNC_REG_MEMBASE	(0xC00048F0)
#define VDEC_LIPSYNC_REG_MEMSIZE		(0x10)
VDEC_LIPSYNC_REG_T	*g_pVDEC_LipsyncReg = NULL;	//VDEC H/W Reg. for Lip Sync Control with ADEC H/W Reg.


/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
/* define a audio reset semaphore */
OS_SEM_T	_gAudResetSemaphore;
UINT32 g_DuringReset	=	0;

/* Related to AUDIO event processing */
LX_AUD_EVENT_TYPE_T gAudSetEvent[LX_AUD_DEV_MAX];
LX_AUD_EVENT_TYPE_T gAudGetEvent[LX_AUD_DEV_MAX];
DECLARE_WAIT_QUEUE_HEAD(gAudPollWaitQueue);
DEFINE_SPINLOCK(gAudEventSpinLock);

DECLARE_WAIT_QUEUE_HEAD(AudWaitDsp0Ready);
DECLARE_WAIT_QUEUE_HEAD(AudWaitDsp1Ready);
DECLARE_WAIT_QUEUE_HEAD(Monitor_WaitQ);

LX_AUD_TASK_MSG_T		g_suiWaitQForMonitor;

//DSP virtual address (ioremap)
#ifdef NOT_MEMMORY_OPTIMIZE
UINT32	*g_pui32GuardStartVirAddr = NULL;
UINT32	*g_pui32GuardEndVirAddr	  = NULL;
#endif
UINT32	*g_pui32DSP0CodecVirAddr  = NULL;
UINT32	*g_pui32DSP1CodecVirAddr  = NULL;

//IPC virtual address (ioremap)
UINT32	*g_pui32IPC_VirAddr 	= NULL;
#ifdef NOT_MEMMORY_OPTIMIZE
UINT32	*g_pui32DebugVirAddr	= NULL;
#endif
UINT32	*g_pui32SEParamVirAddr  = NULL;

//Audio Monitor Task status for checking DSP latch-up
#if defined (USE_H13_AUDIO_DSP_FIRMWARE) && (CONFIG_LX_CHIP_EMUL == 0x0)
BOOLEAN	g_bEnableDspMonitor = TRUE;
#elif defined (USE_M14_A0_AUDIO_DSP_FIRMWARE) && (CONFIG_LX_CHIP_EMUL == 0x0)
BOOLEAN	g_bEnableDspMonitor = TRUE;
#elif defined (USE_H14_A0_AUDIO_DSP_FIRMWARE) && (CONFIG_LX_CHIP_EMUL == 0x0)
BOOLEAN	g_bEnableDspMonitor = TRUE;
#else
BOOLEAN	g_bEnableDspMonitor = FALSE;
#endif

//IPC RW Offset Address
IPC_RW_OFFSET_ADDR_T g_IPCRWOffset = {0, };

//ADEC KDRV Structure
LX_AUD_KDRV_T	g_AudKdrv =
{
	.bInitDone			= FALSE,
	.soundEngineMask	= 0,

	.LoadCodec   		= TRUE,
	.CopyDSP0   		= TRUE,
	.CopyDSP1 			= TRUE,
	.EnableIMCDBG	  	= FALSE,

	.IsDSP0Reseted 		= 0,
	.IsDSP1Reseted 		= 0,
	.IsDSP0Ready		= 0,
	.IsDSP1Ready		= 0,

	.p_aud_mem_share_base = 0,

	.bSuspended			= FALSE,
};


/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static SINT32 AUDIO_StallDspToReset ( void );
static SINT32 AUDIO_CheckResetStatus ( UINT8 ui8ResetFlag );
static SINT32 AUDIO_IORemap( void );

static void AUDIO_WaitForDSP1Ready ( UINT64 startTick );
static void AUDIO_WaitForDSP0Ready ( UINT64 startTick );
static void AUDIO_SignalDSP1Ready ( void* _param, int _paramLen, void* _cbParam );
static void AUDIO_SignalDSP0Ready ( void* _param, int _paramLen, void* _cbParam );
static SINT32 AUDIO_StallDspToReset ( void );
/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/


/*========================================================================================
	Implementation Group
========================================================================================*/
/**
 * Initializes the audio module.
 * Audio module shall be initialized itself with pre-defined configuration value.
 * Audio module includes audio codec, post-processing, SPDIF parts
 * @see AUDIO_Init
*/
SINT32 AUDIO_InitModule ( void )
{
	UINT8	i;
	SINT32 ret = RET_OK;
	BOOLEAN bStatus = FALSE;

	DtoCtrlCmdSetDtorate		setDtoRate;

	//Initialize a audio event variables.
	for(i = 0; i < LX_AUD_DEV_MAX; i++)
	{
		gAudSetEvent[i].allocDev = LX_AUD_DEV_NONE;
		gAudSetEvent[i].eventMsg = LX_AUD_EVENT_MASK_NONE;
		gAudGetEvent[i].allocDev = LX_AUD_DEV_NONE;
		gAudGetEvent[i].eventMsg = LX_AUD_EVENT_MASK_NONE;
	}

	//check a suspend mode
	if(g_AudKdrv.bSuspended == FALSE)
	{
		//ioremap audio buffer memory
		ret = AUDIO_IORemapBuffer();
		if (ret != RET_OK)
		{
			AUD_KDRV_ERROR("%s.%d] error in AUDIO_IORemapBuffer call\n", __F__, __L__);
			return RET_ERROR;
		}
	}

	//ioremap audio memory
	ret = AUDIO_IORemap();
	if (ret != RET_OK)
	{
		AUD_KDRV_ERROR("%s.%d] error in AUDIO_IORemap call\n", __F__, __L__);
		return RET_ERROR;
	}

	//Initializes the clock and reset module register value.
	MIXED_IP_AUDIO_InitClockRegisters();

	//Resets the Clock Source and Mixed IP Registers.
	(void)MIXED_IP_AUDIO_SetClockSource( LX_AUD_SAMPLING_FREQ_48_KHZ, LX_AUD_CLOCK_DTV, LX_AUD_MASTER_SPDIF_ES );

	//Check a DSP1 reset status in boot code.
	bStatus = AUDIO_CheckResetStatus(AUD_OPTION_DSP1);
	if(bStatus == TRUE)
	{
		AUD_KDRV_DEBUG("Flush a DSP1 Firmware Image!!!\n");

		g_AudKdrv.IsDSP1Reseted = 1;
		AUDIO_ClearReset(AUD_OPTION_DSP1);
	}

	//Clear DSP 0 /1 State
	g_AudKdrv.IsDSP0Reseted = 0;
	g_AudKdrv.IsDSP1Reseted = 0;

	//Reset Low : APB, SRC, ADEC DSP and AAD etc...
	AUDIO_ClearReset(AUD_OPTION_ALL);

	//Initializes the Mixed IP Registers.
	(void)MIXED_IP_AUDIO_InitCodecRegisters();

	//Reset High : APB, SRC and Mixed IP etc...
	(void)AUDIO_SetReset(AUD_OPTION_OTHERS);

	// Initialize register
	(void)AUDIO_InitReg();

	// Initialize register for check bit because DSP does not read reg. when CPU access reg.
	(void)AUDIO_InitRegForCheckbit();

	// Initialize IMC & IPC DBUG Module.
	AUDIO_IMC_Init();
	AUDIO_IPC_DBG_Init();

	//Load DSP1 codec for audio processing
	ret = AUDIO_LoadDSP1Codec();
	if (ret != RET_OK)
	{
		AUD_KDRV_ERROR("error in ADEC_L9_LoadDSP1Codec call\n");
		return RET_ERROR;
	}

	// Load DSP0 image
	ret = AUDIO_LoadDSP0Codec();
	if (ret != RET_OK)
	{
		AUD_KDRV_ERROR("error in AUDIO_LoadDSP0Codec call\n");
		return RET_ERROR;
	}
	AUD_KDRV_ERROR("KDRV  CMN Ver. - %02x.%02x.%02x.%c.%02x\n", ADEC_COMMON_VER_MAJOR, ADEC_COMMON_VER_MINOR, ADEC_COMMON_VER_FIX, ADEC_COMMON_VER_FLAG, ADEC_COMMON_VER_EXT);

  	//create buffer interface for DSP module.
	AUDIO_CreateBuffer();

	//Start-up the CODEC IP for mute on.
	(void)MIXED_IP_AUDIO_StartCodec();

  	//Set a DTO Interrupt Source
	MIXED_IP_AUDIO_SetDTOIntSrc(LX_AUD_CLOCK_DTV, LX_AUD_MASTER_SPDIF_ES);

	// Set DTO Clock Type
	//Send a IMC command for DTO setting to be changed clock source.
	setDtoRate.dtorate			= (UINT32)DTO_A_CLOCK_CENTER_TAP_VALUE;
	setDtoRate.force_cnt_clear	= (UINT32)1;
	AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_DTORATE, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetDtorate), &setDtoRate);

  	//Start a DTO  DSP Module.
	MIXED_IP_AUDIO_StartDTO(LX_AUD_CLOCK_DTV, LX_AUD_MASTER_SPDIF_ES);

	//check a suspend mode
	if(g_AudKdrv.bSuspended == FALSE)
	{
		//Initialize AUDIO RESET semaphore
		OS_InitMutex(&_gAudResetSemaphore, OS_SEM_ATTR_DEFAULT);
	}

	/* Update Internal Status Value */
	g_AudKdrv.bInitDone = TRUE;

	AUD_KDRV_PRINT("%s : Done!\n", __F__);
	return RET_OK;
}

/**
 *  Resets a AAD Module using H/W reset signal.
 *  This function resets a AAD module to clear previous setting.
 * @see
*/
SINT32 AUDIO_ResetAADModule(void)
{
	//Reset Low : AAD Module
	AUDIO_ClearReset(AUD_OPTION_AAD);

	//Reset High : AAD Module
	AUDIO_SetReset(AUD_OPTION_AAD);

	AUD_KDRV_DEBUG("AUDIO_ResetAADModule\n");
	return RET_OK;
}

/**
 * Load a DSP1 codec firmware.
 * @see
*/
SINT32 AUDIO_LoadDSP1Codec ( void )
{
	SINT32 retVal = RET_OK;
	UINT64 startTick = 0;

	//Get a current tick time
	startTick = jiffies;
	if(NULL == g_pui32DSP1CodecVirAddr)
	{
		AUD_KDRV_ERROR("ADEC_LoadCodec : Err - g_pui32DSP1CodecVirAddr NULL!\n");
		return RET_ERROR;
	}

	//Reset Low : ADEC DSP1
	AUDIO_ClearReset(AUD_OPTION_DSP1);

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_CopyDSP1Codec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_CopyDSP1Codec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_CopyDSP1Codec();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	//Set DSP1 swreset register
	AUDIO_SetReset(AUD_OPTION_DSP1);

	//Wait for DSP1 fw download completion to DSP1 memory and DSP1 ready
	AUDIO_WaitForDSP1Ready(startTick);

	//Set a current Lip sync , sound engine, SPDIF SCMS and output control mode after dsp auto reset.
	if(g_AudKdrv.bInitDone == TRUE)
	{
		//Set a SPDIF clock for M14 B0 Chip Bug
		if(lx_chip_rev() == LX_CHIP_REV(M14, B0)) // M14B0 only bug
		{
			//Set a default sampling frequency and DTO rate after dsp reset
			(void)MIXED_IP_AUDIO_SetSPDIFSamplingFreq(LX_AUD_SAMPLING_FREQ_48_KHZ, DTO_A_CLOCK_CENTER_TAP_VALUE, AUD_DTO_D);
		}
		else
		{
			//Set a default sampling frequency and DTO rate after dsp reset
			(void)MIXED_IP_AUDIO_SetSPDIFSamplingFreq(LX_AUD_SAMPLING_FREQ_48_KHZ, DTO_A_CLOCK_CENTER_TAP_VALUE, AUD_DTO_A);
		}
	}

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return retVal;
}

/**
 * Load a DSP0 codec firmware.
 * @see
*/
SINT32 AUDIO_LoadDSP0Codec ( void )
{
	SINT32 retVal = RET_OK;
	UINT64 startTick = 0;

	//Get a current tick time
	startTick = jiffies;

	//Check a fw memory
	if(NULL == g_pui32DSP0CodecVirAddr)
	{
		AUD_KDRV_ERROR("ADEC_LoadCodec : Err - g_pui32DSP0CodecVirAddr NULL!\n");
		return RET_ERROR;
	}

	//Check a current loaded codec.
	if(g_AudKdrv.LoadCodec == FALSE)
	{
		AUD_KDRV_PRINT("ADEC_LoadCodec : Codec already loaded\n");
		return RET_OK;
	}

	//Reset Low : ADEC DSP0
	AUDIO_ClearReset(AUD_OPTION_DSP0);

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_CopyDSP0Codec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_CopyDSP0Codec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_CopyDSP0Codec();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	//Set DSP swreset register
	AUDIO_SetReset(AUD_OPTION_DSP0);

	//Wait for DSP fw download completion to DSP memory and DSP ready
	AUDIO_WaitForDSP0Ready(startTick);

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return RET_OK;
}

/**
 * Read debug mask for debug
 * @see
*/
UINT32 AUDIO_ReadDebugMask (void)
{
	UINT32 mask = 0;

	/* Read a audio debug mask reg. */
	mask = AUDIO_ReadReg(LX_AUD_REG_DEBUG_PRINT_MASK);

	return mask;
}

/**
 * Write debug mask for debug
 * @see
*/
void AUDIO_WriteDebugMask (unsigned int mask)
{
	/* Write a audio debug mask reg. */
	(void)AUDIO_WriteReg(LX_AUD_REG_DEBUG_PRINT_MASK, mask);

	return;
}

void AUDIO_Restart(void)
{
	UINT8	i;
	ULONG	flags;

	LX_AUD_DEV_T	allocDev = LX_AUD_DEV_NONE;

	g_DuringReset = 1;

	//Lock a AUDIO Reset Semaphore
	OS_LockMutex(&_gAudResetSemaphore);

	//Stall a DSP
	(void)AUDIO_StallDspToReset();

	//Unlock a AUDIO Reset Semaphore
	OS_UnlockMutex(&_gAudResetSemaphore);
	OS_LockMutex(&_gAudResetSemaphore);

	//IMC Finalize
	(void)AUDIO_IMC_Finalize();
	(void)AUDIO_IPC_DBG_Finalize();

	//IMC Re-Init
	(void)AUDIO_IMC_Init();
	(void)AUDIO_IPC_DBG_Init();

	//Unlock a AUDIO Reset Semaphore
	OS_UnlockMutex(&_gAudResetSemaphore);
	OS_LockMutex(&_gAudResetSemaphore);

	//Download dsp codec
	(void)AUDIO_LoadDSP1Codec();
	(void)AUDIO_LoadDSP0Codec();

	//Unlock a AUDIO Reset Semaphore
	OS_UnlockMutex(&_gAudResetSemaphore);
	OS_LockMutex(&_gAudResetSemaphore);

	KDRV_AUDIO_ResetCapturer();
	KDRV_AUDIO_ResetEncoder();
	KDRV_AUDIO_ResetDisconnectMaster();
	KDRV_AUDIO_ResetRenderer();
	KDRV_AUDIO_ResetDecoder();
	KDRV_AUDIO_ResetMaster();

	//Unlock a AUDIO Reset Semaphore
	OS_UnlockMutex(&_gAudResetSemaphore);
	OS_LockMutex(&_gAudResetSemaphore);

	KDRV_AUDIO_ResetMasterParam();
	KDRV_AUDIO_ResetDecoderParam();
	KDRV_AUDIO_ResetRendererParam();
	KDRV_AUDIO_ResetConnectMaster();
	KDRV_AUDIO_ResetEncoderParam();
	KDRV_AUDIO_ResetCapturerParam();

	//Unlock a AUDIO Reset Semaphore
	OS_UnlockMutex(&_gAudResetSemaphore);
	OS_LockMutex(&_gAudResetSemaphore);

	KDRV_AUDIO_RestartMaster();
	KDRV_AUDIO_RestartDecoder();
	KDRV_AUDIO_RestartRenderer();
	KDRV_AUDIO_RestartEncoder();
	KDRV_AUDIO_RestartCapturer();

	//Unlock a AUDIO Reset Semaphore
	OS_UnlockMutex(&_gAudResetSemaphore);

	g_DuringReset = 0;

	//spin lock for protection
	spin_lock_irqsave(&gAudEventSpinLock, flags);

	//Set a audio GET event type for next event.
	for(i = 0; i < LX_AUD_DEV_REN0; i++)
	{
		if((gAudSetEvent[i].allocDev != LX_AUD_DEV_NONE)
		 &&(gAudSetEvent[i].eventMsg & LX_AUD_EVENT_DSP_RESET) )
		{
			gAudGetEvent[i].allocDev  = gAudSetEvent[i].allocDev;
			gAudGetEvent[i].eventMsg |= LX_AUD_EVENT_DSP_RESET;
			allocDev = i;	//Save a last allocated device.
		}
	}

	//spin unlock for protection
	spin_unlock_irqrestore(&gAudEventSpinLock, flags);

	AUD_KDRV_DEBUG("AUDIO_Restart(allocDev = %d)\n", allocDev);
	return;
}

/**
 * Audio Task for audio fw deadness.
 * @see
*/
SINT32 AUDIO_MonitorTask(void *pParam)
{
	//DSP0, DSP1 running counter
	UINT32		ui32CheckDSP0RunningCnt = 0;
	UINT32		ui32CheckDSP1RunningCnt = 0;

	UINT32		ui32Dsp0Timer = 0;
	UINT32		ui32Dsp1Timer = 0;

	UINT32		ui32PreDsp0Timer = 0;
	UINT32		ui32PreDsp1Timer = 0;

	UINT32		ui32DspResetCount = 0;
	UINT32		ui32ResetCount = 0;

	AUD_KDRV_PRINT("Audio Monitor Task is created\n");

	do
	{
		/* Check stop condition when device is closed. */
		if( kthread_should_stop())
		{
			AUD_KDRV_PRINT("Audio Monitor Task - exit!\n");
			break;
		}

		/* Reset a wait queue monitor variable. */
		g_suiWaitQForMonitor = LX_AUD_TASK_NONE_MSG;

		wait_event_interruptible_timeout( Monitor_WaitQ,	\
										  g_suiWaitQForMonitor != LX_AUD_TASK_NONE_MSG,	\
										  msecs_to_jiffies(WAIT_MONOTOR_TASK_TIMEOUT) );	//200 ms

		AUD_KDRV_TRACE("%s(msg = %d)\n", __F__, g_suiWaitQForMonitor);

		//Process and Check a DSP 0/1 latch-up status
		if(g_bEnableDspMonitor == TRUE && g_suiWaitQForMonitor == LX_AUD_TASK_NONE_MSG &&
		   g_AudKdrv.bSuspended == FALSE)
		{
			//Update a DSP running check count
			ui32CheckDSP0RunningCnt++;
			ui32CheckDSP1RunningCnt++;

			//Check a DSP0 running status
			ui32Dsp0Timer = AUDIO_ReadReg(LX_AUD_REG_DSP0_GSTC);

			//Check a DSP1 running status
			ui32Dsp1Timer = AUDIO_ReadReg(LX_AUD_REG_DSP1_GSTC);

			//Check a previous & current value.
			if(ui32PreDsp0Timer == ui32Dsp0Timer)
			{
				AUD_KDRV_DEBUG("DSP0 Timer not changed : [0x%X:0x%X]\n", ui32Dsp0Timer, ui32PreDsp0Timer);
			}
			else
			{
				//AUD_KDRV_DEBUG_TMP("DSP0 Timer changed : [0x%X:0x%X]\n", ui32Dsp0Timer, ui32PreDsp0Timer);
				ui32CheckDSP0RunningCnt = 0;
			}

			if(ui32PreDsp1Timer == ui32Dsp1Timer)
			{
				AUD_KDRV_DEBUG("DSP1 Timer not changed : [0x%X:0x%X]\n", ui32Dsp1Timer, ui32PreDsp1Timer);
			}
			else
			{
				//AUD_KDRV_DEBUG_TMP("DSP1 Timer changed : [0x%X:0x%X]\n", ui32Dsp1Timer, ui32PreDsp1Timer);
				ui32CheckDSP1RunningCnt = 0;
			}

			//Update a  DSP 0 /1 Timer
			ui32PreDsp0Timer = ui32Dsp0Timer;
			ui32PreDsp1Timer = ui32Dsp1Timer;

			if( (ui32CheckDSP0RunningCnt >= DSP_AUTO_RECOVERY_COUNT)	\
			  ||(ui32CheckDSP1RunningCnt >= DSP_AUTO_RECOVERY_COUNT) )
			{
				AUD_KDRV_ERROR("Reset Audio(count = %d)\n", ui32DspResetCount++);

				//Update DSP0 Reset Counter for debug
				if(ui32PreDsp0Timer == ui32Dsp0Timer && ui32CheckDSP0RunningCnt >= DSP_AUTO_RECOVERY_COUNT)
				{
					ui32ResetCount = AUDIO_ReadReg(LX_AUD_REG_DEBUG_DSP0_RESET);
					(void)AUDIO_WriteReg(LX_AUD_REG_DEBUG_DSP0_RESET, ui32ResetCount + 1);
					AUD_KDRV_ERROR("DSP0 Timer not changed(count = 0x%X) : [0x%X:0x%X]\n", ui32ResetCount + 1, ui32Dsp0Timer, ui32PreDsp0Timer);
				}

				//Update DSP1 Reset Counter for debug
				if(ui32PreDsp1Timer == ui32Dsp1Timer && ui32CheckDSP1RunningCnt >= DSP_AUTO_RECOVERY_COUNT)
				{
					ui32ResetCount = AUDIO_ReadReg(LX_AUD_REG_DEBUG_DSP1_RESET);
					(void)AUDIO_WriteReg(LX_AUD_REG_DEBUG_DSP1_RESET, ui32ResetCount + 1);
					AUD_KDRV_ERROR("DSP1 Timer not changed(count = 0x%X) : [0x%X:0x%X]\n", ui32ResetCount + 1, ui32Dsp1Timer, ui32PreDsp1Timer);
				}

				//Restart audio decoder and renderer after DSP H/W Reset
				AUDIO_Restart();

				ui32CheckDSP0RunningCnt = 0;
				ui32CheckDSP1RunningCnt = 0;
			}
		}

		// if Render does not run, redemand decoded notification.
		if(g_bEnableDspMonitor == TRUE && g_suiWaitQForMonitor == LX_AUD_TASK_NONE_MSG)
		{
			KDRV_AUDIO_RedemandDecodedNofiRenderer();
			KDRV_AUDIO_RedemandDecodedNofiEncoder();
		}
	} while (TRUE);

	return 0;
}

/**
 * ioremap audio memory
 * @see
*/
static SINT32 AUDIO_IORemap( void )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_IOREMAP();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_IOREMAP();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_IOREMAP();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	// VDEC's lipsync basetime register
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		//Mapping virtual memory address for vdec lipsync reg
		if (NULL == g_pVDEC_LipsyncReg)
		{
			g_pVDEC_LipsyncReg = (VDEC_LIPSYNC_REG_T*) ioremap(H14_VDEC_LIPSYNC_REG_MEMBASE,
															   VDEC_LIPSYNC_REG_MEMSIZE);
			if (NULL == g_pVDEC_LipsyncReg)
			{
				AUD_KDRV_ERROR("VDEC Lipsync Reg address ERR!");
				return RET_ERROR;
			}
		}

		AUD_KDRV_PRINT("VDEC Lipsync Reg address-phy [0x%08X]\n", (UINT32) H14_VDEC_LIPSYNC_REG_MEMBASE);
		AUD_KDRV_PRINT("VDEC Lipsync Reg address-virt[0x%08X]\n", (UINT32) g_pVDEC_LipsyncReg);
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		//Mapping virtual memory address for vdec lipsync reg
		if (NULL == g_pVDEC_LipsyncReg)
		{
			g_pVDEC_LipsyncReg = (VDEC_LIPSYNC_REG_T*) ioremap(M14_VDEC_LIPSYNC_REG_MEMBASE,
															   VDEC_LIPSYNC_REG_MEMSIZE);
			if (NULL == g_pVDEC_LipsyncReg)
			{
				AUD_KDRV_ERROR("VDEC Lipsync Reg address ERR!");
				return RET_ERROR;
			}
		}
		AUD_KDRV_PRINT("VDEC Lipsync Reg address-phy [0x%08X]\n", (UINT32) M14_VDEC_LIPSYNC_REG_MEMBASE);
		AUD_KDRV_PRINT("VDEC Lipsync Reg address-virt[0x%08X]\n", (UINT32) g_pVDEC_LipsyncReg);
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		//Mapping virtual memory address for vdec lipsync reg
		if (NULL == g_pVDEC_LipsyncReg)
		{
			g_pVDEC_LipsyncReg = (VDEC_LIPSYNC_REG_T*) ioremap(H13_VDEC_LIPSYNC_REG_MEMBASE,
															   VDEC_LIPSYNC_REG_MEMSIZE);
			if (NULL == g_pVDEC_LipsyncReg)
			{
				AUD_KDRV_ERROR("VDEC Lipsync Reg address ERR!");
				return RET_ERROR;
			}
		}
		AUD_KDRV_PRINT("VDEC Lipsync Reg address-phy [0x%08X]\n", (UINT32) H13_VDEC_LIPSYNC_REG_MEMBASE);
		AUD_KDRV_PRINT("VDEC Lipsync Reg address-virt[0x%08X]\n", (UINT32) g_pVDEC_LipsyncReg);
	}
	else
	{
		//Mapping virtual memory address for vdec lipsync reg
		AUD_KDRV_ERROR("VDEC Lipsync Reg address ERR!");
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return retVal;
}

/**
 * Write a Audio HW Register.
 * @see
*/
SINT32 AUDIO_WriteReg ( LX_AUD_REG_ADDRESS_T addr, UINT32 data )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_WriteReg(addr, data);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_WriteReg(addr, data);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_WriteReg(addr, data);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return retVal;
}

/**
 * Read a Audio HW Register.
 * @see
*/
UINT32 AUDIO_ReadReg ( LX_AUD_REG_ADDRESS_T addr )
{
	UINT32 data = 0;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		data = H14_A0_AUDIO_ReadReg(addr);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		data = M14_A0_AUDIO_ReadReg(addr);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		data = H13_AUDIO_ReadReg(addr);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return data;
}

/**
 *  Checks a Audio DSP reset status in boot code.
 * @see
*/
static SINT32 AUDIO_CheckResetStatus ( UINT8 ui8ResetFlag )
{
	BOOLEAN	dspStatus = FALSE;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		dspStatus = H14_A0_AUDIO_CheckResetStatus(ui8ResetFlag);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		dspStatus = M14_A0_AUDIO_CheckResetStatus(ui8ResetFlag);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		dspStatus = H13_AUDIO_CheckResetStatus(ui8ResetFlag);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_DEBUG("%s : dspStatus(%d)!!!\n", __F__, dspStatus);
	return RET_OK;
}

/**
 * Set a reset for ADEC block.
 * @see
*/
SINT32 AUDIO_SetReset ( UINT8 ui8ResetFlag )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_SetReset(ui8ResetFlag);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_SetReset(ui8ResetFlag);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_SetReset(ui8ResetFlag);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_DEBUG("%s : ui8ResetFlag(%d)!!!\n", __F__, ui8ResetFlag);
	return retVal;
}

/**
 * Clear a reset for AUDIO block.
 * @see
*/
SINT32 AUDIO_ClearReset ( UINT8 ui8ResetFlag )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_ClearReset(ui8ResetFlag);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_ClearReset(ui8ResetFlag);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_ClearReset(ui8ResetFlag);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_DEBUG("%s : ui8ResetFlag(%d)!!!\n", __F__, ui8ResetFlag);
	return retVal;

}

/**
 * Initialize Audio registers.
 * @see AUDIO_InitModule
*/
SINT32 AUDIO_InitReg( void )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_InitRegister();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_InitRegister();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_InitRegister();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return retVal;
}

/**
 * Initialize register for check bit
 * @see AUDIO_InitModule
*/
SINT32 AUDIO_InitRegForCheckbit ( void )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_InitRegForCheckbit();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_InitRegForCheckbit();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_InitRegForCheckbit();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return retVal;
}

/**
 * Wait for DSP1 Ready!.
 * @see
*/
static void AUDIO_WaitForDSP1Ready ( UINT64 startTick )
{
	UINT32 actionID;
	ImcActionParameter actionParam;

	AUD_KDRV_PRINT("Wait %d ms for DSP1 to Ready!!!\n", WAIT_DSP1_READY_TIMEOUT);
	actionParam.actionType = IMC_ACTION_GET_CALLBACK;
	actionParam.repeatType = IMC_ACTION_ONCE;
	actionParam.target = ADEC_MODULE_MAN_ARM;
	actionParam.actionParam.notiParam.noti = (PFN_ImcNoti)AUDIO_SignalDSP1Ready;
	actionParam.actionParam.notiParam.param = NULL;
	actionParam.actionParam.notiParam.level = 1;
	IMC_RegisterEvent(IMC_GetLocalImc(0), ADEC_EVT_DSP_DN_DONE, ADEC_MODULE_MAN_DSP1, &actionID, &actionParam);

	//Wait if DSP1 is ready
	wait_event_interruptible_timeout(AudWaitDsp1Ready, g_AudKdrv.IsDSP1Ready != 0, msecs_to_jiffies(WAIT_DSP1_READY_TIMEOUT));

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		(void)H14_A0_AUDIO_DisplayDSP1VerInfo(startTick);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		(void)M14_A0_AUDIO_DisplayDSP1VerInfo(startTick);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		(void)H13_AUDIO_DisplayDSP1VerInfo(startTick);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return;
}

/**
 * Signal as DSP1 is ready.
 * @see
*/
static void AUDIO_SignalDSP1Ready(void* _param, int _paramLen, void* _cbParam)
{
	g_AudKdrv.IsDSP1Ready = 1;
	wake_up_interruptible(&AudWaitDsp1Ready);

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return;
}

/**
 * Wait for DSP0 Ready!.
 * @see
*/
static void AUDIO_WaitForDSP0Ready ( UINT64 startTick )
{
	UINT32  actionID;
	ImcActionParameter actionParam;

	AUD_KDRV_PRINT("Wait %d ms for DSP0 to Ready!!!\n", WAIT_DSP0_READY_TIMEOUT);
	actionParam.actionType = IMC_ACTION_GET_CALLBACK;
	actionParam.repeatType = IMC_ACTION_ONCE;
	actionParam.target = ADEC_MODULE_MAN_ARM;
	actionParam.actionParam.notiParam.noti = (PFN_ImcNoti)AUDIO_SignalDSP0Ready;
	actionParam.actionParam.notiParam.param = NULL;
	actionParam.actionParam.notiParam.level = 1;
	IMC_RegisterEvent(IMC_GetLocalImc(0), ADEC_EVT_DSP_DN_DONE, ADEC_MODULE_MAN_DSP0, &actionID, &actionParam);

	//Wait if DSP0 is ready
	wait_event_interruptible_timeout(AudWaitDsp0Ready, g_AudKdrv.IsDSP0Ready != 0, msecs_to_jiffies(WAIT_DSP0_READY_TIMEOUT));

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		(void)H14_A0_AUDIO_DisplayDSP0VerInfo(startTick);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		(void)M14_A0_AUDIO_DisplayDSP0VerInfo(startTick);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		(void)H13_AUDIO_DisplayDSP0VerInfo(startTick);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_PRINT("%s is called!!!\n", __F__);
	return;
}

/**
 * Signal as DSP1 is ready.
 * @see
*/
static void AUDIO_SignalDSP0Ready(void* _param, int _paramLen, void* _cbParam)
{
	g_AudKdrv.IsDSP0Ready = 1;
	wake_up_interruptible(&AudWaitDsp0Ready);

	AUD_KDRV_PRINT("AUDIO_SignalDSP0Ready\n");
	return;
}

/**
 * Stall a audio DSP module to reset.
 * @see
*/
static SINT32 AUDIO_StallDspToReset ( void )
{
	SINT32 retVal = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		retVal = H14_A0_AUDIO_StallDspToReset();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		retVal = M14_A0_AUDIO_StallDspToReset();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		retVal = H13_AUDIO_StallDspToReset();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_PRINT("%s!!!\n", __F__);
	return retVal;
}



