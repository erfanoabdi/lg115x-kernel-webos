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



/** @file	audio_kdrv_master.c
 *
 *  main driver implementation for  audio master device.
 *  audio master device will teach you how to make device driver with new platform.
 *
 *  author	wonchang.shin (wonchang.shin@lge.com)
 *  version	0.1
 *  date		2012.04.25
 *  note		Additional information.
 *
 *  @addtogroup lg1150_audio
 *	@{
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>    /**< For isr */
#include <linux/irq.h>			/**< For isr */
#include <linux/ioport.h>		/**< For request_region, check_region etc */
#include <linux/rmap.h>
#include <linux/kthread.h>
#include <asm/io.h>				/**< For ioremap_nocache */
#include <asm/memory.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/version.h>

#ifdef KDRV_CONFIG_PM	// added by SC Jung for quick booting
#include <linux/platform_device.h>
#endif

#include "os_util.h"
#include "base_device.h"

#include "debug_util.h"
#include "audio_cfg.h"
#include "audio_drv.h"
#include "audio_drv_hal.h"

#include "audio_imc_func.h"
#include "audio_buffer.h"

#include "audio_drv_master.h"
#include "audio_drv_debug.h"
#include "audio_drv_manager.h"
#include "audio_drv_decoder.h"
#include "audio_drv_renderer.h"

#define NUM_OF_AUD_PRINT_TYPE	32
#define NUM_OF_AUD_PRINT_COLOR	8

#undef ENABLE_SE_PARAM_DEBUG

#define NUM_OF_LGSEFN004MODE1_BASIC_PARAM	2
#define NUM_OF_LGSEFN004MODE1_PARAM			5

// for multichannel speakers
#define	DEFAULT_I2S_CH		2
#define DEFAULT_NUM_OF_I2S	1

#define DEFAULT_BYTE_PER_SEC		(48000 * 2 * 4)

#ifdef KDRV_CONFIG_PM
#define	SE_PARAM_SIZE	16*1024
#endif

#define	AUD_SE_ADDITIONAL_DELAY	5
#define	USE_BUFFER_DELAY
#define	DEFAULT_MASTER_DELAY		10

OS_SEM_T	_gMasterSema;		// renderer semaphore

#define AUD_MAS_LOCK_INIT()		OS_InitMutex(&_gMasterSema, OS_SEM_ATTR_DEFAULT)
#define AUD_MAS_LOCK()			OS_LockMutex(&_gMasterSema)
#define AUD_MAS_UNLOCK()		OS_UnlockMutex(&_gMasterSema)

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern LX_AUD_KDRV_T				g_AudKdrv;

//define register address
extern LX_AUD_REG_CFG_T 			g_stAudRegCfg[];
extern LX_AUD_REG_CFG_T 			*pstAudRegCfg;
extern LX_AUD_MEM_CFG_S_T 			*pstMemCfgAud;
extern UINT32						*g_pui32SEParamVirAddr;

extern UINT32						g_audModIpcPrint;
extern SINT32						g_AudDebugFd;
extern UINT32						g_audIpcPrintCheckErrorFlag;

extern AUD_RENDER_INFO_T			_gRenderInfo[DEV_REN_NUM];
extern AUD_RENDER_INPUT_CTRL_T		_gInputCtrlInfo[DEV_REN_NUM];

extern BOOLEAN	g_bEnableDspMonitor;

// DTO Rate
extern LX_AUD_CLOCK_SRC_T	g_setClockSrc;

#ifdef KDRV_CONFIG_PM
extern BOOLEAN g_audio_open_done;
#endif

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
AUD_MASTER_INFO_T	_gMasterInfo;
AUD_DEVICE_T		*_gMasterDev;

// For Main Decoder Index for Clock Setting and SPDIF ES Output
AUD_DECODER_ID_T _gMainDecoderIndex = AUD_DECODER_0;

/* AUDIO Debug Print Type */
UINT8 *ui8AudPrintType[NUM_OF_AUD_PRINT_TYPE]  =
						{ "AUD_KDRV_ERROR", "AUD_KDRV_WARN", "AUD_KDRV_NOTI", "AUD_KDRV_INFO", "AUD_KDRV_DEBUG",	\
						  "AUD_KDRV_TRACE", "AUD_KDRV_PRINT", "AUD_KDRV_ISR_DEBUG", "AUD_KDRV_MIP_DEBUG", "AUD_KDRV_DEBUG_TMP",	\
						  "AUD_KDRV_ALSA_PRINT", "AUD_KDRV_ALSA_DEBUG", "AUD_KDRV_IPC_DEBUG", "AUD_KDRV_IPC_MSG0", "AUD_KDRV_IPC_MSG1",	\
						  "AUD_KDRV_IPC_DBG_ERR", "AUD_KDRV_RM", "AUD_KDRV_RMD", "AUD_KDRV_MIP_I2C", "AUD_KDRV_IMC_NOTI",
						  "AUD_KDRV_IMC_COM", "AUD_KDRV_IMC_DEBUG","AUD_KDRV_PRINTF"};

/* AUDIO Debug Print Color */
UINT8 *ui8AudPrintColor[NUM_OF_AUD_PRINT_COLOR] = {
			"COLOR_BLACK",
			"COLOR_RED",
			"COLOR_GREEN",
			"COLOR_YELLOW",
			"COLOR_BLUE",
			"COLOR_PURPLE",
			"COLOR_CYAN",
			"COLOR_GRAY",
};

#ifdef KDRV_CONFIG_PM
UINT8	gSeParam[SE_PARAM_SIZE];
#endif

//This value is set by ADEC_DRV_InitModule function
static LX_AUD_MASTER_SPDIF_OUTPUT_T	g_AudSpdifOutputType = LX_AUD_MASTER_SPDIF_PCM;
static struct task_struct *_gpstAudioMonitorTask = NULL;		//Audio Monitor task

// AUDIO_OpenMaster can be called by ALSA
static BOOLEAN _gbMasterInit = FALSE;

// For SPDIF Output PCM Connection
static UINT32 _gSpdifOutPcmCnt = 0;

// ResetConnect
static UINT32	_gPreConnectDev[LX_AUD_MASTER_OUTPUT_MAX] = {LX_AUD_DEV_MAS0, LX_AUD_DEV_MAS0, LX_AUD_DEV_MAS0, 0};

static SINT32 AUDIO_SetVolume(LX_AUD_MASTER_CONFIG_VOLUME_T *pVolumeConfig);
static SINT32 AUDIO_SetMute(LX_AUD_MASTER_CONFIG_MUTE_T *pMuteConfig);
static SINT32 AUDIO_SetDelay(LX_AUD_MASTER_CONFIG_DELAY_T *pDelayConfig);
static SINT32 AUDIO_SetOutMode(LX_AUD_MASTER_CONFIG_OUT_MODE_T *pOutModeConfig);
static SINT32 AUDIO_SetSpdifOutput(LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T *pSpdifOutputConfig);
static SINT32 AUDIO_GetSpdifOutput(LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T *pSpdifOutputConfig);
static SINT32 AUDIO_SetSpdifScms(LX_AUD_MASTER_CONFIG_SPDIF_SCMS_T *pSpdifScmsConfig);

static SINT32 AUDIO_SetBalance(LX_AUD_MASTER_CONFIG_BALANCE_T *pBalanceConfig);
static SINT32 AUDIO_SetBass(LX_AUD_MASTER_CONFIG_BASS_T *pBassConfig);
static SINT32 AUDIO_SetTreble(LX_AUD_MASTER_CONFIG_TREBLE_T *pTrebleConfig);
static SINT32 AUDIO_SetADCInputPort(UINT32 adcInputPort);
static SINT32 AUDIO_SetPowerCtrl(LX_AUD_MASTER_CONFIG_POWER_CTRL_T *pPowerCtrlConfig);
static SINT32 AUDIO_SetAADSIFInputPort(LX_AUD_SIF_INPUT_T sifSource);

static SINT32 AUDIO_SetSpeakerOutput(LX_AUD_MASTER_CONFIG_SPK_OUTPUT_T *pSpkOutputConfig);
static SINT32 AUDIO_SetSEFunction(LX_AUD_MASTER_PARAM_SE_FN_T *pSeFnParam);
static SINT32 AUDIO_GetSEFunction(LX_AUD_MASTER_PARAM_SE_FN_T *pSeFnParam);
static SINT32 AUDIO_SetMixerDelay(LX_AUD_MASTER_CONFIG_MIXER_DELAY_T *pMixerDelayConfig);
static SINT32 AUDIO_SetDacVolume(LX_AUD_MASTER_CONFIG_DAC_VOLUME_T *pDacVolumeConfig);
static SINT32 AUDIO_SetDacMute(LX_AUD_MASTER_CONFIG_DAC_MUTE_T *pDacMuteConfig);
static SINT32 AUDIO_SetEvent(LX_AUD_EVENT_TYPE_T *pEvent);
static SINT32 AUDIO_GetEvent(LX_AUD_EVENT_TYPE_T *pEvent);
static AUD_SE_FN_MODE_T AUDIO_GetSeFnMode(LX_AUD_MASTER_SE_FN_T seFnMode);

static SINT32 AUDIO_EnableSoundBar(LX_AUD_MASTER_SOUNDBAR_T bOnOff);
static SINT32 AUDIO_SetSoundBarParam(LX_AUD_MASTER_SOUNDBAR_PARAM_T *pSoundbarParam);
static SINT32 AUDIO_SetSoundBarPower(UINT32 bOnOff);
static SINT32 AUDIO_SetSoundBarCmd(LX_AUD_MASTER_SOUNDBAR_CMD_T *pSoundbarCmd);

static SINT32 AUDIO_SetMainDecoderIndex(UINT32 mainDecoderIdx);
static SINT32 AUDIO_GetMainDecoderIndex(UINT32 *pMainDecoderIdx);

static SINT32 AUDIO_SetSpdifOnOff(UINT32 bOnOff);
static SINT32 AUDIO_SetSpdifCategoryCode(UINT32 categoryCode);
static SINT32 AUDIO_SetI2SNumber(UINT32 number);

static SINT32 AUDIO_GetMasterDelay(UINT32 allocDev, UINT32 *pDelay);
static SINT32 AUDIO_ConnectIndexOutput(UINT32 index, LX_AUD_MASTER_OUTPUT_T output,
								LX_AUD_MASTER_SPDIF_OUTPUT_T outType, UINT32 bUseSE);
static SINT32 AUDIO_DisconnectIndexOutput(UINT32 index, LX_AUD_MASTER_OUTPUT_T output,
								LX_AUD_MASTER_SPDIF_OUTPUT_T outType);
static SINT32 AUDIO_GetIndexDelay(LX_AUD_GET_INDEX_DELAY_T *pGetIndexDelay);
static SINT32 _AUDIO_MixerUnderflowCb(void *_param, SINT32 _paramLen, void *_cbParam);
static SINT32 _AUDIO_SeToneDetectCb(void *_param, SINT32 _paramLen, void *_cbParam);
static SINT32 AUDIO_SetInputVolume(UINT32 index, UINT32 volume);
static SINT32 AUDIO_SetInputMute(UINT32 index, UINT32 mute);
static SINT32 AUDIO_SetInputDelay(UINT32 index, UINT32 delay);

static SINT32  AUDIO_GetSEParam(LX_AUD_MASTER_PARAM_SE_FN_T *pSeFnParam);
static SINT32 AUDIO_GetStartInfo(LX_AUD_GET_START_INFO_T	*pGetStartInfo);

/**
 * Register Notification.
 * @param 	pMasterInfo		[in] Master information.
 * @param 	pFuncImcNoti	[in] pointer to callback function.
 * @param 	allocMod		[in] allocated module.
 * @param 	event			[in] event.
 * @return 	void.
 * @see		().
 */
static void _AUDIO_RegisterMasterNoti(
	AUD_MASTER_INFO_T* pMasterInfo,
	PFN_ImcNoti	pFuncImcNoti,
	UINT32 allocMod,
	UINT32 event,
	IMC_ACTION_REPEAT_TYPE repeatType,
	SINT32 notiLevel)
{
	UINT32							actionID;
	AUD_EVENT_T						*pMasterEvent = NULL;
	ImcActionParameter 				actionParam;

	if(pMasterInfo->ui32EventNum >= AUD_EVENT_NUM)
	{
		AUD_KDRV_ERROR("MasterEventNum(%d) is over AUD_EVENT_NUM. \n", pMasterInfo->ui32EventNum);
		return;
	}

	actionParam.actionType = IMC_ACTION_GET_CALLBACK;
	actionParam.repeatType = repeatType;
	actionParam.target = ADEC_MODULE_MAN_ARM;
	actionParam.actionParam.notiParam.noti = (PFN_ImcNoti)pFuncImcNoti;
	actionParam.actionParam.notiParam.param = pMasterInfo;
	actionParam.actionParam.notiParam.level = notiLevel;
	IMC_RegisterEvent(IMC_GetLocalImc(0), event, allocMod, &actionID, &actionParam);

	if(repeatType != IMC_ACTION_ONCE)
	{
		pMasterEvent = &pMasterInfo->mstEvent[pMasterInfo->ui32EventNum];
		pMasterEvent->event = event;
		pMasterEvent->actionID = actionID;
		pMasterEvent->moduleID = allocMod;
		pMasterInfo->ui32EventNum++;
	}

	AUD_KDRV_IMC_NOTI("ev(0x%x) actID(0x%x), mod(0x%x)\n", event, actionID, allocMod);
	return;
}

/**
 * Initialize master Information.
 * @param 	void
 * @return 	void
 * @see		KDRV_AUDIO_OpenMaster
 */
static void _AUDIO_InitMasterInfo(void)
{
	UINT32	i = 0;

	AUD_OUT_CTRL_MODULE_T audOutCtrlMod[LX_AUD_MASTER_OUTPUT_MAX] =
	{
		// ADEC_MODULE_ID, 			ui32Volume,	ui32Mute,	ui32Delay,	ui32Balance, 	outMode,				ui32ConnectDev,		ui32UseSE
		{ ADEC_MODULE_OUT_CTRL_0, 	0x800000, 	0, 			0, 			50,				LX_AUD_MASTER_OUT_LR,	LX_AUD_DEV_MAS0,		TRUE},
		{ ADEC_MODULE_OUT_CTRL_1,	0x800000,	0,			0,			50,				LX_AUD_MASTER_OUT_LR,	LX_AUD_DEV_MAS0,		FALSE},
		{ ADEC_MODULE_OUT_CTRL_2,	0x800000,	0,			0,			50,				LX_AUD_MASTER_OUT_LR,	LX_AUD_DEV_MAS0,		FALSE},
		{ ADEC_MODULE_OUT_CTRL_3,	0x800000,	0,			0,			50,				LX_AUD_MASTER_OUT_LR,	LX_AUD_DEV_NONE,		FALSE},
	};

	memset(&_gMasterInfo, 0, sizeof(AUD_MASTER_INFO_T));

	_gMasterInfo.ui32AllocDev 	   = LX_AUD_DEV_NONE;
	_gMasterInfo.spdifSamplingFreq = LX_AUD_SAMPLING_FREQ_48_KHZ;

	memcpy(&(_gMasterInfo.outCtrlMod[LX_AUD_MASTER_OUTPUT_SPEAKER]),
		   &audOutCtrlMod[LX_AUD_MASTER_OUTPUT_SPEAKER],
		   sizeof(AUD_OUT_CTRL_MODULE_T)*LX_AUD_MASTER_OUTPUT_MAX);

	_gMasterInfo.soundBarStatus.ui32SoundBarOnOff   = FALSE;
	_gMasterInfo.soundBarStatus.ui32WirelessOnOff   = FALSE;
	_gMasterInfo.soundBarStatus.ui32SoundFrameOnOff = FALSE;

	_gMasterInfo.ui32I2SNum = DEFAULT_NUM_OF_I2S;
	_gMasterInfo.spdifCategoryCode = 0x00;

	for(i = 0; i < MAX_IN_PORT; i++)
	{
		_gMasterInfo.mixerInPortVolume[i] = 0x400;
	}

	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_MixerUnderflowCb, ADEC_MODULE_MIX_0,
		MIXER_EVT_UNDERFLOW, IMC_ACTION_REPEAT, 1);

	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_SeToneDetectCb, ADEC_MODULE_SE,
		SE_EVT_DETECT_TONE, IMC_ACTION_REPEAT, 1);

	AUD_MAS_LOCK_INIT();

	//Initialize AUDIO SPDIF semaphore
	OS_InitMutex(&_gMasterInfo.spdifSemaphore, OS_SEM_ATTR_DEFAULT);

	return;
}

/**
 * Sound Bar Status Callback function.
 * @param 	_param			[out] parameters.
 * @param 	_paramLen		[out] length of parameters.
 * @param 	_cbParam		[in] callback parameters.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		AUDIO_SetSoundBarParam
 */
static SINT32 _AUDIO_MstSoundBarStatusCb(void* _param, int _paramLen, void* _cbParam)
{
	SpdifEvtGetFmtForSoundbar *pSoundBarStatus = NULL;

	pSoundBarStatus = (SpdifEvtGetFmtForSoundbar*)_param;

	_gMasterInfo.soundBarStatus.ui32SubFrameID = pSoundBarStatus->id;
	_gMasterInfo.soundBarStatus.ui32SubFrameData = pSoundBarStatus->data;
	_gMasterInfo.soundBarStatus.ui32SubFrameCheckSum = pSoundBarStatus->checksum;

	_gMasterInfo.soundBarStatus.ui32SubFrameData |= (pSoundBarStatus->reserved << 16);

	// Print For Debug
	if(_paramLen != sizeof(SpdifEvtGetFmtForSoundbar))
	{
		AUD_KDRV_ERROR("SoundBar Status : Param Length Error[Expected:%d][Input:%d]\n", sizeof(SpdifEvtGetFmtForSoundbar), _paramLen);
		return RET_ERROR;
	}

	AUD_KDRV_DEBUG("SoundBar Status : id(0x%x), data(0x%x), res(0x%x), CS(0x%x)\n", \
					pSoundBarStatus->id, pSoundBarStatus->data, pSoundBarStatus->reserved, pSoundBarStatus->checksum);
	AUD_KDRV_DEBUG("SoundBar Status : ui32SubFrameData(0x%x)\n", \
					_gMasterInfo.soundBarStatus.ui32SubFrameData);
	return RET_OK;
}

/**
 * DTO_A Rate Callback function.
 * @param 	_param			[out] parameters.
 * @param 	_paramLen		[out] length of parameters.
 * @param 	_cbParam		[in] callback parameters.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		KDRV_AUDIO_OpenMaster
 */
static SINT32 _AUDIO_MstDtoARateCb(void* _param, int _paramLen, void* _cbParam)
{
	DtoEvtChangeDtoRate	*pDtoRate = NULL;

	pDtoRate = (DtoEvtChangeDtoRate*)_param;

	if(_paramLen != sizeof(DtoEvtChangeDtoRate))
	{
		AUD_KDRV_ERROR("Param Length Error : [Expected:%d][Input:%d]\n", sizeof(DtoEvtChangeDtoRate), _paramLen);
		return RET_ERROR;
	}

	//Lock a AUDIO SPDIF Semaphore, Do not use spin lock
	OS_LockMutex(&_gMasterInfo.spdifSemaphore);

	//Set a SPDIF clock for M14 B0 Chip Bug
	if(lx_chip_rev() == LX_CHIP_REV(M14, B0)) // M14B0 only bug
	{
		//Set a decoder sampling frequency and DTO rate
		(void)MIXED_IP_AUDIO_SetSPDIFSamplingFreq(_gMasterInfo.spdifSamplingFreq, pDtoRate->dto_rate, AUD_DTO_D);

		AUD_KDRV_DEBUG("DTO_D Rate(%d)\n", pDtoRate->dto_rate);
	}
	else
	{
		//Set a decoder sampling frequency and DTO rate
		(void)MIXED_IP_AUDIO_SetSPDIFSamplingFreq(_gMasterInfo.spdifSamplingFreq, pDtoRate->dto_rate, AUD_DTO_A);

		AUD_KDRV_DEBUG("DTO_A Rate(%d)\n", pDtoRate->dto_rate);
	}

	//Unlock a AUDIO SPDIF Semaphore
	OS_UnlockMutex(&_gMasterInfo.spdifSemaphore);

	return RET_OK;
}


/**
 * Alloc Master's Modules.
 * @param 	void
 * @return 	void
 * @see		KDRV_AUDIO_OpenMaster
 */
static SINT32 _AUDIO_AllocMaster(LX_AUD_DEV_T allocDev)
{
	SINT32		retVal = 0;

	AUD_MASTER_INFO_T		*pMstInfo = NULL;
	ADEC_MODULE_ID			allocMod_MIX = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_SE = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_OUT_CTRL_0 = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_OUT_CTRL_1 = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_OUT_CTRL_2 = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_OUT_CTRL_3 = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_ES_OUT_CTRL = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			allocMod_OUT_SPDIF = ADEC_MODULE_NOT_DEF;

	pMstInfo = &_gMasterInfo;

	allocMod_MIX = AUDIO_AllocModule(ADEC_MOD_TYPE_MIXER, ADEC_CORE_DSP1, ADEC_MODULE_MIX_0);
	if(allocMod_MIX == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_MIX);
		return RET_ERROR;
	}

	retVal = AUDIO_InsertModuleLast(allocDev, allocMod_MIX, AUD_BUFFER_TYPE_NONE);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("Connection_InsertLast(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	allocMod_SE = AUDIO_AllocModule(ADEC_MOD_TYPE_SE, ADEC_CORE_DSP1, ADEC_MODULE_SE);
	if(allocMod_SE == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_SE);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_SE);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	allocMod_OUT_CTRL_0 = AUDIO_AllocModule(ADEC_MOD_TYPE_OUTCTRL, ADEC_CORE_DSP1, ADEC_MODULE_OUT_CTRL_0);
	if(allocMod_OUT_CTRL_0 == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_OUT_CTRL_0);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_OUT_CTRL_0);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	allocMod_OUT_CTRL_1 = AUDIO_AllocModule(ADEC_MOD_TYPE_OUTCTRL, ADEC_CORE_DSP1, ADEC_MODULE_OUT_CTRL_1);
	if(allocMod_OUT_CTRL_1 == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_OUT_CTRL_1);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_OUT_CTRL_1);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	allocMod_OUT_CTRL_2 = AUDIO_AllocModule(ADEC_MOD_TYPE_OUTCTRL, ADEC_CORE_DSP1, ADEC_MODULE_OUT_CTRL_2);
	if(allocMod_OUT_CTRL_2 == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_OUT_CTRL_2);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_OUT_CTRL_2);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	allocMod_OUT_CTRL_3 = AUDIO_AllocModule(ADEC_MOD_TYPE_OUTCTRL, ADEC_CORE_DSP1, ADEC_MODULE_OUT_CTRL_3);
	if(allocMod_OUT_CTRL_3 == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_OUT_CTRL_3);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_OUT_CTRL_3);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	allocMod_ES_OUT_CTRL = AUDIO_AllocModule(ADEC_MOD_TYPE_ESOUTCTRL, ADEC_CORE_DSP1, ADEC_MODULE_ESOUT_CTRL);
	if(allocMod_ES_OUT_CTRL == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_ES_OUT_CTRL);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_ES_OUT_CTRL);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	allocMod_OUT_SPDIF = AUDIO_AllocModule(ADEC_MOD_TYPE_SINK, ADEC_CORE_DSP1, ADEC_MODULE_OUT_SPDIF);
	if(allocMod_OUT_SPDIF == ADEC_MODULE_NOT_DEF)
	{
		AUD_KDRV_ERROR("Module[%d] is not available \n", allocMod_OUT_SPDIF);
		return RET_ERROR;
	}

	retVal = AUDIO_AddModule(allocDev, allocMod_OUT_SPDIF);
	if(retVal != 0)
	{
		AUD_KDRV_ERROR("AUDIO_AddModule(%d) failed(%d)!!!\n", allocMod_OUT_CTRL_0, retVal);
		return RET_ERROR;
	}

	// Register DTO Rate Callback function
	_AUDIO_RegisterMasterNoti(pMstInfo, (PFN_ImcNoti)_AUDIO_MstDtoARateCb, ADEC_MODULE_DTO_0, DTO_EVT_CHANGE_DTO_RATE, IMC_ACTION_REPEAT, 1);

	AUDIO_IMC_SendCmd(ADEC_CMD_START,allocMod_MIX);

	return RET_OK;
}

/**
 * Dealloc Master's Modules.
 * @param 	void
 * @return 	void
 * @see		KDRV_AUDIO_OpenMaster
 */
static SINT32 _AUDIO_DeallocMaster(LX_AUD_DEV_T allocDev)
{
	SINT32					i = 0;
	SINT32					retVal = 0;
	AUD_EVENT_T				*pMstEvent = NULL;
	AUD_DEV_INFO_T			*pDevInfo = NULL;
	AUD_MOD_INFO_T			*pModInfo = NULL;
	AUD_MASTER_INFO_T		*pMasterInfo = NULL;

	if( allocDev != LX_AUD_DEV_MAS0)
	{
		AUD_KDRV_ERROR("allocDev [%d]\n", allocDev);
		return RET_ERROR;
	}

	pMasterInfo = &_gMasterInfo;

	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_MIX_0);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_OUT_CTRL_0);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_OUT_CTRL_1);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_OUT_CTRL_2);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_OUT_CTRL_3);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_ESOUT_CTRL);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_SE);

	retVal = AUDIO_DisconnectAllModules(allocDev);
	if(retVal == RET_ERROR)
	{
		AUD_KDRV_DEBUG("AUDIO_DisconnectAllModules(%d) failed.\n", allocDev);
	}

	pDevInfo = AUDIO_GetDevInfo(allocDev);
	if(pDevInfo == NULL)
	{
		AUD_KDRV_DEBUG("pModInfo[%d] is NULL.\n", allocDev);
		return RET_ERROR;
	}

	for(i = (pDevInfo->numMod - 1); i >= 0; i--)
	{
		pModInfo = pDevInfo->pModInfo[i];
		if(pModInfo == NULL)
		{
			AUD_KDRV_DEBUG("pModInfo[%d] is NULL.\n", allocDev);
			continue;
		}

		retVal = AUDIO_RemoveModule(allocDev, pModInfo->mod);
		if(retVal < RET_OK)
		{
			AUD_KDRV_DEBUG("AUDIO_RemoveModule(%d, %d) failed(%d)!!!\n", \
				allocDev, pModInfo->mod, retVal);
			continue;
		}

		AUDIO_FreeModule(pModInfo->mod);

		pDevInfo->pModInfo[i] = NULL;
	}

	// Initialize Event
	for(i = 0; i < pMasterInfo->ui32EventNum; i++)
	{
		pMstEvent = &(pMasterInfo->mstEvent[i]);
		IMC_CancelEvent(IMC_GetLocalImc(0), pMstEvent->event , pMstEvent->moduleID, pMstEvent->actionID);
	}

	pMasterInfo->ui32EventNum = 0;

	return RET_OK;
}

/**
 * Check if Other Output uses SE module.
 * @param 	output			[in] setting output.
 * @return 	bUseSe.
 * @see		AUDIO_ConnectOutput(), AUDIO_DisconnectOutput()
 */
static UINT32 _AUDIO_CheckUseSE(LX_AUD_MASTER_OUTPUT_T output)
{
	SINT32	i;
	UINT32	bUseSe = FALSE;

	for(i = 0; i < LX_AUD_MASTER_OUTPUT_MAX; i++)
	{
		if(i != output)
		{
			if(_gMasterInfo.outCtrlMod[i].ui32UseSE == TRUE)
			{
				bUseSe = TRUE;
				break;
			}
		}
	}

	AUD_KDRV_DEBUG("Other Output(%d) uses SE\n", i);

	return bUseSe;
}

/**
 * open handler for audio master device
 *
 */
SINT32	KDRV_AUDIO_OpenMaster(struct inode *inode, struct file *filp)
{
	UINT32		nMajor;
	UINT32		nMinor;

	struct cdev				*pCdev;

	LX_AUD_DEV_T			allocDevice = LX_AUD_DEV_NONE;

	nMajor = imajor(inode);
	nMinor = iminor(inode);

	pCdev	= inode->i_cdev;
	_gMasterDev	= container_of(pCdev, AUD_DEVICE_T, cdev);
	filp->private_data = _gMasterDev;

	_gMasterDev->devType = LX_AUD_DEV_TYPE_MAS;

	allocDevice = AUDIO_OpenMaster();
	if(allocDevice == LX_AUD_DEV_NONE)
		return RET_ERROR;

	_gMasterDev->allocDev =  allocDevice;

	AUD_KDRV_PRINT("Open Master device file (%d:%d)\n", nMajor, nMinor);

	#ifdef KDRV_CONFIG_PM
	g_audio_open_done = TRUE;
	#endif

	return RET_OK;
}

/**
 * close handler for audio master device
 *
 */
SINT32 KDRV_AUDIO_CloseMaster(struct inode *inode, struct file *filp)
{
	SINT32		retVal = RET_OK;
	UINT32		nMajor;
	UINT32		nMinor;

	LX_AUD_DEV_T				allocDevice;

	_gMasterDev = (AUD_DEVICE_T*)filp->private_data;
	if(_gMasterDev == NULL)
	{
		AUD_KDRV_ERROR("private_data is NULL\n");
		return RET_ERROR;
	}

	allocDevice = _gMasterDev->allocDev;

	retVal = AUDIO_CloseMaster(allocDevice);

	nMajor = imajor(inode);
	nMinor = iminor(inode);

	AUD_KDRV_PRINT("Close Master device file(%d:%d)\n", nMajor, nMinor);
	return RET_OK;
}

UINT32	AUDIO_OpenMaster(void)
{
	LX_AUD_DEV_T			allocDevice = LX_AUD_DEV_NONE;

	if(_gbMasterInit != TRUE)
	{
		//Initializes the audio module.
		(void)AUDIO_InitModule();
		(void)AUDIO_DevInfoInit();
		_AUDIO_InitMasterInfo();

		_gbMasterInit = TRUE;
	}

	AUD_MAS_LOCK();

	if(_gMasterInfo.openCount == 0)
	{
		allocDevice = AUDIO_AllocDev(LX_AUD_DEV_TYPE_MAS);
		if(allocDevice == LX_AUD_DEV_NONE)
		{
			AUD_MAS_UNLOCK();
			AUD_KDRV_ERROR("AUDIO_AllocDev(%d) failed.\n", LX_AUD_DEV_TYPE_MAS);
			return LX_AUD_DEV_NONE;
		}

		_AUDIO_AllocMaster(allocDevice);

		// create audio master monitor task
		if(_gpstAudioMonitorTask == NULL)
		{
			_gpstAudioMonitorTask = kthread_run(AUDIO_MonitorTask, NULL, "AUD-KDRV-TASK");

			if (_gpstAudioMonitorTask != NULL)
				AUD_KDRV_PRINT("Monitor task is created\n");
			else
				AUD_KDRV_ERROR("Monitor task creation ERR!!!\n");
		}

		_gMasterInfo.ui32AllocDev = allocDevice;
	}
	else
	{
		allocDevice = _gMasterInfo.ui32AllocDev;
	}

	_gMasterInfo.openCount++;

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("openCount is %u\n", _gMasterInfo.openCount);

	return allocDevice;
}

/**
 * Function for Close Master.
 * Close Master temporary.
 * @see
*/
SINT32 AUDIO_CloseMaster(UINT32 allocDev)
{
	LX_AUD_DEV_T		freeDevice = allocDev;

	AUD_MAS_LOCK();

	if(_gMasterInfo.openCount > 0)
		_gMasterInfo.openCount--;

	if(_gMasterInfo.openCount > 0)
	{
		AUD_MAS_UNLOCK();
		AUD_KDRV_PRINT("openCount is %u\n", _gMasterInfo.openCount);
		return RET_OK;
	}

	_AUDIO_DeallocMaster(freeDevice);
	AUDIO_FreeDev(freeDevice);

	// release monitor task
	if(_gpstAudioMonitorTask != NULL)
	{
		kthread_stop(_gpstAudioMonitorTask);
		_gpstAudioMonitorTask = NULL;

		AUD_KDRV_PRINT("Stop monitor task\n");
	}

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("openCount is %u\n", _gMasterInfo.openCount);

	return RET_OK;
}


/**
 * ioctl handler for audio master device.
 *
 *
 * note: if you have some critial data, you should protect them using semaphore or spin lock.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
int KDRV_AUDIO_IoctlMaster(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
long KDRV_AUDIO_IoctlMaster(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	SINT32				retVal = 0;
	SINT32				err = 0;
	AUD_DEVICE_T		*pDev;

	pDev = (AUD_DEVICE_T*)filp->private_data;

	/*
	* check if IOCTL command is valid or not.
	* - if magic value doesn't match, return error (-ENOTTY)
	* - if command is out of range, return error (-ENOTTY)
	*
	* note) -ENOTTY means "Inappropriate ioctl for device.
	*/
	if(_IOC_TYPE(cmd) != AUD_MASTER_IOC_MAGIC)
	{
		DBG_PRINT_WARNING("invalid magic. magic=0x%02X\n", _IOC_TYPE(cmd));
		return -ENOTTY;
	}
	if(_IOC_NR(cmd) > AUD_MASTER_IOC_MAXNR)
	{
		DBG_PRINT_WARNING("out of ioctl command. cmd_idx=%d\n", _IOC_NR(cmd));
		return -ENOTTY;
	}

	/*
	* check if user memory is valid or not.
	* if memory can't be accessed from kernel, return error (-EFAULT)
	*/
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
	{
		DBG_PRINT_WARNING("memory access error. cmd_idx=%d, rw=%c%c, memptr=%p\n",
							_IOC_NR(cmd),
							(_IOC_DIR(cmd) & _IOC_READ)? 'r':'-',
							(_IOC_DIR(cmd) & _IOC_WRITE)? 'w':'-',
							(void*)arg );
		return -EFAULT;
	}

	AUD_KDRV_TRACE("cmd = %08X (cmd_idx=%d)\n", cmd, _IOC_NR(cmd) );

	switch(cmd)
	{
		case AUD_MASTER_IOW_SET_VOLUME:
		{
			LX_AUD_MASTER_CONFIG_VOLUME_T		volumeConfig;

			AUD_KDRV_TRACE("IOCTL Set Volume!!!\n");

			if(copy_from_user(&volumeConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_VOLUME_T)))
				return RET_ERROR;

			retVal = AUDIO_SetVolume(&volumeConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_DAC_VOLUME:
		{
			LX_AUD_MASTER_CONFIG_DAC_VOLUME_T		dacVolumeConfig;

			AUD_KDRV_TRACE("IOCTL Set DAC Volume!!!\n");

			if(copy_from_user(&dacVolumeConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_DAC_VOLUME_T)))
				return RET_ERROR;

			retVal = AUDIO_SetDacVolume(&dacVolumeConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_MUTE:
		{
			LX_AUD_MASTER_CONFIG_MUTE_T		muteConfig;

			AUD_KDRV_TRACE("IOCTL Set Mute!!!\n");

			if(copy_from_user(&muteConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_MUTE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetMute(&muteConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_DAC_MUTE:
		{
			LX_AUD_MASTER_CONFIG_DAC_MUTE_T		dacMuteConfig;

			AUD_KDRV_TRACE("IOCTL Set DAC Mute!!!\n");

			if(copy_from_user(&dacMuteConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_DAC_MUTE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetDacMute(&dacMuteConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_DELAY:
		{
			LX_AUD_MASTER_CONFIG_DELAY_T		delayConfig;

			AUD_KDRV_TRACE("IOCTL Set Delay!!!\n");

			if(copy_from_user(&delayConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_DELAY_T)))
				return RET_ERROR;

			retVal = AUDIO_SetDelay(&delayConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_OUT_MODE:
		{
			LX_AUD_MASTER_CONFIG_OUT_MODE_T		outModeConfig;

			AUD_KDRV_TRACE("IOCTL Set Out Mode!!!\n");

			if(copy_from_user(&outModeConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_OUT_MODE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetOutMode(&outModeConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_SPK_OUTPUT:
		{
			LX_AUD_MASTER_CONFIG_SPK_OUTPUT_T		spkOutputConfig;

			AUD_KDRV_TRACE("IOCTL Set SPEAKER Output!!!\n");

			if(copy_from_user(&spkOutputConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_SPK_OUTPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_SetSpeakerOutput(&spkOutputConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_SPDIF_OUTPUT:
		{
			LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T		spdifOutputConfig;

			AUD_KDRV_TRACE("IOCTL Set SPDIF Output!!!\n");

			if(copy_from_user(&spdifOutputConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_SetSpdifOutput(&spdifOutputConfig);
		}
		break;

		case AUD_MASTER_IOR_GET_SPDIF_OUTPUT:
		{
			LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T		spdifOutputConfig;

			AUD_KDRV_TRACE("IOCTL Get SPDIF Output!!!\n");

			retVal = AUDIO_GetSpdifOutput(&spdifOutputConfig);

			if(copy_to_user((void *)arg, (void *)&spdifOutputConfig, sizeof(LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOW_SET_SPDIF_SCMS:
		{
			LX_AUD_MASTER_CONFIG_SPDIF_SCMS_T		spdifScmsConfig;

			AUD_KDRV_TRACE("IOCTL Set SPDIF SCMS!!!\n");

			if(copy_from_user(&spdifScmsConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_SPDIF_SCMS_T)))
				return RET_ERROR;

			retVal = AUDIO_SetSpdifScms(&spdifScmsConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_SPDIF_ONOFF:
		{
			UINT32		setSpdifOnOff;

			AUD_KDRV_TRACE("IOCTL Set SPDIF On/OFF!!!\n");

			if(copy_from_user(&setSpdifOnOff, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_SetSpdifOnOff(setSpdifOnOff);
		}
		break;

		case AUD_MASTER_IOW_SET_SPDIF_CATEGORY_CODE:
		{
			UINT32		categoryCode;

			AUD_KDRV_TRACE("IOCTL Set Category Code!!!\n");

			if(copy_from_user(&categoryCode, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_SetSpdifCategoryCode(categoryCode);
		}
		break;

		case AUD_MASTER_IOW_SET_BALANCE:
		{
			LX_AUD_MASTER_CONFIG_BALANCE_T		balancesConfig;

			AUD_KDRV_TRACE("IOCTL Set Balance!!!\n");

			if(copy_from_user(&balancesConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_BALANCE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetBalance(&balancesConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_BASS:
		{
			LX_AUD_MASTER_CONFIG_BASS_T		bassConfig;

			AUD_KDRV_TRACE("IOCTL Set Bass!!!\n");

			if(copy_from_user(&bassConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_BASS_T)))
				return RET_ERROR;

			retVal = AUDIO_SetBass(&bassConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_TREBLE:
		{
			LX_AUD_MASTER_CONFIG_TREBLE_T		trebleConfig;

			AUD_KDRV_TRACE("IOCTL Set TREBLE!!!\n");

			if(copy_from_user(&trebleConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_TREBLE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetTreble(&trebleConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_ADC_INPUT_PORT:
		{
			UINT32		adcInputPort;

			AUD_KDRV_TRACE("IOCTL Set ADC Input Port!!!\n");

			if(copy_from_user(&adcInputPort, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_SetADCInputPort(adcInputPort);
		}
		break;

		case AUD_MASTER_IOW_SET_POWER_CTRL:
		{
			LX_AUD_MASTER_CONFIG_POWER_CTRL_T		powerCtrlConfig;

			AUD_KDRV_TRACE("IOCTL Set Power Ctrl!!!\n");

			if(copy_from_user(&powerCtrlConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_POWER_CTRL_T)))
				return RET_ERROR;

			retVal = AUDIO_SetPowerCtrl(&powerCtrlConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_MIXER_DELAY:
		{
			LX_AUD_MASTER_CONFIG_MIXER_DELAY_T		mixerDelayConfig;

			AUD_KDRV_TRACE("IOW_SET_MIXER_DELAY!!!\n");

			if(copy_from_user(&mixerDelayConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_MIXER_DELAY_T)))
				return RET_ERROR;

			retVal = AUDIO_SetMixerDelay(&mixerDelayConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_MIXER_VOLUME:
		{
			LX_AUD_MASTER_CONFIG_MIXER_VOLUME_T		mixerVolumeConfig;

			AUD_KDRV_TRACE("IOW_SET_MIXER_DELAY!!!\n");

			if(copy_from_user(&mixerVolumeConfig, (void __user *)arg, sizeof(LX_AUD_MASTER_CONFIG_MIXER_VOLUME_T)))
				return RET_ERROR;

			retVal = AUDIO_SetMixerVolume(&mixerVolumeConfig);
		}
		break;

		case AUD_MASTER_IOW_SET_SE_FN:
		{
			LX_AUD_MASTER_PARAM_SE_FN_T		seFnParam;

			AUD_KDRV_TRACE("IOCTL Set SE FN!!!\n");

			if(copy_from_user(&seFnParam, (void __user *)arg, sizeof(LX_AUD_MASTER_PARAM_SE_FN_T)))
				return RET_ERROR;

			retVal = AUDIO_SetSEFunction(&seFnParam);
		}
		break;

		case AUD_MASTER_IOWR_GET_SE_FN:
		{
			LX_AUD_MASTER_PARAM_SE_FN_T		seFnParam;

			AUD_KDRV_TRACE("IOCTL Set SE FN!!!\n");

			if(copy_from_user(&seFnParam, (void __user *)arg, sizeof(LX_AUD_MASTER_PARAM_SE_FN_T)))
				return RET_ERROR;

			retVal = AUDIO_GetSEFunction(&seFnParam);

			if(copy_to_user((void *)arg, (void *)&seFnParam, sizeof(LX_AUD_MASTER_PARAM_SE_FN_T)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOWR_GET_SE_PARAM:
		{
			LX_AUD_MASTER_PARAM_SE_FN_T		seFnParam;

			AUD_KDRV_TRACE("IOCTL Get SE PARAM!!!\n");

			if(copy_from_user(&seFnParam, (void __user *)arg, sizeof(LX_AUD_MASTER_PARAM_SE_FN_T)))
				return RET_ERROR;

			retVal = AUDIO_GetSEParam(&seFnParam);

			if(copy_to_user((void *)arg, (void *)&seFnParam, sizeof(LX_AUD_MASTER_PARAM_SE_FN_T)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOW_ENABLE_SOUNDBAR:
		{
			LX_AUD_MASTER_SOUNDBAR_T		enableSoundBar;

			AUD_KDRV_TRACE("IOCTL Enable Sound Bar!!!\n");

			if(copy_from_user(&enableSoundBar, (void __user *)arg, sizeof(LX_AUD_MASTER_SOUNDBAR_T)))
				return RET_ERROR;

			retVal = AUDIO_EnableSoundBar(enableSoundBar);
		}
		break;

		case AUD_MASTER_IOW_SET_SOUNDBAR_PARAM:
		{
			LX_AUD_MASTER_SOUNDBAR_PARAM_T		soundBarParam;

			AUD_KDRV_TRACE("IOCTL Set SOUNDBAR PARAM!!!\n");

			if(copy_from_user(&soundBarParam, (void __user *)arg, sizeof(LX_AUD_MASTER_SOUNDBAR_PARAM_T)))
				return RET_ERROR;

			retVal = AUDIO_SetSoundBarParam(&soundBarParam);
		}
		break;

		case AUD_MASTER_IOR_GET_SOUNDBAR_STATUS:
		{
			LX_AUD_MASTER_SOUNDBAR_STATUS_T		soundBarStatus;

			AUD_KDRV_TRACE("IOCTL Get SOUNDBAR STATUS!!!\n");

			retVal = AUDIO_GetSoundBarStatus(&soundBarStatus);

			if(copy_to_user((void *)arg, (void *)&soundBarStatus, sizeof(LX_AUD_MASTER_SOUNDBAR_STATUS_T)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOW_SET_SOUNDBAR_POWER:
		{
			UINT32		setSoundBarPower;

			AUD_KDRV_TRACE("IOCTL Set Sound Bar Power!!!\n");

			if(copy_from_user(&setSoundBarPower, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_SetSoundBarPower(setSoundBarPower);
		}
		break;

		case AUD_MASTER_IOW_SET_SOUNDBAR_CMD:
		{
			LX_AUD_MASTER_SOUNDBAR_CMD_T	setSoundBarCmd;

			AUD_KDRV_TRACE("IOCTL Set Sound Bar Cmd!!!\n");

			if(copy_from_user(&setSoundBarCmd, (void __user *)arg, sizeof(LX_AUD_MASTER_SOUNDBAR_CMD_T)))
				return RET_ERROR;

			retVal = AUDIO_SetSoundBarCmd(&setSoundBarCmd);
		}
		break;

		case AUD_MASTER_IOW_SET_MAIN_DECODER:
		{
			UINT32	mainDecoderIdx;

			AUD_KDRV_TRACE("IOCTL Set Main Decoder Cmd!!!\n");

			if(copy_from_user(&mainDecoderIdx, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_SetMainDecoderIndex(mainDecoderIdx);
		}
		break;

		case AUD_MASTER_IOR_GET_MAIN_DECODER:
		{
			UINT32	mainDecoderIdx;

			AUD_KDRV_TRACE("IOCTL Get Main Decoder Cmd!!!\n");

			retVal = AUDIO_GetMainDecoderIndex(&mainDecoderIdx);

			if(copy_to_user((void *)arg, (void *)&mainDecoderIdx, sizeof(UINT32)))
				return RET_ERROR;
		}
		break;


		case AUD_MASTER_IOW_SET_EVENT:
		{
			LX_AUD_EVENT_TYPE_T	event;

			AUD_KDRV_TRACE("IOCTL Set Event\n");

			if(copy_from_user(&event, (void __user *)arg, sizeof(LX_AUD_EVENT_TYPE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetEvent(&event);
		}
		break;

		case AUD_MASTER_IORW_GET_EVENT:
		{
			LX_AUD_EVENT_TYPE_T	event;

			AUD_KDRV_TRACE("IOCTL Get Event\n");

			if (copy_from_user(&event, (void __user *)arg, sizeof(LX_AUD_EVENT_TYPE_T)))
				return RET_ERROR;

			retVal = AUDIO_GetEvent(&event);

			if(copy_to_user((void *)arg, (void *)&event, sizeof(LX_AUD_EVENT_TYPE_T)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOW_SET_I2S_NUM:
		{
			UINT32		i2sNum;

			AUD_KDRV_TRACE("IOCTL Set I2S Number!!!\n");

			if(copy_from_user(&i2sNum, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_SetI2SNumber(i2sNum);
		}
		break;

		case AUD_MASTER_IOR_GET_GSTC:
		{
			UINT32	gstc;

			AUD_KDRV_TRACE("IOCTL Get GSTC\n");

			if(copy_from_user(&gstc, (void __user *)arg, sizeof(UINT64)))
				return RET_ERROR;

			/* Read a audio gstc reg. */
			gstc = AUDIO_ReadReg(LX_AUD_REG_GSTC_LOW);

			if(copy_to_user((void *)arg, (void *)&gstc, sizeof(UINT64)))
				return RET_ERROR;
		}
		break;


		case AUD_MASTER_IO_RESET_AAD_MODULE:
		{
			AUD_KDRV_TRACE("IOCTL RESET AAD MODULE!!!\n");

			retVal = AUDIO_ResetAADModule();
		}
		break;

		case AUD_MASTER_IO_SET_AAD_SIF_INPUT:
		{
			LX_AUD_SIF_INPUT_T sifSource;

			AUD_KDRV_TRACE("IOCTL SET SIF INPUT!!!\n");

			if(copy_from_user(&sifSource, (void __user *)arg, sizeof(LX_AUD_SIF_INPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_SetAADSIFInputPort(sifSource);
		}
		break;


		case AUD_IOW_CUSTOM_IMC:
		{
			LX_AUD_CUSTOM_IMC_T			customImc;

			AUD_KDRV_TRACE("AUD_IOW_CUSTOM_IMC\n");

			if(copy_from_user(&customImc, (void __user *)arg, sizeof(LX_AUD_CUSTOM_IMC_T)))
				return RET_ERROR;

			(void)KDRV_AUDIO_CustomImc(&customImc);

		}
		break;

		case AUD_IOWR_READ_AND_WRITE_REG:
		{
			LX_AUD_REG_INFO_T		regInfo;

			memset(&regInfo, 0x0, sizeof(LX_AUD_REG_INFO_T));

			AUD_KDRV_TRACE("AUD_IORW_READ_AND_WRITE_REG\n");

			if(copy_from_user(&regInfo, (void __user *)arg, sizeof(LX_AUD_REG_INFO_T)))
				return RET_ERROR;

			(void)KDRV_AUDIO_ReadAndWriteReg(&regInfo);

			if(copy_to_user((void *)arg, (void *)&regInfo, sizeof(LX_AUD_REG_INFO_T)))
				return RET_ERROR;
		}
		break;

		case AUD_IOW_DEBUG_PRINT_CTRL:
		{
			LX_AUD_DEBUG_CTRL_T	debugCtrl;

			memset(&debugCtrl, 0x0, sizeof(LX_AUD_DEBUG_CTRL_T));

			AUD_KDRV_TRACE("AUD_IOW_DEBUG_PRINT_CTRL\n");

			if(copy_from_user( &debugCtrl, (void __user *)arg, sizeof(LX_AUD_DEBUG_CTRL_T)))
				return RET_ERROR;


			if((debugCtrl.printType >= NUM_OF_AUD_PRINT_TYPE) || (debugCtrl.printColor >= NUM_OF_AUD_PRINT_COLOR))
			{
				AUD_KDRV_ERROR("Check Argument\n" );
				return RET_ERROR;
			}

			if(debugCtrl.printColor == DBG_COLOR_NONE)
			{
				OS_DEBUG_DisableModuleByIndex(g_AudDebugFd, debugCtrl.printType);
				AUD_KDRV_ERROR("AUD_PRINT_TYPE[%s] is disabled.\n", ui8AudPrintType[debugCtrl.printType]);
			}
			else
			{
				OS_DEBUG_EnableModuleByIndex(g_AudDebugFd, debugCtrl.printType, debugCtrl.printColor);
				AUD_KDRV_ERROR("AUD_PRINT_TYPE[%s] is enabled Color is [%s].\n", \
					ui8AudPrintType[debugCtrl.printType], ui8AudPrintColor[debugCtrl.printColor - 1]);
			}

			return RET_OK;
 		}
		break;

		/* Download DSP0 Image for test */
		case AUD_MASTER_DOWNALOD_DSP0:
		{
			LX_AUD_CODEC_T		codecType;

			AUD_KDRV_TRACE("IOCTL Download DSP0 Image!!!\n");

			if(copy_from_user(&codecType, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_LoadDSP0Codec();
		}
		break;

		/* Download DSP1 Image for test */
		case AUD_MASTER_DOWNALOD_DSP1:
		{
			LX_AUD_CODEC_T		codecType;

			AUD_KDRV_TRACE("IOCTL Download DSP1 Image!!!\n");

			if(copy_from_user(&codecType, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			retVal = AUDIO_LoadDSP1Codec();
		}
		break;

		case AUD_IOW_DEBUG_MEM_DUMP:
		{
			LX_AUD_DEBUG_MEM_DUMP_T		memDump;

			AUD_KDRV_TRACE("IOCTL Download DSP1 Image!!!\n");

			if(copy_from_user(&memDump, (void __user *)arg, sizeof(LX_AUD_DEBUG_MEM_DUMP_T)))
				return RET_ERROR;

			AUDIO_ShowMemoryDump(&memDump);
 		}
		break;

		case AUD_IO_DEBUG_ALLOC_INFO:
		{
			AUDIO_UpdateConnectionInfo();
 		}
		break;

		case AUD_IO_DEBUG_DEV_INFO:
		{
			AUDIO_ShowDevManInfo();
 		}
		break;

		case AUD_IOWR_DEBUG_GET_MOD_INFO:
		{
			LX_AUD_DEBUG_MOD_INFO_T		modInfo;

			memset(&modInfo, 0x0, sizeof(LX_AUD_DEBUG_MOD_INFO_T));

			AUD_KDRV_TRACE("AUD_IOW_DEBUG_GET_MOD_INFO\n");

			if(copy_from_user(&modInfo, (void __user *)arg, sizeof(LX_AUD_DEBUG_MOD_INFO_T)))
				return RET_ERROR;

			(void)KDRV_AUDIO_GetModIdFromStr(&modInfo);

			if(copy_to_user((void *)arg, (void *)&modInfo, sizeof(LX_AUD_DEBUG_MOD_INFO_T)))
				return RET_ERROR;
 		}
		break;

		case AUD_MASTER_IOR_GET_KDRV_HANDLE:
		{
			AUD_KDRV_RMD("[AUD_REN_IOW_GET_KDRV_HANDLE] Called\n");

			if(copy_to_user((void *)arg, (void *)&(pDev->allocDev), sizeof(UINT32)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOW_CONNECT_OUTPUT:
		{
			LX_AUD_MASTER_CONNECT_OUTPUT_T connect;

			AUD_KDRV_RMD("[AUD_MASTER_IOW_CONNECT_OUTPUT] Called\n");

			if(copy_from_user(&connect, (void __user *)arg, sizeof(LX_AUD_MASTER_CONNECT_OUTPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_ConnectOutput(connect.ui32KdrHandler, connect.output, connect.ui32UseSE);
		}
		break;

		case AUD_MASTER_IOW_DISCONNECT_OUTPUT:
		{
			LX_AUD_MASTER_DISCONNECT_OUTPUT_T disconnect;

			AUD_KDRV_RMD("[AUD_MASTER_IOW_DISCONNECT_OUTPUT] Called\n");

			if(copy_from_user(&disconnect, (void __user *)arg, sizeof(LX_AUD_MASTER_DISCONNECT_OUTPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_DisconnectOutput(disconnect.ui32KdrHandler, disconnect.output);
		}
		break;

		case AUD_IOR_GET_FW_DEBUG_PRINT_INFO:
		{
			LX_AUD_FW_DEBUG_INFO_T debug_info;

			AUD_KDRV_RMD("[AUD_IOR_GET_FW_DEBUG_PRINT_INFO] Called\n");
			if(KDRV_AUDIO_GetFwDebugInfo(&debug_info) < 0)
				return RET_ERROR;
			if(copy_to_user((void *)arg, (void *)&debug_info, sizeof(LX_AUD_FW_DEBUG_INFO_T)))
				return RET_ERROR;
		}
		break;

		case AUD_IOW_SET_FW_DEBUG_MASK:
		{
			UINT32 debug_mask;

			if(copy_from_user(&debug_mask, (void __user *)arg, sizeof(UINT32)))
				return RET_ERROR;

			KDRV_AUDIO_SetFwDebugMask(debug_mask);
		}
		break;

		case AUD_IOWR_DEBUG_GET_DECODED_INFO:
		{
			LX_AUD_DEBUG_DECODED_INFO_T		decInfo;

			AUD_KDRV_TRACE("AUD_IOWR_DEBUG_GET_DECODED_INFO\n");

			if(copy_from_user(&decInfo, (void __user *)arg, sizeof(LX_AUD_DEBUG_DECODED_INFO_T)))
				return RET_ERROR;

			(void)KDRV_AUDIO_GetDecodedInfoForDebug(&decInfo);

			if(copy_to_user((void *)arg, (void *)&decInfo, sizeof(LX_AUD_DEBUG_DECODED_INFO_T)))
				return RET_ERROR;
 		}
		break;

		case AUD_IOWR_GET_DEV_INDEX_CTX:
		{
			LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

			AUD_KDRV_TRACE("[AUD_IOWR_GET_DEV_INDEX_CTX] Called\n");

			if(copy_from_user(&devIndexCtx, (void __user *)arg, sizeof(LX_AUD_DEV_INDEX_CTX_T)))
				return RET_ERROR;

			retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);

			if (copy_to_user((void *)arg, (void *)&devIndexCtx, sizeof(LX_AUD_DEV_INDEX_CTX_T)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOR_GET_DELAY:
		{
			UINT32	delay;

			AUD_KDRV_TRACE("[AUD_MASTER_IOR_GET_DELAY] Called\n");

			retVal = AUDIO_GetMasterDelay(pDev->allocDev, &delay);

			if (copy_to_user((void *)arg, (void *)&delay, sizeof(UINT32)))
				return RET_ERROR;
		}
		break;

		case AUD_MASTER_IOW_CONNECT_INDEX_OUTPUT:
		{
			LX_AUD_MASTER_CONNECT_INDEX_OUTPUT_T	conIdxOut;

			AUD_KDRV_TRACE("[AUD_MASTER_IOW_CONNECT_INDEX_OUTPUT] Called\n");

			if(copy_from_user(&conIdxOut, (void __user *)arg, sizeof(LX_AUD_MASTER_CONNECT_INDEX_OUTPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_ConnectIndexOutput(conIdxOut.ui32Index, conIdxOut.output,
										conIdxOut.spdifOutput, conIdxOut.ui32UseSE);
		}
		break;

		case AUD_MASTER_IOW_DISCONNECT_INDEX_OUTPUT:
		{
			LX_AUD_MASTER_DISCONNECT_INDEX_OUTPUT_T	disconIdxOut;

			AUD_KDRV_TRACE("[AUD_MASTER_IOW_DISCONNECT_INDEX_OUTPUT] Called\n");

			if(copy_from_user(&disconIdxOut, (void __user *)arg, sizeof(LX_AUD_MASTER_DISCONNECT_INDEX_OUTPUT_T)))
				return RET_ERROR;

			retVal = AUDIO_DisconnectIndexOutput(disconIdxOut.ui32Index, disconIdxOut.output, disconIdxOut.spdifOutput);
		}
		break;

		case AUD_IOWR_GET_INDEX_DELAY:
		{
			LX_AUD_GET_INDEX_DELAY_T	getIndexDelay;

			AUD_KDRV_TRACE("[AUD_IOWR_GET_INDEX_DELAY] Called\n");

			if(copy_from_user(&getIndexDelay, (void __user *)arg, sizeof(LX_AUD_GET_INDEX_DELAY_T)))
				return RET_ERROR;

			retVal = AUDIO_GetIndexDelay(&getIndexDelay);

			if(copy_to_user((void *)arg, (void *)&getIndexDelay, sizeof(LX_AUD_GET_INDEX_DELAY_T)))
				return RET_ERROR;
		}
		break;

		case AUD_IOR_GET_START_INFO:
		{
			LX_AUD_GET_START_INFO_T	getStartInfo;

			AUD_KDRV_TRACE("[AUD_IOWR_GET_START_INFO] Called\n");

			retVal = AUDIO_GetStartInfo(&getStartInfo);

			if(copy_to_user((void *)arg, (void *)&getStartInfo, sizeof(LX_AUD_GET_START_INFO_T)))
				return RET_ERROR;
		}
		break;

		case AUD_IOW_SET_INPUT_VOLUME:
		{
			LX_AUD_SET_INPUT_VOLUME_T	setInputVolume;

			AUD_KDRV_TRACE("[AUD_IOW_SET_INPUT_VOLUME] Called\n");

			if(copy_from_user(&setInputVolume, (void __user *)arg, sizeof(LX_AUD_SET_INPUT_VOLUME_T)))
				return RET_ERROR;

			retVal = AUDIO_SetInputVolume(setInputVolume.ui32Index, setInputVolume.ui32Volume);
		}
		break;

		case AUD_IOW_SET_INPUT_MUTE:
		{
			LX_AUD_SET_INPUT_MUTE_T	setInputMute;

			AUD_KDRV_TRACE("[AUD_IOW_SET_INPUT_MUTE] Called\n");

			if(copy_from_user(&setInputMute, (void __user *)arg, sizeof(LX_AUD_SET_INPUT_MUTE_T)))
				return RET_ERROR;

			retVal = AUDIO_SetInputMute(setInputMute.ui32Index, setInputMute.ui32Mute);
		}
		break;

		case AUD_IOW_SET_INPUT_DELAY:
		{
			LX_AUD_SET_INPUT_DELAY_T	setInputDelay;

			AUD_KDRV_TRACE("[AUD_IOW_SET_INPUT_DELAY] Called\n");

			if(copy_from_user(&setInputDelay, (void __user *)arg, sizeof(LX_AUD_SET_INPUT_DELAY_T)))
				return RET_ERROR;

			retVal = AUDIO_SetInputDelay(setInputDelay.ui32Index, setInputDelay.ui32Delay);
		}
		break;

		default:
		{
			/* redundant check but it seems more readable */
			AUD_KDRV_ERROR("Invalid IOCTL Call!!!\n");
			retVal = RET_INVALID_IOCTL;
		}
		break;
	}

	return retVal;
}

/**
 * Underflow Callback function.
 * @param 	_param			[out] parameters.
 * @param 	_paramLen		[out] length of parameters.
 * @param 	_cbParam		[in] callback parameters.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		AUDIO_SetRendererParam
 */
static SINT32 _AUDIO_MixerUnderflowCb(void *_param, SINT32 _paramLen, void *_cbParam)
{
	LX_AUD_DEV_T	allocDev = LX_AUD_DEV_MAS0;
	LX_AUD_DEV_INDEX_CTX_T indexCtx;
	ULONG	flags;
	SINT32	retVal = RET_OK;

	MixerEvtUnderflow		*pUnderflow = (MixerEvtUnderflow *)_param;

	indexCtx.ui32Index = pUnderflow->portNum;
	indexCtx.devType = LX_AUD_DEV_TYPE_REN;
	indexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	retVal = AUDIO_GetDevIndexCtx(&indexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	allocDev = indexCtx.dev;

	//Set a event if allocated device and event message is set for RENx.
	if( (gAudSetEvent[allocDev].allocDev != LX_AUD_DEV_NONE)		\
	  &&(gAudSetEvent[allocDev].eventMsg  & LX_AUD_EVENT_UNDERFLOW) )
	{
		//spin lock for protection
		spin_lock_irqsave(&gAudEventSpinLock, flags);

		//Set a audio GET event type for next event.
		gAudGetEvent[allocDev].allocDev  = allocDev;
		gAudGetEvent[allocDev].eventMsg |= LX_AUD_EVENT_UNDERFLOW;

		//spin unlock for protection
		spin_unlock_irqrestore(&gAudEventSpinLock, flags);
	}

	AUD_KDRV_IMC_NOTI("Mixer Port(%d) Underflow : temp %x.\n", pUnderflow->portNum, pUnderflow->tmp);

	return RET_OK;
}

/**
 * Tone Detect Callback function.
 * @param 	_param			[out] parameters.
 * @param 	_paramLen		[out] length of parameters.
 * @param 	_cbParam		[in] callback parameters.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		AUDIO_SetRendererParam
 */
static SINT32 _AUDIO_SeToneDetectCb(void *_param, SINT32 _paramLen, void *_cbParam)
{
	LX_AUD_DEV_T	allocDev = LX_AUD_DEV_MAS0;
	ULONG	flags;

	SeEvtDetectTone		*pToneDetect = (SeEvtDetectTone *)_param;
	AUD_MASTER_INFO_T		*pMasterInfo = (AUD_MASTER_INFO_T *)_cbParam;

	allocDev = pMasterInfo->ui32AllocDev;

	//Set a event if allocated device and event message is set for RENx.
	if( (gAudSetEvent[allocDev].allocDev != LX_AUD_DEV_NONE)		\
	  &&(gAudSetEvent[allocDev].eventMsg & (LX_AUD_EVENT_1KHZ_TONE_ON | LX_AUD_EVENT_1KHZ_TONE_OFF)) )
	{
		//spin lock for protection
		spin_lock_irqsave(&gAudEventSpinLock, flags);

		//Set a audio GET event type for next event.
		gAudGetEvent[allocDev].allocDev = allocDev;

		if(pToneDetect->istone == 1)
			gAudGetEvent[allocDev].eventMsg |= LX_AUD_EVENT_1KHZ_TONE_ON;
		else
			gAudGetEvent[allocDev].eventMsg |= LX_AUD_EVENT_1KHZ_TONE_OFF;

		//spin unlock for protection
		spin_unlock_irqrestore(&gAudEventSpinLock, flags);
	}

	AUD_KDRV_IMC_NOTI("Master(%d) isTone : %x.\n", allocDev, pToneDetect->istone);
	return RET_OK;
}

/**
 * Set Volume for output ports.
 * @see
*/
static SINT32 AUDIO_SetOutCtrlAll(AUD_OUT_CTRL_MODULE_T *pOutCtrlConfig)
{
	ADEC_MODULE_ID			audioModule;
	OutctrlCmdSetAll		setOutCtrlAll;

	if(pOutCtrlConfig == NULL)
	{
		AUD_KDRV_ERROR("output is out of range!!!\n");
		return RET_ERROR;
	}

	audioModule = pOutCtrlConfig->module;
	setOutCtrlAll.Delay = pOutCtrlConfig->ui32Delay;
	setOutCtrlAll.Gain = pOutCtrlConfig->ui32Volume;
	setOutCtrlAll.GainEnable = 1;
	setOutCtrlAll.Mute = pOutCtrlConfig->ui32Mute;
	setOutCtrlAll.Balanced = pOutCtrlConfig->ui32Balance - 50;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_ALL, audioModule, sizeof(OutctrlCmdSetAll), &setOutCtrlAll);

	AUD_KDRV_PRINT("(Module:%d)\n", pOutCtrlConfig->module);

	return RET_OK;
}


/**
 * Set Volume for output ports.
 * @see
*/
static SINT32 AUDIO_SetVolume(LX_AUD_MASTER_CONFIG_VOLUME_T *pVolumeConfig)
{
	ADEC_MODULE_ID		audioModule;
	OutctrlCmdSetGain	setOutCtrlGain;

	if(pVolumeConfig->output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("output is out of range!!!\n");
		return RET_ERROR;
	}

	audioModule = _gMasterInfo.outCtrlMod[pVolumeConfig->output].module;
	setOutCtrlGain.GainEnable = 1;
	setOutCtrlGain.Gain = pVolumeConfig->ui32Volume;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_GAIN, audioModule, sizeof(OutctrlCmdSetGain), &setOutCtrlGain);

	_gMasterInfo.outCtrlMod[pVolumeConfig->output].ui32Volume = pVolumeConfig->ui32Volume;

	AUD_KDRV_PRINT("AUDIO_SetVolume(Output:%d, Volume:0x%x)\n", pVolumeConfig->output, pVolumeConfig->ui32Volume);

	return RET_OK;
}

/**
 * Set Mute for output ports.
 * @see
*/
static SINT32 AUDIO_SetMute(LX_AUD_MASTER_CONFIG_MUTE_T *pMuteConfig)
{
	ADEC_MODULE_ID		audioModule;
	OutctrlCmdSetMute		setOutCtrlMute;
	ADEC_MODULE_ID		spdifModule = ADEC_MODULE_ESOUT_CTRL;
	EsoutctrlCmdSetMute	setSpdifEsMute;

	if(pMuteConfig->output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("output is out of range!!!\n");
		return RET_ERROR;
	}

	audioModule = _gMasterInfo.outCtrlMod[pMuteConfig->output].module;
	setOutCtrlMute.Mute = pMuteConfig->ui32Mute;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_MUTE, audioModule, sizeof(OutctrlCmdSetMute), &setOutCtrlMute);

	if(pMuteConfig->output == LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		setSpdifEsMute.Mute = pMuteConfig->ui32Mute;
		AUDIO_IMC_SendCmdParam(ESOUTCTRL_CMD_SET_MUTE, spdifModule, sizeof(EsoutctrlCmdSetMute), &setSpdifEsMute);
	}

	_gMasterInfo.outCtrlMod[pMuteConfig->output].ui32Mute = pMuteConfig->ui32Mute;

	AUD_KDRV_PRINT("AUDIO_SetMute(Output:%d, Mute:%d)\n", pMuteConfig->output, pMuteConfig->ui32Mute);

	return RET_OK;
}

/**
 * Set Delay for output ports.
 * @see
*/
static SINT32 AUDIO_SetDelay(LX_AUD_MASTER_CONFIG_DELAY_T *pDelayConfig)
{
	ADEC_MODULE_ID		audioModule;
	OutctrlCmdSetDelay	setOutCtrlDelay;
	ADEC_MODULE_ID		spdifModule = ADEC_MODULE_ESOUT_CTRL;
	EsoutctrlCmdSetDelay	setSpdifDelay;

	if(pDelayConfig->output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("output is out of range!!!\n");
		return RET_ERROR;
	}

	audioModule = _gMasterInfo.outCtrlMod[pDelayConfig->output].module;
	setOutCtrlDelay.Delay= pDelayConfig->ui32Delay;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_DELAY, audioModule, sizeof(OutctrlCmdSetDelay), &setOutCtrlDelay);

	if(pDelayConfig->output == LX_AUD_MASTER_OUTPUT_SPDIF && _gMasterInfo.spdifOutputFormat == LX_AUD_MASTER_SPDIF_ES)
	{
		setSpdifDelay.Delay = pDelayConfig->ui32Delay;
		setSpdifDelay.Fs    = _gMasterInfo.spdifSamplingFreq;
		AUDIO_IMC_SendCmdParam(ESOUTCTRL_CMD_SET_DELAY, spdifModule, sizeof(EsoutctrlCmdSetDelay), &setSpdifDelay);
	}

	_gMasterInfo.outCtrlMod[pDelayConfig->output].ui32Delay = pDelayConfig->ui32Delay;

	AUD_KDRV_PRINT("AUDIO_SetDelay(Output:%d, Delay:%d)\n", pDelayConfig->output, pDelayConfig->ui32Delay);

	return RET_OK;
}

/**
 * Set balance for output ports.
 * @see
*/
static SINT32 AUDIO_SetBalance(LX_AUD_MASTER_CONFIG_BALANCE_T *pBalanceConfig)
{
	ADEC_MODULE_ID			audioModule;
	OutctrlCmdSetBalanced	setOutCtrlBalanced;

	if(pBalanceConfig->output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("output is out of range!!!\n");
		return RET_ERROR;
	}

	audioModule = _gMasterInfo.outCtrlMod[pBalanceConfig->output].module;
	setOutCtrlBalanced.Balanced = pBalanceConfig->ui32Balance - 50;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_BALANCED, audioModule, sizeof(OutctrlCmdSetBalanced), &setOutCtrlBalanced);

	_gMasterInfo.outCtrlMod[pBalanceConfig->output].ui32Balance = pBalanceConfig->ui32Balance;

	AUD_KDRV_PRINT("AUDIO_SetBalance(Output:%d, Balance:%d)\n", pBalanceConfig->output, pBalanceConfig->ui32Balance);
	return RET_OK;
}

/**
 * Set Volume for output ports.
 * @see
*/
static SINT32 AUDIO_SetOutMode(LX_AUD_MASTER_CONFIG_OUT_MODE_T *pOutModeConfig)
{
	ADEC_MODULE_ID		audioModule;
	OutctrlCmdSetPcmoutmode	setPcmOutMode;

	if(pOutModeConfig->output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("output is out of range!!!\n");
		return RET_ERROR;
	}

	audioModule = _gMasterInfo.outCtrlMod[pOutModeConfig->output].module;
	setPcmOutMode.OutMode = pOutModeConfig->outMode;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_PCMOUTMODE, audioModule, sizeof(OutctrlCmdSetPcmoutmode), &setPcmOutMode);

	_gMasterInfo.outCtrlMod[pOutModeConfig->output].outMode = pOutModeConfig->outMode;

	AUD_KDRV_PRINT("AUDIO_SetOutMode(Output:%d, OutMode:%d)\n", pOutModeConfig->output, pOutModeConfig->outMode);

	return RET_OK;
}

/**
 * Set SPDIF Output for output ports.
 * @see
*/
static SINT32 AUDIO_SetSpdifOutput(LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T *pSpdifOutputConfig)
{
	LX_AUD_DEV_T		allocDev = LX_AUD_DEV_REN0;
	LX_AUD_DEV_T		renIndex = LX_AUD_DEV_REN0;
	ULONG				flags;

	AUD_MAS_LOCK();

	/* If SPDIF output type is changed, notify G-Streamer for ES only. */
	if(g_AudSpdifOutputType != pSpdifOutputConfig->spdifOutput)
	{
		/* Check a render open status for EMP File Play Case. */
		for(allocDev = LX_AUD_DEV_REN0; allocDev <= LX_AUD_DEV_REN11; allocDev++)
		{
			renIndex = GET_REN_INDEX(allocDev);
			if((renIndex < 0) || (renIndex >= DEV_REN_NUM))
			{
				AUD_MAS_UNLOCK();
				AUD_KDRV_ERROR("renIndex[%d] is not available.\n", renIndex);
				return RET_ERROR;
			}

			if((_gRenderInfo[renIndex].ui32AllocDev != LX_AUD_DEV_NONE) 	\
			 &&(_gRenderInfo[renIndex].renderParam.input == LX_AUD_INPUT_SYSTEM) )
			{
				//Set a event if allocated device and event message is set for RENx.
				if( (gAudSetEvent[allocDev].allocDev != LX_AUD_DEV_NONE)		\
				  &&(gAudSetEvent[allocDev].eventMsg  & LX_AUD_EVENT_SPDIF_CHANGED) )
				{
					//spin lock for protection
					spin_lock_irqsave(&gAudEventSpinLock, flags);

					//Set a audio GET event type for next event.
					gAudGetEvent[allocDev].allocDev  = allocDev;
					gAudGetEvent[allocDev].eventMsg |= LX_AUD_EVENT_SPDIF_CHANGED;

					//spin unlock for protection
					spin_unlock_irqrestore(&gAudEventSpinLock, flags);
				}
			}
		}
	}

	g_AudSpdifOutputType = pSpdifOutputConfig->spdifOutput;

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("AUDIO_SetSpdifOutput(%d)\n", pSpdifOutputConfig->spdifOutput);

	return RET_OK;
}

/**
 * Set SPDIF Output for output ports.
 * @see
*/
static SINT32 AUDIO_GetSpdifOutput(LX_AUD_MASTER_CONFIG_SPDIF_OUTPUT_T *pSpdifOutputConfig)
{
	pSpdifOutputConfig->spdifOutput = g_AudSpdifOutputType;

	AUD_KDRV_PRINT("AUDIO_GetSpdifOutput(%d)\n", pSpdifOutputConfig->spdifOutput);
	return RET_OK;
}

/**
 * Set SPDIF SCMS for output ports.
 * @see
*/
static SINT32 AUDIO_SetSpdifScms(LX_AUD_MASTER_CONFIG_SPDIF_SCMS_T *pSpdifScmsConfig)
{
	SpdifCmdSetScms	setSpdifScms;

	AUD_MAS_LOCK();

	setSpdifScms.scms_type = pSpdifScmsConfig->spdifScms;
	AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_SCMS, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetScms), &setSpdifScms);

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("AUDIO_SetSpdifScms(%d)\n", pSpdifScmsConfig->spdifScms);
	return RET_OK;
}

/**
 * Set On or Off for SPDIF Output Light.
 * @see
*/
static SINT32 AUDIO_SetSpdifOnOff(UINT32 bOnOff)
{
	SpdifCmdSetLight	setSpdifLight;

	AUD_MAS_LOCK();

	setSpdifLight.onoff = bOnOff;
	AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_LIGHT, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetLight), &setSpdifLight);

	_gMasterInfo.spdifOnOff = bOnOff;

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("AUDIO_SetSpdifOnOff(%d)\n", bOnOff);
	return RET_OK;
}

/**
 * Set SPDIF Category Code.
 * @see
*/
static SINT32 AUDIO_SetSpdifCategoryCode(UINT32 categoryCode)
{
	SpdifCmdSetCategoryCode	setSpdifCategoryCode;

	AUD_MAS_LOCK();

	setSpdifCategoryCode.category_code = categoryCode;
	AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_CATEGORY_CODE, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetCategoryCode), &setSpdifCategoryCode);

	_gMasterInfo.spdifCategoryCode = categoryCode;

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("AUDIO_SetSpdifCategoryCode(%d)\n", categoryCode);
	return RET_OK;
}

/**
 * Set bass for output ports.
 * @see
*/
static SINT32 AUDIO_SetBass(LX_AUD_MASTER_CONFIG_BASS_T *pBassConfig)
{
	AUD_KDRV_PRINT("AUDIO_SetBass(%d) : Not Supported!!!\n", pBassConfig->ui32Bass);
	return RET_OK;
}

/**
 * Set treble for output ports.
 * @see
*/
static SINT32 AUDIO_SetTreble(LX_AUD_MASTER_CONFIG_TREBLE_T *pTrebleConfig)
{
	AUD_KDRV_PRINT("AUDIO_SetTreble(%d) : Not Supported!!!\n", pTrebleConfig->ui32Treble);
	return RET_OK;
}

/**
 * Set input port.
 * @see
*/
static SINT32 AUDIO_SetADCInputPort(UINT32 adcInputPort)
{
	//Set ADC Input port for Mixed IP Module
	(void)MIXED_IP_AUDIO_SetADCPortNumber(adcInputPort);

	AUD_KDRV_PRINT("AUDIO_SetADCInputPort(%d)\n", adcInputPort);
	return RET_OK;
}

/**
 * Set Power Block Mode.
 * @see
*/
static SINT32  AUDIO_SetPowerCtrl(LX_AUD_MASTER_CONFIG_POWER_CTRL_T *pPowerCtrlConfig)
{
	//Set ADC Input port for Mixed IP Module
	(void)MIXED_IP_AUDIO_PowerControl(pPowerCtrlConfig->powerBlock, pPowerCtrlConfig->powerMode);

	AUD_KDRV_PRINT("AUDIO_SetPowerCtrl(%d %d)\n", pPowerCtrlConfig->powerBlock, pPowerCtrlConfig->powerMode);
	return RET_OK;
}

/**
 * Set AAD SIF input port.
 * @see
*/
static SINT32 AUDIO_SetAADSIFInputPort(LX_AUD_SIF_INPUT_T sifSource)
{
	//Set AAD SIF Input port for Mixed IP Module
	(void)MIXED_IP_AUDIO_SetAADSIFInputPort(sifSource);

	AUD_KDRV_PRINT("AUDIO_SetAADSIFInputPort(%d)\n", sifSource);
	return RET_OK;
}

/**
 * Set Speaker Output for output ports.
 * @see
*/
static SINT32 AUDIO_SetSpeakerOutput(LX_AUD_MASTER_CONFIG_SPK_OUTPUT_T *pSpkOutputConfig)
{
	SINT32				retVal = RET_OK;
	ADEC_MODULE_ID		audioModule;

	if(pSpkOutputConfig->spkOutput == LX_AUD_MASTER_SPK_PCM)
	{
		audioModule = ADEC_MODULE_OUT_PCM;
		retVal |= AUDIO_IMC_SendCmd(ADEC_CMD_START, audioModule);
	}
	else if(pSpkOutputConfig->spkOutput == LX_AUD_MASTER_SPK_PCM_0)
	{
		audioModule = ADEC_MODULE_OUT_PCM;
		retVal |= AUDIO_IMC_SendCmd(ADEC_CMD_STOP, audioModule);

#if 0	// disable ADEC_MOUDLE_OUT_PCM_0 setting
		audioModule = ADEC_MODULE_OUT_PCM_0;
		retVal |= AUDIO_IMC_SendCmd(ADEC_CMD_START, audioModule);
#endif
	}
	else
	{
		audioModule = ADEC_MODULE_OUT_PCM;
		retVal |= AUDIO_IMC_SendCmd(ADEC_CMD_STOP, audioModule);
	}

	AUD_KDRV_PRINT("AUDIO_SetSpeakerOutput(%d)\n", pSpkOutputConfig->spkOutput);
	return retVal;
}

/**
 * Sets a Sound Engine(SE) Function mode and paramters.
 * The Function mode and each paramter size is defined.
 * @see
*/
static SINT32  AUDIO_SetSEFunction(LX_AUD_MASTER_PARAM_SE_FN_T *pSeFnParam)
{
	UINT32		seParamAddr = 0;
	UINT16		seParamSize	= 0;

	SeCmdSetMode	setSeMode;
	SeCmdSetParam	setSeParam;

	ADEC_MODULE_ID		audioModule = ADEC_MODULE_SE;

#ifdef ENABLE_SE_PARAM_DEBUG
	UINT16		ui16Count = 0;
#endif

	//Sanity check
	if((pSeFnParam->ui32Param == NULL)	\
	  ||(pSeFnParam->ui32NoParam == 0) || (pSeFnParam->ui32NoParam >= 1024))
	{
		AUD_KDRV_ERROR("pParams = %p, noParam = %d\n", pSeFnParam->ui32Param, pSeFnParam->ui32NoParam);
		return RET_ERROR;
	}

	/* Set a SE function mode */
	if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_SET)
	{
		setSeMode.Mode = pSeFnParam->ui32Param[0];
		setSeMode.DeqMode = pSeFnParam->ui32Param[1];
		setSeMode.PeqMode = pSeFnParam->ui32Param[2];
		AUDIO_IMC_SendCmdParam(SE_CMD_SET_MODE, audioModule, sizeof(SeCmdSetMode), &setSeMode);
		memcpy(&(_gMasterInfo.seModeParam[0]),  &setSeMode, sizeof(SeCmdSetMode));
		AUD_KDRV_PRINT("SE mode : %x, Deq : %x, Peq : %x.\n", setSeMode.Mode, setSeMode.DeqMode, setSeMode.PeqMode);

		return RET_OK;
	}

	if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_MAIN)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSE_MAIN;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_MAIN option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSE_MAIN_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_MAIN INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_MAIN;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_000)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN000_CV;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN000_CV + NUM_OF_LGSEFN000_CV_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN000 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN000_CV_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN000 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN000_CV_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN000 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN000;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_001)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN001_AV;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN001_AV + NUM_OF_LGSEFN001_AV_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN001 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN001_AV_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN001 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN001_AV_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN001 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN001;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_002)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN002_DB;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN001 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN002_DB_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN002 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN002;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_003)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN003_DEQ;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN003 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN003_DEQ_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN003 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN003;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_004_MODE1)
	{
		/* Set a SE parameter address and size */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(pSeFnParam->varOption == LX_AUD_MASTER_SE_VAR_ALL) )
		{
			seParamAddr = ADDR_OF_LGSEFN004_PEQMODE1;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(pSeFnParam->varOption < LX_AUD_MASTER_SE_VAR_ALL) )
		{
			seParamAddr = ADDR_OF_LGSEFN004_PEQMODE1 + NUM_OF_LGSEFN004MODE1_BASIC_PARAM +
				(pSeFnParam->varOption * NUM_OF_LGSEFN004MODE1_PARAM);
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE1 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(pSeFnParam->varOption == LX_AUD_MASTER_SE_VAR_ALL) &&
			(seParamSize != NUM_OF_LGSEFN004_PEQ_MODE1_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE1 VARI ALL size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(pSeFnParam->varOption < LX_AUD_MASTER_SE_VAR_ALL) &&
			(seParamSize != NUM_OF_LGSEFN004MODE1_PARAM))
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE1 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN004_MODE1;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_004_MODE2)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES )
		{
			seParamAddr = ADDR_OF_LGSEFN004_PEQMODE2;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE2 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN004_PEQ_MODE2_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE2 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN004_MODE2;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_004_MODE3)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES )
		{
			seParamAddr = ADDR_OF_LGSEFN004_PEQMODE3;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE3 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN004_PEQ_MODE3_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN004_MODE3 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN004_MODE3;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_005)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN005_HARM;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN005 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN005_HARM_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN005 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN005;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_008)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN008_HC;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN008_HC + NUM_OF_LGSEFN008_HC_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN008 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN008_HC_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN008 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN008_HC_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN008 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN008;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_009)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN009_OSD;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN009_OSD + NUM_OF_LGSEFN009_OSD_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN009 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN009_OSD_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN009 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN009_OSD_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN009 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN009;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_010)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN010_IC;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN010_IC + NUM_OF_LGSEFN010_IC_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN010 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN010_IC_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN010 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN010_IC_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN010 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN010;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_011)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN011_IVSE;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN011_IVSE + NUM_OF_LGSEFN011_IVSE_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN011 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN011_IVSE_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN011 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN011_IVSE_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN011 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN011;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_013)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN013_NE;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN013_NE + NUM_OF_LGSEFN013_NE_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN013 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN013_NE_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN013  INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN013_NE_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN013  VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN013;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_014)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN014_ASC;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES)
		{
			seParamAddr = ADDR_OF_LGSEFN014_ASC + NUM_OF_LGSEFN014_ASC_SIZEOF_INIT;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN014 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN014_ASC_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN014 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
		else if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_VARIABLES) &&
			(seParamSize != NUM_OF_LGSEFN014_ASC_SIZEOF_VAR))
		{
			AUD_KDRV_ERROR("FN_MODE_FN014 VARI size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN014;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_016)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSEFN016_ELC;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_FN016 option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSEFN016_ELC_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_FN016 INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_FN016;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_MAIN)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY)
		{
			seParamAddr = ADDR_OF_LGSE_MAIN;
			seParamSize = pSeFnParam->ui32NoParam;
		}
		else
		{
			AUD_KDRV_ERROR("FN_MODE_MAIN option = %d.\n", pSeFnParam->dataOption);
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		/* Sanity Check */
		if((pSeFnParam->dataOption == LX_AUD_MASTER_SE_DATA_INIT_ONLY) &&
			(seParamSize != NUM_OF_LGSE_MAIN_SIZEOF_INIT))
		{
			AUD_KDRV_ERROR("FN_MODE_MAIN INIT size = %d.\n", seParamSize);
			return RET_ERROR;
		}
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
		/* Update a Sound Engine Mask */
		g_AudKdrv.soundEngineMask |= MASK_SE_FN_MODE_MAIN;
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_UPDATE_PARAM)
	{
		seParamSize = 0;
	}
	else
	{
		AUD_KDRV_ERROR("Function Mode Error!!!(%d)\n", pSeFnParam->fnMode);
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("addr = 0x%04X, size = %d(%d)\n", (UINT32)seParamAddr, seParamSize, sizeof(UINT32)*seParamSize);

	//Copy Sound Engine(SE)  parameters
	if(seParamSize != 0)
	{
		if(g_pui32SEParamVirAddr != NULL)
		{
	        if(copy_from_user(g_pui32SEParamVirAddr + seParamAddr, pSeFnParam->ui32Param, sizeof(UINT32) * seParamSize))
	        {
	        	AUD_KDRV_ERROR("copy_from_user error!!!\n");
	        	return RET_ERROR;
	        }
		}
		else
		{
			AUD_KDRV_ERROR("g_pui32SEParamVirAddr is NULL!!!\n");
			return RET_ERROR;
		}

#ifdef ENABLE_SE_PARAM_DEBUG
		//Read parameter value for debug
		AUD_KDRV_ERROR("SE Param Start...[0x%8X]\n", g_pui32SEParamVirAddr[seParamAddr]);
		for(ui16Count = 0; ui16Count < seParamSize; ui16Count++)
		//for(ui16Count = 0; ui16Count < 3; ui16Count++)
		{
			AUD_KDRV_ERROR("Param[%3d] = 0x%8X\n", ui16Count + 1, g_pui32SEParamVirAddr[seParamAddr + ui16Count]);
		}
		AUD_KDRV_ERROR("SE Param End...[0x%8X]\n", g_pui32SEParamVirAddr[seParamAddr + seParamSize - 1]);
#endif		//#ifdef ENABLE_SE_PARAM_DEBUG
	}

	setSeParam.FnMode = AUDIO_GetSeFnMode(pSeFnParam->fnMode);
	setSeParam.DataOption = pSeFnParam->dataOption;
	AUDIO_IMC_SendCmdParam(SE_CMD_SET_PARAM, audioModule, sizeof(SeCmdSetParam), &setSeParam);

	AUD_KDRV_PRINT("SE FnMode : %x, dataOption : %x.\n", setSeParam.FnMode, setSeParam.DataOption);
	return RET_OK;
}

/**
 * Gets a Sound Engine(SE) Function mode and paramters.
 * The Function mode and each paramter size is defined.
 * @see
*/
static SINT32  AUDIO_GetSEFunction(LX_AUD_MASTER_PARAM_SE_FN_T *pSeFnParam)
{
	UINT32		seParamAddr = 0;
	UINT32		seParamSize	= 0;

	UINT32		buffer[7];

	if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_010)
	{
		/* Set a SE parameter address and size */
		if(pSeFnParam->accessMode == LX_AUD_MASTER_SE_ACCESS_READ)
		{
			//Read the sound level for left and right channel
			pSeFnParam->ui32Param[0] = AUDIO_ReadReg(LX_AUD_REG_LEVELMO_L);
			pSeFnParam->ui32Param[1] = AUDIO_ReadReg(LX_AUD_REG_LEVELMO_R);
		}
		else
		{
			pSeFnParam->ui32Param[0] = 0;	//left volume
			pSeFnParam->ui32Param[1] = 0;	//right volume

			AUD_KDRV_ERROR("FN_FN010 access = %d.\n", pSeFnParam->accessMode);
			return RET_ERROR;
		}
	}
	else if(pSeFnParam->fnMode == LX_AUD_MASTER_SE_FN_014)
	{
		seParamAddr = ADDR_OF_LGSEF014_ASC_OUT;
		seParamSize = pSeFnParam->ui32NoParam;

		/* Set a SE parameter address and size */
		if(pSeFnParam->accessMode == LX_AUD_MASTER_SE_ACCESS_READ)
		{
			//Copy Sound Engine(SE)  parameters
			if(seParamSize != 0)
			{
				if(g_pui32SEParamVirAddr != NULL)
				{
					//Copy a local memory
					memcpy(buffer, g_pui32SEParamVirAddr + seParamAddr, sizeof(UINT32) * seParamSize);

					//Copy a from smart sound data from buffer to user memory
					if(copy_to_user((void *)pSeFnParam->ui32Param, (void *)buffer, sizeof(UINT32) * seParamSize))
					//if(copy_to_user((void *)pSeFnParam->ui32Param, (void *)g_pui32SEParamVirAddr + seParamAddr, sizeof(UINT32) * seParamSize))
					{
						AUD_KDRV_ERROR("copy_to_user error!!!\n");
						return RET_ERROR;
					}
				}
				else
				{
					AUD_KDRV_ERROR("g_pui32SEParamVirAddr is NULL!!!\n");
					return RET_ERROR;
				}
			}
		}
		else
		{
			AUD_KDRV_ERROR("FN_FN014 access = %d.\n", pSeFnParam->accessMode);
			return RET_ERROR;
		}
	}
	else
	{
		AUD_KDRV_ERROR("Not supported parameters.\n");
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("L(0x%x), R(0x%x)\n",\
		pSeFnParam->ui32Param[0], pSeFnParam->ui32Param[1]);

	return RET_OK;
}

/**
 * Gets a Sound Engine(SE) Function mode and paramters.
 * The Function mode and each paramter size is defined.
 * @see
*/
static SINT32  AUDIO_GetSEParam(LX_AUD_MASTER_PARAM_SE_FN_T *pSeFnParam)
{
	SeCmdGetParam	getSeParam;

	UINT32		seParamAddr = ADDR_OF_LGSEF_GET_PARAM;
	UINT32		seParamSize	= 0;

	getSeParam.FnMode = AUDIO_GetSeFnMode(pSeFnParam->fnMode);
	getSeParam.DataOption = pSeFnParam->dataOption;
	AUDIO_IMC_SendCmdParam(SE_CMD_GET_PARAM, ADEC_MODULE_SE, sizeof(SeCmdGetParam), &getSeParam);

	OS_MsecSleep(50);

	if(g_pui32SEParamVirAddr != NULL)
	{
		seParamSize = pSeFnParam->ui32NoParam;

		if(seParamSize <= NUM_OF_LGSEF_SIZEOF_GET_PARAM)
		{
			//Copy a from smart sound data from buffer to user memory
			if(copy_to_user((void *)pSeFnParam->ui32Param,
				(void *)(g_pui32SEParamVirAddr + seParamAddr),
				sizeof(UINT32) * seParamSize))
			{
				AUD_KDRV_ERROR("copy_to_user error!!!\n");
				return RET_ERROR;
			}
		}
		else
		{
			AUD_KDRV_ERROR("seParamSize is bigger than NUM_OF_LGSEF_SIZEOF_GET_PARAM!!!\n");
			return RET_ERROR;
		}
	}
	else
	{
		AUD_KDRV_ERROR("g_pui32SEParamVirAddr is NULL!!!\n");
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("AUDIO_GetSEParam(%d, %d, %d)!!!\n", pSeFnParam->fnMode,
		pSeFnParam->dataOption, pSeFnParam->ui32NoParam);

	return RET_OK;
}

/**
 * Set Delay for Mixer Port.
 * @see
*/
static SINT32 AUDIO_SetMixerDelay(LX_AUD_MASTER_CONFIG_MIXER_DELAY_T *pMixerDelayConfig)
{
	ADEC_MODULE_ID		audioModule;
	ADEC_MODULE_ID		srcModule = ADEC_MODULE_PCMCVT_P0;
	MixCmdSetInitDelay	setMixInitDelay;
	MixCmdEnablePort	enabeMixPort;
	ImcCmdFlushParam	flushParam;

	audioModule = pMixerDelayConfig->mixer + ADEC_MODULE_MIX_0;

	enabeMixPort.port = pMixerDelayConfig->ui32Port;
	enabeMixPort.enable = 0;
	AUDIO_IMC_SendCmdParam(MIX_CMD_ENABLE_PORT, audioModule, sizeof(MixCmdEnablePort), &enabeMixPort);

	OS_MsecSleep(10);

	memset(&flushParam, 0, sizeof(ImcCmdFlushParam));
	flushParam.num_of_port = 1;
	flushParam.port_list[0] = MOD_IN_PORT(pMixerDelayConfig->ui32Port);
	AUDIO_IMC_SendCmdParam(ADEC_CMD_FLUSH, audioModule, sizeof(ImcCmdFlushParam), &flushParam);

	//memset(&flushParam, 0, sizeof(ImcCmdFlushParam));
	flushParam.num_of_port	= 1;
	flushParam.port_list[0]	= MOD_IN_PORT(0);
	AUDIO_IMC_SendCmdParam(ADEC_CMD_FLUSH, srcModule, sizeof(ImcCmdFlushParam), &flushParam);

	setMixInitDelay.port = pMixerDelayConfig->ui32Port;
	setMixInitDelay.init_delay_length = pMixerDelayConfig->ui32Delay * 48;
	AUDIO_IMC_SendCmdParam(MIX_CMD_SET_INIT_DELAY, audioModule, sizeof(MixCmdSetInitDelay), &setMixInitDelay);

	enabeMixPort.port = pMixerDelayConfig->ui32Port;
	enabeMixPort.enable = 1;
	AUDIO_IMC_SendCmdParam(MIX_CMD_ENABLE_PORT, audioModule, sizeof(MixCmdEnablePort), &enabeMixPort);

	AUD_KDRV_PRINT("AUDIO_SetDelay(port :%d, delay:%d)\n", setMixInitDelay.port, setMixInitDelay.init_delay_length);

	return RET_OK;
}

/**
 * Set Delay for Mixer Port.
 * @see
*/
SINT32 AUDIO_SetMixerVolume(LX_AUD_MASTER_CONFIG_MIXER_VOLUME_T *pMixerVolumeConfig)
{
	ADEC_MODULE_ID		audioModule;
	MixCmdSetGain		setMixGain;

	if(pMixerVolumeConfig->mixer > LX_AUD_MASTER_MIXER_MAX)
	{
		AUD_KDRV_ERROR("mixer[%d] is out of range!!!\n", \
			pMixerVolumeConfig->mixer);
		return RET_ERROR;
	}

	if(pMixerVolumeConfig->ui32Port >= MAX_IN_PORT)
	{
		AUD_KDRV_ERROR("mixer input port[%d] is out of range!!!\n", \
			pMixerVolumeConfig->ui32Port);
		return RET_ERROR;
	}

	audioModule = pMixerVolumeConfig->mixer + ADEC_MODULE_MIX_0;

	setMixGain.port = pMixerVolumeConfig->ui32Port;
	setMixGain.gain = pMixerVolumeConfig->ui32Volume;
	AUDIO_IMC_SendCmdParam(MIX_CMD_SET_GAIN, audioModule, sizeof(MixCmdSetGain), &setMixGain);

	_gMasterInfo.mixerInPortVolume[pMixerVolumeConfig->ui32Port] = pMixerVolumeConfig->ui32Volume;

	AUD_KDRV_DEBUG("AUDIO_SetMixerVolume(port:%d, gain:%d)\n", setMixGain.port, setMixGain.gain);

	return RET_OK;
}

/**
 * Set Volume for DAC Port.
 * @see
*/
static SINT32 AUDIO_SetDacVolume(LX_AUD_MASTER_CONFIG_DAC_VOLUME_T  *pDacVolumeConfig)
{
	LX_AUD_DAC_VOL_T dacVol;

	dacVol.portNum  = pDacVolumeConfig->dacPort;
	dacVol.hwVolume = (UINT16)pDacVolumeConfig->ui32Volume;
	dacVol.mode 	= LX_AUD_DAC_MIXEDIP;
	dacVol.swVolume	= 0;	//not used now
	dacVol.scartGain= 0;	//not used now

	MIXED_IP_AUDIO_SetDACOutVolume(dacVol);

	AUD_KDRV_PRINT("AUDIO_SetDacVolume(port:%d, volume:%d)\n", dacVol.portNum, dacVol.hwVolume);

	return RET_OK;
}


/**
 * Set Volume for DAC Port.
 * @see
*/
static SINT32 AUDIO_SetDacMute(LX_AUD_MASTER_CONFIG_DAC_MUTE_T *pDacMuteConfig)
{
	LX_AUD_OUTPUT_PORT_MUTE_T dacMute;

	dacMute.portNum	 = pDacMuteConfig->dacPort;
	dacMute.bHwOnOff = (BOOLEAN)pDacMuteConfig->ui32Mute;
	dacMute.mode 	 = pDacMuteConfig->mode;
	dacMute.bSwOnOff = 0;	//not used now

	MIXED_IP_AUDIO_SetDACOutMute(dacMute);

	AUD_KDRV_PRINT("AUDIO_SetDacVolume(port:%d, mute:%d, mode:%d)\n", dacMute.portNum, dacMute.bHwOnOff, dacMute.mode);

	return RET_OK;
}

/**
 * Connect other device to Master Outputs.
 * @see
*/
SINT32 AUDIO_ConnectOutput(UINT32 connectDev, LX_AUD_MASTER_OUTPUT_T output, UINT32 bUseSE)
{
	SINT32					retVal = RET_OK;
	UINT32					bOtherOutUseSE = FALSE;
	AUD_DEV_INFO_T 			*pPreConnectDevInfo = NULL;
	AUD_DEV_INFO_T 			*pConnectDevInfo = NULL;
	ADEC_MODULE_ID			mixerModule = ADEC_MOD_TYPE_NOT_DEF;
	ADEC_MODULE_ID			connectModule = ADEC_MOD_TYPE_NOT_DEF;
	ADEC_MODULE_ID			outCtrlModule = ADEC_MOD_TYPE_NOT_DEF;
	ADEC_MODULE_ID			outModule = ADEC_MOD_TYPE_NOT_DEF;
	SpdifCmdSetOutmode 		setSpdifOutpMode;

	if(output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("Invalid Connection Trial\n");
		return RET_ERROR;
	}

	AUD_MAS_LOCK();

	pConnectDevInfo = AUDIO_GetDevInfo(connectDev);
	if(pConnectDevInfo == NULL)
	{
		AUD_KDRV_DEBUG_TMP("AUDIO_GetDevInfo(%d) failed.\n", connectDev);
		goto error_connect;
	}

	if(_gMasterInfo.outCtrlMod[output].ui32ConnectDev != LX_AUD_DEV_NONE)
	{
		pPreConnectDevInfo = AUDIO_GetDevInfo(_gMasterInfo.outCtrlMod[output].ui32ConnectDev);
		if(pPreConnectDevInfo == NULL)
		{
			AUD_KDRV_DEBUG_TMP("AUDIO_GetDevInfo(%d) failed.\n", connectDev);
			goto error_connect;
		}

		if(_gMasterInfo.outCtrlMod[output].ui32ConnectDev != connectDev)
		{
			AUD_KDRV_DEBUG_TMP("Other device(%d.%d) is already connected to %d.\n", \
							_gMasterInfo.outCtrlMod[output].ui32ConnectDev, connectDev, output);
			if(pPreConnectDevInfo->index != _gMainDecoderIndex)
			{
				AUD_MAS_UNLOCK();
				AUDIO_DisconnectOutput(_gMasterInfo.outCtrlMod[output].ui32ConnectDev, output);
				AUD_MAS_LOCK();
			}
			else
			{
				goto error_connect;
			}
		}
		else if((_gMasterInfo.outCtrlMod[output].ui32ConnectDev == connectDev) &&
			(pPreConnectDevInfo->devOutType == LX_AUD_DEV_OUT_ES))
		{
			AUD_MAS_UNLOCK();
			AUDIO_DisconnectOutput(_gMasterInfo.outCtrlMod[output].ui32ConnectDev, output);
			AUD_MAS_LOCK();
		}
	}

	if((bUseSE == TRUE) && (connectDev == LX_AUD_DEV_MAS0))
	{
		connectModule = AUDIO_FindModuleType(connectDev, ADEC_MOD_TYPE_SE);
		if(connectModule == ADEC_MODULE_NOT_DEF)
		{
			AUD_KDRV_ERROR("connectModule is invalid\n");
			goto error_connect;
		}

		bOtherOutUseSE = _AUDIO_CheckUseSE(output);
		if(bOtherOutUseSE != TRUE)
		{
			mixerModule = AUDIO_FindModuleType(connectDev, ADEC_MOD_TYPE_MIXER);
			if(mixerModule == ADEC_MODULE_NOT_DEF)
			{
				AUD_KDRV_ERROR("mixerModule is invalid\n");
				goto error_connect;
			}

			retVal = AUDIO_ConnectModules(mixerModule, connectModule, AUD_BUFFER_TYPE_NONE);
			if(retVal != RET_OK)
			{
				AUD_KDRV_ERROR("AUDIO_ConnectModules(%d,%d) failed(%d)!!!\n", \
					mixerModule, connectModule, retVal);
				goto error_connect;
			}

			AUDIO_IMC_SendCmd(ADEC_CMD_START, connectModule);
		}
	}
	else if(pConnectDevInfo->devType == LX_AUD_DEV_TYPE_MAS)
	{
		//mix
		connectModule = AUDIO_FindHeadModule(connectDev);
		if(connectModule == ADEC_MODULE_NOT_DEF)
		{
			AUD_KDRV_ERROR("connectModule is invalid \n");
			goto error_connect;
		}
	}
	else if(pConnectDevInfo->devType == LX_AUD_DEV_TYPE_REN)
	{
		//lipes
		connectModule = AUDIO_FindTailModule(connectDev);
		if(connectModule == ADEC_MODULE_NOT_DEF)
		{
			AUD_KDRV_ERROR("connectModule is invalid \n");
			goto error_connect;
		}
	}
	else
	{
		AUD_KDRV_ERROR("devType(%d) is invalid.\n", pConnectDevInfo->devType);
		goto error_connect;
	}

	if(output == LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		if(pConnectDevInfo->devOutType == LX_AUD_DEV_OUT_ES)
		{
			outCtrlModule = ADEC_MODULE_ESOUT_CTRL;
		}
		else
		{
			_gSpdifOutPcmCnt++;
			if(_gSpdifOutPcmCnt > 1)
			{
				AUD_MAS_UNLOCK();
				AUD_KDRV_DEBUG_TMP("_gSpdifOutPcmCnt(%d) \n", _gSpdifOutPcmCnt);
				return RET_OK;
			}

			outCtrlModule = _gMasterInfo.outCtrlMod[output].module;
		}
	}
	else
	{
		outCtrlModule = _gMasterInfo.outCtrlMod[output].module;
	}

	retVal = AUDIO_ConnectModules(connectModule, outCtrlModule, AUD_BUFFER_TYPE_NONE);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_ConnectModules(%d,%d) failed(%d)!!!\n", \
			connectModule, outCtrlModule, retVal);
		goto error_connect_outctrl;
	}

	if(output == LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		if(pConnectDevInfo->devOutType == LX_AUD_DEV_OUT_ES)
		{
			EsoutctrlCmdSetAll	setSpdifAll;

			setSpdifAll.Delay = _gMasterInfo.outCtrlMod[output].ui32Delay;
			setSpdifAll.Fs    = _gMasterInfo.spdifSamplingFreq;
			setSpdifAll.Mute  = _gMasterInfo.outCtrlMod[output].ui32Mute;
			AUDIO_IMC_SendCmdParam(ESOUTCTRL_CMD_SET_ALL, outCtrlModule, sizeof(EsoutctrlCmdSetAll), &setSpdifAll);
		}
	}

	if(output ==LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		outModule = ADEC_MODULE_OUT_SPDIF;

		retVal = AUDIO_ConnectModules(outCtrlModule, outModule, AUD_BUFFER_TYPE_NONE);
		if(retVal != RET_OK)
		{
			AUD_KDRV_ERROR("AUDIO_ConnectModules(%d) failed!!!\n", retVal);
			goto error_connect_out;
		}

		if(pConnectDevInfo->devOutType == LX_AUD_DEV_OUT_ES)
		{
#ifdef __ANDROID__
			LX_AUD_RENDER_PARAM_T	*pRenderParam = NULL;
#endif
			setSpdifOutpMode.out_format = LX_AUD_MASTER_SPDIF_ES;

#ifdef __ANDROID__
			pRenderParam = KDRV_AUDIO_GetRendererInfo(connectDev);
			if(pRenderParam->samplingFreq != LX_AUD_SAMPLING_FREQ_NONE)
				_gMasterInfo.spdifSamplingFreq = pRenderParam->samplingFreq;
#endif
			setSpdifOutpMode.Fs = _gMasterInfo.spdifSamplingFreq;

			_gMasterInfo.spdifOutputFormat = LX_AUD_MASTER_SPDIF_ES;
		}
		else
		{
			setSpdifOutpMode.out_format = LX_AUD_MASTER_SPDIF_PCM;
			setSpdifOutpMode.Fs = LX_AUD_SAMPLING_FREQ_48_KHZ;

			_gMasterInfo.spdifOutputFormat = LX_AUD_MASTER_SPDIF_PCM;
			_gMasterInfo.spdifSamplingFreq = LX_AUD_SAMPLING_FREQ_48_KHZ;
		}
		AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_OUTFORMAT, outModule, sizeof(SpdifCmdSetOutmode), &setSpdifOutpMode);

		//Update a SPDIF Clock Frequency.
		(void)KDRV_AUDIO_UpdateSamplingFreq(_gMasterInfo.spdifSamplingFreq);
	}

	AUDIO_IMC_SendCmd(ADEC_CMD_START, outCtrlModule);
	if(output == LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		AUDIO_IMC_SendCmd(ADEC_CMD_START, outModule);
		MIXED_IP_AUDIO_StartDTO(g_setClockSrc, _gMasterInfo.spdifOutputFormat);
	}

	_gMasterInfo.outCtrlMod[output].ui32UseSE = bUseSE;
	_gMasterInfo.outCtrlMod[output].ui32ConnectDev = connectDev;

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("connectDev:%d, output:%d, bUseSE:%d\n", connectDev, output, bUseSE);

	return RET_OK;

error_connect_out:
	(void)AUDIO_DisconnectModules(connectModule, outCtrlModule);

error_connect_outctrl:
	if((bUseSE == TRUE) && (connectDev == LX_AUD_DEV_MAS0))
	{
		(void)AUDIO_DisconnectModules(mixerModule, connectModule);
	}

	if((output == LX_AUD_MASTER_OUTPUT_SPDIF) &&
		(pConnectDevInfo->devOutType != LX_AUD_DEV_OUT_ES))
	{
		_gSpdifOutPcmCnt--;
	}

error_connect:
	AUD_MAS_UNLOCK();

	AUD_KDRV_DEBUG_TMP("connectDev:%d, output:%d, bUseSE:%d\n",  connectDev, output, bUseSE);
	return RET_ERROR;
}

/**
 * Disconnect other device to Master Outputs.
 * @see
*/
SINT32 AUDIO_DisconnectOutput(UINT32 connectDev, LX_AUD_MASTER_OUTPUT_T output)
{
	SINT32					retVal = RET_OK;
	UINT32					bOtherOutUseSE = FALSE;
	AUD_DEV_INFO_T			*pConnectDevInfo;
	ADEC_MODULE_ID			mixerModule = ADEC_MOD_TYPE_NOT_DEF;
	ADEC_MODULE_ID			connectModule = ADEC_MOD_TYPE_NOT_DEF;
	ADEC_MODULE_ID			outCtrlModule = ADEC_MOD_TYPE_NOT_DEF;
	ADEC_MODULE_ID			outModule = ADEC_MOD_TYPE_NOT_DEF;
	ImcCmdFlushParam		flushParam;

	if(output >= LX_AUD_MASTER_OUTPUT_MAX)
	{
		AUD_KDRV_ERROR("Invalid Disconnection Trial \n");
		AUDIO_UpdateConnectionInfo();
		return RET_ERROR;
	}

	AUD_MAS_LOCK();

	if(_gMasterInfo.outCtrlMod[output].ui32ConnectDev == LX_AUD_DEV_NONE)
	{
		AUD_MAS_UNLOCK();
		AUD_KDRV_DEBUG_TMP("%d is already disconnected to %d.\n", connectDev, output);
		return RET_OK;
	}

	if(_gMasterInfo.outCtrlMod[output].ui32ConnectDev != connectDev)
	{
		AUD_MAS_UNLOCK();
		AUD_KDRV_DEBUG_TMP("Other device(%d.%d) is connected to %d.\n", \
						_gMasterInfo.outCtrlMod[output].ui32ConnectDev, connectDev, output);
		return RET_OK;
	}

	pConnectDevInfo = AUDIO_GetDevInfo(connectDev);
	if(pConnectDevInfo == NULL)
	{
		AUD_MAS_UNLOCK();
		AUD_KDRV_ERROR("Check pConnectDevInfo[%p] \n", pConnectDevInfo);
		return RET_ERROR;
	}

	if(pConnectDevInfo->devType == LX_AUD_DEV_TYPE_MAS)
	{
		if(_gMasterInfo.outCtrlMod[output].ui32UseSE == FALSE)
		{
			//mix
			connectModule = AUDIO_FindHeadModule(connectDev);
			if(connectModule == ADEC_MODULE_NOT_DEF)
			{
				AUD_MAS_UNLOCK();
				AUD_KDRV_ERROR("connectModule is invalid.\n");
				return RET_ERROR;
			}
		}
		else
		{
			//se
			connectModule = AUDIO_FindModuleType(connectDev, ADEC_MOD_TYPE_SE);
			if(connectModule == ADEC_MODULE_NOT_DEF)
			{
				AUD_MAS_UNLOCK();
				AUD_KDRV_ERROR("connectModule is invalid.\n");
				return RET_ERROR;
			}

			bOtherOutUseSE = _AUDIO_CheckUseSE(output);
			if(bOtherOutUseSE != TRUE)
			{
				//mix
				mixerModule = AUDIO_FindHeadModule(connectDev);
				if(mixerModule == ADEC_MODULE_NOT_DEF)
				{
					AUD_MAS_UNLOCK();
					AUD_KDRV_ERROR("mixerModule is invalid.\n");
					return RET_ERROR;
				}

				AUDIO_IMC_SendCmd(ADEC_CMD_STOP, connectModule);

				memset(&flushParam, 0, sizeof(ImcCmdFlushParam));
				flushParam.num_of_port	= 1;
				flushParam.port_list[0]	= MOD_IN_PORT(0);
				AUDIO_IMC_SendCmdParam(ADEC_CMD_FLUSH, connectModule, sizeof(ImcCmdFlushParam), &flushParam);

				retVal = AUDIO_DisconnectModules(mixerModule, connectModule);
				if(retVal != RET_OK)
				{
					AUD_MAS_UNLOCK();
					AUD_KDRV_ERROR("AUDIO_DisconnectModules(%d) failed\n", retVal);
					return RET_ERROR;
				}
			}
		}
	}
	else if(pConnectDevInfo->devType == LX_AUD_DEV_TYPE_REN)
	{
		connectModule = AUDIO_FindTailModule(connectDev);
		if(connectModule == ADEC_MODULE_NOT_DEF)
		{
//			AUD_MAS_UNLOCK();
			AUD_KDRV_ERROR("AUDIO_FindTailModule failed.\n");
//			return RET_ERROR;
		}
	}

	// Stop Modules
	if(output == LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		if(_gMasterInfo.spdifOutputFormat == LX_AUD_MASTER_SPDIF_ES)
		{
			outCtrlModule = ADEC_MODULE_ESOUT_CTRL;
		}
		else
		{
			if(_gSpdifOutPcmCnt > 0)
			{
				_gSpdifOutPcmCnt--;
			}

			if(_gSpdifOutPcmCnt > 0)
			{
				AUD_MAS_UNLOCK();
				AUD_KDRV_DEBUG_TMP("_gSpdifOutPcmCnt(%d) \n", _gSpdifOutPcmCnt);
				return RET_OK;
			}
			outCtrlModule = _gMasterInfo.outCtrlMod[output].module;
		}

		outModule = ADEC_MODULE_OUT_SPDIF;

		AUDIO_IMC_SendCmd(ADEC_CMD_STOP, outModule);
	}
	else
	{
		outCtrlModule = _gMasterInfo.outCtrlMod[output].module;
	}

	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, outCtrlModule);

	// Flush & Disconnect Modules
	if(output == LX_AUD_MASTER_OUTPUT_SPDIF)
	{
		memset(&flushParam, 0, sizeof(ImcCmdFlushParam));
		flushParam.num_of_port	= 1;
		flushParam.port_list[0]	= MOD_IN_PORT(0);
		AUDIO_IMC_SendCmdParam(ADEC_CMD_FLUSH, outModule, sizeof(ImcCmdFlushParam), &flushParam);

		retVal = AUDIO_DisconnectModules(outCtrlModule, outModule);
		if(retVal != RET_OK)
		{
			AUD_MAS_UNLOCK();
			AUD_KDRV_ERROR("AUDIO_DisconnectModules(%d) failed!!!\n", retVal);
			return RET_ERROR;
		}
	}

	memset(&flushParam, 0, sizeof(ImcCmdFlushParam));
	flushParam.num_of_port	= 1;
	flushParam.port_list[0]	= MOD_IN_PORT(0);
	AUDIO_IMC_SendCmdParam(ADEC_CMD_FLUSH, outCtrlModule, sizeof(ImcCmdFlushParam), &flushParam);

	if(connectModule != ADEC_MODULE_NOT_DEF)
	{
		retVal = AUDIO_DisconnectModules(connectModule, outCtrlModule);
		if(retVal != RET_OK)
		{
			AUD_MAS_UNLOCK();
			AUD_KDRV_ERROR("AUDIO_DisconnectModules(%d) failed!!!\n", retVal);
			return RET_ERROR;
		}
	}

	// for ResetConnectMaster
	_gPreConnectDev[output] = _gMasterInfo.outCtrlMod[output].ui32ConnectDev;

	_gMasterInfo.outCtrlMod[output].ui32ConnectDev = LX_AUD_DEV_NONE;

	AUD_MAS_UNLOCK();

	AUD_KDRV_PRINT("connectDev:%d, output:%d\n", connectDev, output);

	return RET_OK;
}

/**
 * Connect other device to Master Outputs.
 * @see
*/
static SINT32 AUDIO_ConnectIndexOutput(UINT32 index, LX_AUD_MASTER_OUTPUT_T output,
								LX_AUD_MASTER_SPDIF_OUTPUT_T outType, UINT32 bUseSE)
{
	SINT32					retVal = RET_OK;
	LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

	devIndexCtx.ui32Index = index;
	devIndexCtx.devType = LX_AUD_DEV_TYPE_REN;
	if(outType == LX_AUD_MASTER_SPDIF_ES)
		devIndexCtx.devOutType = LX_AUD_DEV_OUT_ES;
	else
		devIndexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	retVal = AUDIO_ConnectOutput(devIndexCtx.dev, output, bUseSE);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_ConnectOutput(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("dev:%d, output:%d\n", devIndexCtx.dev, output);

	return RET_OK;
}

/**
 * Disconnect other device to Master Outputs.
 * @see
*/
static SINT32 AUDIO_DisconnectIndexOutput(UINT32 index, LX_AUD_MASTER_OUTPUT_T output,
								LX_AUD_MASTER_SPDIF_OUTPUT_T outType)
{
	SINT32					retVal = RET_OK;
	LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

	devIndexCtx.devType = LX_AUD_DEV_TYPE_REN;
	devIndexCtx.ui32Index = index;
	if(outType == LX_AUD_MASTER_SPDIF_ES)
		devIndexCtx.devOutType = LX_AUD_DEV_OUT_ES;
	else
		devIndexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	retVal = AUDIO_DisconnectOutput(devIndexCtx.dev, output);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_DisconnectOutput(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("dev:%d, output:%d\n", devIndexCtx.dev, output);

	return RET_OK;
}

/**
 * Set input volume for index.
 * @see
*/
SINT32 AUDIO_SetInputVolume(UINT32 index, UINT32 volume)
{
	SINT32					retVal = RET_OK;
	LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

	/* Update global value before module connect. */
	_gInputCtrlInfo[index].ui32InputCtrlVolume = volume;

	devIndexCtx.devType = LX_AUD_DEV_TYPE_REN;
	devIndexCtx.ui32Index = index;
	devIndexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	retVal = AUDIO_SetInputCtrlVolume(devIndexCtx.dev, volume);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_SetInputCtrlVolume(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("dev:%d, volume:%x\n", devIndexCtx.dev, volume);

	return RET_OK;
}

/**
 * Set input mute for index.
 * @see
*/
SINT32 AUDIO_SetInputMute(UINT32 index, UINT32 mute)
{
	SINT32					retVal = RET_OK;
	LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

	/* Update global value before module connect. */
	_gInputCtrlInfo[index].ui32InputCtrlMute = mute;

	devIndexCtx.devType = LX_AUD_DEV_TYPE_REN;
	devIndexCtx.ui32Index = index;
	devIndexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	retVal = AUDIO_SetInputCtrlMute(devIndexCtx.dev, mute);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_SetInputCtrlMute(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("dev:%d, mute:%x\n", devIndexCtx.dev, mute);

	return RET_OK;
}

/**
 * Set input delay for index.
 * @see
*/
SINT32 AUDIO_SetInputDelay(UINT32 index, UINT32 delay)
{
	SINT32					retVal = RET_OK;
	LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

	/* Update global value before module connect. */
	_gInputCtrlInfo[index].ui32InputCtrlDelay = delay;

	devIndexCtx.devType = LX_AUD_DEV_TYPE_REN;
	devIndexCtx.ui32Index = index;
	devIndexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	retVal = AUDIO_SetInputCtrlDelay(devIndexCtx.dev, delay);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_SetInputCtrlDelay(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	AUD_KDRV_PRINT("dev:%d, delay:%d\n", devIndexCtx.dev, delay);

	return RET_OK;
}

/**
 * Get renderer delay.
 *
 * @param 	index			[in] device index.
 * @param 	devType			[in] device type.
 * @param 	pDelay			[out] total delay ms.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		KDRV_AUDIO_IoctlDecoder().
 */
static SINT32 AUDIO_GetIndexDelay(LX_AUD_GET_INDEX_DELAY_T *pGetIndexDelay)
{
	SINT32					retVal = RET_OK;
	UINT32					delay = 0;
	BOOLEAN					lipsyncOnOff = FALSE;
	LX_AUD_DEV_INDEX_CTX_T	devIndexCtx;

	devIndexCtx.devType = pGetIndexDelay->devType;
	devIndexCtx.ui32Index = pGetIndexDelay->ui32Index;
	devIndexCtx.devOutType = LX_AUD_DEV_OUT_PCM;

	// initial Setting
	pGetIndexDelay->ui32Delay = 40;		// DEFAULT_DELAY
	pGetIndexDelay->bLipsyncOnOff = 0;	// OFF

	retVal = AUDIO_GetDevIndexCtx(&devIndexCtx);
	if(retVal != RET_OK)
	{
		AUD_KDRV_ERROR("AUDIO_GetDevIndexCtx(%d) failed!!!\n", retVal);
		return RET_ERROR;
	}

	if(pGetIndexDelay->devType == LX_AUD_DEV_TYPE_REN)
	{
		retVal = AUDIO_GetRenderDelay(devIndexCtx.dev, &delay);
		retVal = AUDIO_GetLipsyncOnOff(devIndexCtx.dev, &lipsyncOnOff);
	}
	else
		retVal = AUDIO_GetDecoderDelay(devIndexCtx.dev, &delay);

	pGetIndexDelay->ui32Delay = delay;
	pGetIndexDelay->bLipsyncOnOff = lipsyncOnOff;

	return retVal;
}

/**
 * Enable SoundBar or Wireless SoundBar.
 * @see
*/
static SINT32 AUDIO_EnableSoundBar(LX_AUD_MASTER_SOUNDBAR_T bOnOff)
{
	LX_AUD_DEV_T	allocDev = LX_AUD_DEV_REN0;
	SINT32	renIndex = 0;
	ULONG	flags;

	/* If sound bar is off, SPDIF ID Data is cleared. DDI does not call AUDIO_SetSoundBarParam function. */
	if( (bOnOff.ui32SoundBarOnOff   == FALSE && _gMasterInfo.soundBarStatus.ui32SoundBarOnOff   == TRUE)	\
	  ||(bOnOff.ui32SoundFrameOnOff == FALSE && _gMasterInfo.soundBarStatus.ui32SoundFrameOnOff == TRUE) )
	{
		AUDIO_IMC_SendCmd(SPDIF_CMD_CLEAR_FMT_FOR_SOUNDBAR, ADEC_MODULE_OUT_SPDIF);
	}

	/* If sound bar is off, SPDIF ID Data is cleared. DDI does not call AUDIO_SetSoundBarParam function. */
	if( (bOnOff.ui32WirelessOnOff 	!= _gMasterInfo.soundBarStatus.ui32WirelessOnOff)	\
	  ||(bOnOff.ui32SoundBarOnOff   != _gMasterInfo.soundBarStatus.ui32SoundBarOnOff)	\
	  ||(bOnOff.ui32SoundFrameOnOff != _gMasterInfo.soundBarStatus.ui32SoundFrameOnOff) )
	{
		/* Check a render open status for EMP File Play Case. */
		for(allocDev = LX_AUD_DEV_REN0; allocDev <= LX_AUD_DEV_REN11; allocDev++)
		{
			renIndex = GET_REN_INDEX(allocDev);

			if((renIndex < 0) || (renIndex >= DEV_REN_NUM))
			{
				AUD_KDRV_ERROR("renIndex[%d] is not available.\n", renIndex);
				return RET_ERROR;
			}

			if((_gRenderInfo[renIndex].ui32AllocDev != LX_AUD_DEV_NONE) 	\
			 &&(_gRenderInfo[renIndex].renderParam.input == LX_AUD_INPUT_SYSTEM) )
			{
				//Set a event if allocated device and event message is set for RENx.
				if( (gAudSetEvent[allocDev].allocDev != LX_AUD_DEV_NONE)		\
				  &&(gAudSetEvent[allocDev].eventMsg  & LX_AUD_EVENT_SPDIF_CHANGED) )
				{
					//spin lock for protection
					spin_lock_irqsave(&gAudEventSpinLock, flags);

					//Set a audio GET event type for next event.
					gAudGetEvent[allocDev].allocDev  = allocDev;
					gAudGetEvent[allocDev].eventMsg |= LX_AUD_EVENT_SPDIF_CHANGED;

					//spin unlock for protection
					spin_unlock_irqrestore(&gAudEventSpinLock, flags);
				}
			}
		}
	}

	_gMasterInfo.soundBarStatus.ui32SoundBarOnOff   = bOnOff.ui32SoundBarOnOff;
	_gMasterInfo.soundBarStatus.ui32WirelessOnOff   = bOnOff.ui32WirelessOnOff;
	_gMasterInfo.soundBarStatus.ui32SoundFrameOnOff = bOnOff.ui32SoundFrameOnOff;

	AUD_KDRV_DEBUG("AUDIO_EnableSoundBar(soundbar : %d, wireless : %d, soundframe : %d)\n",	\
					bOnOff.ui32SoundBarOnOff, bOnOff.ui32WirelessOnOff, bOnOff.ui32SoundFrameOnOff);
	return RET_OK;
}

/**
 * Set SoundBar ID and Data.
 * @see
*/
static SINT32 AUDIO_SetSoundBarParam(LX_AUD_MASTER_SOUNDBAR_PARAM_T *pSoundbarParam)
{
	AUD_MASTER_INFO_T				*pMstInfo = NULL;
	SpdifCmdSetFmtForSoundbar		setSoundbarFormat;

	pMstInfo = &_gMasterInfo;

	_AUDIO_RegisterMasterNoti(pMstInfo, (PFN_ImcNoti)_AUDIO_MstSoundBarStatusCb, ADEC_MODULE_OUT_SPDIF, SPDIF_EVT_GET_FMT_FOR_SOUNDBAR, IMC_ACTION_ONCE, 1);

	setSoundbarFormat.id = pSoundbarParam->ui32ID;
	setSoundbarFormat.mute = (pSoundbarParam->ui32Data >> 7) & 0x1;
	setSoundbarFormat.volume = pSoundbarParam->ui32Data & 0x7F;
	AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_FMT_FOR_SOUNDBAR, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetFmtForSoundbar), &setSoundbarFormat);

	AUD_KDRV_PRINT("AUDIO_SetSoundBarParam(mute:%d vol:%d)\n", setSoundbarFormat.mute, setSoundbarFormat.volume);
	return RET_OK;
}

/**
 * Get SoundBar Status.
 * @see
*/
SINT32 AUDIO_GetSoundBarStatus (LX_AUD_MASTER_SOUNDBAR_STATUS_T *pSoundbarStatus )
{
	memcpy(pSoundbarStatus, &(_gMasterInfo.soundBarStatus), sizeof(LX_AUD_MASTER_SOUNDBAR_STATUS_T));

	AUD_KDRV_PRINT("AUDIO_GetSoundBarStatus(%x %x %x)\n", pSoundbarStatus->ui32SubFrameID, pSoundbarStatus->ui32SubFrameData, pSoundbarStatus->ui32SubFrameCheckSum);
	return RET_OK;
}

/**
 * Set SoundBar Power On/Off.
 * @see
*/
static SINT32 AUDIO_SetSoundBarPower(UINT32 bOnOff)
{
	AUD_MASTER_INFO_T		*pMstInfo = NULL;
	SpdifCmdOnoffSoundbar	setSoundBarPower;

	pMstInfo = &_gMasterInfo;

	_AUDIO_RegisterMasterNoti(pMstInfo, (PFN_ImcNoti)_AUDIO_MstSoundBarStatusCb, ADEC_MODULE_OUT_SPDIF, SPDIF_EVT_GET_FMT_FOR_SOUNDBAR, IMC_ACTION_ONCE, 1);

	setSoundBarPower.onoff = bOnOff;

	AUDIO_IMC_SendCmdParam(SPDIF_CMD_ONOFF_SOUNDBAR, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdOnoffSoundbar), &setSoundBarPower);

	AUD_KDRV_PRINT("AUDIO_SetSoundBarPower(onOff : %d)\n", bOnOff);
	return RET_OK;
}

/**
 * Set SoundBar Command.
 * @see
*/
static SINT32 AUDIO_SetSoundBarCmd(LX_AUD_MASTER_SOUNDBAR_CMD_T *pSoundbarCmd)
{
	AUD_MASTER_INFO_T		*pMstInfo = NULL;

	SpdifCmdSetAutovolumeForSoundbar	setSoundBarAutoVolume;
	SpdifCmdSetWooferlevelForSoundbar	setSoundBarWooferLevel;
	SpdifCmdSetWirelessresetForSoundbar	setSoundBarWirelessReset;

	pMstInfo = &_gMasterInfo;

	_AUDIO_RegisterMasterNoti(pMstInfo, (PFN_ImcNoti)_AUDIO_MstSoundBarStatusCb, ADEC_MODULE_OUT_SPDIF, SPDIF_EVT_GET_FMT_FOR_SOUNDBAR, IMC_ACTION_ONCE, 1);

	/* Set a Soundbar Command Mode */
	if(pSoundbarCmd->cmdMode == LX_AUD_MASTER_SB_CMD_AUTO_VOLUME)
	{
		setSoundBarAutoVolume.onoff = pSoundbarCmd->ui32CmdParam & 0x1;
		AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_AUTOVOLUME_FOR_SOUNDBAR, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetAutovolumeForSoundbar), &setSoundBarAutoVolume);
	}
	else if(pSoundbarCmd->cmdMode == LX_AUD_MASTER_SB_CMD_WOOFER_LEVEL)
	{
		setSoundBarWooferLevel.level = pSoundbarCmd->ui32CmdParam & 0xF;
		AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_WOOFERLEVEL_FOR_SOUNDBAR, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetWooferlevelForSoundbar), &setSoundBarWooferLevel);
	}
	else if(pSoundbarCmd->cmdMode == LX_AUD_MASTER_SB_CMD_WIRELESS_RESET)
	{
		setSoundBarWirelessReset.reset = pSoundbarCmd->ui32CmdParam & 0x1;
		AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_WIRELESSRESET_FOR_SOUNDBAR, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetWirelessresetForSoundbar), &setSoundBarWirelessReset);
	}
	else
	{
		AUD_KDRV_ERROR("(cmd : %d, param : %d)\n", pSoundbarCmd->cmdMode, pSoundbarCmd->ui32CmdParam);
		return RET_ERROR;
	}

	AUD_KDRV_DEBUG("(cmd : %d, param : %d)\n", pSoundbarCmd->cmdMode, pSoundbarCmd->ui32CmdParam);
	return RET_OK;
}

/**
 * Set Main Decoder Index for SPDIF ES Output.
 * @see
*/
static SINT32 AUDIO_SetMainDecoderIndex(UINT32 mainDecoderIdx)
{
	LX_AUD_DEV_T		allocDev = LX_AUD_DEV_REN0;
	LX_AUD_DEV_T		renIndex = LX_AUD_DEV_REN0;
	ULONG				flags;

	/* If SPDIF output type is changed, notify G-Streamer for ES only. */
	if(_gMainDecoderIndex != mainDecoderIdx)
	{
		KDRV_AUDIO_DestroyDecLib();

		/* Check a render open status for EMP File Play Case. */
		for(allocDev = LX_AUD_DEV_REN0; allocDev <= LX_AUD_DEV_REN11; allocDev++)
		{
			renIndex = GET_REN_INDEX(allocDev);

			if((renIndex < 0) || (renIndex >= DEV_REN_NUM))
			{
				AUD_KDRV_ERROR("renIndex[%d] is not available.\n", renIndex);
				return RET_ERROR;
			}

			if((_gRenderInfo[renIndex].ui32AllocDev != LX_AUD_DEV_NONE) 	\
			 &&(_gRenderInfo[renIndex].renderParam.input == LX_AUD_INPUT_SYSTEM) )
			{
				//Set a event if allocated device and event message is set for RENx.
				if( (gAudSetEvent[allocDev].allocDev != LX_AUD_DEV_NONE)		\
				  &&(gAudSetEvent[allocDev].eventMsg  & LX_AUD_EVENT_SPDIF_CHANGED) )
				{
					//spin lock for protection
					spin_lock_irqsave(&gAudEventSpinLock, flags);

					//Set a audio GET event type for next event.
					gAudGetEvent[allocDev].allocDev  = allocDev;
					gAudGetEvent[allocDev].eventMsg |= LX_AUD_EVENT_SPDIF_CHANGED;

					//spin unlock for protection
					spin_unlock_irqrestore(&gAudEventSpinLock, flags);
				}
			}
		}
	}

	_gMainDecoderIndex = mainDecoderIdx;

	AUD_KDRV_DEBUG("AUDIO_SetMainDecoderIndex(%d)\n", mainDecoderIdx);
	return RET_OK;
}

/**
 * Get Main Decoder Index for SPDIF ES Output.
 * @see
*/
static SINT32 AUDIO_GetMainDecoderIndex(UINT32 *pMainDecoderIdx)
{
	//Sanity check
	if(pMainDecoderIdx == NULL)
		return RET_ERROR;

	//Copy main decoder index.
	*pMainDecoderIdx = _gMainDecoderIndex;

	AUD_KDRV_DEBUG("AUDIO_GetMainDecoderIndex(%d)\n", *pMainDecoderIdx);
	return RET_OK;
}


/**
 * Set a audio event type.
 * @see KDRV_AUDIO_PollMaster
 */
static SINT32 AUDIO_SetEvent(LX_AUD_EVENT_TYPE_T *pEvent)
{
	ULONG	flags;
	LX_AUD_DEV_T	allocDev;

	/* Set a audio device. */
	allocDev = pEvent->allocDev;

	/* Check a audio device for callback function. */
	if(allocDev >= LX_AUD_DEV_MAX)
	{
		AUD_KDRV_ERROR("allocDev is LX_AUD_DEV_MAX.(%d))\n", allocDev);
		return RET_ERROR;
	}

	//spin lock for protection
	spin_lock_irqsave(&gAudEventSpinLock, flags);

	//Set a audio SET event type for next event.
	gAudSetEvent[allocDev].allocDev = pEvent->allocDev;
	gAudSetEvent[allocDev].eventMsg = pEvent->eventMsg;

	//Reset a audio GET event type for next event.
	gAudGetEvent[allocDev].allocDev = LX_AUD_DEV_NONE;
	gAudGetEvent[allocDev].eventMsg = LX_AUD_EVENT_MASK_NONE;

	//spin unlock for protection
	spin_unlock_irqrestore(&gAudEventSpinLock, flags);

	AUD_KDRV_DEBUG("(allocDev = %d, event = 0x%X)\n", pEvent->allocDev, pEvent->eventMsg);
	return RET_OK;
}

/**
 * Get a audio event type.
 * @see KDRV_AUDIO_PollMaster
 */
static SINT32 AUDIO_GetEvent(LX_AUD_EVENT_TYPE_T *pEvent)
{
	ULONG	flags;

	LX_AUD_DEV_T		allocDev;
	LX_AUD_EVENT_MSG_T	eventMsg;

	/* Set a audio device. */
	allocDev = pEvent->allocDev;
	eventMsg = pEvent->eventMsg;

	/* Check a audio device for callback function. */
	if(allocDev >= LX_AUD_DEV_MAX)
	{
		AUD_KDRV_ERROR("allocDev is LX_AUD_DEV_MAX.(%d))\n", allocDev);
		return RET_ERROR;
	}

	//spin lock for protection
	spin_lock_irqsave(&gAudEventSpinLock, flags);

	//Check if a allocated device and event message is set.
	if( (gAudGetEvent[allocDev].allocDev != LX_AUD_DEV_NONE)		\
	  &&(gAudGetEvent[allocDev].eventMsg != LX_AUD_EVENT_MASK_NONE) )
	{
		//Set a event for notificaton.
		pEvent->allocDev = gAudGetEvent[allocDev].allocDev;
		pEvent->eventMsg = gAudGetEvent[allocDev].eventMsg;

		//Reset a audio GET event type for next event.
		gAudGetEvent[allocDev].allocDev  = pEvent->allocDev;
		gAudGetEvent[allocDev].eventMsg &= (~eventMsg);
	}
	else
	{
		//Set a event for notificaton.
		pEvent->allocDev = LX_AUD_DEV_NONE;
		pEvent->eventMsg = LX_AUD_EVENT_MASK_NONE;
	}

	//spin unlock for protection
	spin_unlock_irqrestore(&gAudEventSpinLock, flags);

	if(pEvent->eventMsg)
		AUD_KDRV_DEBUG("AUDIO_GetEvent(allocDev = %d, event(%d) = 0x%X)\n", allocDev, pEvent->allocDev, pEvent->eventMsg);
	else
		AUD_KDRV_ISR_DEBUG("AUDIO_GetEvent(allocDev = %d, event(%d) = 0x%X)\n", allocDev, pEvent->allocDev, pEvent->eventMsg);
	return RET_OK;
}

/**
 * Set the number of I2S for multichannel speakers.
 * @see KDRV_AUDIO_IoctlMaster
 */
static SINT32 AUDIO_SetI2SNumber(UINT32 number)
{
	SeCmdSetOutCh			setSeOutCh;
	OutctrlCmdSetChannel	setOutctrlCh;
	PcmoutCmdSetSpkch		setPcmoutSpkCh;

#if 0	//Speaker I2S Configuration
             2 ch           4ch          7.1ch

  L/R        DACLRCH        DACLRCH      DACLRCH
  Lh/Rh                     DACSLRCH     DACSLRCH  -> SUB(Height)
  Lr/Rr/C/Lf                             DACRLRCH, DACCLFCH -> Rear, Center
#endif

	//Check a 7.1 CH(I2S = 4)
	if(number == 4)
	{
		//Check a chip revision
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))	//A1
		{
			#ifdef INCLUDE_H14_CHIP_KDRV
			//Enable DACRLRCH I2S Port
			//CTOP_CTRL_H14A0_RdFL(ctr94);
			//CTOP_CTRL_H14A0_Wr01(ctr94, en_aud_dacrlrch, 0x1);//DAC_RLRCH, Do not use H14 UD Model.(2014.01.03, Chae Seong Lim))
			//CTOP_CTRL_H14A0_WrFL(ctr94);

			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_H14A0_RdFL(ctr44);
			CTOP_CTRL_H14A0_Wr01(ctr44, gp15_6_mux_en, 0x0);	//DAC_CLFCH
			CTOP_CTRL_H14A0_Wr01(ctr44, gp15_7_mux_en, 0x0);	//DAC_SLRCH
			CTOP_CTRL_H14A0_WrFL(ctr44);
			#endif
		}
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			#ifdef INCLUDE_M14_CHIP_KDRV
			//Enable DACRLRCH I2S Port
			CTOP_CTRL_M14B0_RdFL(LEFT, ctr99);
			CTOP_CTRL_M14B0_Wr01(LEFT, ctr99, aud_dacrlrch_enable, 0x1);	//DAC_RLRCH
			CTOP_CTRL_M14B0_WrFL(LEFT, ctr99);

			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_M14B0_RdFL(LEFT, ctr98);
			CTOP_CTRL_M14B0_Wr01(LEFT, ctr98, gp15_6_mux_en, 0x0);	//DAC_CLFCH
			CTOP_CTRL_M14B0_Wr01(LEFT, ctr98, gp15_7_mux_en, 0x0);	//DAC_SLRCH
			CTOP_CTRL_M14B0_WrFL(LEFT, ctr98);
			#endif
		}
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
		{
			#ifdef INCLUDE_M14_CHIP_KDRV
			//Enable DACRLRCH I2S Port
			CTOP_CTRL_M14A0_RdFL(ctr94);
			CTOP_CTRL_M14A0_Wr01(ctr94, en_aud_dacrlrch, 0x1);	//DAC_RLRCH
			CTOP_CTRL_M14A0_WrFL(ctr94);

			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_M14A0_RdFL(ctr44);
			CTOP_CTRL_M14A0_Wr01(ctr44, gp15_6_mux_en, 0x0);	//DAC_CLFCH
			CTOP_CTRL_M14A0_Wr01(ctr44, gp15_7_mux_en, 0x0);	//DAC_SLRCH
			CTOP_CTRL_M14A0_WrFL(ctr44);
			#endif
		}
		else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))		//B3
		{
			#ifdef INCLUDE_H13_CHIP_KDRV
			//Enable DACRLRCH I2S Port
			CTOP_CTRL_H13B0_RdFL(ctr94);
			CTOP_CTRL_H13B0_Wr01(ctr94, en_aud_dacrlrch, 0x1);	//DAC_RLRCH
			CTOP_CTRL_H13B0_WrFL(ctr94);

			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_H13B0_RdFL(ctr44);
			//CTOP_CTRL_H13B0_Wr01(ctr44, gp15_6_mux_en, 0x0);	//DAC_CLFCH, Do not use H13 UD Model.(2013.11.21, chaesu.park)
			CTOP_CTRL_H13B0_Wr01(ctr44, gp15_7_mux_en, 0x0);	//DAC_SLRCH
			CTOP_CTRL_H13B0_WrFL(ctr44);
			#endif
		}
		else
		{
			AUD_KDRV_ERROR("<<< LX_CHIP_REV => Unknown(0x%X)\n", lx_chip_rev());
		}
	}
	//Check a 4 CH(I2S = 2)
	else if(number == 2)
	{
		//Check a chip revision
		if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))	//A1
		{
		#ifdef INCLUDE_H14_CHIP_KDRV
			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_H14A0_RdFL(ctr44);
			CTOP_CTRL_H14A0_Wr01(ctr44, gp15_7_mux_en, 0x0);		//DAC_SLRCH
			CTOP_CTRL_H14A0_WrFL(ctr44);
		#endif
		}
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
		#ifdef INCLUDE_M14_CHIP_KDRV
			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_M14B0_RdFL(LEFT, ctr98);
			CTOP_CTRL_M14B0_Wr01(LEFT, ctr98, gp15_7_mux_en, 0x0);	//DAC_SLRCH
			CTOP_CTRL_M14B0_WrFL(LEFT, ctr98);
		#endif
		}
		else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
		{
		#ifdef INCLUDE_M14_CHIP_KDRV
			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_M14A0_RdFL(ctr44);
			CTOP_CTRL_M14A0_Wr01(ctr44, gp15_7_mux_en, 0x0);		//DAC_SLRCH
			CTOP_CTRL_M14A0_WrFL(ctr44);
		#endif
		}
		else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))	//B3
		{
		#ifdef INCLUDE_H13_CHIP_KDRV
			//Enable DACCLFCH I2S Port(GPIO126)
			CTOP_CTRL_H13B0_RdFL(ctr44);
			CTOP_CTRL_H13B0_Wr01(ctr44, gp15_7_mux_en, 0x0);		//DAC_SLRCH
			CTOP_CTRL_H13B0_WrFL(ctr44);
		#endif
		}
		else
		{
			AUD_KDRV_ERROR("<<< LX_CHIP_REV => Unknown(0x%X)\n", lx_chip_rev());
		}
	}

	// Set Sound Engine
	setSeOutCh.ch = DEFAULT_I2S_CH * number;
	AUDIO_IMC_SendCmdParam(SE_CMD_SET_OUTCH, ADEC_MODULE_SE, sizeof(SeCmdSetOutCh), &setSeOutCh);

	// Set Output Control for Speaker
	setOutctrlCh.Ch = DEFAULT_I2S_CH * number;
	AUDIO_IMC_SendCmdParam(OUTCTRL_CMD_SET_CHANNEL, ADEC_MODULE_OUT_CTRL_0, sizeof(OutctrlCmdSetChannel), &setOutctrlCh);

	// Set Output for Speaker
	setPcmoutSpkCh.channel = DEFAULT_I2S_CH * number;
	AUDIO_IMC_SendCmdParam(PCMOUT_CMD_SET_SPKCH, ADEC_MODULE_OUT_PCM, sizeof(PcmoutCmdSetSpkch), &setPcmoutSpkCh);

	_gMasterInfo.ui32I2SNum = number;

	AUD_KDRV_DEBUG("(number = %d)\n", number);

	return RET_OK;
}


/**
 * Find Max Delay in Output Control Modules.
 * @see AUDIO_SetDelay
 */
UINT32 AUDIO_FindMaxDelay(void)
{
	UINT32 i;
	UINT32 ui32MaxDelay = 0;

	for(i = 0; i < LX_AUD_MASTER_OUTPUT_MAX; i++)
	{
		if(_gMasterInfo.outCtrlMod[i].ui32Delay > ui32MaxDelay)
		{
			ui32MaxDelay = _gMasterInfo.outCtrlMod[i].ui32Delay;
		}
	}

	return ui32MaxDelay;
}

static AUD_SE_FN_MODE_T AUDIO_GetSeFnMode(LX_AUD_MASTER_SE_FN_T seFnMode)
{
	AUD_SE_FN_MODE_T adecSeFnMode;

	switch(seFnMode)
	{
		case LX_AUD_MASTER_SE_FN_000:
			adecSeFnMode = AUD_SE_FN000_CV;
			break;

		case LX_AUD_MASTER_SE_FN_001:
			adecSeFnMode = AUD_SE_FN001_AV;
			break;

		case LX_AUD_MASTER_SE_FN_002:
			adecSeFnMode = AUD_SE_FN002_DB;
			break;

		case LX_AUD_MASTER_SE_FN_003:
			adecSeFnMode = AUD_SE_FN003_DEQ;
			break;

		case LX_AUD_MASTER_SE_FN_004_MODE1:
			adecSeFnMode = AUD_SE_FN004_PEQMODE1;
			break;

		case LX_AUD_MASTER_SE_FN_004_MODE2:
			adecSeFnMode = AUD_SE_FN004_PEQMODE2;
			break;

		case LX_AUD_MASTER_SE_FN_004_MODE3:
			adecSeFnMode = AUD_SE_FN004_PEQMODE3;
			break;

		case LX_AUD_MASTER_SE_FN_005:
			adecSeFnMode = AUD_SE_FN005_HARM;
			break;

		case LX_AUD_MASTER_SE_FN_006:
			adecSeFnMode = AUD_SE_FN006_;
			break;

		case LX_AUD_MASTER_SE_FN_007:
			adecSeFnMode = AUD_SE_FN007_;
			break;

		case LX_AUD_MASTER_SE_FN_008:
			adecSeFnMode = AUD_SE_FN008_HC;
			break;

		case LX_AUD_MASTER_SE_FN_009:
			adecSeFnMode = AUD_SE_FN009_OSD;
			break;

		case LX_AUD_MASTER_SE_FN_010:
			adecSeFnMode = AUD_SE_FN010_IC;
			break;

		case LX_AUD_MASTER_SE_FN_011:
			adecSeFnMode = AUD_SE_FN011_IVSE;
			break;

		case LX_AUD_MASTER_SE_FN_012:
			adecSeFnMode = AUD_SE_FN012_;
			break;

		case LX_AUD_MASTER_SE_FN_013:
			adecSeFnMode = AUD_SE_FN013_NE;
			break;

		case LX_AUD_MASTER_SE_FN_014:
			adecSeFnMode = AUD_SE_FN014_ASC;
			break;

		case LX_AUD_MASTER_SE_FN_015:
			adecSeFnMode = AUD_SE_FN015_;
			break;

		case LX_AUD_MASTER_SE_FN_016:
			adecSeFnMode = AUD_SE_FN016_ELC;
			break;

		case LX_AUD_MASTER_SE_MAIN:
			adecSeFnMode = AUD_SE_MAIN;
			break;

		case LX_AUD_MASTER_SE_FN_UPDATE_PARAM:
			adecSeFnMode = AUD_SE_FN_UPDATE_PARAM;
			break;

		default:
			return AUD_SE_FN_MAX;
	}
	return adecSeFnMode;
}

#ifdef USE_BUFFER_DELAY
/**
 * callback function to repond delay callback.
 * @see		AUDIO_GetRenderDelay().
 */
static void _AUDIO_RspDelayCB(int _iParam, void *_pParam, int _paramLen, void *_cbParam)
{
	SINT32					i;
	AUD_MASTER_INFO_T		*pMasterInfo = NULL;
	CmRspGetDelay			*pRspGetDelay = NULL;

	pMasterInfo = (AUD_MASTER_INFO_T*)_cbParam;
	pRspGetDelay = (CmRspGetDelay*)_pParam;

	if(_pParam != NULL)
	{
		AUD_KDRV_DEBUG("tDelay(%d) ", pRspGetDelay->total_delay);
		for(i = 0; i < pRspGetDelay->num_of_modules; i++)
		{
			AUD_KDRV_DEBUG("mDelay(%d,%d) ", pRspGetDelay->module_list[i].module_id,
			pRspGetDelay->module_list[i].delay_in_ms);
		}
		AUD_KDRV_DEBUG("\n");

		pMasterInfo->ui32Delay = pRspGetDelay->total_delay;
	}
}
#else
/**
 * Get System Delay Callback function.
 * @param 	_param			[out] parameters.
 * @param 	_paramLen		[out] length of parameters.
 * @param 	_cbParam		[in] callback parameters.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		AUDIO_SetRendererParam
 */
static SINT32 _AUDIO_SystemDelayCb(void *_param, SINT32 _paramLen, void *_cbParam)
{
	LX_AUD_DEV_T	allocDev = LX_AUD_DEV_MAS0;

	PcmEvtMasterSystemDelay	*pSystemDelay = (PcmEvtMasterSystemDelay *)_param;
	AUD_MASTER_INFO_T		*pMasterInfo = (AUD_MASTER_INFO_T *)_cbParam;

	if(_param != NULL)
	{
		allocDev = pMasterInfo->ui32AllocDev;

		pMasterInfo->ui32Delay = pSystemDelay->master_system_delay;
	}

	AUD_KDRV_IMC_NOTI("Master System Delay : %d.\n", pSystemDelay->master_system_delay);
	return RET_OK;
}
#endif

/**
 * Get renderer delay.
 *
 * @param 	allocDev		[in] a allocated renderer device.
 * @param 	pDelay			[out] total delay ms.
 * @return 	if succeeded - RET_OK, else - RET_ERROR.
 * @see		KDRV_AUDIO_IoctlDecoder().
 */
static SINT32 AUDIO_GetMasterDelay(UINT32 allocDev, UINT32 *pDelay)
{
#ifdef USE_BUFFER_DELAY
	UINT32					numOfModule = 0;
	UINT32					bUseSE = FALSE;
	UINT32					i2sNum = 0;
	ADEC_MODULE_ID			modSE = ADEC_MODULE_NOT_DEF;
	ADEC_MODULE_ID			modOutputCtrl = ADEC_MODULE_NOT_DEF;
	CmCmdGetDelay			cmdGetDelay;
#else
	PcmoutCmdGetMasterSD		cmdGetMasterSD;
#endif
	AUD_MASTER_INFO_T		*pMasterInfo = NULL;

	if(pDelay == NULL)
	{
		AUD_KDRV_ERROR("pDecodedStatus is NULL !!!\n");
		return RET_ERROR;
	}

	pMasterInfo = &_gMasterInfo;
	pMasterInfo->ui32Delay = DEFAULT_MASTER_DELAY;		// default delay 10ms
#ifdef USE_BUFFER_DELAY
	bUseSE = pMasterInfo->outCtrlMod[LX_AUD_MASTER_OUTPUT_SPEAKER].ui32UseSE;
	i2sNum = pMasterInfo->ui32I2SNum;

	// Find module
	if(bUseSE == TRUE)
	{
		modSE = AUDIO_FindModuleType(allocDev, ADEC_MOD_TYPE_SE);
	}
	modOutputCtrl = AUDIO_FindModuleType(allocDev, ADEC_MOD_TYPE_OUTCTRL);

	// Get module info
	if(modSE != ADEC_MODULE_NOT_DEF)
	{
		cmdGetDelay.module_list[numOfModule].module_id = modSE;
		cmdGetDelay.module_list[numOfModule].module_port = IN_PORT;
		cmdGetDelay.module_list[numOfModule].bytes_per_sec = DEFAULT_BYTE_PER_SEC;
		numOfModule++;
	}

	// i2s channel must be applied  after SE.
	if(modOutputCtrl != ADEC_MODULE_NOT_DEF)
	{
		cmdGetDelay.module_list[numOfModule].module_id = modOutputCtrl;
		cmdGetDelay.module_list[numOfModule].module_port = IN_PORT;
		cmdGetDelay.module_list[numOfModule].bytes_per_sec = DEFAULT_BYTE_PER_SEC * i2sNum;
		numOfModule++;

		cmdGetDelay.module_list[numOfModule].module_id = modOutputCtrl;
		cmdGetDelay.module_list[numOfModule].module_port = OUT_PORT;
		cmdGetDelay.module_list[numOfModule].bytes_per_sec = DEFAULT_BYTE_PER_SEC* i2sNum;
		numOfModule++;

		cmdGetDelay.num_of_modules = numOfModule;
		AUDIO_IMC_SendCmdRsp(CM_CMD_GET_DELAY, ADEC_MODULE_MAN_DSP1, \
		sizeof(CmCmdGetDelay), &cmdGetDelay, _AUDIO_RspDelayCB, pMasterInfo);
	}

	OS_MsecSleep(30);

	*pDelay = pMasterInfo->ui32Delay + AUD_SE_ADDITIONAL_DELAY;
#else
	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_SystemDelayCb, ADEC_MODULE_OUT_PCM,
		PCM_EVT_SYSTEM_DELAY, IMC_ACTION_ONCE, 1);

	cmdGetMasterSD.module_id = ADEC_MODULE_OUT_CTRL_0;
	AUDIO_IMC_SendCmdParam(PCMOUT_CMD_GET_MASTERSYSTEMDELAY, ADEC_MODULE_OUT_PCM,
		sizeof(PcmoutCmdGetMasterSD), &cmdGetMasterSD);

	OS_MsecSleep(30);

	*pDelay = pMasterInfo->ui32Delay;
#endif
	AUD_KDRV_PRINT("Delay(%d)\n", *pDelay);

	return RET_OK;
}

/**
 * Update a input sampling frequency.
 * @see
*/
SINT32 KDRV_AUDIO_UpdateSamplingFreq(LX_AUD_SAMPLING_FREQ_T samplingFreq)
{
	LX_AUD_SPDIF_DTO_RATE_T	spdifDTORate;

	EsoutctrlCmdSetDelay	setSpdifDelay;
	SpdifCmdSetOutmode 		setSpdifOutpMode;

	if(_gMasterInfo.spdifOutputFormat == LX_AUD_MASTER_SPDIF_ES)
	{
		setSpdifDelay.Delay = _gMasterInfo.outCtrlMod[LX_AUD_MASTER_OUTPUT_SPDIF].ui32Delay;
		setSpdifDelay.Fs 	= samplingFreq;
		AUDIO_IMC_SendCmdParam(ESOUTCTRL_CMD_SET_DELAY, ADEC_MODULE_ESOUT_CTRL, sizeof(EsoutctrlCmdSetDelay), &setSpdifDelay);

		setSpdifOutpMode.out_format = LX_AUD_MASTER_SPDIF_ES;
		setSpdifOutpMode.Fs 		= samplingFreq;
		AUDIO_IMC_SendCmdParam(SPDIF_CMD_SET_OUTFORMAT, ADEC_MODULE_OUT_SPDIF, sizeof(SpdifCmdSetOutmode), &setSpdifOutpMode);

		/* Update a global SPDIF sampling frequency */
		if((samplingFreq != LX_AUD_SAMPLING_FREQ_32_KHZ) &&
			(samplingFreq != LX_AUD_SAMPLING_FREQ_44_1KHZ))
			_gMasterInfo.spdifSamplingFreq = LX_AUD_SAMPLING_FREQ_48_KHZ;
		else
			_gMasterInfo.spdifSamplingFreq = samplingFreq;
	}
	else
	{
		/* Update a global SPDIF sampling frequency */
		_gMasterInfo.spdifSamplingFreq = LX_AUD_SAMPLING_FREQ_48_KHZ;
	}

	// Set DTO Int Sampling Frequency
	MIXED_IP_AUDIO_SetDTOIntSamplingFreq(g_setClockSrc, _gMasterInfo.spdifOutputFormat, _gMasterInfo.spdifSamplingFreq);

	//Lock a AUDIO SPDIF Semaphore, Do not use spin lock
	OS_LockMutex(&_gMasterInfo.spdifSemaphore);

	//Set a SPDIF clock for M14 B0 Chip Bug
	if(lx_chip_rev() == LX_CHIP_REV(M14, B0)) // M14B0 only bug
	{
		//Set a decoder sampling frequency and normal DTO rate
		spdifDTORate = MIXED_IP_AUDIO_GetDTORate(g_setClockSrc, _gMasterInfo.spdifSamplingFreq, AUD_DTO_D);
	 	(void)MIXED_IP_AUDIO_SetSPDIFSamplingFreq(_gMasterInfo.spdifSamplingFreq, spdifDTORate, AUD_DTO_D);
	}
	else
	{
		//Set a decoder sampling frequency and normal DTO rate
		spdifDTORate = MIXED_IP_AUDIO_GetDTORate(g_setClockSrc, _gMasterInfo.spdifSamplingFreq, AUD_DTO_A);
	 	(void)MIXED_IP_AUDIO_SetSPDIFSamplingFreq(_gMasterInfo.spdifSamplingFreq, spdifDTORate, AUD_DTO_A);
	}

	//Unlock a AUDIO SPDIF Semaphore
	OS_UnlockMutex(&_gMasterInfo.spdifSemaphore);

	AUD_KDRV_PRINT("Fs %d\n", samplingFreq);
	return RET_OK;
}

static SINT32 AUDIO_GetStartInfo(LX_AUD_GET_START_INFO_T	*pGetStartInfo)
{
	AUD_DEV_INFO_T 	*pDevInfo = NULL;
	UINT32			bStarted = FALSE;
	UINT32			i = 0;
	UINT32			j = 0;

	if(pGetStartInfo == NULL)
	{
		AUD_KDRV_ERROR("pGetStartInfo is NULL !!!\n");
		return RET_ERROR;
	}

	memset(pGetStartInfo, 0, sizeof(LX_AUD_GET_START_INFO_T));

	// Get Decoders' Start Info
	for(i = LX_AUD_DEV_DEC0; i < LX_AUD_DEV_MAX; i++)
	{
		pDevInfo = AUDIO_GetDevInfo(i);
		if((pDevInfo->devType == LX_AUD_DEV_TYPE_DEC) ||
			(pDevInfo->devType == LX_AUD_DEV_TYPE_REN))
		{
			pGetStartInfo->startInfo[j].devType = pDevInfo->devType;
			pGetStartInfo->startInfo[j].dev = pDevInfo->dev;
			pGetStartInfo->startInfo[j].devOutType = pDevInfo->devOutType;
			pGetStartInfo->startInfo[j].ui32Index = pDevInfo->index;
			if(pDevInfo->devType == LX_AUD_DEV_TYPE_DEC)
			{
			 	AUDIO_GetDecStartInfo(pDevInfo->dev, &bStarted);
				pGetStartInfo->startInfo[j].bStarted = bStarted;
			}
			else // if(pDevInfo->devType == LX_AUD_DEV_TYPE_REN)
			{
			 	AUDIO_GetRenStartInfo(pDevInfo->dev, &bStarted);
				pGetStartInfo->startInfo[j].bStarted = bStarted;
			}
			j++;
		}

		if(j >= NUM_OF_START_INFO)
			break;
	}

	return RET_OK;
}

#ifdef KDRV_CONFIG_PM
/**
 * to copy from SE Param address for hibernation.
 * @see
*/
void	AUDIO_SE_CopyFrom(void)
{
	memcpy(gSeParam, g_pui32SEParamVirAddr, SE_PARAM_SIZE);
}

/**
 * to copy to SE Param address for hibernation.
 * @see
*/
void	AUDIO_SE_CopyTo(void)
{
	memcpy(g_pui32SEParamVirAddr, gSeParam, SE_PARAM_SIZE);
}
#endif

void	KDRV_AUDIO_ResetDisconnectMaster(void)
{
	SINT32				i = 0;
	ADEC_MODULE_ID		managerModule;
	ADEC_MODULE_ID		outctrlModule = ADEC_MODULE_OUT_CTRL_3;
	ADEC_MODULE_ID		outModule = ADEC_MODULE_OUT_SPDIF;
	CmCmdDisconnect		disconnectParam;

	managerModule = AUDIO_GetManagerModule(outctrlModule);
	disconnectParam.module = outctrlModule;
	disconnectParam.port = MOD_IN_PORT(0);
	AUDIO_IMC_SendCmdParam(CM_CMD_DISCONNECT, managerModule, sizeof(CmCmdDisconnect), &disconnectParam);

	managerModule = AUDIO_GetManagerModule(outModule);
	disconnectParam.module = outModule;
	disconnectParam.port = MOD_IN_PORT(0);
	AUDIO_IMC_SendCmdParam(CM_CMD_DISCONNECT, managerModule, sizeof(CmCmdDisconnect), &disconnectParam);

	for(i = 0; i < LX_AUD_MASTER_OUTPUT_MAX; i++)
	{
		AUDIO_DisconnectOutput(_gMasterInfo.outCtrlMod[i].ui32ConnectDev, i);
	}
}

void	KDRV_AUDIO_ResetMaster(void)
{
	UINT32			i = 0;
	AUD_EVENT_T		*pMasterEvent = NULL;

	(void)_AUDIO_DeallocMaster(LX_AUD_DEV_MAS0);

	for(i = 0; i < _gMasterInfo.ui32EventNum; i++)
	{
		pMasterEvent = &_gMasterInfo.mstEvent[i];
		IMC_CancelEvent(IMC_GetLocalImc(0), pMasterEvent->event , pMasterEvent->moduleID, pMasterEvent->actionID);
		AUD_KDRV_PRINT("IMC_CancelEvent is (%x, %x, %x)!!!\n", pMasterEvent->event,	\
			pMasterEvent->moduleID, pMasterEvent->actionID);
	}

	OS_MsecSleep(30);

	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_MixerUnderflowCb, ADEC_MODULE_MIX_0,
		MIXER_EVT_UNDERFLOW, IMC_ACTION_REPEAT, 1);

	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_SeToneDetectCb, ADEC_MODULE_SE,
		SE_EVT_DETECT_TONE, IMC_ACTION_REPEAT, 1);

	(void)_AUDIO_AllocMaster(LX_AUD_DEV_MAS0);
}

void	KDRV_AUDIO_ResetMasterParam(void)
{
	UINT32	i;
	LX_AUD_MASTER_PARAM_SE_FN_T 	seFnParam;
	LX_AUD_MASTER_CONFIG_MIXER_VOLUME_T mixerVolumeConfig;

	seFnParam.fnMode = LX_AUD_MASTER_SE_FN_UPDATE_PARAM;
	seFnParam.dataOption = LX_AUD_MASTER_SE_DATA_ALL;
	seFnParam.ui32Param = &(_gMasterInfo.seModeParam[0]);
	seFnParam.ui32NoParam = 3;
	AUDIO_SetSEFunction(&seFnParam);

	seFnParam.fnMode = LX_AUD_MASTER_SE_FN_SET;
	seFnParam.ui32Param = &(_gMasterInfo.seModeParam[0]);
	seFnParam.ui32NoParam = 3;
	AUDIO_SetSEFunction(&seFnParam);

	AUDIO_SetSpdifOnOff(_gMasterInfo.spdifOnOff);
	AUDIO_SetI2SNumber(_gMasterInfo.ui32I2SNum);

	AUDIO_SetSpdifCategoryCode(_gMasterInfo.spdifCategoryCode);

	for(i = 0; i < MAX_IN_PORT; i++)
	{
		mixerVolumeConfig.mixer = LX_AUD_MASTER_MIXER_0;
		mixerVolumeConfig.ui32Port = i;
		mixerVolumeConfig.ui32Volume = _gMasterInfo.mixerInPortVolume[i];
		AUDIO_SetMixerVolume(&mixerVolumeConfig);
	}

	return;
}

void	KDRV_AUDIO_ResetConnectMaster(void)
{
	AUD_OUT_CTRL_MODULE_T			*pOutCtrlMod = NULL;
	SINT32							i = 0;

	for(i = 0; i < LX_AUD_MASTER_OUTPUT_MAX; i++)
	{
		pOutCtrlMod = &(_gMasterInfo.outCtrlMod[i]);
		AUDIO_SetOutCtrlAll(pOutCtrlMod);
		AUDIO_ConnectOutput(_gPreConnectDev[i], i, _gMasterInfo.outCtrlMod[i].ui32UseSE);
	}
}

void	KDRV_AUDIO_RestartMaster(void)
{
	LX_AUD_SPDIF_DTO_RATE_T		spdifDTORate;
	DtoCtrlCmdSetDtorate		setDtoRate;

	//Send a IMC command for DTO setting
	if(g_AudKdrv.bInitDone == TRUE)
	{
		MIXED_IP_AUDIO_SetDTOIntSrc(g_setClockSrc, _gMasterInfo.spdifOutputFormat);

		//Set a SPDIF clock for M14 B0 Chip Bug
		if(lx_chip_rev() == LX_CHIP_REV(M14, B0)) // M14B0 only bug
		{
			// Set DTO Rate
			spdifDTORate = MIXED_IP_AUDIO_GetDTORate(g_setClockSrc, _gMasterInfo.spdifSamplingFreq, AUD_DTO_D);
		}
		else
		{
			// Set DTO Rate
			spdifDTORate = MIXED_IP_AUDIO_GetDTORate(g_setClockSrc, _gMasterInfo.spdifSamplingFreq, AUD_DTO_A);
		}
		setDtoRate.dtorate			= (UINT32)spdifDTORate;
		setDtoRate.force_cnt_clear	= (UINT32)1;
		AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_DTORATE, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetDtorate), &setDtoRate);

		MIXED_IP_AUDIO_StartDTO(g_setClockSrc, _gMasterInfo.spdifOutputFormat);
	}

	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_OUT_SPDIF);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_OUT_CTRL_0);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_OUT_CTRL_1);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_OUT_CTRL_2);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_OUT_CTRL_3);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_ESOUT_CTRL);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_SE);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_MIX_0);
}

void KDRV_AUDIO_RegisterMasterNoti(void)
{
	UINT32			i = 0;
	AUD_EVENT_T		*pMasterEvent = NULL;

	for(i = 0; i < _gMasterInfo.ui32EventNum; i++)
	{
		pMasterEvent = &_gMasterInfo.mstEvent[i];
		IMC_CancelEvent(IMC_GetLocalImc(0), pMasterEvent->event , pMasterEvent->moduleID, pMasterEvent->actionID);
		AUD_KDRV_PRINT("IMC_CancelEvent is (%x, %x, %x)!!!\n", pMasterEvent->event,	\
			pMasterEvent->moduleID, pMasterEvent->actionID);
	}

	OS_MsecSleep(30);

	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_MixerUnderflowCb, ADEC_MODULE_MIX_0,
		MIXER_EVT_UNDERFLOW, IMC_ACTION_REPEAT, 1);

	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_SeToneDetectCb, ADEC_MODULE_SE,
		SE_EVT_DETECT_TONE, IMC_ACTION_REPEAT, 1);

	// Register DTO Rate Callback function
	_AUDIO_RegisterMasterNoti(&_gMasterInfo, (PFN_ImcNoti)_AUDIO_MstDtoARateCb, ADEC_MODULE_DTO_0, DTO_EVT_CHANGE_DTO_RATE, IMC_ACTION_REPEAT, 1);

	return;
}



