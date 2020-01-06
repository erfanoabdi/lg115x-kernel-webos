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
 * date       2011.11.08
 * note       Additional information.
 *
 * @addtogroup lg115x_vdec
 * @{
 */


/*------------------------------------------------------------------------------
 *   Control Constants
 *----------------------------------------------------------------------------*/
#define LOG_NAME	vdec_vsync

/*------------------------------------------------------------------------------
 *   File Inclusions
 *----------------------------------------------------------------------------*/
#include "vsync_drv.h"
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
//#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include "vdisp_cfg.h"
#include "../hal/vsync_hal_api.h"
#include "../hal/top_hal_api.h"

#if defined(CHIP_NAME_d13)
#include "../hal/d13/mcu_base.h"
#endif
#if defined(CHIP_NAME_d14)
#include "../hal/d14/mcu_base.h"
#endif

#include "log.h"
#include <asm/div64.h>

/*------------------------------------------------------------------------------
 *   Constant Definitions
 *----------------------------------------------------------------------------*/
#define		VSYNC_NUM_OF_WORK_Q				0x80
//#define  VSYNC_ISR_PERFORMANCE_EVAL
#ifdef ARM_NOT_USING_MMCU
#define		VVSYNC_TIMING			1
#else
#if defined(CHIP_NAME_d13) || defined(CHIP_NAME_d14)
#define		VVSYNC_TIMING			(0)
#else
#define		VVSYNC_TIMING			(-5)
#endif
#endif
/*------------------------------------------------------------------------------
 *   Macro Definitions
 *----------------------------------------------------------------------------*/
//#define MS_TO_NS(x) (x * 1E6L)
#define MS_TO_NS(x) (x * 1000000L)
#ifdef CHIP_NAME_h13
#define VSYNC_SHIFT_LIMIT		2		// kppm
#define VSYNC_SHIFT_LIMIT2		1		// kppm, for none standard rate
#define VSYNC_SHIFT_DELTA		30		// 27 MHz ticks
#else
#define VSYNC_SHIFT_LIMIT		5		// kppm
#define VSYNC_SHIFT_LIMIT2		2		// kppm, for none standard rate
#define VSYNC_SHIFT_DELTA		300		// 27 MHz ticks
#endif

/*------------------------------------------------------------------------------
 *   Type Definitions
 *----------------------------------------------------------------------------*/
typedef struct
{
	BOOLEAN			bUse;
	UINT8 			ui8VdispCh;
} VDISP_CH_TAG_T;

typedef struct
{
	SINT32		i32PhaseShift;
	SINT32		i32NextPhaseShift;
	UINT32		u32NextResidual;
	UINT32		u32NextDivider;
	BOOLEAN		bNextInterlaced;

	BOOLEAN		bTimerSet;
	SINT32		max_shift;

	UINT32		dco_intv27M;
	UINT32		cur_intv27M;
	UINT32		min_intv27M;
	UINT32		max_intv27M;
	UINT32		base_intv27M;

	struct hrtimer	hr_timer;

	struct
	{
		UINT32		ui32FrameResidual;
		UINT32		ui32FrameDivider;
		UINT32		ui32FrameDuration90K;
		BOOLEAN		bInterlaced;
	} Rate;

	struct
	{
		UINT8		u8VsyncCh;
		VDISP_CH_TAG_T astSyncChTag[VDISP_MAX_NUM_OF_MULTI_CHANNEL];
	} Config;

	struct
	{
		volatile UINT32		ui32DualUseChMask;
		volatile UINT32		ui32ActiveDeIfChMask;
	} Status;

	struct
	{
		UINT32		ui32LogTick_ISR;
		UINT32		ui32LogTick_Dual;
		UINT32		ui32GSTC_Prev;
		UINT32		ui32DurationTick;
		UINT32		ui32DualTickDiff;
	} Debug;

} VSYNC_CH_T;



/*------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *----------------------------------------------------------------------------*/
extern void VDISP_VVsyncCallback(UINT8 u8VsyncCh, UINT8 u8nVdisp,
		UINT8 au8VdispCh[], UINT8 u8FieldInfo);

/*------------------------------------------------------------------------------
 *   External Variables
 *----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *   global Functions
 *----------------------------------------------------------------------------*/
void _vsync_set_rate_info(VSYNC_CH_T *pstVsync, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced);

/*------------------------------------------------------------------------------
 *   global Variables
 *----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *----------------------------------------------------------------------------*/
static void VDEC_ISR_VVSync(UINT8 ui8VSyncCh);
static void _ISR_VSync(UINT8 ui8VSyncCh);

/*------------------------------------------------------------------------------
 *   Static Variables
 *----------------------------------------------------------------------------*/
static VSYNC_CH_T gsVSync[VSYNC_NUM_OF_CH];
//static VSYNC_CB fnVsyncCallback = NULL;
static VSYNC_CB fnFeedCallback = NULL;
#ifdef  VSYNC_ISR_PERFORMANCE_EVAL
static struct timeval tv_start;
#endif

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
void VSync_Init(VSYNC_CB fnCallback)
{
	UINT8	ui8VSyncCh, ui8UseCh;

	log_noti("[VSync][DBG] Vsync Init: 0x%X, %s\n", (UINT32)gsVSync, __FUNCTION__ );

	fnFeedCallback = fnCallback;

	for( ui8VSyncCh = 0; ui8VSyncCh < VSYNC_NUM_OF_CH; ui8VSyncCh++ )
	{
		gsVSync[ui8VSyncCh].Rate.ui32FrameResidual = 0x0;
		gsVSync[ui8VSyncCh].Rate.ui32FrameDivider = 0x1;
		gsVSync[ui8VSyncCh].Rate.ui32FrameDuration90K = 0x0;
		gsVSync[ui8VSyncCh].Rate.bInterlaced = TRUE;

		gsVSync[ui8VSyncCh].dco_intv27M = 0;
		gsVSync[ui8VSyncCh].cur_intv27M = 0;
		gsVSync[ui8VSyncCh].base_intv27M = 0;
		gsVSync[ui8VSyncCh].min_intv27M = 0;
		gsVSync[ui8VSyncCh].max_intv27M = 0xFFFFFFFF;

		gsVSync[ui8VSyncCh].Config.u8VsyncCh = ui8VSyncCh;
		for( ui8UseCh = 0; ui8UseCh < VDISP_MAX_NUM_OF_MULTI_CHANNEL; ui8UseCh++ )
		{
			gsVSync[ui8VSyncCh].Config.astSyncChTag[ui8UseCh].bUse = FALSE;
			gsVSync[ui8VSyncCh].Config.astSyncChTag[ui8UseCh].ui8VdispCh = VDISP_INVALID_CHANNEL;
		}

		gsVSync[ui8VSyncCh].Status.ui32DualUseChMask = FALSE;
		gsVSync[ui8VSyncCh].Status.ui32ActiveDeIfChMask = 0x0;
	}

#if defined(CHIP_NAME_d13)
	{
		UINT32		intr_en;
		intr_en = GetReg(AD_M1_INTR_ENABLE);
		intr_en |= VDO_HW_INT_EN; //VDO interrupt
		SetReg(AD_M1_INTR_ENABLE, intr_en);
	}
#endif
#if defined(CHIP_NAME_d14)
	{
		UINT32		intr_en;
		//VCP interrupt enable -> D14
		intr_en = 0;
		//0xB00800E8 MCU register setting
		intr_en = GetReg(AD_M1_INTR_ENABLE);
		intr_en |= SYNC_HW_INT_EN; //VCP interrupt
		//intr_en |= 0x6E;	//EnableBit [1] HEVC0, [2] HEVC1, [3]H.264, [5]TS {CM3_1, CM3_0, PDEC, TE}, [6]SYNC{VDO, VCP}
		SetReg(AD_M1_INTR_ENABLE, intr_en);

		intr_en = 0;
		// set [6] DISP_INTR = {VDO_intr, VCP_intr}
		//0xB0080218 MCU register setting
		intr_en = GetReg(AD_M1_SYNC_INTR_EN);
		intr_en |= 0x01; //[1] VDO(0), [0] VCP(1)
		SetReg(AD_M1_SYNC_INTR_EN, intr_en);
	}
#endif


	VSync_HAL_Init();
	VSync_HAL_IRQ_Init(_ISR_VSync);
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
enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
	VSYNC_CH_T *pstVsync;
	UINT8		ui8VSyncCh;

	// TODO : vsync ch retrieving
	pstVsync = container_of(timer, VSYNC_CH_T, hr_timer);
	ui8VSyncCh = pstVsync->Config.u8VsyncCh;
	//pstVsync = &gsVSync[ui8VSyncCh];
	pstVsync->bTimerSet = FALSE;

	log_user3("	timer expired %X\n",(UINT32)pstVsync);
	VDEC_ISR_VVSync(ui8VSyncCh);

	return HRTIMER_NORESTART;
}

UINT8 VSync_Open(UINT8 u8VdispCh, BOOLEAN bIsDualDecoding,
		UINT32 ui32FrameRateRes, UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced)
{
	VSYNC_CH_T	*pstVsync;
	UINT8		ui8VSyncCh = VDISP_INVALID_CHANNEL;
	UINT8		i;
	UINT8		ui8EmptyCh;
	BOOLEAN		bIsIntrEnable;
	VDISP_CH_TAG_T	*astSyncChTag;

	if (u8VdispCh>=VDISP_NUM_OF_CH)
	{
		log_error("[Vsync%d][Err] %s\n", u8VdispCh, __FUNCTION__ );
		return VDISP_INVALID_CHANNEL;
	}

	if (bIsDualDecoding == TRUE)
	{
		ui8VSyncCh = u8VdispCh & (~(0x01));
		log_noti("[VSync%d][DBG] Dual - Sync:%d, vdisp:%d, %s\n",
				ui8VSyncCh, u8VdispCh, u8VdispCh, __FUNCTION__ );
	}
	else
		ui8VSyncCh = u8VdispCh;

	pstVsync = &gsVSync[ui8VSyncCh];
	astSyncChTag = pstVsync->Config.astSyncChTag;

	// check already registered vdisp?
	if (pstVsync->Status.ui32ActiveDeIfChMask & (1 << u8VdispCh))
	{
		log_error("[VSync%d][Err] Already Set - ActiveChMask: 0x%X(%d)\n",
				ui8VSyncCh, pstVsync->Status.ui32ActiveDeIfChMask, u8VdispCh);

		// display vdisp ch info for this vsync
		for( i = 0; i < VDISP_MAX_NUM_OF_MULTI_CHANNEL; i++ )
			log_error("[VSync%d:%d] Use:%d, Sync:%d\n", ui8VSyncCh, i,
					astSyncChTag[i].bUse,
					astSyncChTag[i].ui8VdispCh);

		return VDISP_INVALID_CHANNEL;
	}

	log_noti("[VSync%d] Vsync Open Request : "
			"vdisp %d DualDec:%d, ActiveChMask: 0x%X\n",
			ui8VSyncCh, u8VdispCh, bIsDualDecoding,
			pstVsync->Status.ui32ActiveDeIfChMask);

	// find empty slot
	ui8EmptyCh = VDISP_INVALID_CHANNEL;
	for( i = 0; i < VDISP_MAX_NUM_OF_MULTI_CHANNEL; i++ )
	{
		if( astSyncChTag[i].bUse == TRUE )
		{
			if (astSyncChTag[i].ui8VdispCh == u8VdispCh)
			{
				log_error("[VSync%d:%d][Err] Already Set - Sync:%d, DE_IF:%d, %s(%d)\n",
						ui8VSyncCh, i, u8VdispCh, u8VdispCh, __FUNCTION__, __LINE__ );
				return VDISP_INVALID_CHANNEL;
			}
		}
		else
		{
			ui8EmptyCh = i;
			break;
		}
	}

	if( ui8EmptyCh == VDISP_INVALID_CHANNEL )
	{
		log_error("[VSync%d][Err] Not Enough Mux Channel, %s(%d)\n",
				ui8VSyncCh, __FUNCTION__, __LINE__ );
		return VDISP_INVALID_CHANNEL;
	}

	if( bIsDualDecoding == TRUE )
	{
		if( pstVsync->Status.ui32DualUseChMask & (1 << (UINT32)ui8EmptyCh) )
			log_error("[VSync%d][Err] IsDualDecoding - Sync:%d, %s(%d)\n",
					ui8VSyncCh, u8VdispCh, __FUNCTION__, __LINE__ );
		else
			pstVsync->Status.ui32DualUseChMask |= (1 << (UINT32)ui8EmptyCh);
	}
	astSyncChTag[ui8EmptyCh].bUse = TRUE;
	astSyncChTag[ui8EmptyCh].ui8VdispCh = u8VdispCh;
	pstVsync->Status.ui32ActiveDeIfChMask |= (1 << u8VdispCh);

	// if the first vsync usage,
	if( pstVsync->Status.ui32ActiveDeIfChMask == (1 << u8VdispCh) )
	{
		pstVsync->i32PhaseShift = 0;
		pstVsync->i32NextPhaseShift = 0;
		pstVsync->dco_intv27M = 0;

		pstVsync->Debug.ui32LogTick_ISR = 0;
		pstVsync->Debug.ui32LogTick_Dual = 0;
		pstVsync->Debug.ui32GSTC_Prev = 0xFFFFFFFF;
		pstVsync->Debug.ui32DurationTick = 0;
		pstVsync->Debug.ui32DualTickDiff = 0;

		_vsync_set_rate_info(pstVsync, ui32FrameRateRes,
				ui32FrameRateDiv, bInterlaced);

		pstVsync->u32NextResidual = ui32FrameRateRes;
		pstVsync->u32NextDivider = ui32FrameRateDiv;
		pstVsync->bNextInterlaced = bInterlaced;

		pstVsync->bTimerSet = FALSE;
		pstVsync->max_shift = 10800;

		VSync_HAL_SetVsyncField(ui8VSyncCh, ui32FrameRateRes, ui32FrameRateDiv,
				bInterlaced);
		TOP_HAL_SetLQSyncMode(ui8VSyncCh, ui8VSyncCh);

#ifdef ARM_USING_MMCU
		TOP_HAL_EnableInterIntr(VSYNC0+ui8VSyncCh);
#endif

		TOP_HAL_ClearVsyncIntr(VSYNC0+ui8VSyncCh);
		TOP_HAL_EnableVsyncIntr(VSYNC0+ui8VSyncCh);
		VSync_HAL_EnableVsync(ui8VSyncCh);

		log_noti("[VSync%d][DBG] Enable Interrupt, %s \n",
				ui8VSyncCh, __FUNCTION__ );
	}

	bIsIntrEnable = TOP_HAL_IsVsyncIntrEnable(VSYNC0+ui8VSyncCh);
	if( bIsIntrEnable == FALSE )
	{
		log_error("[VSync%d][Err] VSync Channel Interrupt Disabled, %s \n",
				ui8VSyncCh, __FUNCTION__ );
	}

	return ui8VSyncCh;
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
void VSync_Close(UINT8 ui8VSyncCh, UINT8 ui8VdispCh)
{
	VSYNC_CH_T		*pstVsync;
	UINT8			ui8UseCh;
	UINT8			ui8DeleteCh;

	if( ui8VSyncCh >= VSYNC_NUM_OF_CH )
	{
		log_error("[VSync%d][Err] %s\n", ui8VSyncCh, __FUNCTION__ );
		return;
	}

	log_noti("[VSync%d] vdisp %d, %s\n", ui8VSyncCh, ui8VdispCh, __FUNCTION__ );
	pstVsync = &gsVSync[ui8VSyncCh];

	ui8DeleteCh = VDISP_INVALID_CHANNEL;
	for( ui8UseCh = 0; ui8UseCh < VDISP_MAX_NUM_OF_MULTI_CHANNEL; ui8UseCh++ )
	{
		if( pstVsync->Config.astSyncChTag[ui8UseCh].bUse == TRUE )
		{
			if (pstVsync->Config.astSyncChTag[ui8UseCh].ui8VdispCh == ui8VdispCh)
			{
				if( ui8DeleteCh != VDISP_INVALID_CHANNEL )
					log_error("[VSync%d][Err] Not Matched Ch - Sync:%d\n",
							ui8VSyncCh, ui8VdispCh);

				ui8DeleteCh = ui8UseCh;
			}
		}
	}
	if( ui8DeleteCh == VDISP_INVALID_CHANNEL )
	{
		log_error("[VSync%d][Err] Not Matched Ch - Sync:%d\n",
				ui8VSyncCh, ui8VdispCh);
		return;
	}

	pstVsync->Config.astSyncChTag[ui8DeleteCh].ui8VdispCh = VDISP_INVALID_CHANNEL;
	pstVsync->Config.astSyncChTag[ui8DeleteCh].bUse = FALSE;
	pstVsync->Status.ui32DualUseChMask &= ~(1 << (UINT32)ui8DeleteCh);

	log_noti("[VSync%d] Sync:%d, %s \n", ui8VSyncCh, ui8VdispCh, __FUNCTION__);

	pstVsync->Status.ui32ActiveDeIfChMask &= ~(1 << ui8VdispCh);
	if( pstVsync->Status.ui32ActiveDeIfChMask == 0x0 )
	{
		BOOLEAN			bIsIntrEnable;

		pstVsync->Debug.ui32LogTick_ISR = 0;
		pstVsync->Debug.ui32LogTick_Dual = 0;
		pstVsync->Debug.ui32GSTC_Prev = 0xFFFFFFFF;
		pstVsync->Debug.ui32DurationTick = 0;
		pstVsync->Debug.ui32DualTickDiff = 0;

		pstVsync->Rate.ui32FrameResidual = 0x0;
		pstVsync->Rate.ui32FrameDivider = 0x1;
		pstVsync->Rate.ui32FrameDuration90K = 0x0;
		pstVsync->Rate.bInterlaced = TRUE;

#ifdef VDEC_USE_MCU
		hrtimer_cancel(&pstVsync->hr_timer);
#endif

		bIsIntrEnable = TOP_HAL_IsVsyncIntrEnable(VSYNC0+ui8VSyncCh);
		if( bIsIntrEnable == FALSE )
			log_error("[VSync%d] VSync Intr Already Disabled, %s \n",
					ui8VSyncCh, __FUNCTION__ );

		TOP_HAL_DisableVsyncIntr(VSYNC0+ui8VSyncCh);
		TOP_HAL_ClearVsyncIntr(VSYNC0+ui8VSyncCh);
		log_noti("[VSync%d][DBG] Disable Interrupt, %s \n", ui8VSyncCh,
				__FUNCTION__ );
	}
}

int VSync_IsActive (UINT8 ui8VSyncCh)
{
	if (ui8VSyncCh >= VSYNC_NUM_OF_CH)
		return 0;

	return gsVSync[ui8VSyncCh].Status.ui32ActiveDeIfChMask != 0;
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
UINT32 VSync_CalFrameDuration(UINT32 ui32FrameRateRes, UINT32 ui32FrameRateDiv)
{
	//use do_div() function
	UINT64 		ui64FrameRateDiv90K_Scaled;

	if( ui32FrameRateRes == 0 )
		return 0;

	ui64FrameRateDiv90K_Scaled = (UINT64)ui32FrameRateDiv * 90000;
	do_div( ui64FrameRateDiv90K_Scaled, ui32FrameRateRes ); //do_div(x,y) -> x = x/y

	return ui64FrameRateDiv90K_Scaled;
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
void VSync_SetMaxShift(UINT8 ui8VSyncCh, SINT32 max)
{
	log_noti("[Vsync%d] shift %d\n", ui8VSyncCh, max);

	gsVSync[ui8VSyncCh].max_shift = max;
	gsVSync[ui8VSyncCh].max_intv27M = gsVSync[ui8VSyncCh].cur_intv27M+max;
	gsVSync[ui8VSyncCh].min_intv27M = gsVSync[ui8VSyncCh].cur_intv27M-max;
}

void VSync_SetPhaseShift(UINT8 ui8VSyncCh, SINT32 i32ShiftDelta90K)
{
	SINT32 i32ShiftDelta27M;

	log_noti("[Vsync%d] shift %d\n", ui8VSyncCh, i32ShiftDelta90K);

	i32ShiftDelta27M = 27000000/90000 * i32ShiftDelta90K;

	gsVSync[ui8VSyncCh].i32NextPhaseShift = i32ShiftDelta27M;
}

void VSync_SetDcoFrameRate(UINT8 ui8VSyncCh, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv)
{
	VSYNC_CH_T	*pstVsync;
	UINT64		t;

	if (ui8VSyncCh >= VSYNC_NUM_OF_CH)
	{
		log_error("[VSync%d][Err] Channel Number, %s\n",
				ui8VSyncCh, __FUNCTION__);
		return;
	}

	if (ui32FrameRateRes == 0)
	{
		log_error("[VSync%d][Err] wrong frame info. res %d div %d\n",
				ui8VSyncCh, ui32FrameRateRes, ui32FrameRateDiv);
		return;
	}

	pstVsync = &gsVSync[ui8VSyncCh];

	t = (UINT64)ui32FrameRateDiv * 27000000;
	do_div(t, ui32FrameRateRes );

	pstVsync->dco_intv27M = (UINT32)t;

	log_noti("[VSync%d] Set dco rate. res/div %u/%u, 27M %u",
			ui8VSyncCh, ui32FrameRateRes, ui32FrameRateDiv,
			pstVsync->dco_intv27M);
	return;
}

BOOLEAN VSync_SetNextVsyncField(UINT8 ui8VSyncCh, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced)
{
	if( ui8VSyncCh >= VSYNC_NUM_OF_CH )
	{
		log_error("[VSync%d][Err] Channel Number, %s\n", ui8VSyncCh, __FUNCTION__ );
		return FALSE;
	}
	gsVSync[ui8VSyncCh].u32NextResidual = ui32FrameRateRes;
	gsVSync[ui8VSyncCh].u32NextDivider = ui32FrameRateDiv;
	gsVSync[ui8VSyncCh].bNextInterlaced = bInterlaced;

	return TRUE;
}

UINT32 VSync_GetVsyncIntv(UINT8 ui8VSyncCh)
{
	VSYNC_CH_T	*pstVsync;
	UINT32		ret;

	if( ui8VSyncCh >= VSYNC_NUM_OF_CH )
	{
		log_error("[VSync%d][Err] Channel Number, %s\n", ui8VSyncCh, __FUNCTION__ );
		return 3003;
	}

	pstVsync = &gsVSync[ui8VSyncCh];
	ret = pstVsync->Rate.ui32FrameDuration90K;

	if (pstVsync->Rate.bInterlaced)
		ret /= 2;

	return ret;
}

UINT32 _vsync_get_base_intv27M(UINT32 intv27M)
{
	UINT32 ret;
	SINT32 diff10k;

	if (intv27M < 900000/2-1000)
		return 0;
	else if (intv27M < 900900/2+1000)		// 59.94, 60 Hz
		ret = 900000/2;
	else if (intv27M < 1081080/2+1000)		// 49.5, 50 Hz
		ret = 1080000/2;
	else if (intv27M < 900900+1000)			// 29.97, 30 Hz
		ret = 900000;
	else if (intv27M < 1081080+1000)		// 24.97, 25 Hz
		ret = 1080000;
	else if (intv27M < 1126125+1000)		// 23.97, 24 Hz
		ret = 1125000;
	else
		return 0;

	diff10k = ((SINT32)(intv27M-ret))*10000;
	diff10k /= ret;

	if (diff10k>11 || diff10k<-11)
		ret = 0;

	return ret;
}

void _vsync_set_rate_info(VSYNC_CH_T *pstVsync, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced)
{
	UINT32		ui32FrameDuration90K;
	UINT64		t;

	ui32FrameDuration90K = VSync_CalFrameDuration(ui32FrameRateRes,
			ui32FrameRateDiv);

	pstVsync->Rate.ui32FrameResidual = ui32FrameRateRes;
	pstVsync->Rate.ui32FrameDivider = ui32FrameRateDiv;
	pstVsync->Rate.bInterlaced = bInterlaced;
	pstVsync->Rate.ui32FrameDuration90K = ui32FrameDuration90K;

	t = (UINT64)27000000*ui32FrameRateDiv;
	do_div(t, ui32FrameRateRes);
	pstVsync->cur_intv27M = (UINT32)t;
	pstVsync->base_intv27M = _vsync_get_base_intv27M(pstVsync->cur_intv27M);

#if defined(CHIP_NAME_h13)
	if (pstVsync->base_intv27M == 1125000)
	{
		pstVsync->min_intv27M = pstVsync->base_intv27M-1800;
		pstVsync->max_intv27M = pstVsync->base_intv27M+1800;
	}
	else
#endif
	if (pstVsync->base_intv27M != 0)
	{
		t = (UINT64)pstVsync->base_intv27M* (1000-VSYNC_SHIFT_LIMIT);
		do_div(t,1000);
		pstVsync->min_intv27M = t+20;
		t = (UINT64)pstVsync->base_intv27M* (1000+VSYNC_SHIFT_LIMIT);
		do_div(t,1000);
		pstVsync->max_intv27M = t-20;
	}
	else
	{
		pstVsync->base_intv27M = pstVsync->cur_intv27M;
		t = (UINT64)pstVsync->base_intv27M* (1000-VSYNC_SHIFT_LIMIT2);
		do_div(t,1000);
		pstVsync->min_intv27M = t+20;
		t = (UINT64)pstVsync->base_intv27M* (1000+VSYNC_SHIFT_LIMIT2);
		do_div(t,1000);
		pstVsync->max_intv27M = t-20;
	}
}

BOOLEAN VSync_SetVsyncField(UINT8 ui8VSyncCh, UINT32 ui32FrameRateRes,
		UINT32 ui32FrameRateDiv, BOOLEAN bInterlaced)
{
	SINT32		i32NextPhaseShift=0;
	BOOLEAN		bUpdated = FALSE;
	VSYNC_CH_T	*pstVsync;

	if( ui8VSyncCh >= VSYNC_NUM_OF_CH )
	{
		log_error("[VSync%d][Err] Channel Number, %s\n", ui8VSyncCh, __FUNCTION__ );
		return FALSE;
	}

	pstVsync = &gsVSync[ui8VSyncCh];

	if( (ui32FrameRateRes == 0) || (ui32FrameRateDiv == 0) )
	{
		//log_error("[VSync%d] FrameRateRes:%d, FrameRateDiv:%d, %s\n",
		//		ui8VSyncCh, ui32FrameRateRes, ui32FrameRateDiv, __FUNCTION__);
		return FALSE;
	}

	if (pstVsync->dco_intv27M)
		i32NextPhaseShift = pstVsync->dco_intv27M - pstVsync->cur_intv27M;

	if (pstVsync->i32NextPhaseShift!=0)
	{
		if (i32NextPhaseShift==0)
			i32NextPhaseShift = pstVsync->i32NextPhaseShift;
		else if (i32NextPhaseShift>0)
		{
			if (pstVsync->i32NextPhaseShift>i32NextPhaseShift)
				i32NextPhaseShift = pstVsync->i32NextPhaseShift;
			else if (pstVsync->i32NextPhaseShift<0)
				i32NextPhaseShift += pstVsync->i32NextPhaseShift;
		}
		else
		{
			if (pstVsync->i32NextPhaseShift<i32NextPhaseShift)
				i32NextPhaseShift = pstVsync->i32NextPhaseShift;
			else if (pstVsync->i32NextPhaseShift>0)
				i32NextPhaseShift += pstVsync->i32NextPhaseShift;
		}
	}

	log_user2("[VSYNC%d] avg %u avg diff %d chgto %d  cur %d shift %d\n",
			ui8VSyncCh, pstVsync->dco_intv27M,
			pstVsync->dco_intv27M - pstVsync->cur_intv27M,
			i32NextPhaseShift,
			pstVsync->i32PhaseShift,
			pstVsync->i32NextPhaseShift
			);

	// change delta limit
	if (pstVsync->i32PhaseShift-i32NextPhaseShift > VSYNC_SHIFT_DELTA)
		i32NextPhaseShift = pstVsync->i32PhaseShift - VSYNC_SHIFT_DELTA;
	if (pstVsync->i32PhaseShift-i32NextPhaseShift < -VSYNC_SHIFT_DELTA)
		i32NextPhaseShift = pstVsync->i32PhaseShift + VSYNC_SHIFT_DELTA;

	if (pstVsync->cur_intv27M+i32NextPhaseShift>pstVsync->max_intv27M)
		i32NextPhaseShift = pstVsync->max_intv27M-pstVsync->cur_intv27M;
	else if (pstVsync->cur_intv27M+i32NextPhaseShift<pstVsync->min_intv27M)
		i32NextPhaseShift = pstVsync->min_intv27M-pstVsync->cur_intv27M;

	if( (pstVsync->Rate.ui32FrameResidual != ui32FrameRateRes) ||
			(pstVsync->Rate.ui32FrameDivider != ui32FrameRateDiv) ||
			(pstVsync->Rate.bInterlaced != bInterlaced) ||
			pstVsync->i32PhaseShift != i32NextPhaseShift )
	{
		UINT32		ui32FieldRate_Prev;
		UINT32		ui32FieldRate_Curr;
		UINT32		pre_intv90K;

		pre_intv90K = pstVsync->Rate.ui32FrameDuration90K;

		_vsync_set_rate_info(pstVsync, ui32FrameRateRes,
				ui32FrameRateDiv, bInterlaced);

		ui32FieldRate_Prev = (pstVsync->Rate.bInterlaced == TRUE) ? 2 : 1;
		ui32FieldRate_Curr = (bInterlaced == TRUE) ? 2 : 1;

		log_noti("[VSync%d]FieldDuration90k: %d --> %d %d/%d shift %d\n",
				ui8VSyncCh,
				pstVsync->Rate.ui32FrameDuration90K / ui32FieldRate_Prev,
				pre_intv90K / ui32FieldRate_Curr,
				pre_intv90K, ui32FieldRate_Curr,
				i32NextPhaseShift/300);

		if( !VSync_HAL_IntField(ui8VSyncCh) )
		{
			pstVsync->i32PhaseShift = i32NextPhaseShift;
			VSync_HAL_SetVsyncShift(ui8VSyncCh, pstVsync->i32PhaseShift);
		}

		VSync_HAL_SetVsyncField(ui8VSyncCh, ui32FrameRateRes,
				ui32FrameRateDiv, bInterlaced);

		bUpdated = TRUE;
	}

	if (pstVsync->i32NextPhaseShift && !VSync_HAL_IntField(ui8VSyncCh))
	{
		SINT32 pre_break, abs;

		pre_break = pstVsync->i32PhaseShift*pstVsync->i32PhaseShift;
		pre_break /= 2*VSYNC_SHIFT_DELTA;

		if (pstVsync->i32NextPhaseShift>0)
			abs = pstVsync->i32NextPhaseShift;
		else
			abs = -(pstVsync->i32NextPhaseShift);

		if (abs >= pre_break)
			pstVsync->i32NextPhaseShift -= pstVsync->i32PhaseShift;
		else
			pstVsync->i32NextPhaseShift=0;
	}

	return bUpdated;
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
static inline void _VSYNC_Debug_Log(VSYNC_CH_T *pstVsync, UINT32 u32Gstc)
{
	UINT32			ui32DurationTick = 0;
	UINT8			ui8VSyncCh = pstVsync->Config.u8VsyncCh;
	VDISP_CH_TAG_T	*pstSync;

	pstVsync->Debug.ui32LogTick_ISR++;
	if( pstVsync->Debug.ui32LogTick_ISR >= 0x100 )
	{
		static UINT8		ui8UseCh = 0;

		if( pstVsync->Config.astSyncChTag[ui8UseCh].bUse )
		{
			pstSync = &pstVsync->Config.astSyncChTag[ui8UseCh];
			log_noti("[VSy%d:%d,VDS%d] STC:0x%X DBG:0x%X Rate:%d, "
					"DualTickDiff:%d\n",
					ui8VSyncCh, ui8UseCh, pstSync->ui8VdispCh,
					u32Gstc, u32Gstc,
					90000 / (u32Gstc - pstVsync->Debug.ui32GSTC_Prev),
					pstVsync->Debug.ui32DualTickDiff);

			pstVsync->Debug.ui32LogTick_ISR = 0;
		}

		ui8UseCh++;
		if( ui8UseCh >= VDISP_MAX_NUM_OF_MULTI_CHANNEL )
			ui8UseCh=0;
	}

	//if( pstVsync->Debug.ui32GSTC_Prev != 0xFFFFFFFF )
	//{
	//	UINT32		ui32GSTC_End;
	//
	//	ui32GSTC_End = TOP_HAL_GetGSTCC() & 0x7FFFFFFF;
	//
	//	pstVsync->Debug.ui32VsyncShare += (ui32GSTC_End >= u32Gstc) ?
	//		(ui32GSTC_End - u32Gstc) : (ui32GSTC_End + 0x7FFFFFFF - u32Gstc);
	//	pstVsync->Debug.ui32VsyncSum += (u32Gstc >= pstVsync->Debug.ui32GSTC_Prev) ?
	//		(u32Gstc - pstVsync->Debug.ui32GSTC_Prev) :
	//		(u32Gstc + 0x7FFFFFFF - pstVsync->Debug.ui32GSTC_Prev);
	//}

	if( pstVsync->Debug.ui32GSTC_Prev != 0xFFFFFFFF )
	{
		ui32DurationTick = ( u32Gstc >= pstVsync->Debug.ui32GSTC_Prev ) ?
			u32Gstc - pstVsync->Debug.ui32GSTC_Prev :
			0x7FFFFFFF - pstVsync->Debug.ui32GSTC_Prev + u32Gstc;
	}

	if( pstVsync->Debug.ui32DurationTick )
	{
		UINT32		ui32DurationTick_Diff;

		ui32DurationTick_Diff = ( ui32DurationTick >= pstVsync->Debug.ui32DurationTick ) ?
			ui32DurationTick - pstVsync->Debug.ui32DurationTick :
			pstVsync->Debug.ui32DurationTick - ui32DurationTick;

		if( ui32DurationTick_Diff > 270 )	// > 3ms
			log_noti("[VSync%d] Unstable vsync: intv %X->%X\n",
					ui8VSyncCh, pstVsync->Debug.ui32DurationTick, ui32DurationTick);
	}

	pstVsync->Debug.ui32DurationTick = ui32DurationTick;
	pstVsync->Debug.ui32GSTC_Prev = u32Gstc;

#ifdef  VSYNC_ISR_PERFORMANCE_EVAL
	// performance check
	{
		struct timeval tv_end;
		static UINT32 u32TotalTime=0, u32Cnt=0, u32Max=0;
		UINT32 t;

		do_gettimeofday(&tv_end);

		u32Cnt++;
		t = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
				tv_end.tv_usec - tv_start.tv_usec;
		if( t>u32Max ) u32Max = t;

		u32TotalTime += t;

		if( u32Cnt >= 200 )
		{
			log_error("Ev.t %u Max %u\n", u32TotalTime/u32Cnt, u32Max);
			u32Cnt = 0;
			u32TotalTime = 0;
			u32Max = 0;
		}
	}
#endif

}

UINT32 VSYNC_GetFieldDuration(UINT8 u8VSyncCh)
{
	UINT32 u32Ret;

	u32Ret = gsVSync[u8VSyncCh].Rate.ui32FrameDuration90K;
	if( gsVSync[u8VSyncCh].Rate.bInterlaced )
		u32Ret = u32Ret>>1;

	return u32Ret;
}

void VSYNC_SetVVsyncTimer(UINT8 u8VSyncCh, UINT32 u32Time90K)
{
	VSYNC_CH_T	*pstVsync;
	ktime_t ktime;

	pstVsync = &gsVSync[u8VSyncCh];

	if (pstVsync->Status.ui32ActiveDeIfChMask == 0x0)
		return;

	if (pstVsync->bTimerSet)
	{
		log_warning("timer aleady set\n");
		return;
	}
	pstVsync->bTimerSet = TRUE;

	ktime = ktime_set(0, MS_TO_NS(u32Time90K/90));

	hrtimer_init(&pstVsync->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	pstVsync->hr_timer.function = &hrtimer_callback;

	log_user3("Starting timer %ums %u\n", u32Time90K/90, u32Time90K );

	hrtimer_start( &pstVsync->hr_timer, ktime, HRTIMER_MODE_REL );

}


static void VDEC_ISR_VVSync(UINT8 ui8VSyncCh)
{
	VSYNC_CH_T *pstVsync;
	UINT8			i;
	UINT32			u32Gstc;
	UINT32			u32IntnField;
	UINT32			u32nSync = 0;
	UINT8			au8SyncCh[VDISP_MAX_NUM_OF_MULTI_CHANNEL];

	if (ui8VSyncCh >= VSYNC_NUM_OF_CH)
	{
		log_error("[VVSync%d][Err] Channel NO. %s\n", ui8VSyncCh, __FUNCTION__ );
		goto _vvsync_exit;
	}

	pstVsync = &gsVSync[ui8VSyncCh];
	// sync scan
	for( i = 0; i < VDISP_MAX_NUM_OF_MULTI_CHANNEL; i++ )
	{
		if( pstVsync->Config.astSyncChTag[i].bUse == TRUE )
			au8SyncCh[u32nSync++] = pstVsync->Config.astSyncChTag[i].ui8VdispCh;
	}

	u32IntnField = (VSync_HAL_IntField(ui8VSyncCh));
	u32Gstc = TOP_HAL_GetGSTCC() & 0x7FFFFFFF;

	log_user3("[VVsync%d] GSTC %d  Field %d\n", ui8VSyncCh, u32Gstc, u32IntnField);

	VDISP_VVsyncCallback(ui8VSyncCh, u32nSync, au8SyncCh, u32IntnField);

_vvsync_exit:

	return;
}

static inline void _vsync_set_vvsync_timer(UINT8 ui8VSyncCh, SINT8 i8RelativeMs)
{
	UINT32 u32FieldIntv;	// 90K
	SINT32 i32TimerDelta;	// 90K

	u32FieldIntv = VSYNC_GetFieldDuration(ui8VSyncCh);
	i32TimerDelta = u32FieldIntv + (i8RelativeMs*90);	// before 4ms from next vsync

	if (i32TimerDelta<1*90)
		i32TimerDelta = 1*90;

	VSYNC_SetVVsyncTimer(ui8VSyncCh, (UINT32)i32TimerDelta);
}

static void _ISR_VSync(UINT8 ui8VSyncCh)
{
	VSYNC_CH_T *pstVsync;
	UINT8			i;
	UINT32			u32Gstc;
	UINT32			u32IntnField;
	UINT32			u32nSync = 0;
	UINT8			au8SyncCh[VDISP_MAX_NUM_OF_MULTI_CHANNEL];

	if (ui8VSyncCh >= VSYNC_NUM_OF_CH)
	{
		log_error("[VSync%d][Err] Channel NO. %s\n", ui8VSyncCh, __FUNCTION__ );
		goto _vsync_exit;
	}

#ifdef  VSYNC_ISR_PERFORMANCE_EVAL
	{
		//static struct timeval tv_pre_start;
		do_gettimeofday(&tv_start);
		//log_error("intv %u\n",
		//		(tv_start.tv_sec - tv_pre_start.tv_sec) * 1000000 +
		//		tv_start.tv_usec - tv_pre_start.tv_usec);
		//tv_pre_start = tv_start;
	}
#endif

	pstVsync = &gsVSync[ui8VSyncCh];
	// sync scan
	for( i = 0; i < VDISP_MAX_NUM_OF_MULTI_CHANNEL; i++ )
	{
		if( pstVsync->Config.astSyncChTag[i].bUse == TRUE )
			au8SyncCh[u32nSync++] = pstVsync->Config.astSyncChTag[i].ui8VdispCh;
	}

	u32IntnField = (VSync_HAL_IntField(ui8VSyncCh));
	u32Gstc = TOP_HAL_GetGSTCC() & 0x7FFFFFFF;

	log_user3("[Vsync%d] GSTC %d  Field %d\n", ui8VSyncCh, u32Gstc, u32IntnField);

#if (VVSYNC_TIMING>0)
	VDISP_VVsyncCallback(ui8VSyncCh, u32nSync, au8SyncCh, u32IntnField);
#endif

#ifndef ARM_USING_MMCU
	if( fnFeedCallback != NULL )
		fnFeedCallback(ui8VSyncCh, u32nSync, au8SyncCh, u32IntnField);
#endif

	VSync_SetVsyncField(ui8VSyncCh, pstVsync->u32NextResidual,
			pstVsync->u32NextDivider, pstVsync->bNextInterlaced );

#if (VVSYNC_TIMING<0)
	_vsync_set_vvsync_timer(ui8VSyncCh, VVSYNC_TIMING);
#elif (VVSYNC_TIMING==0)
	VDISP_VVsyncCallback(ui8VSyncCh, u32nSync, au8SyncCh, u32IntnField);
#endif

	_VSYNC_Debug_Log(pstVsync, u32Gstc);

_vsync_exit:

	return;
}

void _vsync_print_channel(struct seq_file *m, VSYNC_CH_T *pstVsyncCh)
{
#define VDS_DPRINT(frm, args...)		seq_printf(m, frm, ##args )
	VDS_DPRINT(" i32PhaseShift     : %d\n", (UINT32)pstVsyncCh->i32PhaseShift);
	VDS_DPRINT(" i32NextPhaseShift : %d\n", (UINT32)pstVsyncCh->i32NextPhaseShift);
	VDS_DPRINT(" u32NextResidual   : %u\n", (UINT32)pstVsyncCh->u32NextResidual);
	VDS_DPRINT(" u32NextDivider    : %u\n", (UINT32)pstVsyncCh->u32NextDivider);
	VDS_DPRINT(" bNextInterlaced   : %u\n", (UINT32)pstVsyncCh->bNextInterlaced);
	VDS_DPRINT(" bTimerSet         : %u\n", (UINT32)pstVsyncCh->bTimerSet);
	VDS_DPRINT(" ui32FrameResidual    : %u\n", (UINT32)pstVsyncCh->Rate.ui32FrameResidual);
	VDS_DPRINT(" ui32FrameDivider     : %u\n", (UINT32)pstVsyncCh->Rate.ui32FrameDivider);
	VDS_DPRINT(" ui32FrameDuration90K : %u\n", (UINT32)pstVsyncCh->Rate.ui32FrameDuration90K);
	VDS_DPRINT(" bInterlaced          : %u\n", (UINT32)pstVsyncCh->Rate.bInterlaced);
	VDS_DPRINT(" u8VsyncCh   : %u\n", (UINT32)pstVsyncCh->Config.u8VsyncCh);

	VDS_DPRINT(" max_shift         : %d\n", (UINT32)pstVsyncCh->max_shift);
	VDS_DPRINT(" dco_intv27M       : %u\n", pstVsyncCh->dco_intv27M);
	VDS_DPRINT(" cur_intv27M       : %u\n", pstVsyncCh->cur_intv27M);
	VDS_DPRINT(" min_intv27M       : %u\n", pstVsyncCh->min_intv27M);
	VDS_DPRINT(" max_intv27M       : %u\n", pstVsyncCh->max_intv27M);
	VDS_DPRINT(" base_intv27M      : %u\n", pstVsyncCh->base_intv27M);

	VDS_DPRINT(" ui32DualUseChMask     : %u\n", pstVsyncCh->Status.ui32DualUseChMask);
	VDS_DPRINT(" ui32ActiveDeIfChMask  : %u\n", pstVsyncCh->Status.ui32ActiveDeIfChMask);

	VDS_DPRINT(" ui32LogTick_ISR  : %u\n", (UINT32)pstVsyncCh->Debug.ui32LogTick_ISR);
	VDS_DPRINT(" ui32LogTick_Dual : %u\n", (UINT32)pstVsyncCh->Debug.ui32LogTick_Dual);
	VDS_DPRINT(" ui32GSTC_Prev    : %u\n", (UINT32)pstVsyncCh->Debug.ui32GSTC_Prev);
	VDS_DPRINT(" ui32DurationTick : %u\n", (UINT32)pstVsyncCh->Debug.ui32DurationTick);
	VDS_DPRINT(" ui32DualTickDiff : %u\n", (UINT32)pstVsyncCh->Debug.ui32DualTickDiff);
	VDS_DPRINT("\n\n");
}

void VSYNC_PrintDebug(struct seq_file *m)
{
	VSYNC_CH_T		*pstVsyncCh;
	UINT32			i;

	for (i=0;i<VSYNC_NUM_OF_CH;i++)
	{
		pstVsyncCh = &gsVSync[i];
		VDS_DPRINT("====  Vsync #%d [%X]  ====\n", i, (UINT32)pstVsyncCh);
		_vsync_print_channel(m, pstVsyncCh);
	}

#undef VDS_DPRINT
}

void VSYNC_Suspend(void)
{
	VSync_HAL_Suspend();
}

void VSYNC_Resume(void)
{
	VSync_HAL_Resume();
}
