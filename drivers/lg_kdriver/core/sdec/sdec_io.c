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
 *  sdec driver
 *
 *  @author	Jihoon Lee ( gaius.lee@lge.com)
 *  @author	Jinhwan Bae ( jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010-03-30
 *  @note		Additional information.
 */


/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/
#define PCR_RECOVERY_DEBUG	0

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#include "os_util.h"


#include "sdec_kapi.h"
//#include "sdec_reg.h"
#include "sdec_drv.h"
#include "sdec_io.h"
#include "sdec_pes.h"
#include "sdec_hal.h"

#include "../sys/sys_regs.h"	//for CTOP CTRL Reg. map
#include "h13/sdec_reg_h13a0.h"
#include "m14/sdec_reg_m14a0.h"
#include "m14/sdec_reg_m14b0.h"
#include "h14/sdec_reg_h14a0.h"

#include "sdec_swparser.h"

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/
#ifdef __NEW_PWM_RESET_COND__ /* jinhwan.bae for new PWM reset condition */

#define 	MAX_ERROR_FOR_PWM_RESET		 	10

UINT8	pcr_error_for_reset = 0;

#endif


/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/
extern volatile SDTOP_REG_H13A0_T *stSDEC_TOP_RegH13A0;
extern volatile SDTOP_REG_M14A0_T *stSDEC_TOP_RegM14A0;
extern volatile SDTOP_REG_M14B0_T *stSDEC_TOP_RegM14B0;
extern volatile SDTOP_REG_H14A0_T *stSDEC_TOP_RegH14A0;

extern volatile SDIO_REG_H13A0_T	*stSDEC_IO_RegH13A0[1];
extern volatile SDIO_REG_M14A0_T	*stSDEC_IO_RegM14A0[1];
extern volatile SDIO_REG_M14B0_T	*stSDEC_IO_RegM14B0[LX_SDEC_MAX_NUM_OF_CORE];
extern volatile SDIO_REG_H14A0_T	*stSDEC_IO_RegH14A0[LX_SDEC_MAX_NUM_OF_CORE];

extern volatile MPG_REG_H13A0_T 	*stSDEC_MPG_RegH13A0[2];
extern volatile MPG_REG_M14A0_T 	*stSDEC_MPG_RegM14A0[2];
extern volatile MPG_REG_M14B0_T 	*stSDEC_MPG_RegM14B0[4];
extern volatile MPG_REG_H14A0_T 	*stSDEC_MPG_RegH14A0[4];

extern CTOP_CTRL_REG_H13_T 			gCTOP_CTRL_H13;

/*----------------------------------------------------------------------------------------
 *   global Functions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/
static	void pwm_context_reset(S_SDEC_PARAM_T	*stpSdecParam, UINT8 cur_ch);
static BOOLEAN _SDEC_Is_JPClearPacket(UINT32 ui32PidValue);

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/


/**
********************************************************************************
* @brief
*   initialize Mutex.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_MutexInitialize
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_MutexInitialize");

	//mutex_init(&stpSdecParam->stSdecMutex);
	OS_InitMutex(&stpSdecParam->stSdecMutex, OS_SEM_ATTR_DEFAULT);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_MutexInitialize");

	eRet = OK;

	return (eRet);
}

/**
********************************************************************************
* @brief
*   Deastory Mutex.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_MutexDestroy
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_MutexInitialize");

	//mutex_init(&stpSdecParam->stSdecMutex);
	OS_InitMutex(&stpSdecParam->stSdecMutex, OS_SEM_ATTR_DEFAULT);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_MutexInitialize");

	eRet = OK;

	return (eRet);
}


/**
********************************************************************************
* @brief
*   initialize spinlock.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_SpinLockInitialize
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;
	int ch_idx = 0, flt_idx = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_SpinLockInitialize");

	spin_lock_init(&stpSdecParam->stSdecNotiSpinlock);
	spin_lock_init(&stpSdecParam->stSdecPesSpinlock);
	spin_lock_init(&stpSdecParam->stSdecSecSpinlock);
	spin_lock_init(&stpSdecParam->stSdecResetSpinlock);
	spin_lock_init(&stpSdecParam->stSdecPidfSpinlock);

	for ( ch_idx = 0; ch_idx < SDEC_IO_CH_NUM ; ch_idx++ )
	{
		for ( flt_idx = 0; flt_idx < SDEC_IO_FLT_NUM ; flt_idx++ )
		{
			spin_lock_init(&stpSdecParam->stSdecGpbSpinlock[ch_idx][flt_idx]);
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_SpinLockInitialize");

	eRet = OK;

	return (eRet);
}


/**
********************************************************************************
* @brief
*   initialize Workqueue.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_WorkQueueInitialize
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_WorkQueueInitialize");

	//Sdec work queue init and fnc reg
	stpSdecParam->workqueue = create_workqueue("SDEC");

	if(!stpSdecParam->workqueue){
		SDEC_DEBUG_Print("create work queue failed");
		goto exit;
	}

	INIT_WORK(&stpSdecParam->Notify,		SDEC_Notify);
	INIT_WORK(&stpSdecParam->PcrRecovery,	SDEC_PCRRecovery);
	INIT_WORK(&stpSdecParam->PesProc,		SDEC_PES_Proc);
	INIT_WORK(&stpSdecParam->TPIIntr,		SDEC_TPIIntr);

	eRet = OK;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_WorkQueueInitialize");

exit:
	return (eRet);
}

/**
********************************************************************************
* @brief
*   initialize Workqueue destory.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_WorkQueueDestroy
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_WorkQueueDestroy");

	flush_workqueue(stpSdecParam->workqueue);

	destroy_workqueue(stpSdecParam->workqueue);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_WorkQueueDestroy");

	eRet = OK;

	return (eRet);
}

/**
********************************************************************************
* @brief
*   initialize Watequeue destory.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_WaitQueueInitialize
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_WaitQueueInitialize");

	init_waitqueue_head(&stpSdecParam->wq);
	stpSdecParam->wq_condition = 0;

	/* SW Parser Event Init */
	OS_InitEvent( &stpSdecParam->stSdecSWPEvent);

	/* LIVE_HEVC */
	OS_InitEvent( &stpSdecParam->stSdecRFOUTEvent);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_WaitQueueInitialize");

	eRet = OK;

	return (eRet);
}

/**
********************************************************************************
* @brief
*   initialize sdec module.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam : SDEC parameter
*   stpLXSdecCap : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_InitialaizeModule
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CAP_T*	stpLXSdecCap)
{
	DTV_STATUS_T 	eRet = NOT_OK;
	DTV_STATUS_T 	eResult = NOT_OK;
	UINT8 			ui8ch;
	LX_SDEC_CFG_T* 	pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCap == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_InitialaizeModule");

	ui8ch = stpLXSdecCap->eCh;

	/* jinhwan.bae H13 Latch Up at booting 2013. 06. 25 
	    M14_TBD - can be adjusted with following eCh argument check */
	if(lx_chip_rev() < LX_CHIP_REV(M14, B0))
	{
		if(ui8ch > 3) 
		{
			eRet = OK;
			goto exit;
		}
	}

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8ch);

	/* 20131202 workaround for abnormal interupt in M14Bx */
	if( (lx_chip_rev() >= LX_CHIP_REV(M14, B0)) && (lx_chip_rev() < LX_CHIP_REV(H14, A0)) )
	{
		if( (ui8ch == (UINT8)LX_SDEC_CH_E) || (ui8ch == (UINT8)LX_SDEC_CH_F) )
		{
			/* All init even unused filters */
			eResult = SDEC_Pidf_AllClear(stpSdecParam, ui8ch);
			if( LX_IS_ERR(eResult) ) SDEC_DEBUG_Print("SDEC_Pidf_Clear failed:[%d]", eResult);
			
			eResult = SDEC_Secf_AllClear(stpSdecParam, ui8ch);		
			if( LX_IS_ERR(eResult) ) SDEC_DEBUG_Print("SDEC_Secf_Clear failed:[%d]", eResult);
		}
	}

	/* pidf init */
	if(pSdecConf->chInfo[ui8ch].num_pidf != 0)
	{
		eResult = SDEC_Pidf_Clear(stpSdecParam, ui8ch, CLEAR_ALL_MODE);
		LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_Pidf_Clear failed:[%d]", eResult);
	}
	
	/* secf init */
	if(pSdecConf->chInfo[ui8ch].num_secf != 0)
	{
		eResult = SDEC_Secf_Clear(stpSdecParam, ui8ch, CLEAR_ALL_MODE);
		LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_Secf_Clear failed:[%d]", eResult);
	}

	/* get filter number & depth from channel info structure */
	stpLXSdecCap->ucPidFltNr = pSdecConf->chInfo[ui8ch].num_pidf;
	stpLXSdecCap->ucSecFltNr = pSdecConf->chInfo[ui8ch].num_secf;
	stpLXSdecCap->ucFltDepth = pSdecConf->chInfo[ui8ch].flt_dept;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_InitialaizeModule");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   Set MCU Descrambler Setting Mode in H13, M14A0
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam : SDEC parameter
*   stpLXSdecSetMcumode : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetMCUDescramblerCtrlMode
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SET_MCUMODE_T *stpLXSdecSetMcumode)
{
	/* H13 Only */
	if (lx_chip_rev() < LX_CHIP_REV(M14, A0))
	{
		stpSdecParam->ui8McuDescramblerCtrlMode = stpLXSdecSetMcumode->ui32param;
	}

	return OK;
}

/**
********************************************************************************
* @brief
*   Set MCU Descrambler Setting Mode in H13, M14A0
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam : SDEC parameter
*   stpLXSdecSetMcumode : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetTVCTMode
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SET_TVCTMODE_T *stpLXSdecClearTVCTGathering)
{
	stpSdecParam->ui8ClearTVCTGathering = stpLXSdecClearTVCTGathering->ui32param;
	return OK;
}

/**
********************************************************************************
* @brief
*   Set MCU Descrambler Setting Mode in H13, M14A0
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam : SDEC parameter
*   stpLXSdecSetMcumode : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_UseOrgExtDemodCLKinSerialInput
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SET_SERIALCLKMODE_T *stpLXSdecSetSerialClkMode)
{

	stpSdecParam->ui8UseOrgExtDemodClk = stpLXSdecSetSerialClkMode->ui32param;

	return OK;
}

/**
********************************************************************************
* @brief
*   pid filter clear.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*    ui8PidfIdx :pid idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Pidf_AllClear
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch)
{
	DTV_STATUS_T 	eRet = NOT_OK;
	UINT8 			ui8Pidf_idx = 0x0;
	LX_SDEC_CFG_T* 	pSdecConf = NULL;

	/* it's only for M14Bx abnormal interrupt */

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Pidf_AllClear");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	if( pSdecConf->chInfo[ui8ch].capa_lev == 0 )
	{
		/* Skip CDIC2 setting at first.
		    if the download and PDEC2 pr happened, please check it */

		eRet = OK;
		goto exit;
	}

	//clear all pid filter
  	for (ui8Pidf_idx = 0x0; ui8Pidf_idx < 64; ui8Pidf_idx++)
  	{
		SDEC_SetPidfData(stpSdecParam, ui8ch, ui8Pidf_idx, 0x1FFF0000);
		SDEC_HAL_SECFSetMap(ui8ch, ui8Pidf_idx * 2, 0);
		SDEC_HAL_SECFSetMap(ui8ch, ui8Pidf_idx * 2 + 1, 0);
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Pidf_AllClear");

	eRet = OK;

exit:

	return (eRet);
}

/**
********************************************************************************
* @brief
*   section filter clear.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8PidfIdx :pid idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Secf_AllClear
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch)
{
	DTV_STATUS_T 	eRet = NOT_OK;
	UINT8			ui8Secf_idx = 0x0, ui8Word_idx = 0x0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	/* it's only for M14Bx abnormal interrupt */

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Secf_AllClear");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	/* if pid filter is just simple filter, set ts2pes filter only */
	if( pSdecConf->chInfo[ui8ch].capa_lev == 0 )
	{
		eRet = OK;
		goto exit;
	}

	SDEC_HAL_SECFSetEnable(ui8ch, 0, 0);
	SDEC_HAL_SECFSetEnable(ui8ch, 1, 0);
	SDEC_HAL_SECFSetBufValid(ui8ch, 0, 0);
	SDEC_HAL_SECFSetBufValid(ui8ch, 1, 0);
	SDEC_HAL_SECFSetMapType(ui8ch, 0, 0);
	SDEC_HAL_SECFSetMapType(ui8ch, 1, 0);

	for (ui8Secf_idx = 0x0; ui8Secf_idx < 64; ui8Secf_idx++)
	{
		for (ui8Word_idx = 0x0; ui8Word_idx < 7; ui8Word_idx++)
		{
			SDEC_HAL_SECFSetSecfData(ui8ch, ui8Secf_idx, ui8Word_idx, 0xAA000000);
  		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Secf_AllClear");

	eRet = OK;

exit:
	return (eRet);
}

/**
********************************************************************************
* @brief
*   pid filter clear.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*    ui8PidfIdx :pid idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Pidf_Clear
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch,
	UINT32 ui8PidfIdx)
{
	DTV_STATUS_T 	eRet = NOT_OK;
	UINT8 			ui8Pidf_idx = 0x0;
	UINT8 			ui8PidfNum = 0x0;
	LX_SDEC_CFG_T* 	pSdecConf = NULL;
	UINT8			core = 0, org_ch = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Pidf_Clear");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	/* get pid filter number from channel info structure */
	ui8PidfNum	= pSdecConf->chInfo[ui8ch].num_pidf;

	/* if channel doesn't have pid filter, return error */
	if( ui8PidfNum == 0 )
	{
		SDEC_DEBUG_Print("this channel [%d] doesn't have pid filter !!!!", ui8ch);
		eRet = OK;
		goto exit;
	}

	org_ch = ui8ch;
	SDEC_CONVERT_CORE_CH(core, ui8ch);

#if 0 // jinhwan.bae Remove ts2pes pid filter clear at this function. Remake it another function if ts2pes pid filtercontrol needed. 2013. 06. 15
	/* from H13 A0, CDIC2 has 4 pid filters */
	if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		if( pSdecConf->chInfo[ui8ch].capa_lev == 0 )
		{
			if(ui8PidfIdx == CLEAR_ALL_MODE)
			{
				//clear all pid filter
		      	for (ui8Pidf_idx = 0x0; ui8Pidf_idx < ui8PidfNum; ui8Pidf_idx++)
		      	{
		        	SDEC_HAL_CDIC2PIDFSetPidfData(ui8Pidf_idx, 0x1FFF);
				}
			}
		   	else if (ui8PidfIdx < ui8PidfNum )
		   	{
				SDEC_HAL_CDIC2PIDFSetPidfData(ui8PidfIdx, 0x1FFF);
		   	}

			eRet = OK;
			goto exit;
		}
	}
	else
	{
		/* if pid filter is just simple filter, set ts2pes filter only */
		if( pSdecConf->chInfo[ui8ch].capa_lev == 0 )
		{
			/* pidf init */
			SDEC_HAL_SetTs2PesPid(0x1FFF);
			eRet = OK;
			goto exit;
		}
	}
#else
	if( pSdecConf->chInfo[org_ch].capa_lev == 0 )
	{
		if(ui8PidfIdx == CLEAR_ALL_MODE)
		{
			//clear all pid filter
	      	for (ui8Pidf_idx = 0x0; ui8Pidf_idx < ui8PidfNum; ui8Pidf_idx++)
	      	{
	        	SDEC_HAL_CDIC2PIDFSetPidfData(core, ui8Pidf_idx, 0x1FFF);
			}
		}
	   	else if (ui8PidfIdx < ui8PidfNum )
	   	{
			SDEC_HAL_CDIC2PIDFSetPidfData(core, ui8PidfIdx, 0x1FFF);
	   	}

		eRet = OK;
		goto exit;
	}
#endif

	if(ui8PidfIdx == CLEAR_ALL_MODE)
	{
		//clear all pid filter
      	for (ui8Pidf_idx = 0x0; ui8Pidf_idx < ui8PidfNum; ui8Pidf_idx++)
      	{
			SDEC_SetPidfData(stpSdecParam, org_ch, ui8Pidf_idx, 0x1FFF0000);
			SDEC_HAL_SECFSetMap(org_ch, ui8Pidf_idx * 2, 0);
			SDEC_HAL_SECFSetMap(org_ch, ui8Pidf_idx * 2 + 1, 0);
		}
	}
   	else if (ui8PidfIdx < ui8PidfNum )
   	{
		SDEC_SetPidfData(stpSdecParam, org_ch, ui8PidfIdx, 0x1FFF0000);
		SDEC_HAL_SECFSetMap(org_ch, ui8PidfIdx * 2, 0);
		SDEC_HAL_SECFSetMap(org_ch, ui8PidfIdx * 2 + 1, 0);
   	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Pidf_Clear");

	eRet = OK;

exit:

	return (eRet);
}

/**
********************************************************************************
* @brief
*   section filter clear.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8PidfIdx :pid idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Secf_Clear
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch,
	UINT32 ui8SecfIdx)
{
	DTV_STATUS_T 	eRet = NOT_OK;
	UINT8			ui8Secf_idx = 0x0, ui8Word_idx = 0x0;
	UINT8 			ui8SecfNum 	= 0x0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Secf_Clear");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	/* get section filter number from channel info structure */
	ui8SecfNum	= pSdecConf->chInfo[ui8ch].num_secf;

	/* if channel doesn't have secction filter, return error */
	if( ui8SecfNum == 0 )
	{
		SDEC_DEBUG_Print("this channel doesn't have section filter !!!!");
		eRet = OK;
		goto exit;
	}

	/* if pid filter is just simple filter, set ts2pes filter only */
	if( pSdecConf->chInfo[ui8ch].capa_lev == 0 )
	{
		eRet = OK;
		goto exit;
	}

	//auto increment disable
	//SDEC_Enable_AutoIncr(stpSdecParam, 0);
//	SDEC_HAL_EnableAutoIncr(SDEC_HAL_DISABLE);

	if(ui8SecfIdx == CLEAR_ALL_MODE)
	{
		//clear all section filter
		//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_en[0].secf_en = 0x0;
		//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_en[1].secf_en = 0x0;
		//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secfb_valid[0].secfb_valid = 0x0;
		//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secfb_valid[1].secfb_valid = 0x0;
		//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_mtype[0].secf_mtype = 0x0;
		//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_mtype[1].secf_mtype = 0x0;
		SDEC_HAL_SECFSetEnable(ui8ch, 0, 0);
		SDEC_HAL_SECFSetEnable(ui8ch, 1, 0);
		SDEC_HAL_SECFSetBufValid(ui8ch, 0, 0);
		SDEC_HAL_SECFSetBufValid(ui8ch, 1, 0);
		SDEC_HAL_SECFSetMapType(ui8ch, 0, 0);
		SDEC_HAL_SECFSetMapType(ui8ch, 1, 0);

		for (ui8Secf_idx = 0x0; ui8Secf_idx < ui8SecfNum; ui8Secf_idx++)
		{

			for (ui8Word_idx = 0x0; ui8Word_idx < 7; ui8Word_idx++)
			{
				//secf_addr.secf_idx = ui8Secf_idx;
				//secf_addr.word_idx = ui8Word_idx;
				//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_addr = secf_addr;
  				//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_data.secf_data = 0xAA000000;
  				SDEC_HAL_SECFSetSecfData(ui8ch, ui8Secf_idx, ui8Word_idx, 0xAA000000);
	  		}
		}

	/* For PES H/W bug workaound. See @LX_SDEC_USE_KTHREAD_PES */
	#if ( LX_SDEC_USE_KTHREAD_PES == 1)
		if(pSdecConf->noPesBug == 0)
		{
			/* if there is pes h/w buf, do it */
			SDEC_PES_AllClearPESFlt(ui8ch);
		}
	#endif

   	}
   	else if ( ui8SecfIdx < ui8SecfNum )
   	{
	   	//clear only one section filter

		//clear__bit(stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_en[ui8IsHigh].secf_en, (ui8SecfIdx%32));
		//clear__bit(stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secfb_valid[ui8IsHigh].secfb_valid, (ui8SecfIdx % 32));
		SDEC_HAL_SECFClearEnableBit(ui8ch, ui8SecfIdx);
		SDEC_HAL_SECFClearBufValidBit(ui8ch, ui8SecfIdx);
		for (ui8Word_idx = 0x0; ui8Word_idx < 7; ui8Word_idx++)
		{
			//secf_addr.secf_idx = ui8SecfIdx;
			//secf_addr.word_idx = ui8Word_idx;
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_addr = secf_addr;
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->secf_data.secf_data = 0xAA000000;
			SDEC_HAL_SECFSetSecfData(ui8ch, ui8SecfIdx, ui8Word_idx, 0xAA000000);
		}
	/* For PES H/W bug workaound. See @LX_SDEC_USE_KTHREAD_PES */
#if ( LX_SDEC_USE_KTHREAD_PES == 1)
		if(pSdecConf->noPesBug == 0)
		{
			/* if there is pes h/w buf, do it */
			SDEC_PES_ClearPESFlt(ui8ch, ui8SecfIdx);
		}
#endif
   	}

	//auto increment enable
	//SDEC_Enable_AutoIncr(stpSdecParam, 1);
//	SDEC_HAL_EnableAutoIncr(SDEC_HAL_ENABLE);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Secf_Clear");

	eRet = OK;

exit:
	return (eRet);
}

/**
********************************************************************************
* @brief
*   GPB init.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8GpbSize :gpb size
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Gpb_Init
	(void)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0, ui8Idx = 0;
	UINT32 gpbBaseAddr = 0, gpbBaseAddr_H = 0, gpbBaseAddr_L = 0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Gpb_Init");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	LX_SDEC_CHECK_CODE( pSdecConf == NULL, goto exit, "pSdecConf is NULL" );

	gpbBaseAddr = gMemCfgSDECGPB[pSdecConf->memCfg].gpb_memory_base;
	gpbBaseAddr &= ~GPB_BASE_ADDR_MASK;
	gpbBaseAddr_H = ( gpbBaseAddr >> 16 ) & 0xFFFF;
	gpbBaseAddr_L = ( gpbBaseAddr >> 0  ) & 0xFFFF;

	/* initialize BND & Write & Read pointer Register as base address */
	for ( ui8Ch = 0 ; ui8Ch < 2 ; ui8Ch++ )
	{
		for ( ui8Idx = 0 ; ui8Idx < 64 ; ui8Idx++ )
		{
			SDEC_HAL_GPBSetBnd(ui8Ch, ui8Idx, gpbBaseAddr_H, gpbBaseAddr_L);
			SDEC_HAL_GPBSetReadPtr(ui8Ch, ui8Idx, gpbBaseAddr);
			SDEC_HAL_GPBSetWritePtr(ui8Ch, ui8Idx, gpbBaseAddr);
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Gpb_Init");

	eRet = OK;

exit:
	return (eRet);
}

DTV_STATUS_T SDEC_GpbSet
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch,
	LX_SDEC_GPB_SIZE_T eGpbSize,
	UINT32 ui32GpbBaseAddr,
	UINT32 ui32GpbIdx)
{
	DTV_STATUS_T eRet = NOT_OK;

	GPB_BND stGpbBnd;
	UINT32 ui32Lower_Bound = 0x0, ui32Upper_Bound = 0x0, ui32GpbSize = 0x0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_GpbSet");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	if( ui8ch >= pSdecConf->nChannel )
	{
		SDEC_DEBUG_Print("over channel range %d", ui8ch);
		goto exit;
	}

	if( ui32GpbIdx >= pSdecConf->chInfo[ui8ch].num_secf )
	{
		SDEC_DEBUG_Print("over GPB range %d", ui32GpbIdx);
		goto exit;
	}

	ui32GpbSize = eGpbSize * 0x1000;

	ui32Lower_Bound = ui32GpbBaseAddr & 0x0FFFF000;
	ui32Upper_Bound = ui32Lower_Bound + ui32GpbSize;

	/* set GPB boudary register */
	stGpbBnd.l_bnd = (ui32Lower_Bound >> 12) & 0x0000FFFF;
	stGpbBnd.u_bnd = (ui32Upper_Bound >> 12) & 0x0000FFFF;

	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		if((ui8ch < LX_SDEC_CH_G) && ( (ui8ch != LX_SDEC_CH_C) || (ui8ch != LX_SDEC_CH_D) ) )
		{
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->gpb_bnd[ui32GpbIdx]= stGpbBnd;
			SDEC_HAL_GPBSetBnd(ui8ch, ui32GpbIdx, stGpbBnd.l_bnd, stGpbBnd.u_bnd);

			/* set GPB read & write pointer register */
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->gpb_w_ptr[ui32GpbIdx].gpb_w_ptr =	ui32Lower_Bound;
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->gpb_r_ptr[ui32GpbIdx].gpb_r_ptr =	ui32Lower_Bound;
			SDEC_HAL_GPBSetReadPtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);
			SDEC_HAL_GPBSetWritePtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);

			/* enable full intr */
			SDEC_HAL_GPBSetFullIntr(ui8ch, 	ui32GpbIdx);
		}
	}
	else
	{
		if(ui8ch < LX_SDEC_CH_C)
		{
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->gpb_bnd[ui32GpbIdx]= stGpbBnd;
			SDEC_HAL_GPBSetBnd(ui8ch, ui32GpbIdx, stGpbBnd.l_bnd, stGpbBnd.u_bnd);

			/* set GPB read & write pointer register */
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->gpb_w_ptr[ui32GpbIdx].gpb_w_ptr =	ui32Lower_Bound;
			//stpSdecParam->stSDEC_MPG_Reg[ui8ch]->gpb_r_ptr[ui32GpbIdx].gpb_r_ptr =	ui32Lower_Bound;
			SDEC_HAL_GPBSetReadPtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);
			SDEC_HAL_GPBSetWritePtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);

			/* enable full intr */
			SDEC_HAL_GPBSetFullIntr(ui8ch, 	ui32GpbIdx);
		}
	}

	/* initialize base & end pointer */
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32Baseptr 		= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32Endptr 		= ui32GpbBaseAddr + ui32GpbSize;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32Readptr 		= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32UsrReadptr	= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].stGpbBnd 			= stGpbBnd;

	/* set to channel C section buffer */
	if(ui8ch == LX_SDEC_CH_C)
	{
		SDEC_SWP_SetSectionBuffer(stpSdecParam);
	}

	SDEC_DTV_SOC_Message( SDEC_NORMAL, "l_bnd:[0x%08x]", ui32Lower_Bound);
	SDEC_DTV_SOC_Message( SDEC_NORMAL, "u_bnd:[0x%08x]", ui32Upper_Bound);
	SDEC_DTV_SOC_Message( SDEC_NORMAL, "gpb_w_ptr:[0x%08x]", ui32Lower_Bound);
	SDEC_DTV_SOC_Message( SDEC_NORMAL, "gpb_r_ptr:[0x%08x]", ui32Lower_Bound);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_GpbSet");

	eRet = OK;

exit:
	return (eRet);
}

DTV_STATUS_T SDEC_DummyGpbSet
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch,
	LX_SDEC_GPB_SIZE_T eGpbSize,
	UINT32 ui32GpbBaseAddr,
	UINT32 ui32GpbIdx)
{
	DTV_STATUS_T eRet = NOT_OK;

	GPB_BND stGpbBnd;
	UINT32 ui32Lower_Bound = 0x0, ui32Upper_Bound = 0x0, ui32GpbSize = 0x0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_DummyGpbSet");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	if( ui8ch >= pSdecConf->nChannel )
	{
		SDEC_DEBUG_Print("over channel range %d", ui8ch);
		goto exit;
	}

	if( ui32GpbIdx >= 64 )
	{
		SDEC_DEBUG_Print("over GPB range %d", ui32GpbIdx);
		goto exit;
	}

	ui32GpbSize = eGpbSize * 0x1000;

	ui32Lower_Bound = ui32GpbBaseAddr & 0x0FFFF000;
	ui32Upper_Bound = ui32Lower_Bound + ui32GpbSize;

	/* set GPB boudary register */
	stGpbBnd.l_bnd = (ui32Lower_Bound >> 12) & 0x0000FFFF;
	stGpbBnd.u_bnd = (ui32Upper_Bound >> 12) & 0x0000FFFF;

	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		if((ui8ch < LX_SDEC_CH_G) && ( (ui8ch != LX_SDEC_CH_C) || (ui8ch != LX_SDEC_CH_D) ) )
		{
			SDEC_HAL_GPBSetBnd(ui8ch, ui32GpbIdx, stGpbBnd.l_bnd, stGpbBnd.u_bnd);

			/* set GPB read & write pointer register */
			SDEC_HAL_GPBSetReadPtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);
			SDEC_HAL_GPBSetWritePtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);
		}
	}
	else
	{
		if(ui8ch < LX_SDEC_CH_C)
		{
			SDEC_HAL_GPBSetBnd(ui8ch, ui32GpbIdx, stGpbBnd.l_bnd, stGpbBnd.u_bnd);

			/* set GPB read & write pointer register */
			SDEC_HAL_GPBSetReadPtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);
			SDEC_HAL_GPBSetWritePtr(ui8ch, 	ui32GpbIdx, ui32Lower_Bound);
		}
	}

	/* initialize base & end pointer */
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32Baseptr 		= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32Endptr 		= ui32GpbBaseAddr + ui32GpbSize;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32Readptr 		= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].ui32UsrReadptr	= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8ch][ui32GpbIdx].stGpbBnd 			= stGpbBnd;

	SDEC_DTV_SOC_Message( SDEC_NORMAL, "l_bnd:[0x%08x]", ui32Lower_Bound);
	SDEC_DTV_SOC_Message( SDEC_NORMAL, "u_bnd:[0x%08x]", ui32Upper_Bound);
	SDEC_DTV_SOC_Message( SDEC_NORMAL, "gpb_w_ptr:[0x%08x]", ui32Lower_Bound);
	SDEC_DTV_SOC_Message( SDEC_NORMAL, "gpb_r_ptr:[0x%08x]", ui32Lower_Bound);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_DummyGpbSet");

	eRet = OK;

exit:
	return (eRet);
}

/**
********************************************************************************
* @brief
*   parameter init.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
UINT32 gh13_mcu_ipc;
UINT32 gPVRBusGateVirtAddr = 0;
DTV_STATUS_T SDEC_Intialize
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;
	DTV_STATUS_T eResult = NOT_OK;
	LX_SDEC_CFG_T* pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Intialize");

	/* initialize HAL api */
	SDEC_HAL_TOPInit();
	SDEC_HAL_IOInit();
	SDEC_HAL_MPGInit();
	SDEC_Gpb_Init();

	//mutex init
	eResult = SDEC_MutexInitialize(stpSdecParam);
	if(LX_IS_ERR(eResult))
	{
		SDEC_DEBUG_Print("SDEC_MutexInitialize failed:[%d]", eResult);
		goto exit;
	}

	//work queue init
	eResult = SDEC_WorkQueueInitialize(stpSdecParam);
	if(LX_IS_ERR(eResult))
	{
		SDEC_DEBUG_Print("SDEC_WorkQueueInitialize failed:[%d]", eResult);
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_NORMAL, "SDEC_WorkQueueInitialize success");

	//spin lock init
	eResult = SDEC_SpinLockInitialize(stpSdecParam);
	if(LX_IS_ERR(eResult))
	{
		SDEC_DEBUG_Print("SDEC_SpinLockInitialize failed:[%d]", eResult);
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_NORMAL, "SDEC_SpinLockInitialize success");

	//wait queue init
	eResult = SDEC_WaitQueueInitialize(stpSdecParam);

	if(LX_IS_ERR(eResult))
	{
		SDEC_DEBUG_Print("SDEC_WaitQueueInitialize failed:[%d]", eResult);
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_NORMAL, "SDEC_WaitQueueInitialize success");

	pSdecConf = SDEC_CFG_GetConfig();
	SDEC_HAL_SetGPBBaseAddr(0, (gMemCfgSDECGPB[pSdecConf->memCfg].gpb_memory_base & GPB_BASE_ADDR_MASK) >> 28);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_SetGPBBaseAddr(1, (gMemCfgSDECGPB[pSdecConf->memCfg].gpb_memory_base & GPB_BASE_ADDR_MASK) >> 28);
	}

	/* pwm reset */
	SDEC_PWM_Init(stpSdecParam);

	//intr src set
	SDEC_HAL_EnableInterrupt( 0, PCR, 				SDEC_HAL_ENABLE );
	SDEC_HAL_EnableInterrupt( 0, GPB_DATA_CHA_GPL, 	SDEC_HAL_ENABLE );
	SDEC_HAL_EnableInterrupt( 0, GPB_DATA_CHA_GPH, 	SDEC_HAL_ENABLE );
	SDEC_HAL_EnableInterrupt( 0, GPB_DATA_CHB_GPL, 	SDEC_HAL_ENABLE );
	SDEC_HAL_EnableInterrupt( 0, GPB_DATA_CHB_GPH, 	SDEC_HAL_ENABLE );

	/* 2012.02.06 gaius.lee
	 * Bug exist in L9 HW.
	 * While CPU access Read/Write/Bound Register, SDEC HW accesses write register, write pointer goes to read/write/bound regitser which CPU access.
	 * So, remove access to read register. That's why we disable full interrupt */
	if(pSdecConf->staticGPB == 0)
	{
		SDEC_HAL_EnableInterrupt( 0, GPB_FULL_CHA_GPL, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 0, GPB_FULL_CHA_GPH, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 0, GPB_FULL_CHB_GPL, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 0, GPB_FULL_CHB_GPH, 	SDEC_HAL_ENABLE );
	}
#if 1 // jinhwan.bae for MCU test
	SDEC_HAL_EnableInterrupt( 0, TP_INFO_CHA, 		SDEC_HAL_ENABLE );
	SDEC_HAL_EnableInterrupt( 0, TP_INFO_CHB, 		SDEC_HAL_ENABLE );
#else
	SDEC_HAL_EnableInterrupt( 0, TP_INFO_CHA, 		SDEC_HAL_DISABLE );
	SDEC_HAL_EnableInterrupt( 0, TP_INFO_CHB, 		SDEC_HAL_DISABLE );
#endif
	SDEC_HAL_EnableInterrupt( 0, ERR_RPT, 			SDEC_HAL_DISABLE );
	SDEC_HAL_EnableInterrupt( 0, TB_DCOUNT, 		SDEC_HAL_ENABLE );

#if 1 // jinhwan.bae for BDRC Serial Out Buf Level Interrupt
	SDEC_HAL_EnableInterrupt( 0, BDRC_3, 			SDEC_HAL_ENABLE );
#endif

	/* jinhwan.bae 20140320 tvct problem */
	SDEC_HAL_EnableInterrupt( 0, SEC_ERR_CHA,		SDEC_HAL_ENABLE );
	SDEC_HAL_EnableInterrupt( 0, SEC_ERR_CHB,		SDEC_HAL_ENABLE );

	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_EnableInterrupt( 1, PCR, 				SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, GPB_DATA_CHA_GPL, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, GPB_DATA_CHA_GPH, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, GPB_DATA_CHB_GPL, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, GPB_DATA_CHB_GPH, 	SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, TP_INFO_CHA, 		SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, TP_INFO_CHB, 		SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, ERR_RPT, 			SDEC_HAL_DISABLE );
		SDEC_HAL_EnableInterrupt( 1, TB_DCOUNT, 		SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, BDRC_3, 			SDEC_HAL_ENABLE );

		/* jinhwan.bae 20140320 tvct problem */
		SDEC_HAL_EnableInterrupt( 1, SEC_ERR_CHA,		SDEC_HAL_ENABLE );
		SDEC_HAL_EnableInterrupt( 1, SEC_ERR_CHB,		SDEC_HAL_ENABLE );
	}

	SDEC_HAL_SDMWCLastBValidMode(0, 0);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_SDMWCLastBValidMode(1, 0);
	}

	/* Auto Increment Disable - jinhwan.bae from M14A0 2013. 06. 17 */
	SDEC_HAL_EnableAutoIncr(0, SDEC_HAL_DISABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_EnableAutoIncr(1, SDEC_HAL_DISABLE);
	}

	SDEC_HAL_EnableVideoReady(0, 0, SDEC_HAL_DISABLE);
	SDEC_HAL_EnableVideoReady(0, 1, SDEC_HAL_DISABLE);
	SDEC_HAL_EnableAudioReady(0, 0, SDEC_HAL_DISABLE);
	SDEC_HAL_EnableAudioReady(0, 1, SDEC_HAL_DISABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_EnableVideoReady(1, 0, SDEC_HAL_DISABLE);
		SDEC_HAL_EnableVideoReady(1, 1, SDEC_HAL_DISABLE);
		SDEC_HAL_EnableAudioReady(1, 0, SDEC_HAL_DISABLE);
		SDEC_HAL_EnableAudioReady(1, 1, SDEC_HAL_DISABLE);
	}

	/* Set gpb full level as 256 byte */
	SDEC_HAL_ExtConfGPBFullLevel(0, 1);
	SDEC_HAL_ExtConfGPBFullLevel(1, 1);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_ExtConfGPBFullLevel(4, 1);
		SDEC_HAL_ExtConfGPBFullLevel(5, 1);
	}

	/* M14_TBD, H14_TBD, pes ready is not working at M14B0, H14 */
	//SDEC_HAL_ConfSetPESReadyCheck(0, 0, SDEC_HAL_ENABLE);
	//SDEC_HAL_ConfSetPESReadyCheck(0, 1, SDEC_HAL_ENABLE);
	SDEC_HAL_ConfSetPESReadyCheck(1, 0, SDEC_HAL_ENABLE);
	SDEC_HAL_ConfSetPESReadyCheck(1, 1, SDEC_HAL_ENABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		//SDEC_HAL_ConfSetPESReadyCheck(4, 0, SDEC_HAL_ENABLE);
		//SDEC_HAL_ConfSetPESReadyCheck(4, 1, SDEC_HAL_ENABLE);
		SDEC_HAL_ConfSetPESReadyCheck(5, 0, SDEC_HAL_ENABLE);
		SDEC_HAL_ConfSetPESReadyCheck(5, 1, SDEC_HAL_ENABLE);
	}

	/* SDEC SW Parser Init */
	SDEC_SWP_Init(stpSdecParam);

	/* LIVE_HEVC */
	SDEC_RFOUT_Init(stpSdecParam);

	/* jinhwan.bae for mcu descrambler mode , H13 Only */
	/* ioremap ipc base */
	if( lx_chip_rev() < LX_CHIP_REV(M14, A0))
	{
		gh13_mcu_ipc = (UINT32)ioremap(0xC0004800, 0x100);
	}

	/* Set Bus Gate On in H14 only */
	// PVR Download is not working at H14 , bus gate blocked default! 2013. 07. 08
	// C0006060 : SDEC Core 0, C00006068 : PVR, C0006070 : SDEC Core 1
	// CHECK H15 more!!!!
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
#if 0
		virtAddr	= (UINT32) ioremap(0xC0006060, 0x20);
		*(UINT32*)(virtAddr+0x08) = (UINT32)0x1;		
#else
		gPVRBusGateVirtAddr = (UINT32) ioremap(0xC0006060, 0x20);
		*(UINT32*)(gPVRBusGateVirtAddr+0x08) = (UINT32)0x1;		
#endif
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Intialize");

	eRet = OK;

exit:
	return (eRet);
}

/**
********************************************************************************
* @brief
*   sdec param init.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_ParamInit
	(S_SDEC_PARAM_T *stpSdecParam)
{
	DTV_STATUS_T eRet = NOT_OK;

	UINT8 ui8Count = 0x0, i = 0;
	UINT8 ui8Countch = 0x0;

	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;
	UINT8			ui8ChNum		= 0;
	UINT8			ui8PidfNum		= 0;
	UINT8			ui8SecfNum		= 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_ParamInit");

#if 0
	//work queue init
	eResult = SDEC_WorkQueueInitialize(stpSdecParam);
	if(LX_IS_ERR(eResult))
	{
		SDEC_DEBUG_Print("SDEC_WorkQueueInitialize failed:[%d]", eResult);
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_NORMAL, "SDEC_WorkQueueInitialize success");
#endif

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	/* get informations from channel info structure */
	ui8ChNum	= pSdecConf->nChannel;

	for(ui8Countch = 0; ui8Countch < ui8ChNum; ui8Countch++)
	{
		/* get pid filter number from channel info structure */
		ui8PidfNum	= pSdecConf->chInfo[ui8Countch].num_pidf;

		/* pid filter map init */
		for(ui8Count = 0; ui8Count < ui8PidfNum; ui8Count++)
		{
			stpSdecParam->stPIDMap[ui8Countch][ui8Count].used = 0x0;
			stpSdecParam->stPIDMap[ui8Countch][ui8Count].flag = FALSE;
			stpSdecParam->stPIDMap[ui8Countch][ui8Count].mode = 0x0;
			stpSdecParam->stPIDMap[ui8Countch][ui8Count].stStatusInfo.w = 0x0;
		}

		/* get pid filter number from channel info structure */
		ui8SecfNum	= pSdecConf->chInfo[ui8Countch].num_secf;
		/* section filter map init */
		for(ui8Count = 0; ui8Count < ui8SecfNum; ui8Count++)
		{
			stpSdecParam->stSecMap[ui8Countch][ui8Count].used = 0x0;
			stpSdecParam->stSecMap[ui8Countch][ui8Count].flag = FALSE;
			stpSdecParam->stSecMap[ui8Countch][ui8Count].mode = 0x0;
			stpSdecParam->stSecMap[ui8Countch][ui8Count].stStatusInfo.w = 0x0;

			stpSdecParam->stSdecMeminfo[ui8Countch][ui8Count].ui32Baseptr = 0x0;
			stpSdecParam->stSdecMeminfo[ui8Countch][ui8Count].ui32Endptr = 0x0;
			stpSdecParam->stSdecMeminfo[ui8Countch][ui8Count].ui32Readptr = 0x0;
			stpSdecParam->stSdecMeminfo[ui8Countch][ui8Count].ui32UsrReadptr = 0x0;
			stpSdecParam->stSdecMeminfo[ui8Countch][ui8Count].ui8PidFltIdx = 0x0;
		}
	}

	//gpbdata structure init
	memset(stpSdecParam->stGPBInfo, 0x0, sizeof(LX_SDEC_NOTIFY_PARAM_T) * LX_SDEC_MAX_GPB_DATA);

	//valuable init
	stpSdecParam->ui8CurrentCh = 0x0;
	stpSdecParam->ui8CurrentMode = 0x0;
	stpSdecParam->bPcrRecoveryFlag[0] = 0x0;
	stpSdecParam->bPcrRecoveryFlag[1] = 0x0;
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		for(i=0;i<SDEC_IO_CH_NUM;i++)
		{
			stpSdecParam->bPcrRecoveryFlag[i] = 0x0;
		}
	}

	stpSdecParam->ui8GpbInfoWIdx = 0x0;
	stpSdecParam->ui8GpbInfoRIdx = 0x0;

	/* set reset mode */
	stpSdecParam->ui8CDICResetMode		= E_SDEC_RESET_MODE_ENABLE;
	stpSdecParam->ui8SDMWCResetMode	= E_SDEC_RESET_MODE_ONETIME;

	/* set reset number as 0 */
	stpSdecParam->ui8CDICResetNum		= 0;
	stpSdecParam->ui8SDMWCResetNum		= 0;

#if 0
	stext_conf = stpSdecParam->stSDEC_MPG_Reg[0]->ext_conf;
	stext_conf.dpkt_vid = 0x0;
	stext_conf.dpkt_dcont = 0x1;
	stext_conf.seci_cce = 0x0;
	stext_conf.seci_dcont = 0x0;
	stpSdecParam->stSDEC_MPG_Reg[0]->ext_conf = stext_conf;
#endif
	SDEC_HAL_ExtConfSECIDcont(0, SDEC_HAL_DISABLE);
	SDEC_HAL_ExtConfSECIDcont(1, SDEC_HAL_DISABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_ExtConfSECIDcont(4, SDEC_HAL_DISABLE);
		SDEC_HAL_ExtConfSECIDcont(5, SDEC_HAL_DISABLE);
	}

	SDEC_HAL_ExtConfSECICCError(0, SDEC_HAL_DISABLE);
	SDEC_HAL_ExtConfSECICCError(1, SDEC_HAL_DISABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_ExtConfSECICCError(4, SDEC_HAL_DISABLE);
		SDEC_HAL_ExtConfSECICCError(5, SDEC_HAL_DISABLE);
	}

	SDEC_HAL_ExtConfVideoDupPacket(0, SDEC_HAL_DISABLE);
	SDEC_HAL_ExtConfVideoDupPacket(1, SDEC_HAL_DISABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_ExtConfVideoDupPacket(4, SDEC_HAL_DISABLE);
		SDEC_HAL_ExtConfVideoDupPacket(5, SDEC_HAL_DISABLE);
	}

#if 0 
	/* 2013.02.07 Set from enable to disable. All Decoders should process Discontinuity Counter by theirselves
       SarnOff ATSC_02.ts have discontinuity indicator in the first TS packet including PES header of I frame,
       1st part of GOP is decoded unexpectedly (color tone is yellow , expected blue)
       it's requested by VDEC */
	SDEC_HAL_ExtConfDcontDupPacket(0, SDEC_HAL_ENABLE);
	SDEC_HAL_ExtConfDcontDupPacket(1, SDEC_HAL_ENABLE);
#else
	SDEC_HAL_ExtConfDcontDupPacket(0, SDEC_HAL_DISABLE);
	SDEC_HAL_ExtConfDcontDupPacket(1, SDEC_HAL_DISABLE);
	if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		SDEC_HAL_ExtConfDcontDupPacket(4, SDEC_HAL_DISABLE);
		SDEC_HAL_ExtConfDcontDupPacket(5, SDEC_HAL_DISABLE);
	}
#endif

	/* for JP MCU Descrambler Mode, initial value = 0, for all market , and JP set this to 1 in SDEC_IO_Init */
	stpSdecParam->ui8McuDescramblerCtrlMode	= 0;

	/* for using original external demod clock in serial input, ex> Columbia, set 0 for all market, and Columbia set this to 1 in SDEC_IO_Init */
	stpSdecParam->ui8UseOrgExtDemodClk = 0;

	/* for UHD bypass channel */
	stpSdecParam->ui8RFBypassChannel = 0;

	/* for UHD SDT channel in H13 */
	stpSdecParam->ui8SDTChannel = 0;
	
	/* for ATSC TVCT mode */
	stpSdecParam->ui8ClearTVCTGathering = 0;
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_ParamInit");

	eRet = OK;

	return (eRet);
}


/**
********************************************************************************
* @brief
*   sdec notify
* @remarks
*  wake up interrupt for signaling to user
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/
void SDEC_Notify
	(struct work_struct *work)
{
	S_SDEC_PARAM_T *stpSdecParam;

	LX_SDEC_CHECK_CODE( work == NULL, return, "Invalid parameters" );

	stpSdecParam =  container_of (work, S_SDEC_PARAM_T, Notify);

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_Notify");

	stpSdecParam->wq_condition = 1;
	wake_up_interruptible(&stpSdecParam->wq);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_Notify");
}

/**
********************************************************************************
* @brief
*   sdec notify
* @remarks
*  wake up interrupt for signaling to user
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/
BOOLEAN SDEC_SetNotifyParam(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_NOTIFY_PARAM_T notiParam)
{
	BOOLEAN isFound = FALSE;
	unsigned long flags = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return isFound, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_SetNotifyParam");

	spin_lock_irqsave(&stpSdecParam->stSdecNotiSpinlock, flags);

	/*
	 * overflow check
	 *
	 * case 1 : one more packet, then overflow will happen.
	 *	  ui8GpbInfoWIdx
	 *			V
	 * |---------------------------------------------------------|
	 *			 ^
	 *	  ui8GpbInfoRIdx
	 *
	 * case 2 : read pointer is 0, write pointer is ( LX_SDEC_MAX_GPB_DATA - 1 ).
	 *		  if there is one more packet, write pointer is wrapped around and same with read pointer.
	 *												   ui8GpbInfoWIdx
	 *															 V
	 * |---------------------------------------------------------|
	 * ^
	 * ui8GpbInfoRIdx
	 */

	/* stored buf info	*/
	stpSdecParam->stGPBInfo[stpSdecParam->ui8GpbInfoWIdx] = notiParam;

	/* increase write pointer */
	stpSdecParam->ui8GpbInfoWIdx = (stpSdecParam->ui8GpbInfoWIdx + 1) % LX_SDEC_MAX_GPB_DATA;

	if( ( stpSdecParam->ui8GpbInfoRIdx - stpSdecParam->ui8GpbInfoWIdx) != 1 &&							// case 1.
		( stpSdecParam->ui8GpbInfoWIdx - stpSdecParam->ui8GpbInfoRIdx) != ( LX_SDEC_MAX_GPB_DATA - 1 )	// case 2.
		)
	{
		/* buffer is not full */

		/* set found flag */
		isFound = TRUE;
	}
	else
	{
		/* buffer is full, reset to 0 */
		stpSdecParam->ui8GpbInfoRIdx = 0;
		stpSdecParam->ui8GpbInfoWIdx = 0;
		SDEC_DEBUG_Print( "Krdv->User Msg Queue is full");
	}

	spin_unlock_irqrestore(&stpSdecParam->stSdecNotiSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_SetNotifyParam");

	return isFound;
}

/**
********************************************************************************
* @brief
*   Check overflowed msg in msg queue from kdrv -> user level
* @remarks
*  check if this section or pes data is overflowed by overflow, and if, remove.
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/
#if ( LX_SDEC_USE_GPB_OW == 1 )
void SDEC_CheckNotifyOvf(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_NOTIFY_PARAM_T *pNotiParam)
{
	unsigned long 	flags = 0;
	UINT8			ui8GpbInfoRIdx = 0, ui8GpbInfoWIdx = 0, ui8Ch = 0, ui8Idx = 0;
	UINT8			ui8DeleteMargin = 0xff;
	UINT32			rdPtr = 0, wrPtr = 0, bfStr = 0, bfEnd = 0;
	UINT32			urPtr = 0;				/* user read pointer */
	UINT32			cwPtr = 0;				/* current write pointer */
	SINT32			si32DataSize = 0;		/* size to delete */
	SINT32			si32SectionSize = 0;	/* size of a section */

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return , "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_CheckNotifyOvf");

	spin_lock_irqsave(&stpSdecParam->stSdecNotiSpinlock, flags);

	ui8Ch			= pNotiParam->channel;
	ui8Idx			= pNotiParam->index;

	ui8GpbInfoRIdx 	= stpSdecParam->ui8GpbInfoRIdx;
	ui8GpbInfoWIdx 	= stpSdecParam->ui8GpbInfoWIdx;

	bfStr			= stpSdecParam->stSdecMeminfo[ui8Ch][ui8Idx].ui32Baseptr;
	bfEnd			= stpSdecParam->stSdecMeminfo[ui8Ch][ui8Idx].ui32Endptr;

	SDEC_DTV_SOC_Message(SDEC_PIDSEC, "SDEC_CheckNotifyOvf : CH%d[%d] e[%08x]\n",
				pNotiParam->channel,
				ui8Idx,
				pNotiParam->writePtr);

	/* while loop */
	while( ui8GpbInfoRIdx != ui8GpbInfoWIdx )
	{
		if( stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].channel == ui8Ch &&
			stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].index == ui8Idx &&
			stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].status & LX_SDEC_FLTSTATE_DATAREADY)
		{
			if(ui8Ch == LX_SDEC_CH_C)
			{
				stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].status = LX_SDEC_FLTSTATE_DELETED;

				/* increase user read pointer to section write pointer */
				stpSdecParam->stSdecMeminfo[ui8Ch][ui8Idx].ui32UsrReadptr = wrPtr;

				/* increase read pointer */
				ui8GpbInfoRIdx = (ui8GpbInfoRIdx + 1) % LX_SDEC_MAX_GPB_DATA;
				continue;
			}

			/* get section read pointer */
			rdPtr = stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].readPtr;
			/* get section write pointer */
			wrPtr = stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].writePtr;
			/* get user read pointer. this means real read pointer of user */
			//urPtr = SDEC_HAL_GPBGetReadPtr(ui8Ch, ui8Idx) 	| (bfStr & GPB_BASE_ADDR_MASK);
			urPtr = stpSdecParam->stSdecMeminfo[ui8Ch][ui8Idx].ui32UsrReadptr | (bfStr & GPB_BASE_ADDR_MASK);
			/* get current real-time write pointer */
			cwPtr = SDEC_HAL_GPBGetWritePtr(ui8Ch, ui8Idx) 	| (bfStr & GPB_BASE_ADDR_MASK);

			/* 1. calculate size to delete */
			if( ui8DeleteMargin == 0xff )
			{
				stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].status = LX_SDEC_FLTSTATE_DELETED;

				if( rdPtr <= cwPtr && cwPtr < wrPtr )
				{

					/* case 1. normal 1 ( probably in SDEC_Section_Proc(). But occasually in SDEC_Full_Proc() )
					 *
					 *             rdPtr      wrPtr
					 *              V          V
					 * |---------------------------------------------------------|
					 *                  ^
					 *	              cwPtr
					 */
					/* 정상적으로 current write pointer가 중간에 있다. */
					LX_MEMSIZE(si32DataSize, 		rdPtr, cwPtr, bfStr, bfEnd);
					LX_MEMSIZE(si32SectionSize, 	cwPtr, wrPtr, bfStr, bfEnd);
					ui8DeleteMargin = 2;
				}
				else if( rdPtr < cwPtr && wrPtr < cwPtr )
				{

					/* case 2. normal 2 ( probably in SDEC_Full_Proc() )
					 *
					 *             rdPtr      wrPtr
					 *              V          V
					 * |---------------------------------------------------------|
					 *                           ^
					 *	                       cwPtr
					 */
					/* current write pointer가 이 Table을 지나 있다. ( 많이 빠르므로 3개 정도 앞서서 삭제 )*/
					LX_MEMSIZE(si32DataSize, 		rdPtr, cwPtr, bfStr, bfEnd);
					LX_MEMSIZE(si32SectionSize, 	rdPtr, wrPtr, bfStr, bfEnd);
					ui8DeleteMargin = 3;
				}
				else
				{
					/* case 3. sometimes ( probably in SDEC_Full_Proc() )
					 *
					 *             rdPtr      wrPtr
					 *              V          V
					 * |---------------------------------------------------------|
					 *             ^
					 *	         cwPtr
					 */
					/* GPB_F_INTR 에서 가끔 이렇게 먼저 뜰 때가 있다. 이럴경우 그냥 앞에 있는 애들만 지워도 됨. */
					LX_MEMSIZE(si32DataSize, 		rdPtr, wrPtr, bfStr, bfEnd);
					LX_MEMSIZE(si32SectionSize, 	rdPtr, wrPtr, bfStr, bfEnd);
					ui8DeleteMargin = 1;
				}

				SDEC_DTV_SOC_Message(SDEC_PIDSEC, "DS[%d] SS[%d] rp[%08x] wp[%08x] cw[%08x] ur[%08x] m[%d]\n",
					si32DataSize,
					si32SectionSize,
					rdPtr,
					wrPtr,
					pNotiParam->writePtr,
					urPtr,
					ui8DeleteMargin
					);

				/* decrease data size as section size */
				si32DataSize -= si32SectionSize;
				/* increase user read pointer to section write pointer */
				//SDEC_HAL_GPBSetReadPtr(ui8Ch, ui8Idx, wrPtr);
				stpSdecParam->stSdecMeminfo[ui8Ch][ui8Idx].ui32UsrReadptr = wrPtr;
			}
			/* 2. Delete while size is not under zero, or margin is not zero. */
			else if(si32DataSize > 0 || ui8DeleteMargin)
			{
				SDEC_DTV_SOC_Message(SDEC_PIDSEC,"[%d] [%d]\n", si32DataSize, ui8DeleteMargin);

				stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].status = LX_SDEC_FLTSTATE_DELETED;

				/* increase user read pointer to section write pointer */
				//SDEC_HAL_GPBSetReadPtr(ui8Ch, ui8Idx, wrPtr);
				stpSdecParam->stSdecMeminfo[ui8Ch][ui8Idx].ui32UsrReadptr = wrPtr;

				/* calculate section size */
				LX_MEMSIZE(si32SectionSize, rdPtr, wrPtr, bfStr, bfEnd);

				/* decrease data size as section size */
				si32DataSize -= si32SectionSize;

				/* if data size is under zero, delete more packet as margin. Sometimes data speed is too fast. */
				if(si32DataSize < 0) ui8DeleteMargin--;
			}
			else
			{
				break;
			}
		}

		/* increase read pointer */
		ui8GpbInfoRIdx = (ui8GpbInfoRIdx + 1) % LX_SDEC_MAX_GPB_DATA;
	}

	spin_unlock_irqrestore(&stpSdecParam->stSdecNotiSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_CheckNotifyOvf");
}
#endif

/**
********************************************************************************
* @brief
*   sdec notify
* @remarks
*  wake up interrupt for signaling to user
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/
void SDEC_DeleteInNotify(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch, UINT8 ui32SecFltId)
{
	UINT8	ui8GpbInfoRIdx = 0, ui8GpbInfoWIdx;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return , "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_CheckNotify");

	spin_lock(&stpSdecParam->stSdecNotiSpinlock);

	ui8GpbInfoRIdx 	= stpSdecParam->ui8GpbInfoRIdx;
	ui8GpbInfoWIdx 	= stpSdecParam->ui8GpbInfoWIdx;

	//printk("%s : CH%d[%d]\n", __FUNCTION__, ui8Ch, ui32SecFltId );

	/* while loop */
	while( ui8GpbInfoRIdx != ui8GpbInfoWIdx )
	{
		if( stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].channel == ui8Ch &&
			stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].index == ui32SecFltId &&
			stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].status & LX_SDEC_FLTSTATE_DATAREADY)
		{
			stpSdecParam->stGPBInfo[ui8GpbInfoRIdx].status = LX_SDEC_FLTSTATE_DELETED;

			printk("CH%d[%d] DELETE R[%d] W[%d]\n", ui8Ch, ui32SecFltId, ui8GpbInfoRIdx, ui8GpbInfoWIdx );
		}

		/* increase read pointer */
		ui8GpbInfoRIdx = (ui8GpbInfoRIdx + 1) % LX_SDEC_MAX_GPB_DATA;
	}

	//printk("%s : END\n", __FUNCTION__ );

	spin_unlock(&stpSdecParam->stSdecNotiSpinlock);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_CheckNotify");
}

/**
********************************************************************************
* @brief
*   sdec TPI register set
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8Ch :channel
*   stpSdecParam :SDEC parameter
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

DTV_STATUS_T SDEC_TPI_Set
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_TPI_Set");

	//TPI_ICONF Reg set
	// gaius.lee 2010.10.26
	// Setting interrupt as payload unit start indicator bit is 1.
	// This means we want to check every PES packet for scramble status checking. This is for speed.
#if 1 //	jinhwan.bae mcu test
	SDEC_HAL_TPISetIntrPayloadUnitStartIndicator(ui8Ch, SDEC_HAL_ENABLE);
	SDEC_HAL_TPISetIntrAutoScCheck(ui8Ch, SDEC_HAL_DISABLE);
#else
//	jinhwan.bae mcu test SDEC_HAL_TPISetIntrPayloadUnitStartIndicator(ui8Ch, SDEC_HAL_ENABLE);
//	SDEC_HAL_TPISetIntrAutoScCheck(ui8Ch, SDEC_HAL_DISABLE);
//	jinhwan.bae mcu test SDEC_HAL_TPISetIntrAutoScCheck(ui8Ch, SDEC_HAL_ENABLE);
#endif

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_TPI_Set");

	eRet = OK;

	return (eRet);
}


/**
********************************************************************************
* @brief
*   sdec TPI_IEN in PIDF_DATA register set
* @remarks
*   DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   ui8Ch :channel
*   ui8PIDIdx : PID filter index
*   val : PIDF_DATA value
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

int SDEC_SetPidfData
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch,
	UINT8 ui8PIDIdx,
	UINT32 val
	)
{
	int ret = RET_ERROR;
	unsigned long 		flags = 0;

	spin_lock_irqsave(&stpSdecParam->stSdecPidfSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_NORMAL,"Set PIDF_DATA ch[%d]idx[%d][0x%x]", ui8Ch, ui8PIDIdx, val);
	ret = SDEC_HAL_PIDFSetPidfData(ui8Ch, ui8PIDIdx, val);

	spin_unlock_irqrestore(&stpSdecParam->stSdecPidfSpinlock, flags);

	return ret;

}


/**
********************************************************************************
* @brief
*   sdec TPI_IEN in PIDF_DATA register get
* @remarks
*   DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   ui8Ch :channel
*   ui8PIDIdx : PID filter index
*   work_struct
* @return
*  PIDF_DATA value
********************************************************************************
*/

UINT32 SDEC_GetPidfData
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch,
	UINT8 ui8PIDIdx
	)
{
	UINT32 ret_value = 0;
	unsigned long 		flags = 0;

	spin_lock_irqsave(&stpSdecParam->stSdecPidfSpinlock, flags);

	ret_value = SDEC_HAL_PIDFGetPidfData(ui8Ch, ui8PIDIdx);
	SDEC_DTV_SOC_Message( SDEC_NORMAL,"Get PIDF_DATA ch[%d]idx[%d][0x%x]", ui8Ch, ui8PIDIdx, ret_value);

	spin_unlock_irqrestore(&stpSdecParam->stSdecPidfSpinlock, flags);

	return ret_value;
}


/**
********************************************************************************
* @brief
*   sdec PIDF enable
* @remarks
*   DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   ui8Ch :channel
*   ui8PIDIdx : PID filter index
*   en : enable/disable
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

int SDEC_SetPidf_Enable
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch,
	UINT8 ui8PIDIdx,
	UINT8 en
	)
{
	int ret = RET_ERROR;
	unsigned long 		flags = 0;

	spin_lock_irqsave(&stpSdecParam->stSdecPidfSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_NORMAL,"PIDF_EN ch[%d]idx[%d][%d]", ui8Ch, ui8PIDIdx, en);
	ret = SDEC_HAL_PIDFEnable(ui8Ch, ui8PIDIdx, en);

	spin_unlock_irqrestore(&stpSdecParam->stSdecPidfSpinlock, flags);

	return ret;
}


/**
********************************************************************************
* @brief
*   sdec TPI_IEN in PIDF_DATA register set
* @remarks
*   DETAIL INFORMATION
*   Added by jinhwan.bae, 2012.04.26, No Audio at long time aging test with US/Cable
*   US/Cable Check Scramble Status periodically, Scramble Check is used pusi interrupt -> check tsc
*   Process Context : start scramble check -> write TPI_IEN in Audio PIDF_DATA
*   Interrupt Context : Video pusi interrupt -> check tsc -> disable scramble check (since H/W Bug) -> write TPI_IEN in Video PIDF_DATA
*   If interrupt context write TPI_IEN during process context stopped just after writing PIDF_INDEX,
*   PIDF_DATA in process context after interrupt context operation is changed to Video PIDF_DATA,
*   so Video PIDF_DATA | TPI_IEN will be writen to Audio PIDF_DATA
*   all added pidf spin lock codes are related to this
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   ui8Ch :channel
*   ui8PIDIdx : PID filter index
*   en : Enable or Disable
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

int SDEC_SetPidf_TPI_IEN_Enable
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch,
	UINT8 ui8PIDIdx,
	UINT8 en
	)
{
	int ret = RET_ERROR;
	unsigned long 		flags = 0;

	spin_lock_irqsave(&stpSdecParam->stSdecPidfSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_NORMAL,"TPI_IEN idx[%d][%d][%d]", ui8Ch, ui8PIDIdx, en);
	ret = SDEC_HAL_PIDFScrambleCheck(ui8Ch, ui8PIDIdx, en);

	spin_unlock_irqrestore(&stpSdecParam->stSdecPidfSpinlock, flags);

	return ret;
}


/**
********************************************************************************
* @brief
*   sdec PIDF Descrambler enable
* @remarks
*   DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   ui8Ch :channel
*   ui8PIDIdx : PID filter index
*   en : enable/disable
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

int SDEC_SetPidf_Disc_Enable
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch,
	UINT8 ui8PIDIdx,
	UINT8 en
	)
{
	int ret = RET_ERROR;
	unsigned long 		flags = 0;

	spin_lock_irqsave(&stpSdecParam->stSdecPidfSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_NORMAL,"PIDF_DESC_EN ch[%d]idx[%d][%d]", ui8Ch, ui8PIDIdx, en);
	ret = SDEC_HAL_PIDFDescEnable(ui8Ch, ui8PIDIdx, en);

	spin_unlock_irqrestore(&stpSdecParam->stSdecPidfSpinlock, flags);

	return ret;
}


/**
********************************************************************************
* @brief
*   sdec PIDF Download enable
* @remarks
*   DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   ui8Ch :channel
*   ui8PIDIdx : PID filter index
*   en : enable/disable
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

int SDEC_SetPidf_DN_Enable
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch,
	UINT8 ui8PIDIdx,
	UINT8 en
	)
{
	int ret = RET_ERROR;
	unsigned long 		flags = 0;

	spin_lock_irqsave(&stpSdecParam->stSdecPidfSpinlock, flags);

	SDEC_DTV_SOC_Message( SDEC_NORMAL,"PIDF_DN_EN ch[%d]idx[%d][%d]", ui8Ch, ui8PIDIdx, en);
	ret = SDEC_HAL_PIDFDownloadEnable(ui8Ch, ui8PIDIdx, en);

	spin_unlock_irqrestore(&stpSdecParam->stSdecPidfSpinlock, flags);

	return ret;
}



/**
********************************************************************************
* @brief
*   sdec Err interrrupt register set
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8Ch :channel
*   stpSdecParam :SDEC parameter
*   work_struct
* @return
*  DTV_STATUS_T
********************************************************************************
*/

DTV_STATUS_T SDEC_ERR_Intr_Set
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8Ch)
{
	DTV_STATUS_T eRet = NOT_OK;

	//ERR_INTR_EN stErrIntrEn;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_ERR_Intr_Set");

	if(ui8Ch < SDEC_CORE_CH_NUM) 
	{	/* Core 0  */
		SDEC_HAL_EnableErrorInterrupt( 0, MPG_SD, 		0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, MPG_CC, 		0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, MPG_DUP, 		0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, MPG_TS, 		0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, MPG_PD, 		0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, STCC_DCONT, 	0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, CDIF_RPAGE, 	0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, CDIF_WPAGE, 	0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, CDIF_OVFLOW, 	0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, SB_DROPPED, 	0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, SYNC_LOST, 	0x3);
		SDEC_HAL_EnableErrorInterrupt( 0, TEST_DCONT, 	0x1);
	}
	else
	{	/* Core 1  */
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			SDEC_HAL_EnableErrorInterrupt( 1, MPG_SD, 		0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, MPG_CC, 		0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, MPG_DUP, 		0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, MPG_TS, 		0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, MPG_PD, 		0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, STCC_DCONT, 	0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, CDIF_RPAGE, 	0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, CDIF_WPAGE, 	0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, CDIF_OVFLOW, 	0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, SB_DROPPED, 	0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, SYNC_LOST, 	0x3);
			SDEC_HAL_EnableErrorInterrupt( 1, TEST_DCONT, 	0x1);
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_ERR_Intr_Set");

	eRet = OK;

	return (eRet);
}

/**
********************************************************************************
* @brief
*   get Chip Configuration
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetChipCfg
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CHIP_CFG_T *stpLXSdecGetChipCfg)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T* pSdecConf = NULL;
	
	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetChipCfg == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetChipCfg");

	/* get config */
	pSdecConf = SDEC_CFG_GetConfig();

	stpLXSdecGetChipCfg->chip_rev 		= pSdecConf->chip_rev;
	stpLXSdecGetChipCfg->nChannel 		= pSdecConf->nChannel;
	stpLXSdecGetChipCfg->nVdecOutPort 	= pSdecConf->nVdecOutPort;
	stpLXSdecGetChipCfg->noPesBug 		= pSdecConf->noPesBug;
	stpLXSdecGetChipCfg->staticGPB 		= pSdecConf->staticGPB;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetChipCfg");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   get the current STC ASG value.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetSTCCStatus
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_STCC_STATUS_T *stpLXSdecGetSTCCStatus)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_STCC_STATUS_T *p = NULL;
	UINT8 	ui8Ch = 0x0;
	UINT32 	uiSTCCRegValue_32b = 0;
	UINT8	core = 0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetSTCCStatus == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetSTCCStatus");

	ui8Ch = stpLXSdecGetSTCCStatus->eCh;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	uiSTCCRegValue_32b = SDEC_HAL_STCCGetReg(core, ui8Ch);

	p = &(stpLXSdecGetSTCCStatus->stcc_status);
	p->bEn 			= ( (uiSTCCRegValue_32b & 0x80000000) >> 31 );
	p->ui8Chan 		= ( (uiSTCCRegValue_32b & 0x20000000) >> 29 );
	p->ui16Pcr_Pid 	= ( (uiSTCCRegValue_32b & 0x1fff0000) >> 16 );
	p->bRd_Mode		= ( (uiSTCCRegValue_32b & 0x00000100) >> 8 );
	p->bRate_Ctrl 	= ( (uiSTCCRegValue_32b & 0x00000010) >> 4 );
	p->bCopy_En 	= ( (uiSTCCRegValue_32b & 0x00000002) >> 1 );
	p->bLatch_En 	= ( (uiSTCCRegValue_32b & 0x00000001) );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetSTCCStatus");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   get the current STC ASG value.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetSTCCASGStatus
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_STCCASG_T *stpLXSdecGetSTCCASG)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT32 ui32ASGRegValue = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetSTCCASG == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetSTCCASGStatus");

	ui32ASGRegValue = SDEC_HAL_STCCGetASG(_SDEC_CORE_0); /* M14_TBD , H14_TBD */

	stpLXSdecGetSTCCASG->ui8Main = (ui32ASGRegValue & 0x01);
	stpLXSdecGetSTCCASG->ui8Aud1 = ((ui32ASGRegValue & 0x80000) >> 19);
	stpLXSdecGetSTCCASG->ui8Aud0 = ((ui32ASGRegValue & 0x40000) >> 18);
	stpLXSdecGetSTCCASG->ui8Vid1 = ((ui32ASGRegValue & 0x20000) >> 17);
	stpLXSdecGetSTCCASG->ui8Vid0 = ((ui32ASGRegValue & 0x10000) >> 16);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetSTCCASGStatus");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   set the current STC ASG value.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetSTCCASGStatus
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_STCCASG_T *stpLXSdecSetSTCCASG)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSetSTCCASG == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetSTCCASGStatus");

	/* At this time, only implemented to assign main PCR Channel , Later all fields will be added */
	SDEC_HAL_STCCSetMain(_SDEC_CORE_0, stpLXSdecSetSTCCASG->ui8Main); /* M14_TBD, H14_TBD */

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetSTCCASGStatus");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   get the current STC value.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetCurrentSTCPCR
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_STC_PCR_T *stpLXSdecGetSTCPCR)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T *pSdecConf = NULL;
	UINT8 ui8Ch = 0x0;
	UINT8 core = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetSTCPCR == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetCurrentSTC");

	ui8Ch = stpLXSdecGetSTCPCR->eCh;
	
	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	stpLXSdecGetSTCPCR->uiStc_hi = SDEC_HAL_STCCGetSTC(core, ui8Ch, 1);
	stpLXSdecGetSTCPCR->uiStc_lo = SDEC_HAL_STCCGetSTC(core, ui8Ch, 0);

	stpLXSdecGetSTCPCR->uiPcr_hi = SDEC_HAL_STCCGetPCR(core, ui8Ch, 1);
	stpLXSdecGetSTCPCR->uiPcr_lo = SDEC_HAL_STCCGetPCR(core, ui8Ch, 0);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetCurrentSTC");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   get the current PCR, STC value. Only For Debugging
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   	unsigned long *pcr
*	unsigned long *stc
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_GetCurrentSTCPCR
	(unsigned long *pcr,
	unsigned long *stc)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 uiStc_hi = 0, uiStc_lo = 0;
	UINT32 uiPcr_hi = 0, uiPcr_lo = 0;

	/* jinhwan.bae this function is only for the profiling, only for test , so remain CORE0
	    maybe , can be deleted M14_TBD, H14_TBD */
	LX_SDEC_CHECK_CODE( pcr == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stc == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_GetCurrentSTCPCR");

	uiStc_hi = SDEC_HAL_STCCGetSTC(_SDEC_CORE_0, ui8Ch, 1);
	uiStc_lo = SDEC_HAL_STCCGetSTC(_SDEC_CORE_0, ui8Ch, 0);
	uiStc_hi = ((uiStc_hi << 1 ) & 0xFFFFFFFF);
	uiStc_lo = ((uiStc_lo >> 9 ) & 0x1);
	(*stc) = (uiStc_hi | uiStc_lo);

	uiPcr_hi = SDEC_HAL_STCCGetPCR(_SDEC_CORE_0, ui8Ch, 1);
	uiPcr_lo = SDEC_HAL_STCCGetPCR(_SDEC_CORE_0, ui8Ch, 0);
	uiPcr_hi = ((uiPcr_hi << 1 ) & 0xFFFFFFFF);
	uiPcr_lo = ((uiPcr_lo >> 9 ) & 0x1);
	(*pcr) = (uiPcr_hi | uiPcr_lo);
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_GetCurrentSTCPCR");

	eRet = OK;

	return (eRet);
}
EXPORT_SYMBOL(SDEC_GetCurrentSTCPCR);



/**
********************************************************************************
* @brief
*   get the current GSTC value.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   	unsigned long *pcr
*	unsigned long *stc
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetCurrentGSTC
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_GSTC_T *stpLXSdecGetGSTC)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT32 gstc_sdec[2] = { 0, 0 };

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetGSTC == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetCurrentGSTC");

	SDEC_HAL_GSTC(_SDEC_CORE_0, &gstc_sdec[0], &gstc_sdec[1]);

	stpLXSdecGetGSTC->gstc_32_1 	= gstc_sdec[0];
	stpLXSdecGetGSTC->gstc_0 		= gstc_sdec[1];
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetCurrentGSTC");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   get the current GSTC value. (New Version)
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   	unsigned long *pcr
*	unsigned long *stc
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetCurrentGSTC32
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_GSTC32_T *stpLXSdecGetGSTC)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT32 gstc_sdec[2] = { 0, 0 };
	UINT8 core = 0, index = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetGSTC == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	core = stpLXSdecGetGSTC->core;
	index = stpLXSdecGetGSTC->index;

	/* argument check */
	LX_SDEC_CHECK_CODE( core > _SDEC_CORE_1, goto exit, "over core range %d", core);
	LX_SDEC_CHECK_CODE( index > 1 /* MAX GSTC Num */, goto exit, "over GSTC index range %d", index);
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetCurrentGSTC32");

	if(index == 0)	SDEC_HAL_GSTC(core, &gstc_sdec[0], &gstc_sdec[1]);
	else			SDEC_HAL_GSTC1(core, &gstc_sdec[0], &gstc_sdec[1]);

	stpLXSdecGetGSTC->gstc_32_1 	= gstc_sdec[0];
	stpLXSdecGetGSTC->gstc_0 		= gstc_sdec[1];
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetCurrentGSTC32");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   set the current GSTC value.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetCurrentGSTC
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SET_GSTC_T *stpLXSdecSetGSTC)
{
	DTV_STATUS_T eRet = NOT_OK;
	int ret = -1;
	UINT8 index = 0;
	UINT8 core = 0;
	UINT32 gstc_base = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSetGSTC == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	core = stpLXSdecSetGSTC->core;
	index = stpLXSdecSetGSTC->index;
	gstc_base = stpLXSdecSetGSTC->gstc_base;
		
	/* argument check */
	LX_SDEC_CHECK_CODE( core > _SDEC_CORE_1, goto exit, "over core range %d", core);
	LX_SDEC_CHECK_CODE( index > 1 /* MAX GSTC Num */, goto exit, "over GSTC index range %d", index);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetCurrentGSTC [%d][%d][0x%x]", core, index, gstc_base);
	
	ret = SDEC_HAL_GSTCSetValue(core, index, gstc_base);
	LX_SDEC_CHECK_CODE( ret != RET_OK, goto exit, "SDEC_HAL_GSTCSetValue error [%d][%d]", core, index);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetCurrentGSTC");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}



/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SelInputPort
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SEL_INPUT_T *stpLXSdecSelPort)
{
	DTV_STATUS_T eRet = NOT_OK, eResult = NOT_OK;
	UINT8 ui8Ch = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSelPort == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SelInputPort");

	ui8Ch = stpLXSdecSelPort->eCh;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);

	/* CDIC Set Src and Reset */
	eResult = SDEC_InputSet(stpSdecParam, stpLXSdecSelPort);
	LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_InputSet failed:[%d]", eResult);

	/* Error Interupt Setting */
	eResult = SDEC_ERR_Intr_Set(stpSdecParam, ui8Ch);
	LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_ERR_Intr_Set failed:[%d]", eResult);

	/* TPI Interrupt Setting, Only for Normal Channel no regiseter in C,D,G,H */
	if (lx_chip_rev() < LX_CHIP_REV(M14, B0))
	{
		if(ui8Ch >= LX_SDEC_CH_C) goto exit_without_error;
	}
	
	if(SDEC_IS_NORMAL_CHANNEL(ui8Ch))
	{
		/**	caution
		*	should set after input port set. Because if u set this register before input port set, and that register all bit clear
		*/
		eResult = SDEC_TPI_Set(stpSdecParam, ui8Ch);
		LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_TPI_Set failed:[%d]", eResult);
	}

exit_without_error:	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SelInputPort");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_CfgInputPort
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CFG_INPUT_PARAM_T *stpLXSdecCfgPortParam)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_INPUT_T *stpLXSdecCfgPort;
	UINT8 core = 0, ch = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCfgPortParam == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_CfgInputPort");

	stpLXSdecCfgPort = &(stpLXSdecCfgPortParam->input_param);
	ch = stpLXSdecCfgPortParam->ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ch);

	switch (stpLXSdecCfgPort->eInputPort)
	{
		case LX_SDEC_INPUT_PARALLEL0:
			SDEC_DTV_SOC_Message( SDEC_IO, "PARALLEL0");
			SDEC_HAL_CDIP(core, 0, stpLXSdecCfgPort);
			SDEC_HAL_CDIP(core, 1, NULL);
			SDEC_HAL_CDIP(core, 2, NULL);
			break;

		case LX_SDEC_INPUT_PARALLEL1:
			SDEC_DTV_SOC_Message( SDEC_IO, "PARALLEL1");
			SDEC_HAL_CDIPA(core, 0, stpLXSdecCfgPort);
			SDEC_HAL_CDIPA(core, 1, NULL);
			SDEC_HAL_CDIPA(core, 2, NULL);
			break;

		case LX_SDEC_INPUT_SERIAL0:
			SDEC_DTV_SOC_Message( SDEC_IO, "SERIAL0");
			SDEC_HAL_CDIP(core, 0, stpLXSdecCfgPort);
			SDEC_HAL_CDIP(core, 1, NULL);
			SDEC_HAL_CDIP(core, 2, NULL);
			break;
			
		case LX_SDEC_INPUT_SERIAL1:
			SDEC_DTV_SOC_Message( SDEC_IO, "SERIAL1");
			SDEC_HAL_CDIPA(core, 0, stpLXSdecCfgPort);
			SDEC_HAL_CDIPA(core, 1, NULL);
			SDEC_HAL_CDIPA(core, 2, NULL);
			break;
			
		case LX_SDEC_INPUT_SERIAL2:
			/* CDIN3 characteristic is set to CDIP[3] */
			SDEC_HAL_CDIP(core, 3, stpLXSdecCfgPort);
			/* 20131223 jinhwan.bae
			    add to support using external original clock in serial input */
			if(stpSdecParam->ui8UseOrgExtDemodClk == 1)
			{
				SDEC_HAL_CDIPn_2ND_UseExternalOrgClock(core, 3, 1);
			}
			else
			{
				SDEC_HAL_CDIPn_2ND_UseExternalOrgClock(core, 3, 0);
			}
			break;

		case LX_SDEC_INPUT_SERIAL3:
			/* CDIN4 characteristic is set to CDIOP[0] */
			SDEC_HAL_CDIOP(core, 0, stpLXSdecCfgPort);
			/* 20131223 jinhwan.bae
			    add to support using external original clock in serial input */
			if(stpSdecParam->ui8UseOrgExtDemodClk == 1)
			{
				SDEC_HAL_CDIPn_2ND_UseExternalOrgClock(core, 4, 1);
			}
			else
			{
				SDEC_HAL_CDIPn_2ND_UseExternalOrgClock(core, 4, 0);
			}
			break;
			
		default:
			SDEC_DEBUG_Print("Invalid channel:[%d]", stpLXSdecCfgPort->eInputPort);
			goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_CfgInputPort");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SelParInput
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SEL_PAR_INPUT_T *stpLXSdecParInput)
{
	DTV_STATUS_T 	eRet 		= NOT_OK;
	LX_SDEC_INPUT_T	eInputPort;
	UINT8			ui4ts_sel	= 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecParInput == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SelParInput");

	eInputPort = stpLXSdecParInput->eInputPort;

	/* M14_TBD, H14_TBD */
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC; /* XX00 */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3; /*00XX */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H14A0_WrFL(ctr58);
				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H14A0_WrFL(ctr58);
				break;
			case LX_SDEC_CI_INPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_INPUT [P%d]", eInputPort - 2);

				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Wr01(ctr58, tpio_sel_ctrl, 1);	/* Set Parallel IO as Input ?? jinhwan.bae -> yes, so if you need ci input signal, select external one instead of CI_INPUT */
				// CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* Always Output !We want to get output data ??  */
				CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x3;	/* XX11 */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x8;	/* 10XX */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H14A0_WrFL(ctr58);
				break;
			case LX_SDEC_CI_OUTPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_OUTPUT [P%d]", eInputPort - 2);

				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* CI_INPUT port changed to Always Output !We want to get output data ??  */
				CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x2;	/* XX10 */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0xC;	/* 11XX */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H14A0_WrFL(ctr58);
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
				CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC; /* XX00 */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3; /*00XX */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14B0_WrFL(TOP, ctr19);
				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
				CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14B0_WrFL(TOP, ctr19);
				break;
			case LX_SDEC_CI_INPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_INPUT [P%d]", eInputPort - 2);

#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
				CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
				CTOP_CTRL_M14B0_Wr01(LEFT, ctr96, tpio_sel_ctrl, 1);	/* Set Parallel IO as Input ?? jinhwan.bae -> yes, so if you need ci input signal, select external one instead of CI_INPUT */
				CTOP_CTRL_M14B0_WrFL(LEFT, ctr96);
				// CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* Always Output !We want to get output data ??  */
#else
				CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
				CTOP_CTRL_M14B0_Wr01(TOP, ctr08, tpio_sel_ctrl, 1);	/* Set Parallel IO as Input ?? jinhwan.bae -> yes, so if you need ci input signal, select external one instead of CI_INPUT */
				CTOP_CTRL_M14B0_WrFL(TOP, ctr08);
#endif

				CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
				CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x3;	/* XX11 */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x8;	/* 10XX */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14B0_WrFL(TOP, ctr19);
				break;
			case LX_SDEC_CI_OUTPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_OUTPUT [P%d]", eInputPort - 2);

#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
				CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
				CTOP_CTRL_M14B0_Wr01(LEFT, ctr96, tpio_sel_ctrl, 0);	/* CI_INPUT port changed to Always Output !We want to get output data ??  */
				CTOP_CTRL_M14B0_WrFL(LEFT, ctr96);
#else
				CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
				CTOP_CTRL_M14B0_Wr01(TOP, ctr08, tpio_sel_ctrl, 0);	/* CI_INPUT port changed to Always Output !We want to get output data ??  */
				CTOP_CTRL_M14B0_WrFL(TOP, ctr08);
#endif

				CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
				CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x2;	/* XX10 */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0xC;	/* 11XX */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14B0_WrFL(TOP, ctr19);
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC; /* XX00 */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3; /*00XX */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14A0_WrFL(ctr58);
				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14A0_WrFL(ctr58);
				break;
			case LX_SDEC_CI_INPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_INPUT [P%d]", eInputPort - 2);

				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Wr01(ctr58, tpio_sel_ctrl, 1);	/* Set Parallel IO as Input ?? jinhwan.bae -> yes, so if you need ci input signal, select external one instead of CI_INPUT */
				// CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* Always Output !We want to get output data ??  */
				CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x3;	/* XX11 */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x8;	/* 10XX */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14A0_WrFL(ctr58);
				break;
			case LX_SDEC_CI_OUTPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_OUTPUT [P%d]", eInputPort - 2);

				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* CI_INPUT port changed to Always Output !We want to get output data ??  */
				CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x2;	/* XX10 */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0xC;	/* 11XX */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_M14A0_WrFL(ctr58);
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC; /* XX00 */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3; /*00XX */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H13A0_WrFL(ctr58);
				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD [P%d]", eInputPort - 2);

				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H13A0_WrFL(ctr58);
				break;
			case LX_SDEC_CI_INPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_INPUT [P%d]", eInputPort - 2);

				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, 1);	/* Set Parallel IO as Input ?? jinhwan.bae -> yes, so if you need ci input signal, select external one instead of CI_INPUT */
				// CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* Always Output !We want to get output data ??  */
				CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x3;	/* XX11 */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0x8;	/* 10XX */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H13A0_WrFL(ctr58);
				break;
			case LX_SDEC_CI_OUTPUT:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_CI_OUTPUT [P%d]", eInputPort - 2);

				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, 0); /* CI_INPUT port changed to Always Output !We want to get output data ??  */
				CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);

				if( eInputPort == LX_SDEC_INPUT_PARALLEL1 )
				{
					ui4ts_sel &= 0xC;
					ui4ts_sel |= 0x2;	/* XX10 */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}
				else if ( eInputPort == LX_SDEC_INPUT_PARALLEL0 )
				{
					ui4ts_sel &= 0x3;
					ui4ts_sel |= 0xC;	/* 11XX */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
				}

				CTOP_CTRL_H13A0_WrFL(ctr58);
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SelParInput");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SelCiInput
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SEL_CI_INPUT_T *stpLXSdecParInput)
{
	DTV_STATUS_T 	eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecParInput == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SelCiInput");
	SDEC_DTV_SOC_Message( SDEC_IO, "%s : stpLXSdecParInput->eParInput = %d", __FUNCTION__, stpLXSdecParInput->eParInput);

	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD");

				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Wr01(ctr58, tpo_sel_ctrl, 0);
				CTOP_CTRL_H14A0_WrFL(ctr58);

				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD");

				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Wr01(ctr58, tpo_sel_ctrl, 1);
				CTOP_CTRL_H14A0_WrFL(ctr58);

				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD");
#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
				CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
				CTOP_CTRL_M14B0_Wr01(LEFT, ctr96, tpo_sel_ctrl, 0);
				CTOP_CTRL_M14B0_WrFL(LEFT, ctr96);
#else
				CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
				CTOP_CTRL_M14B0_Wr01(TOP, ctr08, tpo_sel_ctrl, 0);
				CTOP_CTRL_M14B0_WrFL(TOP, ctr08);
#endif
				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD");
#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
				CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
				CTOP_CTRL_M14B0_Wr01(LEFT, ctr96, tpo_sel_ctrl, 1);
				CTOP_CTRL_M14B0_WrFL(LEFT, ctr96);
#else
				CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
				CTOP_CTRL_M14B0_Wr01(TOP, ctr08, tpo_sel_ctrl, 1);
				CTOP_CTRL_M14B0_WrFL(TOP, ctr08);
#endif
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD");

				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Wr01(ctr58, tpo_sel_ctrl, 0);
				CTOP_CTRL_M14A0_WrFL(ctr58);

				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD");

				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Wr01(ctr58, tpo_sel_ctrl, 1);
				CTOP_CTRL_M14A0_WrFL(ctr58);

				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		switch (stpLXSdecParInput->eParInput)
		{
			case LX_SDEC_INTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_INTERNAL_DEMOD");

				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Wr01(ctr58, tpo_sel_ctrl, 0);
				CTOP_CTRL_H13A0_WrFL(ctr58);

				break;
			case LX_SDEC_EXTERNAL_DEMOD:
				SDEC_DTV_SOC_Message( SDEC_IO, "LX_SDEC_EXTERNAL_DEMOD");

				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Wr01(ctr58, tpo_sel_ctrl, 1);
				CTOP_CTRL_H13A0_WrFL(ctr58);

				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", stpLXSdecParInput->eParInput);
				goto exit;
			break;
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SelCiInput");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetParInput
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_PAR_INPUT_T *stpLXSdecParInput)
{
	DTV_STATUS_T 	eRet 		= NOT_OK;
	LX_SDEC_INPUT_T	eInputPort;
	UINT8			ui4ts_sel	= 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecParInput == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetParInput");

	eInputPort = stpLXSdecParInput->eInputPort;

	/* M14_TBD, H14_TBD */
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (eInputPort)
		{
			case LX_SDEC_INPUT_PARALLEL0:
				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
				ui4ts_sel = ( (ui4ts_sel & 0xC) >> 2 ); /*VVxx */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				break;
				
			case LX_SDEC_INPUT_PARALLEL1:
				CTOP_CTRL_H14A0_RdFL(ctr58);
				CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
				ui4ts_sel = (ui4ts_sel & 0x3); /*xxVV */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", eInputPort);
				break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (eInputPort)
		{
			case LX_SDEC_INPUT_PARALLEL0:
				CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
				CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);
				ui4ts_sel = ( (ui4ts_sel & 0xC) >> 2 ); /*VVxx */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				break;
				
			case LX_SDEC_INPUT_PARALLEL1:
				CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
				CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);
				ui4ts_sel = (ui4ts_sel & 0x3); /*xxVV */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", eInputPort);
				break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (eInputPort)
		{
			case LX_SDEC_INPUT_PARALLEL0:
				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
				ui4ts_sel = ( (ui4ts_sel & 0xC) >> 2 ); /*VVxx */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				break;
				
			case LX_SDEC_INPUT_PARALLEL1:
				CTOP_CTRL_M14A0_RdFL(ctr58);
				CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
				ui4ts_sel = (ui4ts_sel & 0x3); /*xxVV */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", eInputPort);
				break;
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		/*--------------------------------------------------------
			--> ORIGINAL Design Concept but not real
				(Please Check After H13A0!!, they might be changed to original)
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [5:4])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [7:6])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In

			--> H13 A0 Ctr58 Bits are Crossed Abnormally as follows
			LX_SDEC_INPUT_PARALLEL0 (CDIN0 Selection) (Ctr58 [7:6])
			00b : Internal Demod 	01b : External Demod
			10b : tpo,bidir,CI In	11b : tpi, CI Out
			LX_SDEC_INPUT_PARALLEL1 (CDINA Selection) (Ctr58 [5:4])
			00b : Internal Demod	01b : External Demod
			10b : tpi, CI Out		11b : tpo,bidir,CI In
		   --------------------------------------------------------*/
		switch (eInputPort)
		{
			case LX_SDEC_INPUT_PARALLEL0:
				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);
				ui4ts_sel = ( (ui4ts_sel & 0xC) >> 2 ); /*VVxx */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				break;
				
			case LX_SDEC_INPUT_PARALLEL1:
				CTOP_CTRL_H13A0_RdFL(ctr58);
				CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);
				ui4ts_sel = (ui4ts_sel & 0x3); /*xxVV */
				if(ui4ts_sel == 0) stpLXSdecParInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
				else if(ui4ts_sel == 1) stpLXSdecParInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
				else if(ui4ts_sel == 2) stpLXSdecParInput->eParInput = LX_SDEC_CI_OUTPUT;
				else if(ui4ts_sel == 3) stpLXSdecParInput->eParInput = LX_SDEC_CI_INPUT;
				break;

			default:
				SDEC_DEBUG_Print("Invalid Port:[%d]", eInputPort);
				break;
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetParInput");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetCiInput
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_CI_INPUT_T *stpLXSdecCiInput)
{
	DTV_STATUS_T 	eRet 		= NOT_OK;
	UINT8 tpo_sel_ctrl = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCiInput == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetCiInput");

	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		CTOP_CTRL_H14A0_RdFL(ctr58);
		CTOP_CTRL_H14A0_Rd01(ctr58, tpo_sel_ctrl, tpo_sel_ctrl);

		if(tpo_sel_ctrl == 0) stpLXSdecCiInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
		else if(tpo_sel_ctrl == 1) stpLXSdecCiInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
		CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
		CTOP_CTRL_M14B0_Rd01(LEFT, ctr96, tpo_sel_ctrl, tpo_sel_ctrl);
#else
		CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
		CTOP_CTRL_M14B0_Rd01(TOP, ctr08, tpo_sel_ctrl, tpo_sel_ctrl);
#endif
		if(tpo_sel_ctrl == 0) stpLXSdecCiInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
		else if(tpo_sel_ctrl == 1) stpLXSdecCiInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		CTOP_CTRL_M14A0_RdFL(ctr58);
		CTOP_CTRL_M14A0_Rd01(ctr58, tpo_sel_ctrl, tpo_sel_ctrl);

		if(tpo_sel_ctrl == 0) stpLXSdecCiInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
		else if(tpo_sel_ctrl == 1) stpLXSdecCiInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		CTOP_CTRL_H13A0_RdFL(ctr58);
		CTOP_CTRL_H13A0_Rd01(ctr58, tpo_sel_ctrl, tpo_sel_ctrl);

		if(tpo_sel_ctrl == 0) stpLXSdecCiInput->eParInput = LX_SDEC_INTERNAL_DEMOD;
		else if(tpo_sel_ctrl == 1) stpLXSdecCiInput->eParInput = LX_SDEC_EXTERNAL_DEMOD;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetCiInput");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   set esa mode.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetCipherEnable
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CIPHER_ENABLE_T *stLXSdecCipherEnable)
{
	DTV_STATUS_T eRet = NOT_OK;
	int ret = RET_ERROR;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	BOOLEAN bEnable = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stLXSdecCipherEnable == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetCipherEnable");

	ui8Ch = stLXSdecCipherEnable->eCh;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	
	ui32PidFltId  = stLXSdecCipherEnable->uiPidFltId;
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "over pid filter range %d", ui32PidFltId);
	
	bEnable	= stLXSdecCipherEnable->bEnable;

	ret = SDEC_SetPidf_Disc_Enable(stpSdecParam, ui8Ch, ui32PidFltId, bEnable);
	LX_SDEC_CHECK_CODE( ret != OK, goto exit, "SDEC_SetPidf_Disc_Enable Error" );

	eRet = OK;

exit:

	SDEC_DTV_SOC_Message( SDEC_DESC, "SDEC_IO_SetCipherEnable : Ch[%d] FilterId[%d] bEnable[%d]", ui8Ch, ui32PidFltId, bEnable);
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetCipherEnable ret[%d]", (UINT32)eRet);

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   set esa mode.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetCipherMode
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CIPHER_MODE_T *stpLXSdecCipherMode)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8	ui8Ch = 0x0;
	UINT8	core = 0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCipherMode == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetCipherMode");

	ui8Ch = stpLXSdecCipherMode->eCh;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	SDEC_HAL_DSCSetCasType(core, ui8Ch, stpLXSdecCipherMode->eCasType);
	SDEC_HAL_DSCSetBlkMode(core, ui8Ch, stpLXSdecCipherMode->eBlkMode);
	SDEC_HAL_DSCSetResMode(core, ui8Ch, stpLXSdecCipherMode->eResMode);
	SDEC_HAL_DSCSetKeySize(core, ui8Ch, stpLXSdecCipherMode->eKeySize);
	SDEC_HAL_DSCEnablePESCramblingCtrl(core, ui8Ch, stpLXSdecCipherMode->ePSCEn);
	SDEC_HAL_DSCSetEvenMode(core, ui8Ch, 0x2);
	SDEC_HAL_DSCSetOddMode(core, ui8Ch, 0x3);

	SDEC_DTV_SOC_Message( SDEC_DESC, "SDEC_IO_SetCipherMode : Ch[%d] Type(3:AES)[%d]", ui8Ch, stpLXSdecCipherMode->eCasType);
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetCipherMode");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   set Cipher key.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
UINT32 gWRCNT = 0;
#define SDEC_MAX_KEY_LEN	32
DTV_STATUS_T SDEC_IO_SetCipherKey
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CIPHERKEY_T *stpLXSdecCipherKey)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 	ui8count = 0x0;
	UINT8 	key_type = 0, pid_idx = 0, odd_key = 0;
	UINT32	*ui32pAddr;
	UINT32  ipc_addr = 0;
	UINT32 	ui32rdcnt = 0;
	UINT32	ui32mcutimeout = 0;
	LX_SDEC_CFG_T *pSdecConf = NULL;
	UINT8	key_buf[SDEC_MAX_KEY_LEN];
	int		err = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCipherKey == NULL, return INVALID_ARGS, "Invalid argument" );
	LX_SDEC_CHECK_CODE( stpLXSdecCipherKey->puiCipherKey == NULL, return INVALID_ARGS, "Invalid argument key mem is null" );
	LX_SDEC_CHECK_CODE( stpLXSdecCipherKey->uiKeySize > MAX_KEY_WORD_IDX, return INVALID_ARGS, "Invalid argument uiKeySize [%d]", stpLXSdecCipherKey->uiKeySize);

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( stpLXSdecCipherKey->eCh >= (LX_SDEC_CH_T)(pSdecConf->nChannel), return INVALID_ARGS, "over channel range %d", stpLXSdecCipherKey->eCh);

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_DESC, "<--SDEC_IO_SetCipherKey Ch[%d] idx[%d] eKeyMode[%d] key[0x%x] uiKeySize(word)[%d]", 
					stpLXSdecCipherKey->eCh, stpLXSdecCipherKey->uiPidFltId, stpLXSdecCipherKey->eKeyMode, (UINT32)(stpLXSdecCipherKey->puiCipherKey), stpLXSdecCipherKey->uiKeySize);

	memset(key_buf, 0x00, SDEC_MAX_KEY_LEN);
	err = copy_from_user(key_buf, stpLXSdecCipherKey->puiCipherKey, (stpLXSdecCipherKey->uiKeySize)*4);
	LX_SDEC_CHECK_CODE( err != OK, goto func_exit, "copy_from_user error \n");

	if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN || stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD)//Key type :even or ODD
	{
		key_type = 0x0;
		pid_idx =  stpLXSdecCipherKey->uiPidFltId;
		if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN)
		{
			odd_key = 0x0;
		}
		else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD)
		{
			odd_key = 0x1;
		}

		ui8count = 0x0;	//by skpark
	}
	else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD_IV || stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN_IV)//Key type :IV
	{
		key_type = 0x1;
		pid_idx =  stpLXSdecCipherKey->uiPidFltId;
		if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN_IV)
		{
			odd_key = 0x0;
		}
		else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD_IV)
		{
			odd_key = 0x1;
		}

		ui8count = 0x0;	//by skpark
	}
	else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_MULTI2)
	{
		key_type = 0x02;
		ui8count = 0x0;				//by skpark
	}

	// ui32pAddr = (UINT32 *)stpLXSdecCipherKey->puiCipherKey;
	ui32pAddr = (UINT32 *)key_buf;

	/* jinhwan.bae mcu test */
	if( (stpSdecParam->ui8McuDescramblerCtrlMode == 1) && (stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN || stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD) )
	{		
		/* Only JP Platform Process following Routine */
		SDEC_DTV_SOC_Message( SDEC_DESC, "SDEC_IO_SetCipherKey with IPC : Ch[%d] idx[%d] eKeyMode[%d]", stpLXSdecCipherKey->eCh, stpLXSdecCipherKey->uiPidFltId, stpLXSdecCipherKey->eKeyMode);

		/* H13 Only */
		if (lx_chip_rev() < LX_CHIP_REV(M14, A0))
		{
			ui32rdcnt = REG_READ32(gh13_mcu_ipc + 0x14);

			ui32mcutimeout = msecs_to_jiffies(500) + jiffies;
			while(ui32rdcnt != gWRCNT)
			{
				ui32rdcnt = REG_READ32(gh13_mcu_ipc + 0x14);
				SDEC_DTV_SOC_Message( SDEC_DESC, "Infinite Loop in Key Set RdCnt[%d] WrCnt[%d]", ui32rdcnt, gWRCNT);
				if(jiffies > ui32mcutimeout)
				{
					SDEC_DTV_SOC_Message( SDEC_DESC, "Infinite Loop Timeout in Key Set RdCnt[%d] WrCnt[%d]", ui32rdcnt, gWRCNT);
					break;
				}
			}

			{
				/* set ipc */
				ipc_addr = 0;
				REG_WRITE32(gh13_mcu_ipc + 0x00 ,  stpLXSdecCipherKey->eCh );		/* channel */
				ipc_addr |= (pid_idx << 6);
				ipc_addr |= (odd_key << 5);
				REG_WRITE32(gh13_mcu_ipc + 0x04 ,  ipc_addr);						/* all IPC address */

				REG_WRITE32(gh13_mcu_ipc + 0x08 ,  swab32(*ui32pAddr));				/* data 0 */
				ui32pAddr++;
				REG_WRITE32(gh13_mcu_ipc + 0x0c ,  swab32(*ui32pAddr));				/* data 1 */

				gWRCNT++;
				REG_WRITE32(gh13_mcu_ipc + 0x10 ,  gWRCNT);							/* write count */
			}
		}
	}
	else
	{
		/* sys and iv is done by original way */

		SDEC_DTV_SOC_Message( SDEC_DESC, "SDEC_IO_SetCipherKey : Ch[%d] idx[%d] eKeyMode[%d]", stpLXSdecCipherKey->eCh, stpLXSdecCipherKey->uiPidFltId, stpLXSdecCipherKey->eKeyMode);

		if( ( key_type != 0x1 ) || ( pid_idx != 0xFF ) )	/* For BCAS IV, Only key_type == 0x1 && pid_idx == 0xFF should be set to all, others are set by original way */
		{
			/* Followings are Original Source Code before BCAS */
			//by skpark
			for( ; ui8count < stpLXSdecCipherKey->uiKeySize; ui8count++)
			{
				SDEC_HAL_KMEMSet( stpLXSdecCipherKey->eCh, key_type, pid_idx, odd_key, ui8count, *ui32pAddr);
				ui32pAddr++;
			}
		}
		else
		{
			UINT8 	ui8pidfilter 	= 0x0;
			UINT8	ui8PidfNum 		= 0x0;
			LX_SDEC_CFG_T* 	pSdecConf 	= NULL;

			/* get chip configuation */
			pSdecConf = SDEC_CFG_GetConfig();

			/* get pid filter number from channel info structure */
			ui8PidfNum	= pSdecConf->chInfo[stpLXSdecCipherKey->eCh].num_pidf;

			// IV vector set to all PID filter jinhwan.bae
			for(ui8pidfilter = 0x0; ui8pidfilter < ui8PidfNum; ui8pidfilter++)
			{
//				ui32pAddr = (UINT32 *)stpLXSdecCipherKey->puiCipherKey;
				ui32pAddr = (UINT32 *)key_buf;

				for(ui8count = 0x0 ; ui8count < stpLXSdecCipherKey->uiKeySize; ui8count++)
				{
					SDEC_HAL_KMEMSet( stpLXSdecCipherKey->eCh, key_type, ui8pidfilter, odd_key, ui8count, *ui32pAddr);
					ui32pAddr++;
				}
			}
		}

	}// jinhwan.bae for mcu test 

	SDEC_DTV_SOC_Message( SDEC_DESC, "-->SDEC_IO_SetCipherKey");

	eRet = OK;

func_exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   set Cipher key.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetCipherKey
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_CIPHERKEY_T *stpLXSdecCipherKey)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8count = 0x0, key_type = 0, pid_idx = 0, odd_key = 0;
	UINT32 *ui32pAddr;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCipherKey == NULL, return INVALID_ARGS, "Invalid argument" );
	
	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( stpLXSdecCipherKey->eCh >= (LX_SDEC_CH_T)(pSdecConf->nChannel), return INVALID_ARGS, "over channel range %d", stpLXSdecCipherKey->eCh);

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetCipherKey");

	if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN || stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD)//Key type :even or ODD
	{
		key_type = 0x0;
		pid_idx =  stpLXSdecCipherKey->uiPidFltId;
		if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN)
		{
			odd_key = 0x0;
		}
		else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD)
		{
			odd_key = 0x1;
		}

		ui8count = 0x0;	//by skpark
	}
	else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD_IV || stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN_IV)//Key type :IV
	{
		key_type = 0x1;
		pid_idx =  stpLXSdecCipherKey->uiPidFltId;
		if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_EVEN_IV)
		{
			odd_key = 0x0;
		}
		else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_ODD_IV)
		{
			odd_key = 0x1;
		}

		ui8count = 0x0;	//by skpark
	}
	else if(stpLXSdecCipherKey->eKeyMode == LX_SDEC_CIPHER_KEY_MULTI2)
	{
		key_type = 0x02;
		ui8count = 0x0;				//by skpark
	}

	ui32pAddr = (UINT32 *)(&stpLXSdecCipherKey->uiCipherKey[0]);

    //by skpark
	for( ; ui8count < stpLXSdecCipherKey->uiKeySize; ui8count++)
	{
		*ui32pAddr = swab32(SDEC_HAL_KMEMGet( stpLXSdecCipherKey->eCh, key_type, pid_idx, odd_key, ui8count));
		ui32pAddr++;
	}

	SDEC_DTV_SOC_Message( SDEC_DESC, "SDEC_IO_GetCipherKey : idx[%d] eKeyMode[%d]", stpLXSdecCipherKey->uiPidFltId, stpLXSdecCipherKey->eKeyMode);
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetCipherKey");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   Set PCR PID
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   stpLXSdecPIDFltSetPID : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetPCRPID
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_SET_PCRPID_T *stpLXSdecPIDFltSetPID)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidValue = 0x0;
	BOOLEAN bMain = 0x0;
	UINT8 core = 0, org_ch = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltSetPID == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetPCRPID");

	ui8Ch = stpLXSdecPIDFltSetPID->eCh;
	ui32PidValue = stpLXSdecPIDFltSetPID->uiPidValue;
	bMain = stpLXSdecPIDFltSetPID->bMain;

	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);
	LX_SDEC_CHECK_CODE( ui8Ch > 2, goto exit, "channel is invalid org_ch[%d]ui8Ch[%d]", org_ch, ui8Ch );

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] PCR  PidValue[0x%08x]", org_ch,	ui32PidValue);

	if(ui32PidValue == 0x1fff)
	{
		// chan | pcr_pid
		SDEC_HAL_STCCSetReg(core, ui8Ch, 0x00000000 | (ui8Ch << 29) | (ui32PidValue << 16));
		if(bMain) pwm_context_reset(stpSdecParam, ui8Ch);
	}
	else
	{
		// en | chan | pcr_pid | copy_en | latch_en
		SDEC_HAL_STCCSetReg(core, ui8Ch, 0x80000000 | (ui8Ch << 29) );
		SDEC_HAL_STCCSetSTC(core, ui8Ch, 0);
		SDEC_HAL_STCCSetReg(core, ui8Ch, 0x80000003 | (ui8Ch << 29) | (ui32PidValue << 16));
	}

	/* if current channel is main channel, set main
	 * warning *
	 * viewing channel is not main channel, main channel is pcr recovery channel */
	if(bMain)
	{
		SDEC_HAL_STCCSetMain(core, ui8Ch);
		pwm_context_reset(stpSdecParam, ui8Ch);	// added by jinhwan.bae for support pip operation 
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetPCRPID");

	eRet = OK;

exit:
	
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}


/**
********************************************************************************
* @brief
*   set pcr recovery.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   stpLXSdecSetPCRRecovery : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetPcrRecovery
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SET_PCR_RECOVERY_T *stpLXSdecSetPCRRecovery)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT8 ui8PcrCmd = 0x0;
	UINT8 core = 0, org_ch = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSetPCRRecovery == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetPcrRecovery");

	ui8Ch = stpLXSdecSetPCRRecovery->eCh;
	ui8PcrCmd = stpLXSdecSetPCRRecovery->eCmd;

	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);
	LX_SDEC_CHECK_CODE( ui8Ch > 2, goto exit, "channel is invalid org_ch[%d]ui8Ch[%d]", org_ch, ui8Ch );

	switch (ui8PcrCmd)
	{
		case LX_SDEC_PCR_CMD_DISABLE:
			stpSdecParam->bPcrRecoveryFlag[org_ch] = FALSE;
//			SDEC_HAL_STCCReset(core, ui8Ch);
			break;
		case LX_SDEC_PCR_CMD_ENABLE:
			stpSdecParam->bPcrRecoveryFlag[org_ch] = TRUE;
#if 0	/* M14_TBD jinhwan.bae to add for stc counter reset to 0 , which is A/V decoder STC counter value reset , pause -> play sequence */
			SDEC_HAL_AVSTCReset(core, ui8Ch);
			usleep(50);	/* actually 12us needed to send the reset value to A/V Decoder */
#endif
			SDEC_HAL_STCCReset(core, ui8Ch);

#ifdef __NEW_PWM_RESET_COND__
			pcr_error_for_reset = 0;
#endif
			break;
		case LX_SDEC_PCR_CMD_RESET:
			SDEC_HAL_STCCReset(core, ui8Ch);
			SDEC_HAL_STCCEnableCopy(core, ui8Ch, SDEC_HAL_ENABLE);
		break;
		default:
			SDEC_DEBUG_Print("Invalid PCR cmd:[%d]", ui8PcrCmd);
			goto exit;
		break;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetPcrRecovery");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   get information of input port setting
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
*   stpLXSdecGetInputPort : ioctrl arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetInputPort
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_GET_INPUT_T *stpLXSdecGetInputPort)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	CDIC cdic;
	UINT8 core = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGetInputPort == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetPcrRecovery");

	ui8Ch = stpLXSdecGetInputPort->eCh;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	SDEC_HAL_CIDCGetStatus(core, ui8Ch, &cdic);

	stpLXSdecGetInputPort->uiSrc			= cdic.src;
	stpLXSdecGetInputPort->uiNo_wdata		= cdic.no_wdata;
	stpLXSdecGetInputPort->uiSync_lost		= cdic.sync_lost;
	stpLXSdecGetInputPort->uiSb_dropped 	= cdic.sb_dropped;
	stpLXSdecGetInputPort->uiCdif_empty 	= cdic.cdif_empty;
	stpLXSdecGetInputPort->uiCdif_full		= cdic.cdif_full;
	stpLXSdecGetInputPort->uiPd_wait		= cdic.pd_wait;
	stpLXSdecGetInputPort->uiCdif_ovflow	= cdic.cdif_ovflow;
	stpLXSdecGetInputPort->uiCdif_wpage 	= cdic.cdif_wpage;
	stpLXSdecGetInputPort->uiCdif_rpage 	= cdic.cdif_rpage;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetPcrRecovery");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   set video output port
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetVidOutport
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SET_VDEC_PORT_T *stpLXSdecSetVidOutort)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T* pSdecConf = NULL;

	UINT8 ui8Ch = 0x0;
	UINT8 ui8sel = 0x0;
	int ret = RET_ERROR;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSetVidOutort == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SetVidOutport");

	/* M14_TBD, channel converting is not needed? */
	ui8Ch 	= (UINT8)stpLXSdecSetVidOutort->ucPort;
	ui8sel	= (UINT8)stpLXSdecSetVidOutort->eFrom;

	/* get config */
	pSdecConf = SDEC_CFG_GetConfig();

	/* if port number is out of range, return errror */
	LX_SDEC_CHECK_CODE( ui8Ch > pSdecConf->nVdecOutPort, goto exit, "Invalid Channel [%d][%d]",ui8Ch, pSdecConf->nVdecOutPort );
	
	ret = SDEC_HAL_SetVideoOut(ui8Ch, ui8sel);
	LX_SDEC_CHECK_CODE( ret != RET_OK, goto exit, "SDEC_HAL_SetVideoOut Error [%d][%d]",ui8Ch, ui8sel );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SetVidOutport");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   set  SDEC input port enable and disable select
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_InputPortEnable(UINT8 core, LX_SDEC_INPUT_T eInputPath, UINT8 en)
{
	int ret = RET_ERROR;

	switch (eInputPath)
	{
		case LX_SDEC_INPUT_PARALLEL0:
			SDEC_HAL_CDIPEnable(core, 0, en);
			break;
		case LX_SDEC_INPUT_PARALLEL1:
			if (lx_chip_rev() < LX_CHIP_REV(L9, B0))
			{
				SDEC_HAL_CDIPEnable(core, 0, en);
			}
			else
			{
				SDEC_HAL_CDIPAEnable(core, 0, en);
			}
			break;

		case LX_SDEC_INPUT_SERIAL0:

			if( en == SDEC_HAL_ENABLE)
			{
				if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				{
					UINT8			ui4ts_sel	= 0x0;

					CTOP_CTRL_H14A0_RdFL(ctr58);
					CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0x3; 
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
					CTOP_CTRL_H14A0_WrFL(ctr58);
				}
				else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				{
					UINT8			ui4ts_sel	= 0x0;
					
					CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
					CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0x3; 
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
					CTOP_CTRL_M14B0_WrFL(TOP, ctr19);
				}
				else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				{
					UINT8			ui4ts_sel	= 0x0;
					
					CTOP_CTRL_M14A0_RdFL(ctr58);
					CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0x3; 
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
					CTOP_CTRL_M14A0_WrFL(ctr58);
				}
				else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				{
					UINT8			ui4ts_sel	= 0x0;
					
					CTOP_CTRL_H13A0_RdFL(ctr58);
					CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0x3; 
					ui4ts_sel |= 0x4;	/* 01XX */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
					CTOP_CTRL_H13A0_WrFL(ctr58);
				}
			}
	
			SDEC_HAL_CDIPEnable(core, 0, en);
			
			break;

		case LX_SDEC_INPUT_SERIAL1:

			if( en == SDEC_HAL_ENABLE)
			{
				if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				{
					UINT8			ui4ts_sel	= 0x0;

					CTOP_CTRL_H14A0_RdFL(ctr58);
					CTOP_CTRL_H14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0xC; 
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_H14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
					CTOP_CTRL_H14A0_WrFL(ctr58);
				}
				else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				{
					UINT8			ui4ts_sel	= 0x0;
					
					CTOP_CTRL_M14B0_RdFL(TOP, ctr19);
					CTOP_CTRL_M14B0_Rd01(TOP, ctr19, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0xC; 
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_M14B0_Wr01(TOP, ctr19, ts_sel, ui4ts_sel);
					CTOP_CTRL_M14B0_WrFL(TOP, ctr19);
				}
				else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				{
					UINT8			ui4ts_sel	= 0x0;
					
					CTOP_CTRL_M14A0_RdFL(ctr58);
					CTOP_CTRL_M14A0_Rd01(ctr58, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0xC; 
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_M14A0_Wr01(ctr58, ts_sel, ui4ts_sel);
					CTOP_CTRL_M14A0_WrFL(ctr58);
				}
				else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				{
					UINT8			ui4ts_sel	= 0x0;
					
					CTOP_CTRL_H13A0_RdFL(ctr58);
					CTOP_CTRL_H13A0_Rd01(ctr58, ts_sel, ui4ts_sel);
					ui4ts_sel &= 0xC; 
					ui4ts_sel |= 0x1;	/* XX01 */
					CTOP_CTRL_H13A0_Wr01(ctr58, ts_sel, ui4ts_sel);
					CTOP_CTRL_H13A0_WrFL(ctr58);
				}
			}
			
			SDEC_HAL_CDIPAEnable(core, 0, en);

			break;

		case LX_SDEC_INPUT_SERIAL2:

			SDEC_HAL_CDIPEnable(core, 3, en);

			break;

		case LX_SDEC_INPUT_SERIAL3:

			SDEC_HAL_CDIOPEnable(core, 0, en);

			break;

		default:
			goto exit;
	}

	ret = RET_OK;

exit:
	return ret;
}


/**
********************************************************************************
* @brief
*   set  SDEC input port enable and disable select
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_InputPortEnable
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_ENABLE_INPUT_T *stpLXSdecEnableInput)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CH_T eCh, org_eCh = LX_SDEC_CH_A;
	LX_SDEC_INPUT_T eInputPath;
	LX_SDEC_CFG_T	*pSdecConf = NULL;
	UINT8 ui8En = 0, ui8cdif_full = 0, ui8cdif_ovflow = 0;
	CDIC cdic;
	unsigned long flags = 0;
	UINT8 core = 0;
	int ret = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecEnableInput == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_InputPortEnable");

	eCh 		= stpLXSdecEnableInput->eCh;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( eCh >= (LX_SDEC_CH_T)(pSdecConf->nChannel), goto exit, "over channel range %d", eCh);
	
	org_eCh		= eCh;
	eInputPath 	= stpLXSdecEnableInput->eInputPath;
	ui8En		= stpLXSdecEnableInput->bEnable ? 0x1 : 0x0;

	SDEC_CONVERT_CORE_CH(core, eCh);

	ret = SDEC_HAL_CIDCGetStatus(core, eCh, &cdic);
	LX_SDEC_CHECK_CODE( ret != RET_OK, goto exit, "SDEC_HAL_CIDCGetStatus error" );

	ui8cdif_full	= cdic.cdif_full;
	ui8cdif_ovflow	= cdic.cdif_ovflow;

	/* reset CDIC before enable CDIP */
	if(stpLXSdecEnableInput->bEnable)
	{
		if( ui8cdif_full || ui8cdif_ovflow )
		{
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);

			SDEC_DTV_SOC_Message(SDEC_RESET, "CDIC       [0x%08x]", SDEC_HAL_CIDCGet(core, eCh));	/* SDIO-CDIC */
			SDEC_DTV_SOC_Message(SDEC_RESET, "SDMWC      [0x%08x]", SDEC_HAL_SDMWCGet(core));		/* SDIO-SDMWC */
			SDEC_DTV_SOC_Message(SDEC_RESET, "SDMWC_STAT [0x%08x]", SDEC_HAL_SDMWCGetStatus(core));		/* SDIO-SDMWC_STAT */
			SDEC_DTV_SOC_Message(SDEC_RESET, "CHAN_STAT  [0x%08x]", SDEC_HAL_GetChannelStatus2(org_eCh));	/* SDCORE-CHAN_STAT */
			SDEC_DTV_SOC_Message(SDEC_RESET, "CC_CHK_EN  [0x%08x%08x]", SDEC_HAL_CCCheckEnableGet(org_eCh, 0), SDEC_HAL_CCCheckEnableGet(org_eCh, 1));	/* SDCORE-CC_CHK_EN */

			/* disable channel input and wait 0.01 ms */
			SDEC_InputPortEnable(core, stpSdecParam->eInputPath[org_eCh], SDEC_HAL_DISABLE);
			
			OS_UsecDelay(10);
			
			SDEC_InputPortReset(org_eCh);

			OS_UsecDelay(10);

			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
		}

		/* CLEAR CC_CHK_EN Register */
		SDEC_HAL_CCCheckEnableSet(org_eCh, 0, 0);
		SDEC_HAL_CCCheckEnableSet(org_eCh, 1, 0);
	}

	SDEC_InputPortEnable(core, eInputPath,	ui8En);

	/* save input mode for polling check SDEC stuck */
	if(SDEC_IS_NORMAL_CHANNEL(eCh))	stpSdecParam->eInputPath[org_eCh] = eInputPath;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_InputPortEnable");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   set  SDEC input port enable and disable select
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SelectPVRSource
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_DL_SEL_T *stpLXSdecDlSel)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT8 core = 0, org_ch = 0;
	int ret = RET_ERROR;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecDlSel == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	ui8Ch = stpLXSdecDlSel->eCh;
	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SelectPVRSource [%d][%d]", ui8Ch, (UINT32)(stpLXSdecDlSel->eSrc));

	/* For CDIC2 Download for SDT set */
	/* H14_TBD, M14_TBD, set source from sdec0,1 should be inserted, jinhwan.bae 2013. 06. 15 */
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC2 )
		{
			/* SDT S/W Parser Case, CDIC2 -> DL */
			/* set TOP Reg, PVR source as SDEC (PVR DN CH, SDEC0 or 1 or SENC) */
			if(core == 0) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_0);
			else if(core == 1) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_1);
			else SDEC_DTV_SOC_Message( SDEC_ERROR, "%s, %d core is invalid(%d) chanel(%d)",__FUNCTION__,__LINE__,core, org_ch);
			/* set PVR SDEC source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(core, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC2 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(core, ui8Ch, 1);
		}
		else if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC )
		{
			/* ALL DN Case, CDIC3 -> DL */
			/* set TOP Reg, PVR source as SDEC (PVR DN CH, SDEC0 or 1 or SENC) */
			if(core == 0) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_0);
			else if(core == 1) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_1);
			else SDEC_DTV_SOC_Message( SDEC_ERROR, "%s, %d core is invalid(%d) chanel(%d)",__FUNCTION__,__LINE__,core, org_ch);
			/* set PVR source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(core, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC3 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(core, ui8Ch, 0);
		}
		else
		{
			/* set PVR source as SDEC Core , Normal PVR Case  */
			/* set TOP Reg, PVR source as SDEC (PVR DN CH, SDEC0 or 1 or SENC) */
			if(core == 0) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_0);
			else if(core == 1) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_1);
			else SDEC_DTV_SOC_Message( SDEC_ERROR, "%s, %d core is invalid(%d) chanel(%d)",__FUNCTION__,__LINE__,core, org_ch);
			ret = SDEC_HAL_CIDC3DlConf(core, ui8Ch, 0);
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC2 )
		{
			/* SDT S/W Parser Case, CDIC2 -> DL */
			/* set TOP Reg, PVR source as SDEC (PVR DN CH, SDEC0 or 1 or SENC) */
			if(core == 0) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_0);
			else if(core == 1) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_1);
			else SDEC_DTV_SOC_Message( SDEC_ERROR, "%s, %d core is invalid(%d) chanel(%d)",__FUNCTION__,__LINE__,core, org_ch);
			/* set PVR SDEC source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(core, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC2 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(core, ui8Ch, 1);
		}
		else if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC )
		{
			/* ALL DN Case, CDIC3 -> DL */
			/* set TOP Reg, PVR source as SDEC (PVR DN CH, SDEC0 or 1 or SENC) */
			if(core == 0) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_0);
			else if(core == 1) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_1);
			else SDEC_DTV_SOC_Message( SDEC_ERROR, "%s, %d core is invalid(%d) chanel(%d)",__FUNCTION__,__LINE__,core, org_ch);
			/* set PVR source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(core, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC3 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(core, ui8Ch, 0);
		}
		else
		{
			/* set PVR source as SDEC Core , Normal PVR Case  */
			/* set TOP Reg, PVR source as SDEC (PVR DN CH, SDEC0 or 1 or SENC) */
			if(core == 0) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_0);
			else if(core == 1) ret = SDEC_HAL_SetPVRSrc(ui8Ch, E_SDEC_DN_OUT_FROM_CORE_1);
			else SDEC_DTV_SOC_Message( SDEC_ERROR, "%s, %d core is invalid(%d) chanel(%d)",__FUNCTION__,__LINE__,core, org_ch);
			ret = SDEC_HAL_CIDC3DlConf(core, ui8Ch, 0);
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		/* H13 and M14A0 is same, fix to CORE 0  */
		if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC2 )
		{
			/* set PVR source as SDEC (SDEC / SENC) */
			ret = SDEC_HAL_SetPVRSrc(ui8Ch, LX_SDEC_FROM_SDEC);
			/* set PVR SDEC source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(_SDEC_CORE_0, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC2 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(_SDEC_CORE_0, ui8Ch, 1);
		}
		else if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC )
		{
			/* set PVR source as SDEC (SDEC / SENC) */
			ret = SDEC_HAL_SetPVRSrc(ui8Ch, LX_SDEC_FROM_SDEC);
			/* set PVR source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(_SDEC_CORE_0, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC3 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(_SDEC_CORE_0, ui8Ch, 0);
		}
		else
		{
			/* set PVR source as SDEC Core */
			ret = SDEC_HAL_CIDC3DlConf(_SDEC_CORE_0, ui8Ch, 0);
			ret = SDEC_HAL_SetPVRSrc(ui8Ch, stpLXSdecDlSel->eSrc);
		}
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC2 )
		{
			/* set PVR source as SDEC (SDEC / SENC) */
			ret = SDEC_HAL_SetPVRSrc(ui8Ch, LX_SDEC_FROM_SDEC);
			/* set PVR SDEC source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(_SDEC_CORE_0, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC2 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(_SDEC_CORE_0, ui8Ch, 1);
		}
		else if( stpLXSdecDlSel->eSrc == LX_SDEC_FROM_CDIC )
		{
			/* set PVR source as SDEC (SDEC / SENC) */
			ret = SDEC_HAL_SetPVRSrc(ui8Ch, LX_SDEC_FROM_SDEC);
			/* set PVR source as CDIC3 (SDEC(A,B) / CDIC3) */
			ret = SDEC_HAL_CIDC3DlConf(_SDEC_CORE_0, ui8Ch, 1);
			/* set PVR SDEC CDIC source as CDIC3 (CDIC3 / CDIC2) */
			ret = SDEC_HAL_CDIC2DlExtConf(_SDEC_CORE_0, ui8Ch, 0);
		}
		else
		{
			/* set PVR source as SDEC Core */
			ret = SDEC_HAL_CIDC3DlConf(_SDEC_CORE_0, ui8Ch, 0);
			ret = SDEC_HAL_SetPVRSrc(ui8Ch, stpLXSdecDlSel->eSrc);
		}
	}

	LX_SDEC_CHECK_CODE( ret != RET_OK, goto exit, "error in HAL" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SelectPVRSource");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   set input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_InputSet
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SEL_INPUT_T *stpLXSdecSelPort)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0, org_ch = 0, core = 0;
	CDIC cdic;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_InputSet");

	ui8Ch = stpLXSdecSelPort->eCh;
	org_ch = ui8Ch;

	if(lx_chip_rev() < LX_CHIP_REV(M14, B0))
	{
		/* 20140103 jinhwan.bae for UHD Live Bypass Setting with Channel */
		if(lx_chip_plt() != LX_CHIP_PLT_UHD)
		{
			if(ui8Ch > 3) 
			{
				eRet = OK;
				goto exit;
			}
		}
	}

	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	/* checking channel sanity */
	if( ui8Ch >= LX_SDEC_CH_NUM)
	{
		SDEC_DEBUG_Print("Invalid channel:[%d]", stpLXSdecSelPort->eCh);
		goto exit;
	}

	/* set variable 0 */
	memset( &cdic, 0, sizeof(cdic));

	//cdic.rst		= 1;
	//cdic.min_sb_det = 3;
	//cdic.max_sb_drp = 3;
	SDEC_HAL_CIDCMinSyncByteDetection(core, ui8Ch, 3);
	SDEC_HAL_CIDCMaxSyncByteDrop(core, ui8Ch, 3);

	switch (stpLXSdecSelPort->eInputPath)
	{
		case LX_SDEC_INPUT_SERIAL0:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] SERIAL0", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_CDIN0);
			break;
		case LX_SDEC_INPUT_SERIAL1:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] SERIAL1", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_CDINA);
			break;
		case LX_SDEC_INPUT_SERIAL2:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] SERIAL2", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_CDIN3);
			break;
		case LX_SDEC_INPUT_SERIAL3:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] SERIAL3", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_CDIN4);
			break;
		case LX_SDEC_INPUT_PARALLEL0:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] PARALLEL0", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_CDIN0);
			break;
		case LX_SDEC_INPUT_PARALLEL1:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] PARALLEL1", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_CDINA);
			break;
		case LX_SDEC_INPUT_DVR:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] DVR", stpLXSdecSelPort->eCh);
			if(ui8Ch == LX_SDEC_CH_B)
				SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_UPLOAD1);
			else
				SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_UPLOAD0);

			// jinhwan.bae for bypass all upload data without sync check packet drop. 20130726, found by JP model
			// upload packet started with Video PES header, 3 TP dropped, causes 1 GOP drop.
			// fixed without sync checking, 0 packet used to detect sync
			SDEC_HAL_CIDCMinSyncByteDetection(core, ui8Ch, 0);
			
			break;
		case LX_SDEC_INPUT_BDRC:
			SDEC_DTV_SOC_Message( SDEC_IO, "Ch[%d] DVR", stpLXSdecSelPort->eCh);
			SDEC_HAL_CIDCSetSrc(core, ui8Ch, E_SDEC_CDIC_SRC_GPB);
			break;
		default:
				SDEC_DEBUG_Print("Invalid input path:[%d]", stpLXSdecSelPort->eInputPath);
				goto exit;
			break;
	}

	SDEC_DTV_SOC_Message( SDEC_IO, "Reset");

	if(stpLXSdecSelPort->bPortReset == TRUE)
	{
		/* disable sdec input port */
		SDEC_InputPortEnable(core, stpLXSdecSelPort->eInputPath,	SDEC_HAL_DISABLE);
		OS_UsecDelay(10);
		SDEC_InputPortReset(org_ch);
		OS_UsecDelay(10);
		/* enable sdec input port */
		SDEC_InputPortEnable(core, stpLXSdecSelPort->eInputPath,	SDEC_HAL_ENABLE);
	}

	/* jinhwan.bae added 2013. 02. 19 work around for CDIF_FULL, MWF_OVF, in US mode reset was done with SERIAL (default value)
	    if SetInputConfig(ENABLE) is not called, no input is defined stpSdecParam->eInputPath[ch], default serial is set  */
	/* save input mode for polling check SDEC stuck */
	if( stpLXSdecSelPort->eCh < LX_SDEC_CH_C )	stpSdecParam->eInputPath[stpLXSdecSelPort->eCh] = stpLXSdecSelPort->eInputPath;

	SDEC_DTV_SOC_Message( SDEC_IO, "Done");
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_InputSet");

	eRet = OK;

exit:
	return (eRet);
}

#if 0
/**
********************************************************************************
* @brief
*   cdip conf set.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8CdipIdx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_CdipConfSet
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8SettingIdx,
	LX_SDEC_STREAMMODE_T eSdecStreamMode)
{
	DTV_STATUS_T eRet = NOT_OK;
	CDIP stCdip;
	UINT8 ui8CdipIdx = 0;

	static  S_SDEC_CDIP_CONF_SET_T staSdecCdipConfSet[] = {
#if 0
	       //serial 0
		{E_SDEC_CDIP_3, E_SDEC_CDIP_ENABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_ENABLE, E_SDEC_CDIP_VAL_ENABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIP_SERIAL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x14, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_POS_SOP, E_SDEC_CDIP_ATSC}, //CDIP3
#endif /* #if 0 */
		//serial 1

#if 0
		{E_SDEC_CDIOP_0, E_SDEC_CDIP_ENABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_ENABLE, E_SDEC_CDIP_VAL_ENABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIOP_VAL_SEL_2, E_SDEC_CDIOP_SERIAL_0, 0x0, 0x14, 0x0, 0x0, E_SDEC_CDIP_POS_SOP, E_SDEC_CDIP_ATSC},//CDIOP0
#endif /* #if 0 */

		//parallel 0
		{E_SDEC_CDIP_0, E_SDEC_CDIP_ENABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_DISABLE, E_SDEC_CDIP_VAL_ENABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIP_PARALLEL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x14, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_47DETECTION, E_SDEC_CDIP_ATSC},//CDIP0
		{E_SDEC_CDIP_1, E_SDEC_CDIP_DISABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_DISABLE, E_SDEC_CDIP_VAL_DISABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIP_SERIAL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x8, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_47DETECTION, E_SDEC_CDIP_ATSC},//CDIP1
		{E_SDEC_CDIP_2, E_SDEC_CDIP_DISABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_DISABLE, E_SDEC_CDIP_VAL_DISABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIP_SERIAL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x8, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_47DETECTION, E_SDEC_CDIP_ATSC},//CDIP2
		 //serial 0
		{E_SDEC_CDIP_3, E_SDEC_CDIP_ENABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_DISABLE, E_SDEC_CDIP_VAL_ENABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIP_SERIAL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x14, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_POS_SOP, E_SDEC_CDIP_ATSC}, //CDIP3
		 //serial 2
		{E_SDEC_CDIP_3, E_SDEC_CDIP_ENABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_LOW,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_DISABLE, E_SDEC_CDIP_VAL_DISABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIP_SERIAL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x14, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_POS_SOP, E_SDEC_CDIP_ATSC}, //CDIP3
	};


	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	if( ui8SettingIdx > 3 ) 	ui8CdipIdx = 3;
	else						ui8CdipIdx = ui8SettingIdx;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_CdipConfSet");

	stCdip = stpSdecParam->stSDEC_IO_Reg->cdip[ui8SettingIdx];

	stCdip.dtype      		= eSdecStreamMode;//staSdecCdipConfSet[ui8SettingIdx].eSdecCdipDtype;
	stCdip.sync_type  	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipSyncType;
	//reg_sdec_io->cdip[SdecCdipIdx].f.ba_val_dis 		= staSdecCdipConfSet[stpSdecParam->ui8InputSetIdx].eSdecCdipBaSop;
	//reg_sdec_io->cdip[eSdecCdipIdx].f.ba_val_dis 		= staSdecCdipConfSet[stpSdecParam->ui8InputSetIdx].eSdecCdipBaVal;
	stCdip.clk_div    		= staSdecCdipConfSet[ui8SettingIdx].ui32ClkDiv;
	//reg_sdec_io->cdip[eSdecCdipIdx].f.ba_clk_dis	 	= eSdecCdipBaClk;
	stCdip.pconf      		= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipPconf;
	stCdip.req_en    	 	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipReqEn;
	stCdip.val_en     	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipValEn;
	stCdip.err_en     	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipErrEn;
	stCdip.req_act_lo 	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipReqPol;
	stCdip.val_act_lo 	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipValPol;
	stCdip.clk_act_lo 	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipClkPol;
	stCdip.err_act_hi 	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipErrPol;
	stCdip.test_en    	= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipTestEn;
	stCdip.en         		= staSdecCdipConfSet[ui8SettingIdx].eSdecCdipEn;

	stpSdecParam->stSDEC_IO_Reg->cdip[ui8CdipIdx] = stCdip;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_CdipConfSet");

	eRet = OK;

exit:
	return (eRet);
}

DTV_STATUS_T SDEC_CdiopConfInSet
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8CdipIdx,
	LX_SDEC_STREAMMODE_T eSdecStreamMode)
{
	DTV_STATUS_T eRet = NOT_OK;
	CDIOP stCdiop[2];

	static  S_SDEC_CDIP_CONF_IN_SET_T staSdecCdipConfInSet[] = {
	{E_SDEC_CDIOP_0, E_SDEC_CDIP_ENABLE, E_SDEC_CDIP_TEST_DISABLE, E_SDEC_CDIP_ERR_ACT_LOW, E_SDEC_CDIP_CLK_ACT_HIGH,
		  E_SDEC_CDIP_VAL_ACT_HIGH, E_SDEC_CDIP_REQ_ACT_HIGH, E_SDEC_CDIP_ERR_DISABLE, E_SDEC_CDIP_VAL_ENABLE, E_SDEC_CDIP_REQ_DISABLE,
		  E_SDEC_CDIOP_VAL_SEL_2, E_SDEC_CDIOP_SERIAL_0, E_SDEC_CDIP_BA_CLK_ENABLE, 0x14, E_SDEC_CDIP_BA_VAL_ENABLE, E_SDEC_CDIP_BA_SOP_ENABLE, E_SDEC_CDIP_POS_SOP, E_SDEC_CDIP_ATSC}//CDIOP0
	};

	if (stpSdecParam == NULL)
	{
		SDEC_DEBUG_Print("Invalid parameters");
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_CdipConfInSet");

	stCdiop[ui8CdipIdx] = stpSdecParam->stSDEC_IO_Reg->cdiop[ui8CdipIdx];

	stCdiop[ui8CdipIdx].dtype      	= eSdecStreamMode;//staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipDtype;
	stCdiop[ui8CdipIdx].sync_type  	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipSyncType;
	//reg_sdec_io->cdip[SdecCdipIdx].f.ba_val_dis 		= staSdecCdipConfSet[stpSdecParam->ui8InputSetIdx].eSdecCdipBaSop;
	//reg_sdec_io->cdip[eSdecCdipIdx].f.ba_val_dis 		= staSdecCdipConfSet[stpSdecParam->ui8InputSetIdx].eSdecCdipBaVal;
	stCdiop[ui8CdipIdx].clk_div    		= staSdecCdipConfInSet[ui8CdipIdx].ui32ClkDiv;
	//reg_sdec_io->cdip[eSdecCdipIdx].f.ba_clk_dis	 	= eSdecCdipBaClk;
	stCdiop[ui8CdipIdx].pconf      		= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipPconf;
	stCdiop[ui8CdipIdx].req_en    	 	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipReqEn;
	stCdiop[ui8CdipIdx].val_en     	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipValEn;
	stCdiop[ui8CdipIdx].err_en     	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipErrEn;
	stCdiop[ui8CdipIdx].req_act_lo 	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipReqPol;
	stCdiop[ui8CdipIdx].val_act_lo 	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipValPol;
	stCdiop[ui8CdipIdx].clk_act_lo 	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipClkPol;
	stCdiop[ui8CdipIdx].err_act_hi 	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipErrPol;
	stCdiop[ui8CdipIdx].test_en    	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipTestEn;
	stCdiop[ui8CdipIdx].en         	= staSdecCdipConfInSet[ui8CdipIdx].eSdecCdipEn;


	stpSdecParam->stSDEC_IO_Reg->cdiop[ui8CdipIdx] = stCdiop[ui8CdipIdx];

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_CdipConfInSet");

	eRet = OK;

exit:
	return (eRet);
}
#endif

/**
********************************************************************************
* @brief
*   SDEC core input port setting.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterAlloc
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_ALLOC_T *stpLXSdecPIDFltAlloc)
{
	DTV_STATUS_T eRet = NOT_OK, eResult = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT8 ui8PidIdx = 0x0;
	UINT16 ui16Pid = 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltAlloc == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterAlloc");

	ui8Ch 	= stpLXSdecPIDFltAlloc->eCh;
	ui16Pid	= stpLXSdecPIDFltAlloc->uiPidValue;

	eResult = SDEC_PIDIdxCheck(stpSdecParam, ui8Ch, &ui8PidIdx, stpLXSdecPIDFltAlloc->ePidFltMode, ui16Pid);
	LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_SelPidFilterIdx failed:[%d]", eResult);
	
	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] returned PID Idx:[%d]", ui8Ch, ui8PidIdx);

	stpLXSdecPIDFltAlloc->uiPidFltId = ui8PidIdx;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterAlloc");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC PID idx check.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8pPidIdx :return pid idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_PIDIdxCheck
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch,
	UINT8 *ui8pPidIdx,
	LX_SDEC_PIDFLT_MODE_T ePidFltMode,
	UINT16 ui16PidValue )
{
	DTV_STATUS_T 	eRet 			= NOT_OK;
	UINT8 			ui8Count 		= 0x0;
	UINT8 			ui8PidfNum		= 0x0;
	UINT8 			ui8FltMode 	= 0x0;
	BOOLEAN 		bFind		 	= FALSE;
	UINT32			ui32PidfData	= 0x0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;
	UINT8			core = 0, org_ch = ui8ch;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_PIDIdxCheck");

	SDEC_CONVERT_CORE_CH(core, ui8ch);

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	ui8FltMode = ePidFltMode;

	if( org_ch >= pSdecConf->nChannel )
	{
		SDEC_DEBUG_Print("over channel range %d", org_ch);
		goto exit;
	}

	/* get pid filter number from channel info structure */
	ui8PidfNum = pSdecConf->chInfo[org_ch].num_pidf;

	/* Check CH_C */
	if( pSdecConf->chInfo[org_ch].capa_lev == 0)
	{
		for(ui8Count = 0; ui8Count < ui8PidfNum; ui8Count++)
		{

			/* Check if there is same pid filter */
			ui32PidfData = SDEC_HAL_CDIC2PIDFGetPidfData(core, ui8Count);
			if( ui16PidValue == ( ui32PidfData & 0x1FFF ) )
			{
				SDEC_DEBUG_Print( "Same PID is exist!!!! pidval[%04x] idx[%2d]", ui16PidValue, ui8Count );

				/* if there is same pid filter, clear!! */
				SDEC_Pidf_Clear(stpSdecParam, org_ch, ui8Count);
				stpSdecParam->stPIDMap[org_ch][ui8Count].used = 0x0;
				stpSdecParam->stPIDMap[org_ch][ui8Count].flag = FALSE;
				stpSdecParam->stPIDMap[org_ch][ui8Count].mode = 0x0;
				stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_FREE = 0x0;
				stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x0;
				stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_OVERFLOW = 0x0;
				stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x0;
			}

			if( (stpSdecParam->stPIDMap[org_ch][ui8Count].used) == 0x0)
			{
				stpSdecParam->stPIDMap[org_ch][ui8Count].used = 0x1;
				stpSdecParam->stPIDMap[org_ch][ui8Count].mode = ui8FltMode;
				stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x1;
				*ui8pPidIdx = ui8Count;
				bFind = TRUE;

				break;
			}
		}

		goto check_find;
	}

	for(ui8Count = 0; ui8Count < ui8PidfNum; ui8Count++)
	{

		ui32PidfData = SDEC_GetPidfData(stpSdecParam, org_ch, ui8Count);

		if( ui16PidValue == ( ( ui32PidfData >> 16 ) & 0x1FFF) )
		{
			SDEC_DEBUG_Print( "Same PID is exist!!!! pidval[%04x] idx[%2d]", ui16PidValue, ui8Count );

			/* if there is same pid filter, clear!! */
			SDEC_Pidf_Clear(stpSdecParam, org_ch, ui8Count);
			stpSdecParam->stPIDMap[org_ch][ui8Count].used = 0x0;
			stpSdecParam->stPIDMap[org_ch][ui8Count].flag = FALSE;
			stpSdecParam->stPIDMap[org_ch][ui8Count].mode = 0x0;
			stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_FREE = 0x0;
			stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x0;
			stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_OVERFLOW = 0x0;
			stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x0;
		}

		if( (stpSdecParam->stPIDMap[org_ch][ui8Count].used) == 0x0)
		{
			stpSdecParam->stPIDMap[org_ch][ui8Count].used = 0x1;
			stpSdecParam->stPIDMap[org_ch][ui8Count].mode = ui8FltMode;
			stpSdecParam->stPIDMap[org_ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x1;
			*ui8pPidIdx = ui8Count;
			bFind = TRUE;
			
			break;
		}
	}

check_find:
	if(bFind == FALSE )
	{
		SDEC_DEBUG_Print( RED_COLOR"Ch[%d] PID filter alloc failed"NORMAL_COLOR, org_ch);
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_PIDIdxCheck");

	eRet = OK;

exit:
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Sec idx check.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8pPidIdx :return Sec idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_SecIdxCheck
	(S_SDEC_PARAM_T *stpSdecParam,
	UINT8 ui8ch,
	UINT8 *ui8pSecIdx)
{
	DTV_STATUS_T 	eRet = NOT_OK;

	UINT8 			ui8SecfNum		= 0x0;
	UINT8 			ui8Count 		= 0x0;
	BOOLEAN 		bBufFullFlag 	= FALSE;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_SecIdxCheck");

	/* get chip configuation */
	pSdecConf = SDEC_CFG_GetConfig();

	if( ui8ch >= pSdecConf->nChannel )
	{
		SDEC_DEBUG_Print("over channel range %d", ui8ch);
		goto exit;
	}

	/* get section filter number from channel info structure */
	ui8SecfNum = pSdecConf->chInfo[ui8ch].num_secf;

	if( ui8ch < LX_SDEC_CH_C )
	{
		/* gaius.lee 2011.06.08
		 * set this because of problem in L8 & L9 A0
		 * there is h/w issue in GPB0, so we shall not use GPB0
		 */
		//if (lx_chip_rev() <= LX_CHIP_REV(L9, A1)) 	ui8Count = 1;
		//else											ui8Count = 0;
		/* gaius.lee 2012.01.05
		 * upper issue is still in L9 B0
		 */
		ui8Count = 1;

		//sec filter map check
		for(; ui8Count < ui8SecfNum; ui8Count++)
		{
			if((stpSdecParam->stSecMap[ui8ch][ui8Count].used) == 0x0)
			{
				stpSdecParam->stSecMap[ui8ch][ui8Count].used = 0x1;
				//empty GPB idx set
				//stpSdecParam->stSecMap[ui8ch][ui8Count].flag = TRUE;
				stpSdecParam->stSecMap[ui8ch][ui8Count].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x1;

				*ui8pSecIdx = ui8Count;

				bBufFullFlag = TRUE;

				break;
			}
		}
	}
	else
	{
		stpSdecParam->stSecMap[ui8ch][0].used = 0x1;
		stpSdecParam->stSecMap[ui8ch][0].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x1;
		*ui8pSecIdx = 0;
		bBufFullFlag = TRUE;
	}

	//allock fail, buffer full
	if(bBufFullFlag  != TRUE)
	{
		SDEC_DEBUG_Print( RED_COLOR"Ch[%d] Section  filter allock failed"NORMAL_COLOR, ui8ch);
		goto exit;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_SecIdxCheck");

	eRet = OK;

exit:
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC Check Available Section Filter Number
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui8pPidIdx :return Sec idx
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterGetAvailableNumber
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_GET_AVAIL_NUMBER_T *stpLXSdecSecFltAvailNum)
{
	DTV_STATUS_T 	eRet = NOT_OK;
	UINT8 			ui8SecfNum		= 0x0;
	UINT8 			ui8Count 		= 0x0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;
	UINT8 			ui8Ch = 0x0;
	UINT32 			uiAvailFilter = 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecFltAvailNum == NULL, return INVALID_ARGS, "Invalid argument" );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterGetAvailableNumber");

	ui8Ch = (UINT8)stpLXSdecSecFltAvailNum->eCh;

	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);

	/* get section filter number from channel info structure */
	ui8SecfNum = pSdecConf->chInfo[ui8Ch].num_secf;

	if( SDEC_IS_NORMAL_CHANNEL((LX_SDEC_CH_T)ui8Ch) )
	{
		/* gaius.lee 2011.06.08
		 * set this because of problem in L8 & L9 A0
		 * there is h/w issue in GPB0, so we shall not use GPB0
		 */
		ui8Count = 1;

		/* check section filter map */
		for(; ui8Count < ui8SecfNum; ui8Count++)
		{
			if((stpSdecParam->stSecMap[ui8Ch][ui8Count].used) == 0x0)
			{
				uiAvailFilter++;
			}
		}
	}
	else if( ( (LX_SDEC_CH_T)ui8Ch == LX_SDEC_CH_C) || ((LX_SDEC_CH_T)ui8Ch == LX_SDEC_CH_G) ) 
	{
		uiAvailFilter = 1;
	}
	else
	{
		/* CH_D, CH_H case */
		uiAvailFilter = 0;
	}

	/* set return value */
	stpLXSdecSecFltAvailNum->uiAvailFilter = uiAvailFilter;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterGetAvailableNumber");
	eRet = OK;

exit:
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC PID filter free.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterFree
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_FREE_T *stpLXSdecPIDFltFree)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltFree == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterFree");

	ui8Ch = stpLXSdecPIDFltFree->eCh;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
		
	ui32PidFltId = stpLXSdecPIDFltFree->uiPidFltId;
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "over pid filter range %d", ui32PidFltId);

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] PidFltId[%d] Free", ui8Ch, ui32PidFltId);

	SDEC_Pidf_Clear(stpSdecParam, ui8Ch, ui32PidFltId);

	//PID Map init
	stpSdecParam->stPIDMap[ui8Ch][ui32PidFltId].used = 0x0;
	stpSdecParam->stPIDMap[ui8Ch][ui32PidFltId].flag = FALSE;
	stpSdecParam->stPIDMap[ui8Ch][ui32PidFltId].mode = 0x0;
	stpSdecParam->stPIDMap[ui8Ch][ui32PidFltId].stStatusInfo.w = 0x0;
	stpSdecParam->stPIDMap[ui8Ch][ui32PidFltId].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x0;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterFree");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   Decide Packet is Clear or Not
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32PidValue : PID
* @return
*   TRUE or FALSE
********************************************************************************
*/
static BOOLEAN _SDEC_Is_JPClearPacket(UINT32 ui32PidValue)
{
	/* If PAT and DVB-SI Table, it's clear in JP - get the confirmation from Tokyo Lab. */
	if( (ui32PidValue == 0x00) || (ui32PidValue == 0x01) || (ui32PidValue == 0x02) || (ui32PidValue == 0x10)
		|| (ui32PidValue == 0x11) || (ui32PidValue == 0x12) || (ui32PidValue == 0x13) || (ui32PidValue == 0x14)
		|| (ui32PidValue == 0x16) || (ui32PidValue == 0x1E) || (ui32PidValue == 0x1F) )
	{
		return TRUE;
	}

	return FALSE;
}


/**
********************************************************************************
* @brief
*   SDEC PID filter set.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterSetPID
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_SET_PID_T *stpLXSdecPIDFltSetPID)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0, ui32PidValue = 0x0, ui32PidfData = 0x0, ui32PidfDest = DEST_RESERVED, ui32dest= 0x0;
	LX_SDEC_CFG_T* pSdecConf = NULL;
	BOOLEAN	bSection = FALSE, bEnable = TRUE;
	BOOLEAN bClearPacket = FALSE; // JP WorkAround of MCU WorkAround
	UINT8 core = 0, org_ch = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltSetPID == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterSetPID");

	ui8Ch = stpLXSdecPIDFltSetPID->eCh;
	ui32PidFltId = stpLXSdecPIDFltSetPID->uiPidFltId;
	ui32PidValue = stpLXSdecPIDFltSetPID->uiPidValue;
	ui32dest = stpLXSdecPIDFltSetPID->uiDest;
	bSection = stpLXSdecPIDFltSetPID->bSection;
	bEnable = stpLXSdecPIDFltSetPID->bEnable;

	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "filter type:[%08x]", ui32dest);

	/* check channel number */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( org_ch >= pSdecConf->nChannel, goto exit, "over channel range %d", org_ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "PID Filter Index Error [%d]", ui32PidFltId);

	/* from H13 A0, CDIC2 has 4 pid filters */
	if( pSdecConf->chInfo[org_ch].capa_lev == 0 )
	{
		SDEC_HAL_CDIC2PIDFSetPidfData(core, ui32PidFltId, ui32PidValue);
		SDEC_HAL_CDIC2PIDFEnablePidFilter(core, ui32PidFltId, SDEC_HAL_ENABLE);	/* seperate enable as L9 correction */
		SDEC_SWP_SetPID(ui32PidFltId, ui32PidValue);
		goto exit_without_error;
	}

	/* setting PID */
	ui32PidfData = PID(ui32PidValue);

	if(ui32dest & LX_SDEC_PIDFLT_DEST_VDEC0)
	{
		ui32PidfData |= PAYLOAD_PES;
		ui32PidfDest = VID_DEV0;
		SDEC_HAL_STCCSetVideoAssign(core, 0, ui8Ch);
		if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			SDEC_HAL_SetVideoOut(0, core);
		}
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] VID0 PidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}
	else if(ui32dest & LX_SDEC_PIDFLT_DEST_VDEC1)
	{
		ui32PidfData |= PAYLOAD_PES;
		ui32PidfDest = VID_DEV1;
		SDEC_HAL_STCCSetVideoAssign(core, 1, ui8Ch);
		if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			SDEC_HAL_SetVideoOut(1, core);
		}
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] VID1 PidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}
	else if(ui32dest & LX_SDEC_PIDFLT_DEST_ADEC0)
	{
		ui32PidfData |= PAYLOAD_PES;
		ui32PidfDest = AUD_DEV0;
		SDEC_HAL_STCCSetAudioAssign(core, 0, ui8Ch);
		if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			SDEC_HAL_SetAudioOut(0, core);
		}
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] AUD0 PidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}
	else if(ui32dest & LX_SDEC_PIDFLT_DEST_ADEC1)
	{
		ui32PidfData |= PAYLOAD_PES;
		ui32PidfDest = AUD_DEV1;
		SDEC_HAL_STCCSetAudioAssign(core, 1, ui8Ch);
		if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
			SDEC_HAL_SetAudioOut(1, core);
		}
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] AUD1 PidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}
	else if(ui32dest & LX_SDEC_PIDFLT_DEST_GPB)
	{
		ui32PidfDest = DEST_RESERVED;
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] GPBPidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}
	else if(ui32dest & LX_SDEC_PIDFLT_DEST_RAWTS)
	{
		//for saving raw ts
		ui32PidfData |= TS_DN;
		ui32PidfDest = DEST_RESERVED;
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] GPBPidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}
	else if(ui32dest & LX_SDEC_PIDFLT_DEST_OTHER)
	{
		ui32PidfDest = DEST_RESERVED;
		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] GPBPidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}

	if( ui32dest & LX_SDEC_PIDFLT_DEST_DVR )
	{
		/* To delete PAYLOAD_PES for Section Download - for JP 2013.02.04 jinhwan.bae */
		//ui32PidfData |= PAYLOAD_PES;
		/* jinhwan.bae 2013. 02.17
		    Purpose : Support Section Download and Parsing at the same time, for netcast4.0 JP
		    Reason  : To support this, remove PLOAD_PES at download set pid.
		                  But Video Data is inserted to Section GPB
		    WorkAround : To divide set value as Section/PES type.
		                       If PES, set PLOAD_PES at download set pid. so need flag bSection */
		if(bSection == TRUE)
		{
			/* Not Insert PLOAD_PES at Download */
		}
		else
		{
			/* Insert PLOAD_PES at Download */
			ui32PidfData |= PAYLOAD_PES;
		}
		
		ui32PidfData |= DL_EN;

		SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] DVR PidFltId[%d] PidValue[0x%08x]", org_ch, ui32PidFltId, ui32PidValue);
	}

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] ui32PidfData[0x%08x] ui32PidfDest[0x%08x]", org_ch, ui32PidfData, ui32PidfDest);

	if(bEnable == TRUE)
	{
		// if( (stpSdecParam->ui8McuDescramblerCtrlMode == 1) && (ui8Ch == 1) && (ui32PidValue == 0x0))  /* in JP , PAT, CH_B */
		// jinhwan.bae for MCU Test H13 JP WorkAround
		// Channel 1 's triggering : PSI Section, so TPI_IEN is needed to send MCU the timimg to set key . cf. CH0 -> PCR
		/* 20140213 jinhwan.bae webOS adaptation
		    TPI_IEN is needed only at the download channel.
		    There are lots of way, but simplicity, only check DL_EN */
		if(stpSdecParam->ui8McuDescramblerCtrlMode == 1) // JP Mode
		{
			bClearPacket = _SDEC_Is_JPClearPacket(ui32PidValue);
#if 0			
			if((org_ch == 1) && (bClearPacket == TRUE))
#else
			if((ui32PidfData & DL_EN) && (bClearPacket == TRUE))
#endif
			{
				SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Set DN Channel TPI_IEN for MCU Descrambler Ch[%d] ui32PidFltId[%d] ui32PidfData[0x%08x]", org_ch, ui32PidFltId, ui32PidfData);
				SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, DEC_EN | TPI_IEN | ui32PidfData | ui32PidfDest );
			}
			else
			{
				SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, DEC_EN | ui32PidfData | ui32PidfDest );
			}
		}
		else
		{
			/* 20140324 tvct problem */
			if( (stpSdecParam->ui8ClearTVCTGathering == 1) && (ui32PidValue == TVCT_PID) )
			{
				SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, DEC_EN | NO_DSC | ui32PidfData | ui32PidfDest );
			}
			else
			{
			// same as previous version
			SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, DEC_EN | ui32PidfData | ui32PidfDest );
		}
	}
	}
	else
	{
		// if( (stpSdecParam->ui8McuDescramblerCtrlMode == 1) && (ui8Ch == 1) && (ui32PidValue == 0x0))  /* in JP , PAT, CH_B */
		// jinhwan.bae for MCU Test H13 JP WorkAround
		// Channel 1 's triggering : PSI Section, so TPI_IEN is needed to send MCU the timimg to set key . cf. CH0 -> PCR
		/* 20140213 jinhwan.bae webOS adaptation
		    TPI_IEN is needed only at the download channel.
		    There are lots of way, but simplicity, only check DL_EN */
		if(stpSdecParam->ui8McuDescramblerCtrlMode == 1) // JP Mode
		{
			bClearPacket = _SDEC_Is_JPClearPacket(ui32PidValue);
#if 0			
			if((org_ch == 1) && (bClearPacket == TRUE))
#else
			if((ui32PidfData & DL_EN) && (bClearPacket == TRUE))
#endif
			{
				SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Set DN Channel TPI_IEN for MCU Descrambler Ch[%d] ui32PidFltId[%d] ui32PidfData[0x%08x]", org_ch, ui32PidFltId, ui32PidfData);
				SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, TPI_IEN | ui32PidfData | ui32PidfDest );
			}
			else
			{
				SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, ui32PidfData | ui32PidfDest );
			}
		}
		else
		{
			/* 20140324 tvct problem */
			if( (stpSdecParam->ui8ClearTVCTGathering == 1) && (ui32PidValue == TVCT_PID) )
			{
				SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, NO_DSC | ui32PidfData | ui32PidfDest );
			}
			else
			{
			// same as previous version
			SDEC_SetPidfData(stpSdecParam, org_ch, ui32PidFltId, ui32PidfData | ui32PidfDest );
		}
	}
	}

exit_without_error:
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterSetPID");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}


/**
********************************************************************************
* @brief
*   SDEC PID filter map select.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterMapSelect
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_SELSECFLT_T *stpLXSdecPIDFltSelect)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T* 	pSdecConf = NULL;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0, ui32SecFltId = 0x0, pidf_data = 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltSelect == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterMapSelect");

	ui8Ch = stpLXSdecPIDFltSelect->eCh;
	ui32PidFltId = stpLXSdecPIDFltSelect->uiPidFltId;
	ui32SecFltId = stpLXSdecPIDFltSelect->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "PID Filter Index Error Ch[%d]Pidf[%d]Total[%d]", 
							ui8Ch, ui32PidFltId, pSdecConf->chInfo[ui8Ch].num_pidf);

	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	pidf_data = SDEC_GetPidfData(stpSdecParam, ui8Ch, ui32PidFltId);

	/* If PES mode, section filter is not used, But for simple structure, use section filter */
	if(	stpLXSdecPIDFltSelect->eGpbMode == LX_SDEC_PIDFLT_GPB_MODE_PES )
	{
		/* Disconnect section filter connection and remove destination */
		pidf_data &= ~ ( SF_MAN_EN | DEST );

		/* Enable PES_Paylod and link gpd index */
		pidf_data |=  PAYLOAD_PES | ui32SecFltId;

		/* For PES H/W bug workaound. See @LX_SDEC_USE_KTHREAD_PES */
#if	( LX_SDEC_USE_KTHREAD_PES == 1)
		if(pSdecConf->noPesBug == 0 )
		{
			/* if there is pes h/w buf, do it */
			/* added TS_DN. Gather TS packet and parse manually */
			pidf_data |=  PAYLOAD_PES | TS_DN | ui32SecFltId;

			SDEC_PES_SetPESFlt(ui8Ch, ui32SecFltId);
			SDEC_PES_SetDstBuf(stpSdecParam, ui8Ch, ui32SecFltId);
		}
#endif
	}
	else if ( stpLXSdecPIDFltSelect->eGpbMode == LX_SDEC_PIDFLT_GPB_MODE_RAWTS )
	{
		/* Disconnect section filter connection and remove destination */
		pidf_data &= ~ ( SF_MAN_EN | DEST );

		/* Enable PES_Paylod and link gpd index */
		pidf_data |=  PAYLOAD_PES | GPB_IRQ_CONF | TS_DN | ui32SecFltId;
	}
	else
	{
		/* connect section filter connection */
		pidf_data |= SF_MAN_EN;

		SDEC_HAL_SECFSetMapBit(ui8Ch, ui32PidFltId, ui32SecFltId);
	}

	SDEC_SetPidfData(stpSdecParam, ui8Ch, ui32PidFltId, pidf_data);

	stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui8PidFltIdx = ui32PidFltId;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].flag = true;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x1;

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] SecMap true flag idx[%d]", ui8Ch, ui32SecFltId);
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterMapSelect");

	eRet = OK;
	
exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC PID filter enable or disable.
* @remarks
*  Enable is made by jinhwan.bae for fixing L9 Issue at google TV
*  There are some abnormal stream, some EIT have same PID as PAT,
*  So, IF the pid filter were enabled before section filter condition setting,
*  PAT callback is called with EIT payload.
*  the solution is to divide pid filter set and pid filter enable, as I remember at 2013.8.
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterEnable
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltEnable)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	BOOLEAN bPidFltEnable = 0x0;
	UINT8 core = 0, org_ch = 0;
	LX_SDEC_CFG_T* 	pSdecConf 		= NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltEnable == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterEnable");

	ui8Ch = stpLXSdecPIDFltEnable->eCh;
	ui32PidFltId = stpLXSdecPIDFltEnable->uiPidFltId;
	bPidFltEnable = stpLXSdecPIDFltEnable->bPidFltEnable;

	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] FltId[%d] flag[0x%08x]", org_ch, ui32PidFltId, bPidFltEnable);

	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( org_ch >= pSdecConf->nChannel, goto exit, "over channel range %d", org_ch);
	LX_SDEC_CHECK_CODE( ui8Ch == LX_SDEC_CH_D, goto exit, "Channel is Invalid (%d)",ui8Ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[org_ch].num_pidf, goto exit, "CH[%d] PID Filter ID is Invalid (%d) Capa (%d)",
									org_ch, ui32PidFltId, pSdecConf->chInfo[org_ch].num_pidf);
	
	switch (bPidFltEnable)
	{
		case TRUE:

			if( pSdecConf->chInfo[org_ch].capa_lev == 0 )
			{
				SDEC_HAL_CDIC2PIDFEnablePidFilter(core, ui32PidFltId, SDEC_HAL_ENABLE);
			}
			else
            {
				SDEC_SetPidf_Enable(stpSdecParam, org_ch, ui32PidFltId, SDEC_HAL_ENABLE);
            }

			stpSdecParam->stPIDMap[org_ch][ui32PidFltId].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x1;
			stpSdecParam->stPIDMap[org_ch][ui32PidFltId].flag = TRUE;

		break;
		
		case FALSE:

			if( pSdecConf->chInfo[org_ch].capa_lev == 0 )
			{
				SDEC_HAL_CDIC2PIDFEnablePidFilter(core, ui32PidFltId, SDEC_HAL_DISABLE);
			}
			else
			{
				SDEC_SetPidf_Enable(stpSdecParam, org_ch, ui32PidFltId, SDEC_HAL_DISABLE);
			}

			stpSdecParam->stPIDMap[org_ch][ui32PidFltId].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x0;
			stpSdecParam->stPIDMap[org_ch][ui32PidFltId].flag = FALSE;
		break;
		
		default:
			SDEC_DEBUG_Print("Invalid parameter:[%d]", bPidFltEnable);
			goto exit;
		break;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterEnable");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   CRC check scheme of SDEC PID filter enable or disable.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterCRCEnable
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltPESCRCEnable)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	BOOLEAN bPidFltEnable = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltPESCRCEnable == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterCRCEnable");

	ui8Ch = stpLXSdecPIDFltPESCRCEnable->eCh;
	ui32PidFltId = stpLXSdecPIDFltPESCRCEnable->uiPidFltId;
	bPidFltEnable = stpLXSdecPIDFltPESCRCEnable->bPidFltEnable;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "PID Filter Index Error Ch[%d]Pidf[%d]Total[%d]", 
							ui8Ch, ui32PidFltId, pSdecConf->chInfo[ui8Ch].num_pidf);

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] FltId[%d] flag[0x%08x]", ui8Ch, ui32PidFltId, bPidFltEnable);

	if( bPidFltEnable)
		SDEC_HAL_PIDFSetCRCBit(ui8Ch, ui32PidFltId);
	else 
		SDEC_HAL_PIDFClearCRCBit(ui8Ch, ui32PidFltId); 

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterCRCEnable");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC PID filter get state.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterGetState
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_STATE_T *stpLXSdecPIDFltState)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT32	pidfData = 0x0;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	LX_SDEC_CFG_T* pSdecConf = NULL;
	UINT8 core = 0, org_ch = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltState == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterGetState");

	ui8Ch = stpLXSdecPIDFltState->eCh;
	ui32PidFltId = stpLXSdecPIDFltState->uiPidFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "PID Filter Index Error Ch[%d]Pidf[%d]Total[%d]", 
							ui8Ch, ui32PidFltId, pSdecConf->chInfo[ui8Ch].num_pidf);

	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	/* from H13 A0, CDIC2 has 4 pid filters */
	if( pSdecConf->chInfo[org_ch].capa_lev == 0 )
	{
		stpLXSdecPIDFltState->uiPidValue = SDEC_HAL_CDIC2PIDFGetPidfData(core, stpLXSdecPIDFltState->uiPidFltId);
		stpLXSdecPIDFltState->bDec_en	= SDEC_HAL_CDIC2GetPIDFEnable(core, stpLXSdecPIDFltState->uiPidFltId);

		/* jinhwan.bae more information need? */
		goto exit_without_error;
	}

	pidfData = SDEC_GetPidfData(stpSdecParam, org_ch, stpLXSdecPIDFltState->uiPidFltId);

	/* output */
	stpLXSdecPIDFltState->uiPidValue = extract_bits(pidfData, 0x1FFF, 16);
	stpLXSdecPIDFltState->bDec_en	= extract_bits(pidfData, 0x1, 31);
	stpLXSdecPIDFltState->bDl_en	= extract_bits(pidfData, 0x1, 30);
	stpLXSdecPIDFltState->bPload_pes= extract_bits(pidfData, 0x1, 12);
	stpLXSdecPIDFltState->bSf_map_en= extract_bits(pidfData, 0x1, 11);
	stpLXSdecPIDFltState->uiDest	= extract_bits(pidfData, 0x7f, 0);
	stpLXSdecPIDFltState->uiSecf_map[0] = SDEC_HAL_SECFGetMap(org_ch, ui32PidFltId*2);
	stpLXSdecPIDFltState->uiSecf_map[1] = SDEC_HAL_SECFGetMap(org_ch, ui32PidFltId*2 + 1);
	stpLXSdecPIDFltState->uiFltState = stpSdecParam->stPIDMap[org_ch][ui32PidFltId].stStatusInfo.w;
	stpLXSdecPIDFltState->uiRegValue = pidfData;

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] pididx[%d] value[0x%08x]", org_ch, ui32PidFltId, stpLXSdecPIDFltState->uiPidValue);

exit_without_error:
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterGetState");
	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}

/**
********************************************************************************
* @brief
*   SDEC PID filter enable or disable.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterEnableSCMBCHK
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltEnable)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	BOOLEAN bPidFltEnable = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltEnable == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterEnable");

	ui8Ch = stpLXSdecPIDFltEnable->eCh;
	ui32PidFltId = stpLXSdecPIDFltEnable->uiPidFltId;
	bPidFltEnable = stpLXSdecPIDFltEnable->bPidFltEnable;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "PID Filter Index Error Ch[%d]Pidf[%d]Total[%d]", 
							ui8Ch, ui32PidFltId, pSdecConf->chInfo[ui8Ch].num_pidf);

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] FltId[%d] flag[0x%08x]", ui8Ch, ui32PidFltId, bPidFltEnable);

	// jinhwan.bae for No Audio Issue, 2012.04.26 SDEC_HAL_PIDFScrambleCheck(ui8Ch, ui32PidFltId, bPidFltEnable);
	// replaced following spin lock I/F
	SDEC_SetPidf_TPI_IEN_Enable(stpSdecParam, ui8Ch, ui32PidFltId, bPidFltEnable);

	/* enable tp interrupt */
	if(bPidFltEnable)
		SDEC_HAL_TPISetIntrPayloadUnitStartIndicator(ui8Ch, SDEC_HAL_ENABLE);
	else
		SDEC_HAL_TPISetIntrPayloadUnitStartIndicator(ui8Ch, SDEC_HAL_DISABLE);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterEnable");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   Enable download as SDEC PID filter
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PIDFilterEnableDownload
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltEnableDownload)
{
	DTV_STATUS_T eRet = NOT_OK; 
	UINT8 ui8Ch = 0x0;
	UINT32 ui32PidFltId = 0x0;
	UINT8  bEnable = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;
	UINT32 ui32PidValue = 0x0, pidfData = 0x0;
	BOOLEAN bClearPacket = FALSE; // JP WorkAround of MCU WorkAround

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecPIDFltEnableDownload == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PIDFilterEnableDownload");

	ui8Ch = stpLXSdecPIDFltEnableDownload->eCh;
	ui32PidFltId = stpLXSdecPIDFltEnableDownload->uiPidFltId;
	bEnable = stpLXSdecPIDFltEnableDownload->bPidFltEnable;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32PidFltId >= pSdecConf->chInfo[ui8Ch].num_pidf, goto exit, "PID Filter Index Error Ch[%d]Pidf[%d]Total[%d]", 
							ui8Ch, ui32PidFltId, pSdecConf->chInfo[ui8Ch].num_pidf);

	SDEC_SetPidf_DN_Enable(stpSdecParam, ui8Ch, ui32PidFltId, bEnable);

	/* 20140503 jinhwan.bae for H13 JP Mode
	 * MCU TPI Interrupt is not used for key setting yet,
	 * because call sequence is changed
	 * from DN_EN -> Request, to Request -> DN_EN
	 * so add second case defensive code without any touching to Main PCR */
	if(stpSdecParam->ui8McuDescramblerCtrlMode == 1) // H13 JP Mode
	{
		pidfData = SDEC_GetPidfData(stpSdecParam, ui8Ch, ui32PidFltId);
		ui32PidValue = extract_bits(pidfData, 0x1FFF, 16);
		bClearPacket = _SDEC_Is_JPClearPacket(ui32PidValue);
		if(bClearPacket == TRUE)
		{
			SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Set TPI_IEN in DN_EN for MCU Descrambler Ch[%d] ui32PidFilId[%d]", ui8Ch, ui32PidFltId);
			SDEC_SetPidf_TPI_IEN_Enable(stpSdecParam, ui8Ch, ui32PidFltId, bEnable);
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PIDFilterEnableDownload");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


//from here for section filter

/**
********************************************************************************
* @brief
*   SDEC Section filter alloc.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterAlloc
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_ALLOC_T *stpLXSdecSecFltAlloc)
{
	DTV_STATUS_T eRet = NOT_OK, eResult = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT8 ui8SecIdx = 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecFltAlloc == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterAlloc");

	ui8Ch = stpLXSdecSecFltAlloc->eCh;

	eResult = SDEC_SecIdxCheck(stpSdecParam, ui8Ch, &ui8SecIdx);
	LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_SelSecFilterIdx failed:[%d]", eResult);
	
	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] returned Section PID Idx:[%d]", ui8Ch, ui8SecIdx);

	stpLXSdecSecFltAlloc->uiSecFltId = ui8SecIdx;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterAlloc");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/* Work Around for si11~ stream, PMT updated stream but all client request/cancel independantly
    so, MHEG DSMCC and Subtitle with same PID are enabled, and Subtitle Canceled
    But PES_PLOAD is not removed in PID filter, so, abnormal situation happened. */
/**
********************************************************************************
* @brief
*   SDEC PES filter free.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_PESFilterFree
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_FREE_T *stpLXSecPIDFltFree)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T* pSdecConf = NULL;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 ui8PidFltIdx = 0x0;

	unsigned long flags = 0;

	UINT32 pidf_data = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSecPIDFltFree == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_PESFilterFree");

	ui8Ch = stpLXSecPIDFltFree->eCh;
	ui32SecFltId = stpLXSecPIDFltFree->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	ui8PidFltIdx = stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui8PidFltIdx;

	pidf_data = SDEC_GetPidfData(stpSdecParam, ui8Ch, ui8PidFltIdx);

	/* Clear PAYLOAD_PES Fields */
	pidf_data &= ~ ( PAYLOAD_PES | DEST );
	pidf_data |= DEST_RESERVED; /* 4F */

	SDEC_SetPidfData(stpSdecParam, ui8Ch, ui8PidFltIdx, pidf_data);

	if(SDEC_IS_NORMAL_CHANNEL(ui8Ch))
	{
		SDEC_HAL_SECFClearMapBit(ui8Ch, ui8PidFltIdx, ui32SecFltId);
	}

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] PesFltId[%d] Free", ui8Ch, ui32SecFltId);

	SDEC_GPB_LOCK(stpSdecParam, ui8Ch, ui32SecFltId);

	/* delete msge in notify queue */
	SDEC_DeleteInNotify(stpSdecParam, ui8Ch, ui32SecFltId);

	/* Section Filter  Map init */
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].used = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].flag = FALSE;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].mode = 0x0;

	SDEC_Secf_Clear(stpSdecParam, ui8Ch, ui32SecFltId);

	/* 2012.02.06 gaius.lee
	 * Bug exist in L9 HW.
	 * While CPU access Read/Write/Bound Register, SDEC HW accesses write register, write pointer goes to read/write/bound regitser which CPU access.
	 * So, we make GPB map as static. That's why we disable this section. */
	if((pSdecConf->staticGPB == 0) || (ui8Ch == LX_SDEC_CH_C))
	{
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32Baseptr 	= 0x00000000;
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32Endptr	= 0x00000000;
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32Readptr 	= 0x00000000;
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32UsrReadptr 	= 0x00000000;
	}
	stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui8PidFltIdx	= 0x0;

	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.w = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_OVERFLOW = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_DATAREADY = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_SCRAMBLED = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x0;

	/* clear full buffer interrupt */
	if(SDEC_IS_NORMAL_CHANNEL(ui8Ch))
	{
		SDEC_HAL_GPBClearFullIntr(ui8Ch, ui32SecFltId);
	}

	SDEC_GPB_UNLOCK(stpSdecParam, ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_PESFilterFree");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC Section filter free.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterFree
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_FREE_T *stpLXSecPIDFltFree)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T* pSdecConf = NULL;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 ui8PidFltIdx = 0x0;
	unsigned long flags = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSecPIDFltFree == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterFree");

	ui8Ch = stpLXSecPIDFltFree->eCh;
	ui32SecFltId = stpLXSecPIDFltFree->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	ui8PidFltIdx = stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui8PidFltIdx;

	if(SDEC_IS_NORMAL_CHANNEL(ui8Ch))
	{
		SDEC_HAL_SECFClearMapBit(ui8Ch, ui8PidFltIdx, ui32SecFltId);
	}

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] SecFltId[%d] Free", ui8Ch, ui32SecFltId);

	SDEC_GPB_LOCK(stpSdecParam, ui8Ch, ui32SecFltId);

	/* delete msge in notify queue */
	SDEC_DeleteInNotify(stpSdecParam, ui8Ch, ui32SecFltId);

	/* Section Filter  Map init */
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].used = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].flag = FALSE;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].mode = 0x0;

	SDEC_Secf_Clear(stpSdecParam, ui8Ch, ui32SecFltId);

	/* 2012.02.06 gaius.lee
	 * Bug exist in L9 HW.
	 * While CPU access Read/Write/Bound Register, SDEC HW accesses write register, write pointer goes to read/write/bound regitser which CPU access.
	 * So, we make GPB map as static. That's why we disable this section. */
	if((pSdecConf->staticGPB == 0) || (ui8Ch == LX_SDEC_CH_C))
	{
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32Baseptr 	= 0x00000000;
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32Endptr	= 0x00000000;
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32Readptr 	= 0x00000000;
		stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32UsrReadptr 	= 0x00000000;
	}
	stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui8PidFltIdx	= 0x0;

	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.w = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_OVERFLOW = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_DATAREADY = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_SCRAMBLED = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_ENABLE = 0x0;
	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.f.SDEC_FLTSTATE_ALLOC = 0x0;

	/* clear full buffer interrupt */
	if(SDEC_IS_NORMAL_CHANNEL(ui8Ch))
	{
		SDEC_HAL_GPBClearFullIntr(ui8Ch,	ui32SecFltId);
	}

	SDEC_GPB_UNLOCK(stpSdecParam, ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterFree");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Section filter pattern.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterPattern
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_PATTERN_T *stpLXSecFltPattern)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0, bHigh = 0x0, ui8Mode = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 *ui32pPattern;
	UINT32 *ui32pMask;
	UINT32 *ui32pNeq;
	UINT32 ui32Pattern_L = 0x0, ui32Pattern_H = 0x0;
	UINT32 ui32Mask_L = 0x0, ui32Mask_H = 0x0;
	UINT32 ui32Neq_L = 0x0, ui32Neq_H = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSecFltPattern == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterPattern");

	ui8Ch = stpLXSecFltPattern->eCh;
	ui32SecFltId = stpLXSecFltPattern->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	/* if section filter index is higher than 32, sometimes use other register */
	if(ui32SecFltId > 32)	bHigh = 0x1;
	else					bHigh = 0x0;

 	ui8Mode = (UINT8)stpLXSecFltPattern->uiSecFltMode;

	if((ui8Ch == LX_SDEC_CH_C) || (ui8Ch == LX_SDEC_CH_G))
	{
		SDEC_SWP_SetSectionFilterPattern(stpLXSecFltPattern);
		stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].mode = ui8Mode;

		SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterPattern");
		OS_UnlockMutex(&stpSdecParam->stSdecMutex);
		return OK;
	}

	ui32pPattern = (UINT32 *)stpLXSecFltPattern->pucPattern;
	ui32pMask = (UINT32 *)stpLXSecFltPattern->pucMask;
	ui32pNeq = (UINT32 *)stpLXSecFltPattern->pucNotEqual;

	ui32Pattern_H	= swab32(*ui32pPattern); 	ui32pPattern++;
	ui32Pattern_L	= swab32(*ui32pPattern);
	ui32Mask_H		= swab32(*ui32pMask);		ui32pMask++;
	ui32Mask_L 		= swab32(*ui32pMask);
	ui32Neq_H 		= swab32(*ui32pNeq);		ui32pNeq++;
	ui32Neq_L 		= swab32(*ui32pNeq);

	stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].mode = ui8Mode;

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] Section Filter Mode:[%d]", ui8Ch, stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].mode);

	SDEC_HAL_SECFSetBufValidBit(ui8Ch, ui32SecFltId);

	if( ui8Mode & LX_SDEC_FLTMODE_NOCRCCHK )
		SDEC_HAL_SECFClearCRCBit(ui8Ch, ui32SecFltId);
	else
		SDEC_HAL_SECFSetCRCBit(ui8Ch, ui32SecFltId);

	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 0, ui32Pattern_H);
	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 1, ui32Pattern_L);
	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 2, ui32Mask_H);
	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 3, ui32Mask_L);
	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 4, ui32Neq_H);
	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 5, ui32Neq_L);
	SDEC_HAL_SECFSetSecfData(ui8Ch, ui32SecFltId, 6, ui32SecFltId);

	SDEC_HAL_SECFSetEnableBit(ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterPattern");

	eRet = OK;
	
exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Section filter buffer reset.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterBufferReset
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_BUFFER_RESET *stpLXSdecSecfltBufferReset)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32GpbIdx = 0x0, ui32GpbBaseAddr = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltBufferReset == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterBufferReset");

	ui8Ch 		= stpLXSdecSecfltBufferReset->eCh;
	ui32GpbIdx 	= stpLXSdecSecfltBufferReset->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32GpbIdx >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "GPB Index Error Ch[%d]GPB[%d]Total[%d]", 
							ui8Ch, ui32GpbIdx, pSdecConf->chInfo[ui8Ch].num_secf);

	ui32GpbBaseAddr = stpSdecParam->stSdecMeminfo[ui8Ch][ui32GpbIdx].ui32Baseptr;

	SDEC_DTV_SOC_Message( SDEC_ERROR, "SDEC_IO_SectionFilterBufferReset CH%c gpb[%d]", (ui8Ch)?'B':'A', ui32GpbIdx);

	SDEC_HAL_GPBSetReadPtr(ui8Ch, 	ui32GpbIdx, ui32GpbBaseAddr);
	SDEC_HAL_GPBSetWritePtr(ui8Ch, 	ui32GpbIdx, ui32GpbBaseAddr);

	/* initialize base & end pointer */
	stpSdecParam->stSdecMeminfo[ui8Ch][ui32GpbIdx].ui32Baseptr 	= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8Ch][ui32GpbIdx].ui32Readptr 	= ui32GpbBaseAddr;
	stpSdecParam->stSdecMeminfo[ui8Ch][ui32GpbIdx].ui32UsrReadptr 	= ui32GpbBaseAddr;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterBufferReset");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Section filter  buffer set.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterBufferSet
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_BUFFER_SET_T *stpLXSdecSecfltBufferSet)
{
	DTV_STATUS_T eRet = NOT_OK, eResult = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0, ui32BufAddress = 0x0;
	LX_SDEC_GPB_SIZE_T eBufferSize = LX_SDEC_GPB_SIZE_4K;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltBufferSet == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	ui8Ch = stpLXSdecSecfltBufferSet->eCh;
	ui32SecFltId = stpLXSdecSecfltBufferSet->uiSecFltId;
	ui32BufAddress = stpLXSdecSecfltBufferSet->uiBufAddress;
	eBufferSize = stpLXSdecSecfltBufferSet->eBufferSize;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterBufferSet [%d][%d]",ui8Ch, ui32SecFltId);

	/* GPB Set */
	eResult = SDEC_GpbSet(stpSdecParam,	ui8Ch, eBufferSize, ui32BufAddress, ui32SecFltId);
	LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_GpbSet failed");

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] GPB Base address:[0x%08x]", ui8Ch, stpLXSdecSecfltBufferSet->uiBufAddress);
	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] GPB size:[%d]", ui8Ch, stpLXSdecSecfltBufferSet->eBufferSize);
	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] ui32PidFltId:[%d]", ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterBufferSet");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}

/**
********************************************************************************
* @brief
*   SDEC Section filter dummy buffer set.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterDummyBufferSet
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_BUFFER_SET_T *stpLXSdecSecfltBufferSet)
{
	DTV_STATUS_T eRet = NOT_OK, eResult = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0, ui32BufAddress = 0x0;
	LX_SDEC_GPB_SIZE_T eBufferSize = LX_SDEC_GPB_SIZE_4K;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltBufferSet == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	ui8Ch = stpLXSdecSecfltBufferSet->eCh;
	ui32SecFltId = stpLXSdecSecfltBufferSet->uiSecFltId;
	ui32BufAddress = stpLXSdecSecfltBufferSet->uiBufAddress;
	eBufferSize = stpLXSdecSecfltBufferSet->eBufferSize;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= 64, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]", 
							ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterDummyBufferSet [%d][%d]",ui8Ch, ui32SecFltId);

	/* GPB Set */
	eResult = SDEC_DummyGpbSet(stpSdecParam,	ui8Ch, eBufferSize, ui32BufAddress, ui32SecFltId);
	LX_SDEC_CHECK_CODE( LX_IS_ERR(eResult), goto exit, "SDEC_DummyGpbSet failed");

	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] Dummy GPB Base address:[0x%08x]", ui8Ch, stpLXSdecSecfltBufferSet->uiBufAddress);
	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] Dummy GPB size:[%d]", ui8Ch, stpLXSdecSecfltBufferSet->eBufferSize);
	SDEC_DTV_SOC_Message( SDEC_PIDSEC, "Ch[%d] Dummy ui32PidFltId:[%d]", ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterDummyBufferSet");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}

/**
********************************************************************************
* @brief
*   SDEC Section filter  get infot.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterGetInfo
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_BUFFER_GET_INFO_T *stpLXSdecSecfltBufferGetInfo)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltBufferGetInfo == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterGetInfo");

	ui8Ch = stpLXSdecSecfltBufferGetInfo->eCh;
	ui32SecFltId = stpLXSdecSecfltBufferGetInfo->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	LX_SDEC_CHECK_CODE( !SDEC_IS_NORMAL_CHANNEL(ui8Ch), goto exit, "ch[%d] is Invalid parameters", ui8Ch );

	stpLXSdecSecfltBufferGetInfo->uiReadPtr 	= SDEC_HAL_GPBGetReadPtr(ui8Ch, ui32SecFltId);
	stpLXSdecSecfltBufferGetInfo->uiWritePtr 	= SDEC_HAL_GPBGetWritePtr(ui8Ch, ui32SecFltId);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterGetInfo");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Section filter  set read pointer.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterSetReadPtr
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_READPTR_SET_T *stpLXSdecSecfltReadPtrSet)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_CFG_T* pSdecConf = NULL;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 ui32ReadPtr = 0x0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltReadPtrSet == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterSetReadPtr");

	ui8Ch 			= stpLXSdecSecfltReadPtrSet->eCh;
	ui32SecFltId 	= stpLXSdecSecfltReadPtrSet->uiSecFltId;
	ui32ReadPtr 	= stpLXSdecSecfltReadPtrSet->uiReadPtr;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	/* 2012.02.06 gaius.lee
	 * Bug exist in L9 HW.
	 * While CPU access Read/Write/Bound Register, SDEC HW accesses write register, write pointer goes to read/write/bound regitser which CPU access.
	 * So, remove access to read register. That's why we disable this line. */
	if(pSdecConf->staticGPB == 0)
	{
		SDEC_HAL_GPBSetReadPtr(ui8Ch, ui32SecFltId, ui32ReadPtr);
	}
	
	/* save user read pointer */
	stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui32UsrReadptr = ui32ReadPtr;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterSetReadPtr");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Section filter  buffer get state.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterGetState
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_STATE_T *stpLXSdecSecfltState)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT8 ui8wordIdx = 0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltState == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterGetState");

	ui8Ch = stpLXSdecSecfltState->eCh;
	ui32SecFltId = stpLXSdecSecfltState->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	/* Get Status of Section Filter @see LX_SDEC_FLTSTATE_T */
	stpLXSdecSecfltState->uiFltState = stpSdecParam->stSecMap[ui8Ch][ui32SecFltId].stStatusInfo.w;

	/* Get Map Type */
	stpLXSdecSecfltState->ucSecf_mtype = SDEC_HAL_SECFGetMapTypeBit(ui8Ch, ui32SecFltId);

	/* Get Linked PID Filter */
	stpLXSdecSecfltState->uiPidFltId = stpSdecParam->stSdecMeminfo[ui8Ch][ui32SecFltId].ui8PidFltIdx;

	for( ui8wordIdx = 0 ; ui8wordIdx < 7 ; ui8wordIdx++ )
	{
		/* jinhwan.bae , static analysis, protect overrun */
		if( ui8wordIdx < 2 ) stpLXSdecSecfltState->uiPattern[ui8wordIdx] = SDEC_HAL_SECFGetSecfData(ui8Ch, ui32SecFltId, ui8wordIdx);
		else if( ui8wordIdx < 4 ) stpLXSdecSecfltState->uiMask[ui8wordIdx - 2] = SDEC_HAL_SECFGetSecfData(ui8Ch, ui32SecFltId, ui8wordIdx);
		else if( ui8wordIdx < 6 ) stpLXSdecSecfltState->uiNotEqual[ui8wordIdx - 4] = SDEC_HAL_SECFGetSecfData(ui8Ch, ui32SecFltId, ui8wordIdx);
		else if( ui8wordIdx == 6 ) stpLXSdecSecfltState->uiGpbIdx = SDEC_HAL_SECFGetSecfData(ui8Ch, ui32SecFltId, ui8wordIdx);
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterGetState");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   enable log.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpVdecParam :VDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetGPBBaseOffset
	(S_SDEC_PARAM_T *stpSdecParam, 
	LX_SDEC_GPB_BASE_OFFSET_T *stpLXSdecGPBBaseOffset)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT8 core = 0x0;
	UINT32 gpb_base_offset = 0x0;	
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecGPBBaseOffset == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_GetGPBBaseOffset");

	ui8Ch = (UINT8)stpLXSdecGPBBaseOffset->eCh;
	
	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	gpb_base_offset = SDEC_HAL_GetGPBBaseAddr(core);
	stpLXSdecGPBBaseOffset->uiGPBBaseOffset = (gpb_base_offset & 0xF0000000);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_GetGPBBaseOffset");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   enable log.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpVdecParam :VDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_EnableLog(S_SDEC_PARAM_T *stpSdecParam, UINT32 *pulArg)
{
	DTV_STATUS_T eRet = NOT_OK;
	int idx = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( pulArg == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_EnableLog");

	stpSdecParam->ui32MsgMask = *pulArg;

	for (idx = 0; idx < LX_MAX_MODULE_DEBUG_NUM; idx++)
	{
		if ( *pulArg & (1 << idx) ) OS_DEBUG_EnableModuleByIndex ( g_sdec_debug_fd, idx, DBG_COLOR_NONE );
		else						OS_DEBUG_DisableModuleByIndex( g_sdec_debug_fd, idx);
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_EnableLog");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   Read Register value
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpVdecParam :VDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_GetRegister(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_REG_T *stpLXSdecReadRegisters)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_BLOCK_T eSdecBlock;
	UINT32 ui32Offset = 0x0;
	UINT32* reg_ptr = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecReadRegisters == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	eSdecBlock	= stpLXSdecReadRegisters->eSdecBlock;
	ui32Offset 	= stpLXSdecReadRegisters->uiOffset;

	switch(eSdecBlock) 
	{
		case LX_SDEC_BLOCK_TOP:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegH14A0;
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegM14B0;
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegM14A0;
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegH13A0;
			break;

		case LX_SDEC_BLOCK_IO:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0)) 
				reg_ptr = (UINT32*)stSDEC_IO_RegH14A0[0]; 
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			 	reg_ptr = (UINT32*)stSDEC_IO_RegM14B0[0]; 
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0)) 
				reg_ptr = (UINT32*)stSDEC_IO_RegM14A0[0]; 
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
 				reg_ptr = (UINT32*)stSDEC_IO_RegH13A0[0];
 			break;

		case LX_SDEC_BLOCK_CORE_A:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14A0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH13A0[0];
			break;

		case LX_SDEC_BLOCK_CORE_B:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14A0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH13A0[1];
			break;

		case LX_SDEC_BLOCK_CORE1_IO:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_IO_RegH14A0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_IO_RegM14B0[1];
			else
			{
				SDEC_DTV_SOC_Message(SDEC_ERROR, "Not Supported in this chip rev");
				goto exit;
			}
			break;

		case LX_SDEC_BLOCK_CORE1_CORE_A:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[2];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[2];
			else
			{
				SDEC_DTV_SOC_Message(SDEC_ERROR, "Not Supported in this chip rev");
				goto exit;
			}
			break;

		case LX_SDEC_BLOCK_CORE1_CORE_B:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[3];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[3];
			else
			{
				SDEC_DTV_SOC_Message(SDEC_ERROR, "Not Supported in this chip rev");
				goto exit;
			}
			break;

		default :
			eRet = NOT_OK;
			goto exit;
	}

	eRet = OK;

	if(reg_ptr != NULL)
		stpLXSdecReadRegisters->uiValue = *(reg_ptr + (ui32Offset/4));

	goto exit;

exit:

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   Write Register value
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpVdecParam :VDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SetRegister(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_REG_T *stpLXSdecWriteRegisters)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_BLOCK_T eSdecBlock;
	UINT32 ui32Offset = 0x0;
	UINT32* reg_ptr = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecWriteRegisters == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	eSdecBlock	= stpLXSdecWriteRegisters->eSdecBlock;
	ui32Offset 	= stpLXSdecWriteRegisters->uiOffset;

	switch(eSdecBlock)
	{
		case LX_SDEC_BLOCK_TOP:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegH14A0;
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegM14B0;
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegM14A0;
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_TOP_RegH13A0;
			break;

		case LX_SDEC_BLOCK_IO:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_IO_RegH14A0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_IO_RegM14B0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_IO_RegM14A0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_IO_RegH13A0[0];
			break;

		case LX_SDEC_BLOCK_CORE_A:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14A0[0];
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH13A0[0];
			break;

		case LX_SDEC_BLOCK_CORE_B:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14A0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH13A0[1];
			break;
		
		case LX_SDEC_BLOCK_CORE1_IO:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_IO_RegH14A0[1];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_IO_RegM14B0[1];
			else
			{
				SDEC_DTV_SOC_Message(SDEC_ERROR, "Not Supported in this chip rev");
				goto exit;
			}
			break;

		case LX_SDEC_BLOCK_CORE1_CORE_A:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[2];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[2];
			else
			{
				SDEC_DTV_SOC_Message(SDEC_ERROR, "Not Supported in this chip rev");
				goto exit;
			}
			break;

		case LX_SDEC_BLOCK_CORE1_CORE_B:
			if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegH14A0[3];
			else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
				reg_ptr = (UINT32*)stSDEC_MPG_RegM14B0[3];
			else
			{
				SDEC_DTV_SOC_Message(SDEC_ERROR, "Not Supported in this chip rev");
				goto exit;
			}
			break;
			
		default :
			eRet = NOT_OK;
			goto exit;
	}

	eRet = OK;

	if(reg_ptr != NULL)
		*(reg_ptr + (ui32Offset/4)) = stpLXSdecWriteRegisters->uiValue;
		
	goto exit;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   Reset  SDEC input port
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_InputPortReset(UINT8 ui8Ch)
{
	int 			ret = RET_ERROR;
	UINT32			sdmwc_stat = 0;
	UINT8			chk_cnt = 0;
	UINT8			core = 0;

	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	/* wait until SDMWC is idle */
	sdmwc_stat = ( SDEC_HAL_SDMWCGetStatus(core) & 0x00000333 );

	while(sdmwc_stat != 0 && chk_cnt++ < 3)
	{
		sdmwc_stat = ( SDEC_HAL_SDMWCGetStatus(core) & 0x00000333 );
		OS_UsecDelay(1000);
	}

	SDEC_DTV_SOC_Message(SDEC_RESET, "chk_cnt[%d] ", chk_cnt );

	/* if not idle, reset SDMWC */
	if(chk_cnt >= 3)
	{
		SDEC_DTV_SOC_Message(SDEC_RESET, "NOT IDLE!");

		/* 0xC000B264(SDIO-SDMWC_STAT)*/
		SDEC_DTV_SOC_Message(SDEC_RESET, "SDMWC_STAT [0x%08x]", SDEC_HAL_SDMWCGetStatus(core));

		/* This block is added from L9 B0 */
		/* Reset */
		SDEC_HAL_SDMWCReset(core, 0x30000000);
		/* wait 10 us */
		OS_UsecDelay(10);
		/* Clear */
		SDEC_HAL_SDMWCReset(core, 0x00000000);
		/* wait 10 us */
		OS_UsecDelay(10);
	}


	/* reset channel */
	//stpSdecParam->stSDEC_IO_Reg->cdic[ui8Ch].rst = 1;
	SDEC_HAL_CIDCReset(core, ui8Ch);

	/* wait 10 us */
	OS_UsecDelay(10);

	ret = RET_OK;

	return ret;
}


/**
********************************************************************************
* @brief
*   Reset  SDEC Memory write Controller
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_SDMWCReset(UINT8 ui8Ch)
{
	int 			ret = RET_ERROR;
	UINT8			core = 0;

	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	/* This block is added from L9 B0 */
	/* Reset */
	SDEC_HAL_SDMWCReset(core, 0x30000000);

	/* wait 10 us */
	OS_UsecDelay(10);

	/* Clear */
	SDEC_HAL_SDMWCReset(core, 0x00000000);

	/* wait 1 ms */
	OS_UsecDelay(1000);

	ret = RET_OK;

	return ret;
}

/**
********************************************************************************
* @brief
*   Reset  SDEC H/W block. Using Chip TOP.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Register_Backup(UINT32 *pRegStore, S_REG_BACKUP_LIST_T *pBackList)
{
	int 			ret = RET_ERROR;
	UINT8			i = 0, offset = 0;

	if(pBackList->bInit == FALSE )
	{
		/* Make a list about register backup offset list */
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIP0]			= 0x00;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIP1]			= 0x04;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIP2]			= 0x08;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIP3]			= 0x0C;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIOP0]			= 0x10;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIOP1]			= 0x14;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIC_DSC0]		= 0x28;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIC_DSC1]		= 0x2C;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_INTR_EN]			= 0x100;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_ERR_INTR_EN]		= 0x110;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_GPB_BASE]			= 0x120;
		pBackList->regIO[E_SDEC_REGBACKUP_IO_CDIC2]			= 0x12C;

		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_EXT_CONF]		= 0x10;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_TPI_ICONF]	= 0x40;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_KMEM_ADDR]	= 0x98;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_KMEM_DATA]	= 0x9C;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_SECF_EN_L] 	= 0xB0;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_SECF_EN_H] 	= 0xB4;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_SECF_MTYPE_L]	= 0xB8;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_SECF_MTYPE_H]	= 0xBC;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_SECFB_VALID_L]= 0xC0;
		pBackList->regCORE[E_SDEC_REGBACKUP_CORE_SECFB_VALID_H]= 0xC4;

		pBackList->bInit = TRUE;
	}

	/* SD IO Bakcup */
	for ( i = 0 ; i < E_SDEC_REGBACKUP_IO_NUM ; i++, offset++ )
	{
		pRegStore[i] = REG_READ32(L9_SDEC_IO_REG_BASE + pBackList->regIO[i]);

		SDEC_DTV_SOC_Message(SDEC_RESET, "BACKUP  IO    OFFSET[0x%03x] VAL[0x%08x]", pBackList->regIO[i], pRegStore[offset]);
	}


	/* SD CORE0 Bakcup */
	for ( i = 0 ; i < E_SDEC_REGBACKUP_CORE_NUM ; i++, offset++ )
	{
		pRegStore[offset] = REG_READ32(L9_SDEC_MPG_REG_BASE0 + pBackList->regCORE[i]);

		SDEC_DTV_SOC_Message(SDEC_RESET, "BACKUP  CORE0 OFFSET[0x%03x] VAL[0x%08x]", pBackList->regIO[i], pRegStore[offset]);
	}

	/* SD CORE0 Bakcup */
	for ( i = 0 ; i < E_SDEC_REGBACKUP_CORE_NUM ; i++, offset++)
	{
		pRegStore[offset] = REG_READ32(L9_SDEC_MPG_REG_BASE0 + pBackList->regCORE[i]);

		SDEC_DTV_SOC_Message(SDEC_RESET, "BACKUP  CORE1 OFFSET[0x%03x] VAL[0x%08x]", pBackList->regIO[i], pRegStore[offset]);
	}

	ret = RET_OK;

	return ret;
}

/**
********************************************************************************
* @brief
*   Reset  SDEC H/W block. Using Chip TOP.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_Register_Restore(UINT32 *pRegStore, S_REG_BACKUP_LIST_T *pBackList)
{
	int 			ret = RET_ERROR;
	UINT8			i = 0, offset = 0;

	if(pBackList->bInit == FALSE )
	{
		SDEC_DTV_SOC_Message(SDEC_RESET, "SDEC_Register_Restore list is not initialized. ");
		goto exit;
	}

	/* SD IO Restore */
	for ( i = 0 ; i < E_SDEC_REGBACKUP_IO_NUM ; i++, offset++ )
	{
		SDEC_DTV_SOC_Message(SDEC_RESET, "RESTORE IO    OFFSET[0x%03x] VAL[0x%08x]", pBackList->regIO[i], pRegStore[offset]);
		REG_WRITE32(L9_SDEC_IO_REG_BASE + pBackList->regIO[i], pRegStore[i]);
	}

	/* SD CORE0 Restore */
	for ( i = 0 ; i < E_SDEC_REGBACKUP_CORE_NUM ; i++, offset++ )
	{
		SDEC_DTV_SOC_Message(SDEC_RESET, "RESTORE CORE0 OFFSET[0x%03x] VAL[0x%08x]", pBackList->regIO[i], pRegStore[offset]);
		REG_WRITE32(L9_SDEC_MPG_REG_BASE0 + pBackList->regCORE[i], pRegStore[offset]);
	}

	/* SD CORE1 Restore */
	for ( i = 0 ; i < E_SDEC_REGBACKUP_CORE_NUM ; i++, offset++ )
	{
		SDEC_DTV_SOC_Message(SDEC_RESET, "RESTORE CORE1 OFFSET[0x%03x] VAL[0x%08x]", pBackList->regIO[i], pRegStore[offset]);
		REG_WRITE32(L9_SDEC_MPG_REG_BASE0 + pBackList->regCORE[i], pRegStore[offset]);
	}

	ret = RET_OK;

exit:
	return ret;
}

static UINT32 				_gsRegStore[E_SDEC_REGBACKUP_IO_NUM + E_SDEC_REGBACKUP_CORE_NUM  + E_SDEC_REGBACKUP_CORE_NUM];	/* IO + CORE0 + CORE1 */
static S_REG_BACKUP_LIST_T 	_gsBackupList = { .bInit = FALSE };


/**
********************************************************************************
* @brief
*   Reset  SDEC H/W block. Using Chip TOP.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_TE_Reset(UINT8 core)
{
	int 			ret = RET_ERROR;

	/* Register Backup */
	SDEC_Register_Backup( &_gsRegStore[0], &_gsBackupList);

	/* Port Disable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_DISABLE);

	/* Port Enable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_ENABLE);

	/* Register Restore */
	SDEC_Register_Restore(&_gsRegStore[0], &_gsBackupList);

	ret = RET_OK;

	return ret;
}

/**
********************************************************************************
* @brief
*   Reset  SDEC H/W block. Using Chip TOP.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_TEA_Reset(UINT8 core)
{
	int 			ret = RET_ERROR;

	/* Register Backup */
	SDEC_Register_Backup(_gsRegStore, &_gsBackupList);

	/* Port Disable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_DISABLE);

	/* Port Enable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_ENABLE);

	/* Register Restore */
	SDEC_Register_Restore(_gsRegStore, &_gsBackupList);

	ret = RET_OK;

	return ret;
}


/**
********************************************************************************
* @brief
*   Reset  SDEC H/W block. Using Chip TOP.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_TEAH_Reset(UINT8 core)
{
	int 			ret = RET_ERROR;

	/* Register Backup */
	SDEC_Register_Backup(_gsRegStore, &_gsBackupList);

	/* Port Disable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_DISABLE);

	/* Port Enable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_ENABLE);

	/* Register Restore */
	SDEC_Register_Restore(_gsRegStore, &_gsBackupList);

	ret = RET_OK;

	return ret;
}

/**
********************************************************************************
* @brief
*   Reset  SDEC H/W block. Using Chip TOP.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_TEAHFlush_Reset(UINT8 core)
{
	int 			ret = RET_ERROR;

	/* Register Backup */
	SDEC_Register_Backup(_gsRegStore, &_gsBackupList);

	/* Port Disable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_DISABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_DISABLE);

	/* Port Enable */
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL0, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_SERIAL1, 		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL0,		SDEC_HAL_ENABLE);
	SDEC_InputPortEnable(core, LX_SDEC_INPUT_PARALLEL1,		SDEC_HAL_ENABLE);

	/* Register Restore */
	SDEC_Register_Restore(_gsRegStore, &_gsBackupList);

	ret = RET_OK;

	return ret;
}


/**
********************************************************************************
* @brief
*   Send Debug Command to Kdrv.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_DebugCommand
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_DBG_CMD_T 	*stpLXSdecDbgCmd)
{
	DTV_STATUS_T 		eRet = NOT_OK;
	UINT8				ui8Ch = 0, core = 0, org_ch = 0;
	unsigned long 		flags = 0;
	LX_SDEC_CFG_T 		*pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecDbgCmd == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_DebugCommand");

	/* get channel */
	ui8Ch = (UINT8)stpLXSdecDbgCmd->inParam;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	
	org_ch = ui8Ch;
	SDEC_CONVERT_CORE_CH(core, ui8Ch);

	switch(stpLXSdecDbgCmd->eCmd)
	{
		case LX_SDEC_CMD_RESET_CDIC:
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);
			/* disable channel input and wait 0.01 ms */
			SDEC_InputPortEnable(core, stpSdecParam->eInputPath[org_ch], 		SDEC_HAL_DISABLE);
			OS_UsecDelay(10);
			SDEC_InputPortReset(org_ch);
			OS_UsecDelay(10);
			/* enable channel input */
			SDEC_InputPortEnable(core, stpSdecParam->eInputPath[org_ch], 		SDEC_HAL_ENABLE);
			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
			break;

		case LX_SDEC_CMD_RESET_SDMWC:
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);
			/* disable channel input and wait 0.01 ms */
			SDEC_InputPortEnable(core, stpSdecParam->eInputPath[org_ch], SDEC_HAL_DISABLE);
			OS_UsecDelay(10);
			SDEC_SDMWCReset(org_ch);
			OS_UsecDelay(10);
			/* enable channel input */
			SDEC_InputPortEnable(core, stpSdecParam->eInputPath[org_ch], SDEC_HAL_ENABLE);
			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
			break;

		case LX_SDEC_CMD_RESET_TE:
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);
			SDEC_TE_Reset(core);
			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
			break;

		case LX_SDEC_CMD_RESET_TEA:
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);
			SDEC_TEA_Reset(core);
			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
			break;

		case LX_SDEC_CMD_RESET_TEAH:
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);
			SDEC_TEAH_Reset(core);
			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
			break;

		case LX_SDEC_CMD_RESET_TEAH_FLUSH:
			spin_lock_irqsave(&stpSdecParam->stSdecResetSpinlock, flags);
			SDEC_TEAHFlush_Reset(core);
			spin_unlock_irqrestore(&stpSdecParam->stSdecResetSpinlock, flags);
			break;

		case LX_SDEC_CMD_EN_CIDC :
			if(stpLXSdecDbgCmd->inParam == E_SDEC_RESET_MODE_ONETIME)	stpSdecParam->ui8CDICResetNum = 0;
			stpSdecParam->ui8CDICResetMode = (UINT8)stpLXSdecDbgCmd->inParam;
			break;

		case LX_SDEC_CMD_EN_SDMWC:
			if(stpLXSdecDbgCmd->inParam == E_SDEC_RESET_MODE_ONETIME)	stpSdecParam->ui8SDMWCResetNum = 0;
			stpSdecParam->ui8SDMWCResetMode = (UINT8)stpLXSdecDbgCmd->inParam;
			break;

		case LX_SDEC_CMD_SET_FCW:
			SDEC_IO_SettingFCW(stpSdecParam , stpLXSdecDbgCmd->inParam);
			break;
			
		default :
			break;
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_DebugCommand");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/* Serial TS OUT Stub */


/**
********************************************************************************
* @brief
*	Serial TS Out
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*	ui32Arg :arguments from userspace
*	stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SerialTSOUT_SetSrc
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SERIALTSOUT_SET_SOURCE_T *stpLXSdecSTSOutSetSrc)
{
	DTV_STATUS_T eRet = NOT_OK;
	LX_SDEC_SERIALTSOUT_SRC_T src = LX_SDEC_SO_NONE;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSTSOutSetSrc == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	src = stpLXSdecSTSOutSetSrc->src;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SerialTSOUT_SetSrc [%d]", (UINT8)src);

	SDEC_HAL_CDOC_SetSrc(_SDEC_CORE_0, 1 /* CDOC 1 only available */, (UINT8)src);

	/* reset with set src */
	SDEC_HAL_CDOC_Reset(_SDEC_CORE_0, 1 /* CDOC 1 only available */);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SerialTSOUT_SetSrc");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}



/**
********************************************************************************
* @brief
*	Serial TS Out
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*	ui32Arg :arguments from userspace
*	stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_BDRC_SetData
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_BDRC_T *stpLXSdecBDRCSetData)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8	bdrc_idx = 0, en = 0, rst = 0, wptr_auto = 0, gpb_chan = 0, gpb_idx = 0, q_len = 0, dtype = 0;						

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecBDRCSetData == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	bdrc_idx = stpLXSdecBDRCSetData->bdrc_idx;
	en = stpLXSdecBDRCSetData->en;
	rst = stpLXSdecBDRCSetData->rst;
	wptr_auto = stpLXSdecBDRCSetData->wptr_auto;
	gpb_chan = stpLXSdecBDRCSetData->gpb_chan;
	gpb_idx = stpLXSdecBDRCSetData->gpb_idx;
	q_len = stpLXSdecBDRCSetData->q_len;
	dtype = stpLXSdecBDRCSetData->dtype;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_BDRC_SetData [%d][%d][%d][%d][%d][%d][%d][%d]",bdrc_idx,en,rst,wptr_auto,gpb_chan,gpb_idx,q_len,dtype);
	
	/* BDRC 3 is only available for TS out, BDRC 1 is used for SDEC input */
	SDEC_HAL_BDRC_Reset( _SDEC_CORE_0, bdrc_idx );
	SDEC_HAL_BDRC_EnableWptrAutoUpdate( _SDEC_CORE_0, bdrc_idx, wptr_auto );
	SDEC_HAL_BDRC_SetGPBChannel( _SDEC_CORE_0, bdrc_idx, gpb_chan );
	SDEC_HAL_BDRC_SetGPBIndex( _SDEC_CORE_0, bdrc_idx, gpb_idx );
	SDEC_HAL_BDRC_SetQlen( _SDEC_CORE_0, bdrc_idx, q_len );
	SDEC_HAL_BDRC_SetDtype( _SDEC_CORE_0, bdrc_idx, dtype );
	SDEC_HAL_BDRC_Enable( _SDEC_CORE_0, bdrc_idx, en );
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_BDRC_SetData");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*	Serial TS Out
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*	ui32Arg :arguments from userspace
*	stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_BDRC_Enable
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_BDRC_ENABLE_T *stpLXSdecBDRCEnable)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8	bdrc_idx = 0, en = 0;
	
	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecBDRCEnable == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	bdrc_idx = stpLXSdecBDRCEnable->bdrc_idx;
	en = stpLXSdecBDRCEnable->en;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_BDRC_Enable [%d][%d]",bdrc_idx,en);

	SDEC_HAL_BDRC_Enable( _SDEC_CORE_0, bdrc_idx, en );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_BDRC_SetData");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);

}


/**
********************************************************************************
* @brief
*	Serial TS Out
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*	ui32Arg :arguments from userspace
*	stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SerialTSOUT_SetBufELevel
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SERIALTSOUT_SET_BUFELEV_T *stpLXSdecSTSOutSetBufELev)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT32 buf_e_lev = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSTSOutSetBufELev == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	buf_e_lev = stpLXSdecSTSOutSetBufELev->buf_e_lev;
	
	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SerialTSOUT_SetBufELevel [%d]",buf_e_lev);

	/* BDRC 3 is only available for TS out */
	SDEC_HAL_BDRC_SetBufferEmptyLevel( _SDEC_CORE_0, 3, buf_e_lev );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SerialTSOUT_SetBufELevel");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_CfgOutputPort
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CFG_OUTPUT_T *stpLXSdecCfgPort)
{
	DTV_STATUS_T eRet = NOT_OK;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCfgPort == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_CfgOutputPort");

	SDEC_HAL_CDIOP_SetOut(_SDEC_CORE_0, 1, stpLXSdecCfgPort);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_CfgOutputPort");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC Section filter  set read pointer.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterSetWritePtr
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_WRITEPTR_SET_T *stpLXSdecSecfltWritePtrSet)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 ui32WritePtr = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltWritePtrSet == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterSetWritePtr");

	ui8Ch 			= stpLXSdecSecfltWritePtrSet->eCh;
	ui32SecFltId 	= stpLXSdecSecfltWritePtrSet->uiSecFltId;
	ui32WritePtr 	= stpLXSdecSecfltWritePtrSet->uiWritePtr;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	SDEC_HAL_GPBSetWritePtr(ui8Ch, ui32SecFltId, ui32WritePtr);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterSetWritePtr");

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*	Serial TS Out
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*	ui32Arg :arguments from userspace
*	stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_BDRC_SetWritePtrUpdated
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_BDRC_WPTRUPD_T *stpLXSdecBDRCSetWptrUpd)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 idx = 0;					

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecBDRCSetWptrUpd == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	idx = stpLXSdecBDRCSetWptrUpd->idx;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_BDRC_SetWritePtrUpdated [%d]",idx);

	/* BDRC 3 is only available for TS out */
	SDEC_HAL_BDRC_SetWptrUpdate(_SDEC_CORE_0, idx );

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_BDRC_SetWritePtrUpdated");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}


/**
********************************************************************************
* @brief
*   SDEC Section filter  set read pointer.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterGetReadPtr
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_GET_READPTR_T *stpLXSdecSecfltReadPtrGet)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 ui32ReadPtr = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;
		
	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltReadPtrGet == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterGetReadPtr");

	ui8Ch 			= stpLXSdecSecfltReadPtrGet->eCh;
	ui32SecFltId 	= stpLXSdecSecfltReadPtrGet->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]", 
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	ui32ReadPtr = SDEC_HAL_GPBGetReadPtr(ui8Ch, ui32SecFltId);

	stpLXSdecSecfltReadPtrGet->uiValue = ui32ReadPtr;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterGetReadPtr (%x)", stpLXSdecSecfltReadPtrGet->uiValue);

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/**
********************************************************************************
* @brief
*   SDEC Section filter  get write pointer.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_SectionFilterGetWritePtr
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_SECFLT_GET_WRITEPTR_T *stpLXSdecSecfltWritePtrGet)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT8 ui8Ch = 0x0;
	UINT32 ui32SecFltId = 0x0;
	UINT32 ui32ReadPtr = 0x0;
	LX_SDEC_CFG_T *pSdecConf = NULL;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecSecfltWritePtrGet == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_SectionFilterGetReadPtr");

	ui8Ch 			= stpLXSdecSecfltWritePtrGet->eCh;
	ui32SecFltId 	= stpLXSdecSecfltWritePtrGet->uiSecFltId;

	/* argument check */
	pSdecConf = SDEC_CFG_GetConfig();
	LX_SDEC_CHECK_CODE( ui8Ch >= pSdecConf->nChannel, goto exit, "over channel range %d", ui8Ch);
	LX_SDEC_CHECK_CODE( ui32SecFltId >= pSdecConf->chInfo[ui8Ch].num_secf, goto exit, "Section Filter Index Error Ch[%d]Secf[%d]Total[%d]",
							ui8Ch, ui32SecFltId, pSdecConf->chInfo[ui8Ch].num_secf);

	ui32ReadPtr = SDEC_HAL_GPBGetWritePtr(ui8Ch, ui32SecFltId);

	stpLXSdecSecfltWritePtrGet->uiValue = ui32ReadPtr;

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_SectionFilterGetWritePtr (%x)", stpLXSdecSecfltWritePtrGet->uiValue);

	eRet = OK;

exit:
	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}



/**
********************************************************************************
* @brief
*   select input port.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_IO_CfgIOPort
	(S_SDEC_PARAM_T *stpSdecParam,
	LX_SDEC_CFG_IO_T *stpLXSdecCfgIOPort)
{
	DTV_STATUS_T 		eRet 		= NOT_OK;
	UINT8				reg_val		= 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( stpLXSdecCfgIOPort == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_LockMutex(&stpSdecParam->stSdecMutex);

	SDEC_DTV_SOC_Message( SDEC_TRACE, "<--SDEC_IO_CfgIOPort");

	if(stpLXSdecCfgIOPort->ePort == LX_SDEC_SERIAL_IO_0)
	{
		/* stpio_en_ctrl, 1 is set by output, 0 is set by input */
		if(stpLXSdecCfgIOPort->en_out != 0) reg_val = 1;
	
		if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
		{
			CTOP_CTRL_H14A0_RdFL(ctr58);
			CTOP_CTRL_H14A0_Wr01(ctr58, stpio_en_ctrl, reg_val);
			CTOP_CTRL_H14A0_WrFL(ctr58);
		}
		else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
			CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
			CTOP_CTRL_M14B0_Wr01(LEFT, ctr96, stpio_en_ctrl, reg_val);
			CTOP_CTRL_M14B0_WrFL(LEFT, ctr96);
#else
			CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
			CTOP_CTRL_M14B0_Wr01(TOP, ctr08, stpio_en_ctrl, reg_val);
			CTOP_CTRL_M14B0_WrFL(TOP, ctr08);
#endif
		}
		else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
		{
			CTOP_CTRL_M14A0_RdFL(ctr58);
			CTOP_CTRL_M14A0_Wr01(ctr58, stpio_en_ctrl, reg_val);
			CTOP_CTRL_M14A0_WrFL(ctr58);
		}
		else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
		{
			CTOP_CTRL_H13A0_RdFL(ctr58);
			CTOP_CTRL_H13A0_Wr01(ctr58, stpio_en_ctrl, reg_val);
			CTOP_CTRL_H13A0_WrFL(ctr58);
		}
	}
	else if(stpLXSdecCfgIOPort->ePort == LX_SDEC_PARALLEL_IO_0)
	{
		/* tpio_sel_ctrl, 1 is set by input, 0 is set by output */
		if(stpLXSdecCfgIOPort->en_out == 0) reg_val = 1;
		else 								reg_val = 0;
		
		if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
		{
			CTOP_CTRL_H14A0_RdFL(ctr58);
			CTOP_CTRL_H14A0_Wr01(ctr58, tpio_sel_ctrl, reg_val);
			CTOP_CTRL_H14A0_WrFL(ctr58);
		}
		else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
		{
#if 0 /* 20131030 jinhwan.bae LEFT, ctr96 is not real control register, TOP, ctr08 is real control register */
			CTOP_CTRL_M14B0_RdFL(LEFT, ctr96);
			CTOP_CTRL_M14B0_Wr01(LEFT, ctr96, tpio_sel_ctrl, reg_val);
			CTOP_CTRL_M14B0_WrFL(LEFT, ctr96);
#else
			CTOP_CTRL_M14B0_RdFL(TOP, ctr08);
			CTOP_CTRL_M14B0_Wr01(TOP, ctr08, tpio_sel_ctrl, reg_val);
			CTOP_CTRL_M14B0_WrFL(TOP, ctr08);
#endif
		}
		else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
		{
			CTOP_CTRL_M14A0_RdFL(ctr58);
			CTOP_CTRL_M14A0_Wr01(ctr58, tpio_sel_ctrl, reg_val);
			CTOP_CTRL_M14A0_WrFL(ctr58);
		}
		else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
		{
			CTOP_CTRL_H13A0_RdFL(ctr58);
			CTOP_CTRL_H13A0_Wr01(ctr58, tpio_sel_ctrl, reg_val);
			CTOP_CTRL_H13A0_WrFL(ctr58);
		}
	}

	SDEC_DTV_SOC_Message( SDEC_TRACE, "-->SDEC_IO_CfgIOPort");

	eRet = OK;

	OS_UnlockMutex(&stpSdecParam->stSdecMutex);
	return (eRet);
}

/* End Of Serial TS OUT Stub */

/**
 * Below code is from LG D2A STB
 *
 * Modified and adjusted by Jihoon Lee ( gaius.lee@lge.com )
 *
 * 2010.06.14 @ DTV Lab.
 */


/**
 *
 * Filter context - initialised by calling pwm_init
 * @ingroup sdec_struct_type
 */
typedef struct
{
  unsigned long service				: 1, 	///< 0x00(bit 1) : pcr service doing
                recovery_Enable     : 1, 	///< 0x00(bit 1) : enable pcr recovery
                first_pcr_arrived	: 1,	///< 0x00(bit 1) : first_pcr_arrived
                reserved 			: 29;	///< 0x00(bit 28): reserved
  unsigned long fcw_base;					///< 0x04        : fcw base for selected Memory Clock
  signed   long fcw_offset;					///< 0x08        : fcw offset for first reset after each ch. change.
  unsigned long fcw_value;					///< 0x0c        : temperary fcw value
  unsigned long lastUpdateTime;				///< 0x10        : last updated Time Tick
  unsigned long new_reset_discard;			///< 0x14        : after reset recovery discarding count
  unsigned long new_adjust_skip;			///< 0x18        : skip adjusting right after adjust
  unsigned long skip_chk_overshoot;		 	///< 0x1c        : check divergence
  unsigned long fcw_trickSpeed;				///< 0x20        : multiply value for trick mode
  unsigned int	max_ignore_count;			///			 : max ignore count for protecting first jitter
} pwm_context_t;

UINT32	max_fcw_value;
UINT32	min_fcw_value;

UINT32	pwm_fcw_default		= 0;
UINT32	pwm_fcw_threshold		= 0;
UINT32	pwm_fcw_threshold_with_max_ignore = 0;
UINT32	pwm_fcw_window_sz		= 0;
SINT32	pwm_fcw_sz_step		= 0;


/*---------------------------------------------------------
 * Constant definition
---------------------------------------------------------*/
#define DCO_INPUT_CLOCK 	198
#define DCO_COEF			8388608		/* 2^23 */

#define FCW_DEFAULT_VALUE_27025 0x0011789A	/* 27 Mhz */
#define FCW_DEFAULT_VALUE 		0x0011745D	/* 27 Mhz */
//#define FCW_DEFAULT_VALUE (UINT32)(KHZ_TO_FCW(24000))
#define FCW_PREFIX			0

#define FREQ_THRESHOLD_KHZ 						100
#define FREQ_THRESHOLD_KHZ_WITH_MAX_IGNORE	    5

#define PWM_DEBUG_LEVEL		2

#define FCW_WINDOW_SZ		8000

#define FCW_SZ_STEP 		0x9			/* 정확하게 계산하면 8.67 정도 됨 */

#define	PWM_MAX_DIFF_IGNORE		 36		/* 36 unit in 45 Khz PCR Frequency	  */
#define	PWM_MIN_DIFF_IGNORE		  4		/*  4 unit in 45 Khz PCR Frequency	  */
#define	PWM_DELTA_WINDOW		 24		/* Window size of direction filtering */
#define PWM_SUM_WINDOW			  4		/* Number of sum windows              */

#define MTS_HUGE_PCR_DIFFERENCE  (45*5000) /* <-- For stream wrap arround , 45*5000ms 5 sec --- 45MHz 020524 */

//#define MAX_DELTA_CLK 				(2 * 45)
#define MAX_DELTA_CLK 				(2* 45 * 10)

/*---------------------------------------------------------
 * Macro definition
---------------------------------------------------------*/
#define FCW_TO_KHZ(fcw)	( ((fcw) & 0x1FFFFF) * DCO_INPUT_CLOCK  / ( DCO_COEF / 1000 ))
#define KHZ_TO_FCW(khz)	( (khz)  * (DCO_COEF / DCO_INPUT_CLOCK) / 1000)

/* SINT64 divide macro with do_div */
#define DO_DIV_SINT64(n, div) { \
	if(n < 0 ) \
	{ \
		UINT64	u = (UINT64) (-n); \
		do_div(u, div); \
		n = (SINT64) (-u); \
	} \
	else \
		do_div(n, div); \
};
/*---------------------------------------------------------
    External 변수와 함수 prototype 선언
    (External Variables & Function Prototypes Declarations)
---------------------------------------------------------*/

/*---------------------------------------------------------
    Static 변수와 함수 prototype 선언
    (Static Variables & Function Prototypes Declarations)
---------------------------------------------------------*/
/*prototypes*/
static UINT32		nDeltaChkCount	= 0;	/* No. of data in nPwmWindow  */

static UINT32		_TimeStamps[PWM_SUM_WINDOW+1];		/* 시간  */
static SINT64		_MeanOfDelta[PWM_SUM_WINDOW*2];	/* pcr - stc의 차이를 저장한 nSumOfDelta를 시간으로 나눈 값 */
static UINT32		_AbsOfDelta[PWM_SUM_WINDOW*2];		/* pcr - stc의 차이를 저장한 nSumOfDelta를 시간으로 나눈 값의 절대값 */

static UINT32		*nTimeStamps = &_TimeStamps[1];	/* [1]을 가리키는 이유는 nTimeStamps[-1]로 이전 시간을 얻기 위해서다 */
static UINT32		*nAbsOfDelta = &_AbsOfDelta[PWM_SUM_WINDOW];
static SINT64		*nMeanOfDelta = &_MeanOfDelta[PWM_SUM_WINDOW];

static SINT32		nSumOfDelta[PWM_SUM_WINDOW];		/* pcr - stc의 차이를 저장한 값 */
static SINT32		nMstElapsed[PWM_SUM_WINDOW];		/* _timeStamp 값들의 차이값.. 즉, nSumOfDelta를 모을 때 걸린  시간. 이 값으로 _MeanofDelta나누어서 freq를 측정한다. */


static pwm_context_t pwm_context = {
	FALSE,				  // service
	FALSE,				  // recovery_Enable
	FALSE,				  // first_pcr_arrived
	    0,				  // reserved
		0,				  // fcw_base
	    0,				  // fcw_offset
	    0,				  // fcw_value
	    0,				  // lastUpdateTime
	    5,                // new_reset_discard
	    3,				  // new_adjust_skip
	    0,				  // skip_chk_overshoot
	    0,				  // fcw_trickSpeed
	    0				  // max_ignore_count
	};



UINT32 SDEC_IO_SettingFCW(S_SDEC_PARAM_T *stpSdecParam, UINT32 new_fcw_value)
{
	UINT32	ui32stcReal_32 = 0, ui32stcReal_31_0 = 0;

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );

	/* 2013. 09. 25. jinhwan.bae
	    CPU TOP Register Should Be SET to run with NEW DCO Value
	    CPU_TOP register base and offset is same at H14, M14B0, H13B0
	    DCO Set CTOP Register are different, H14, M14
	    H14 : 0x154 (CTR85) 
	    M14-B0 : 0x5C (CTOP_TOP CTR23) */

	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		CTOP_CTRL_H14_WRITE(0x154, new_fcw_value);

		////////////////////////////////////////////////////////////////////////////
		/* Setting DCO Value */
		/* STC Referrence - will be set time which will apply DCO to system */
		/* Actually, During Read/Write, All DCO Related Function Control Needed.ex. Stop, Set and Restart */
		/* But even if not, Read/Write Action is needed to adapt to the DCO Value really */
		/* Before Read/Write, DCO Value is not adapted really */
		/* read real STC value */
		ui32stcReal_32		= CPU_TOP_H14_READ(0x150);
		ui32stcReal_31_0	= CPU_TOP_H14_READ(0x154);

		/* setting referrence STC value with real stc + 0x10. */
		CPU_TOP_H14_WRITE(0x148, ui32stcReal_32);
		CPU_TOP_H14_WRITE(0x14C, ui32stcReal_31_0 + 0x10);
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		CTOP_CTRL_M14B0_WRITE(TOP, 0x5C, new_fcw_value);

		////////////////////////////////////////////////////////////////////////////
		/* Setting DCO Value */
		/* STC Referrence - will be set time which will apply DCO to system */
		/* Actually, During Read/Write, All DCO Related Function Control Needed.ex. Stop, Set and Restart */
		/* But even if not, Read/Write Action is needed to adapt to the DCO Value really */
		/* Before Read/Write, DCO Value is not adapted really */
		/* read real STC value */
		ui32stcReal_32		= CPU_TOP_M14_READ(0x150);
		ui32stcReal_31_0	= CPU_TOP_M14_READ(0x154);

		/* setting referrence STC value with real stc + 0x10. */
		CPU_TOP_M14_WRITE(0x148, ui32stcReal_32);
		CPU_TOP_M14_WRITE(0x14C, ui32stcReal_31_0 + 0x10);

	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		CTOP_CTRL_M14_WRITE(0x154, new_fcw_value);

		////////////////////////////////////////////////////////////////////////////
		/* Setting DCO Value */
		/* STC Referrence - will be set time which will apply DCO to system */
		/* Actually, During Read/Write, All DCO Related Function Control Needed.ex. Stop, Set and Restart */
		/* But even if not, Read/Write Action is needed to adapt to the DCO Value really */
		/* Before Read/Write, DCO Value is not adapted really */
		/* read real STC value */
		ui32stcReal_32		= CPU_TOP_M14_READ(0x150);
		ui32stcReal_31_0	= CPU_TOP_M14_READ(0x154);

		/* setting referrence STC value with real stc + 0x10. */
		CPU_TOP_M14_WRITE(0x148, ui32stcReal_32);
		CPU_TOP_M14_WRITE(0x14C, ui32stcReal_31_0 + 0x10);

	}
	else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		CTOP_CTRL_H13_WRITE(0x154, new_fcw_value);

		////////////////////////////////////////////////////////////////////////////
		/* Setting DCO Value */
		/* STC Referrence - will be set time which will apply DCO to system */
		/* Actually, During Read/Write, All DCO Related Function Control Needed.ex. Stop, Set and Restart */
		/* But even if not, Read/Write Action is needed to adapt to the DCO Value really */
		/* Before Read/Write, DCO Value is not adapted really */
		/* read real STC value */
		ui32stcReal_32		= CPU_TOP_H13_READ(0x150);
		ui32stcReal_31_0	= CPU_TOP_H13_READ(0x154);

		/* setting referrence STC value with real stc + 0x10. */
		CPU_TOP_H13_WRITE(0x148, ui32stcReal_32);
		CPU_TOP_H13_WRITE(0x14C, ui32stcReal_31_0 + 0x10);
	}

	return RET_OK;
}


#define NORMAL_STC 0x0011743D

DTV_STATUS_T SDEC_IO_SetSTCMultiply(S_SDEC_PARAM_T *stpSdecParam, UINT32 *pui32Arg)
{
	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return INVALID_PARAMS, "Invalid parameters" );
	LX_SDEC_CHECK_CODE( pui32Arg == NULL, return INVALID_ARGS, "Invalid argument" );

	pwm_context.fcw_trickSpeed = ((*pui32Arg) * NORMAL_STC) / 10;
	pwm_context.fcw_trickSpeed |= FCW_PREFIX;

	if((*pui32Arg) != 10)
		SDEC_IO_SettingFCW(stpSdecParam, pwm_context.fcw_trickSpeed);
	else
		SDEC_IO_SettingFCW(stpSdecParam, pwm_context.fcw_value);

	return OK;
}


/**
 * pwm status clear and data structure init.
 *
 * @param 		fResetCount   pwm reset value
 * @return		void
 * @ingroup sdec_pwm_func
 */
static void pwm_context_reset(S_SDEC_PARAM_T *stpSdecParam, UINT8 cur_ch)
{
	int		i;
	UINT8 	core = 0;

	SDEC_DTV_SOC_Message( SDEC_PCR, "pwm_context_reset CH[%d]",cur_ch);
	
	SDEC_CONVERT_CORE_CH(core, cur_ch);

	for (i = 0; i < PWM_SUM_WINDOW; i++)
		nSumOfDelta[i] = 0;

	pwm_context.new_reset_discard = 5;
	pwm_context.new_adjust_skip   = PWM_SUM_WINDOW-1;
	pwm_context.fcw_value = pwm_context.fcw_base = pwm_fcw_default;

	max_fcw_value = pwm_context.fcw_base + (pwm_fcw_window_sz * pwm_fcw_sz_step);
	min_fcw_value = pwm_context.fcw_base - (pwm_fcw_window_sz * pwm_fcw_sz_step);

	SDEC_IO_SettingFCW(stpSdecParam, pwm_context.fcw_value);

	nDeltaChkCount		= 0;

	SDEC_HAL_STCCEnableCopy(core, cur_ch, SDEC_HAL_ENABLE);

#ifdef __NEW_PWM_RESET_COND__
	pcr_error_for_reset = 0;
#endif
}

/**
 *
 *  Initialisation function, resets hardware on first call
 *
 *  @return			void
 *
 * 	@ingroup sdec_pwm_func
 */
void SDEC_PWM_Init(S_SDEC_PARAM_T	*stpSdecParam )
{
	SDEC_DTV_SOC_Message( SDEC_PCR, "SDEC_PWM_Init\n");

	if(0) // --> fix to original 27M configuration (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		pwm_fcw_default		= ( FCW_DEFAULT_VALUE_27025 | FCW_PREFIX );
	}
	else
	{
		pwm_fcw_default		= ( FCW_DEFAULT_VALUE | FCW_PREFIX );
	}
	pwm_fcw_threshold		= KHZ_TO_FCW(FREQ_THRESHOLD_KHZ);
	pwm_fcw_threshold_with_max_ignore = KHZ_TO_FCW(FREQ_THRESHOLD_KHZ_WITH_MAX_IGNORE);
	pwm_fcw_sz_step		= FCW_SZ_STEP;
	pwm_fcw_window_sz		= FCW_WINDOW_SZ;

	pwm_context_reset(stpSdecParam, 0);
	pwm_context.first_pcr_arrived = FALSE;
}


#ifdef __NEW_PWM_RESET_COND__ /* jinhwan.bae for NEW PWM Reset Condition */

/**
********************************************************************************
* @brief
*   Set PWM Reset Condition
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
* @return
*  DTV_STATUS_T
********************************************************************************
*/
void SDEC_IO_SetPWMResetCondition(S_SDEC_PARAM_T *stpSdecParam, UINT8 ch, BOOLEAN reset)
{
	SDEC_DTV_SOC_Message( SDEC_PCR, "Set PWM Reset Condition CH[%d] Reset[%d] pcr_error_for_reset[%d] ", ch, reset, pcr_error_for_reset );
	
	/* Reset Condition */
	if(reset)
	{
		if(pcr_error_for_reset < MAX_ERROR_FOR_PWM_RESET * 2)
		{
			pcr_error_for_reset++;
		}

		if( pcr_error_for_reset > MAX_ERROR_FOR_PWM_RESET )
		{
			SDEC_DTV_SOC_Message( SDEC_PCR, "PWM Reset by Condition");
			pwm_context_reset(stpSdecParam, ch);
		}
	}
	else /* Normal Case */
	{
		if( pcr_error_for_reset > 0 )
		{
			pcr_error_for_reset--;
		}
	}
}


#endif


/**
********************************************************************************
* @brief
*   Get Download Status in SDEC Driver
* @remarks
*   It's a temporal function for H13, Netcast 4.0 .
*   It's only for detecting Delayed Play mode in Time Shift
*   Return Value is used for work around of our stupid upload speed 
* @par requirements:
*
* @param
* @return
*  DTV_STATUS_T
********************************************************************************
*/

/* SHOULD BE MODIFIED in M14_TBD, H14_TBD */
static BOOLEAN SDEC_IsDownloading(UINT8 ch, S_SDEC_PARAM_T *stpSdecParam)
{
	int i, check_ch = 0;
	BOOLEAN retval = FALSE;

	if(ch == 0) check_ch = 1;
	else if(ch == 1) check_ch = 0;
	else return FALSE;

	for(i=0;i<4;i++)
	{
		if(stpSdecParam->stPIDMap[check_ch][i].used == 0x1)
		{
			retval = TRUE;
			break;
		}
	}

	return retval;
}


/**
********************************************************************************
* @brief
*   PCR Recovery Algorithm Routine
* @remarks
*   HISTORY
*	jinhwan.bae
*	This Routine had been composed at the time of beginning of DTV development in LGE.
*	Now, nobody remember exactly about history of algorithm.
*	It's one of the reason why we can't modify with instantaneous temptation.
*	It's LG's own algorithm accumulated many years.
*	- 2012. 11. 16
*	Inserted one thing to send the command for freerunning with invalid PCR stream
*	Main Target Stream is 330MHz_SR6956_256QAM_STV_Estonia.ts
*	Recovery Characteristics
*		- Almost time : after 500ms from previous PCR interrupt, PCR-STC > 20ms
*		- Sometimes  : corrected pwm value is more than 100 KHz
*	Condition for Free Run
*		- insert delta_error : raising at > 20ms, falling at first pwm correction
*
* @par requirements:
* @param
* @return
*  DTV_STATUS_T
********************************************************************************
*/
void action_pcr(S_SDEC_PARAM_T	*stpSdecParam, UINT8 cur_ch, UINT32 cur_pcr, UINT32 cur_stc)
{
	static SINT32	prev_pcr, prev_stc, prev_jit;
	UINT32 		cur_fcw_value = pwm_context.fcw_value;
	UINT32 		new_fcw_value = pwm_context.fcw_value;
	UINT32		capture_ms = OS_GetMsecTicks();
	SINT32		sDelta_clk;
	UINT32		uDelta_clk;
	SINT32		pcr_diff = 0;
	SINT32		stc_diff = 0;
	SINT32		pcr_jitt = 0;
	UINT32		cur_ch_src = 0;
	BOOLEAN		is_delayedplay = FALSE;
	UINT8		core = 0, org_ch = cur_ch;

	SDEC_CONVERT_CORE_CH(core, cur_ch);

	/* Check if the PCR Recovery @ channel is enabled or not */
	if ( stpSdecParam->bPcrRecoveryFlag[org_ch] == FALSE )
	{
		SDEC_DTV_SOC_Message( SDEC_PCR, "No need to check , PCR Recovery is not Started CH[%d]", org_ch);
  		return;
	}

	/* jinhwan.bae there is only copy and reset by jitter in Delayed Play Mode */
	/* How about remove PCR Recovery at the time of Uploading ? M14_TBD, H14_TBD  */
	/* read uploading status */
	SDEC_HAL_CIDCRead(core, cur_ch, &cur_ch_src);
	cur_ch_src = ((cur_ch_src >> 20) & 0xF);
	if((cur_ch_src == 0x6) || (cur_ch_src == 0x7))
	{
		if( TRUE == SDEC_IsDownloading(cur_ch, stpSdecParam) )
		{
			SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[32m" "uploading mode" "\x1b[0m");
			is_delayedplay = TRUE;
		}
	}
	
	sDelta_clk = (SINT32)cur_pcr - (SINT32)cur_stc;
	uDelta_clk = abs(sDelta_clk);

	/* After pwm_context_reset, we would 5 pcr interrupt to start recovery  */
	if(pwm_context.new_reset_discard != 0)
	{
		pwm_context.new_reset_discard--;
		if(pwm_context.new_reset_discard == 0)
		{
			prev_pcr = cur_pcr;
			prev_stc = cur_stc;
			pwm_context.lastUpdateTime = capture_ms;
			pwm_context.max_ignore_count = 0;
			SDEC_DTV_SOC_Message( SDEC_PCR, "5th pcr interrupt after reset arrived. start PCR recovery at next time");
		}
		return;
	}

	/* Calulate PCR/STC difference, 45KHz Based */
    pcr_diff = cur_pcr - prev_pcr;
	stc_diff = cur_stc - prev_stc;
	pcr_jitt = cur_pcr - cur_stc;

	SDEC_DTV_SOC_Message( SDEC_PCR, "CH[%d],(%5d KHz), FCW[0x%08x]FCW_OFFSET[%6ld], PCR[0x%08x]-STC[0x%08x]=[%d]",
			org_ch,	FCW_TO_KHZ(cur_fcw_value), cur_fcw_value, cur_fcw_value-pwm_context.fcw_base, cur_pcr, cur_stc , pcr_jitt);

	/* Reduce the frequency of PCR Recovery Based on PCR difference. 2000 was from SAA7219 Reference driver , now use 2250 */
	if (abs(pcr_diff) < 2250)	/* 50 ms */
	{
		return; 
	}

	/* No Recovery @ TimeShift Delayed Play Mode, Only Check Reset Condition */
	if (is_delayedplay == TRUE)
	{
		prev_pcr = cur_pcr;
		prev_stc = cur_stc;
		prev_jit = pcr_jitt;
		
		if( abs(pcr_diff) >= MTS_HUGE_PCR_DIFFERENCE)
		{
			SDEC_DTV_SOC_Message( SDEC_PCR, "DPMode: CurPCR[0x%08x]-PrevPCR[0x%08x]=Diff(%d) TOO BIG > MTS_HUGE_PCR_DIFFERENCE -> reset", cur_pcr, prev_pcr, pcr_diff);
			pwm_context_reset(stpSdecParam, org_ch);
		}
		return;	/* No Recovery @ TimeShift Delayed Play Mode */
	}
	
	/* --- Calculate the error, and a correction value, the correction value is based on the VCXO specs, 
		   and should cause the error value to fall to zero in approximately 1 second,
		   ie an adjustment of 1 digit should result in a modification to the clock of approximately 64 cycles per second. 
		   NOTE if we are closely synchronised then the correction value may be zero.                         --- */
	if( abs(pcr_diff) >= MTS_HUGE_PCR_DIFFERENCE)
	{
		/* Current PCR - Previous PCR Check */
		SDEC_DTV_SOC_Message( SDEC_PCR, "CurPCR[0x%08x]-PrevPCR[0x%08x]=Diff(%d) TOO BIG > MTS_HUGE_PCR_DIFFERENCE -> reset", cur_pcr, prev_pcr, pcr_diff);
		pwm_context_reset(stpSdecParam, org_ch);
		return;
	}
	else if(uDelta_clk >= MTS_HUGE_PCR_DIFFERENCE)
	{
		/* Current PCR - Current STC Check */
		SDEC_DTV_SOC_Message( SDEC_PCR, "PCR-STC=(%d) TOO BIG > MTS_HUGE_PCR_DIFFERENCE -> reset", uDelta_clk);
		pwm_context_reset(stpSdecParam, org_ch);
		return;
	}

	/*----- Main Branch Routine -----*/
	if (pwm_context.lastUpdateTime && (uDelta_clk < PWM_MAX_DIFF_IGNORE))
	{
		/* pwm_context.lastUpdateTime : not 0 @ first trial of PCR Recovery after reset. If at least 1 FCW adjust(PCR Recovery) is done after reset, lastUpdateTime is 0 */
		/* If first trial and PCR-STC is less than PWM_MAX_DIFF_IGNORE(36), no recovery is needed */
		/* BYPASS Condition */
		/* jinhwan.bae . BUT if the max_ignore_value is big, the next FCW adjust may be burden of total recovery, because so big */
		SDEC_DTV_SOC_Message( SDEC_PCR, "small PCR-STC. save & return. max_ignore_count [%d]", pwm_context.max_ignore_count);

#ifdef __NEW_PWM_RESET_COND__
		/* jinhwan.bae, Normal and Recovery Sequence, set --  */
		SDEC_IO_SetPWMResetCondition( stpSdecParam, org_ch, FALSE);
#endif
		if(pwm_context.max_ignore_count < 100) /* jinhwan.bae temporally limit to 100, FIXME in M14~) */
		{
			pwm_context.max_ignore_count++;
		}

		/* Goto Save Current PCR, STC Value and Return , Wait Next PCR */
	}
	else if (pwm_context.lastUpdateTime && ((capture_ms - pwm_context.lastUpdateTime) <= 500))
	{
		/* If first trial and rapid arrival of PCR, wait Next PCR to make less the calculation error */
		/* It means, bypass current PCR data if the pcr sample time is small than 500 */
		/* This routine also prevent from "divide by zero" */
		SDEC_DTV_SOC_Message( SDEC_PCR, "PCR arrival is so rapid after reset waiting(%ld) < 500. Bypass and wait Next PCR",(capture_ms - pwm_context.lastUpdateTime));
	}
	else if(uDelta_clk >= MAX_DELTA_CLK)
	{
		SDEC_DTV_SOC_Message( SDEC_PCR, "|PCR-STC| > MAX_DELTA_CLK, update reset condition");
		/* jinhwan.bae, Error Case but need to waiting */
		SDEC_IO_SetPWMResetCondition( stpSdecParam, org_ch, TRUE);
	}
	else if (pwm_context.lastUpdateTime)
	{
		UINT32 elapsed_ms;
		SINT32 adjust;
		SINT32 coef = (27000000/45000);

		/*---------- condition reached to first recovery after reset -----------*/
		/* uDelta_clk is based on 45Khz clock	*/
		if(capture_ms > pwm_context.lastUpdateTime)
			elapsed_ms = capture_ms - pwm_context.lastUpdateTime;
		else
			elapsed_ms = capture_ms + (0xffffffff - pwm_context.lastUpdateTime);

		SDEC_DTV_SOC_Message( SDEC_PCR, "First Recovery after pwm_context_reset, elapsed ms(%5d) ",elapsed_ms);

		adjust = ( ((coef * uDelta_clk * 1000 ) / elapsed_ms) * pwm_fcw_sz_step ) / 250;

		if (sDelta_clk < 0)
			adjust = -adjust;

		SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[31m" "estimated instantaneous frequency = %lu.%03lu Mhz" "\x1b[0m",
			FCW_TO_KHZ(pwm_context.fcw_value + adjust) / 1000, FCW_TO_KHZ(pwm_context.fcw_value + adjust) % 1000);

		/*---------- IF adjustment is error case, wait next pcr ------------*/
		if(pwm_context.max_ignore_count > 10 )
		{
			/* it means, lots of stable pcr were arrived before first recovery after pwm_context_reset. */
			/* so if the current adjust is so big, skip the adjustment (it may jitter), and wait next pcr */
			/* The state(condition) leaves as lastupdatetime, it means, wait next pcr at first recovery state */
			/* jinhwan.bae this routine is made 2013. 03. for protecting timeshift clock fluctuation */
			/* it means, if we block pcr recovery at timeshift, this case never happen */
			if( abs(adjust) > pwm_fcw_threshold_with_max_ignore )
			{
				SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[31m" "adjust[%d]>threshold[%d] It May Jitter! first recovery after reset and stable max ignore pcr!" "\x1b[0m",
					adjust,	pwm_fcw_threshold_with_max_ignore);

				prev_pcr = cur_pcr;
				prev_stc = cur_stc;
				prev_jit = pcr_jitt;
				return;
			}
		}
		else
		{
			/* we dicide, recovery is needed. but the adjustment value is bigger than threshold, reset pwm and wait next recovery */
			if( abs(adjust) > pwm_fcw_threshold)
			{
				SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[31m" "adjust[%d]>threshold[%d] reset!" "\x1b[0m",
					adjust,	pwm_fcw_threshold);

				pwm_context_reset(stpSdecParam, org_ch);
				return;
			}
		}

 		/*--------  First Adjustment !! --------*/
		pwm_context.fcw_value			+= (adjust * 103 ) / 100;
		pwm_context.lastUpdateTime		 = 0;
		pwm_context.new_adjust_skip		 = PWM_SUM_WINDOW-1;
		pwm_context.skip_chk_overshoot	 = PWM_SUM_WINDOW;
		nTimeStamps[-1] = capture_ms;

		new_fcw_value = pwm_context.fcw_value;

		if      (new_fcw_value < min_fcw_value) new_fcw_value = min_fcw_value;
		else if (new_fcw_value > max_fcw_value) new_fcw_value = max_fcw_value;

		SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[33m" "FIRST Adjustment! CH[%d]ADJ[%6d]FCWDiff[%6ld]NewFCW[0x%08x]CurFCW[%08x]Elapsedms[%5d]PCR[%08x]-STC[%08x]=[%d]" "\x1b[0m",
					cur_ch,	adjust,	new_fcw_value-pwm_context.fcw_base,	new_fcw_value, cur_fcw_value, elapsed_ms, cur_pcr, cur_stc , pcr_jitt);

		SDEC_IO_SettingFCW(stpSdecParam, new_fcw_value);

		pwm_context.fcw_value = new_fcw_value;
#ifdef __NEW_PWM_RESET_COND__
		/* jinhwan.bae, Normal and Recovery Sequence, set --  */
		SDEC_IO_SetPWMResetCondition( stpSdecParam, org_ch, FALSE);
#endif
	}
	else
	{
		UINT32	changeMask;
		SINT32	adjust = 0;
		SINT32	nSignOfDelta;
		SINT32	nSignOfSlope, nDiffSlope;
		UINT32	index, o_idx;
		int		i, nOvershoot = 0;

		/*---------- condition reached to recovery after first recovery, normal case -----------*/

		o_idx	= nDeltaChkCount / PWM_DELTA_WINDOW;
		index	= (o_idx % PWM_SUM_WINDOW);

		nSumOfDelta[index] += sDelta_clk;
		nDeltaChkCount++;

#ifdef __NEW_PWM_RESET_COND__
		/* jinhwan.bae, Normal and Recovery Sequence, set --  */
		SDEC_IO_SetPWMResetCondition( stpSdecParam, org_ch, FALSE);
#endif
		if ((nDeltaChkCount % PWM_DELTA_WINDOW) == 0)
		{
			nTimeStamps[index]		= capture_ms;
			nMstElapsed[index]		= nTimeStamps[index] - nTimeStamps[index-1];
			nMeanOfDelta[index] 	= 600 * 1000 * (SINT64) nSumOfDelta[index];
			DO_DIV_SINT64(nMeanOfDelta[index],PWM_DELTA_WINDOW);
			DO_DIV_SINT64(nMeanOfDelta[index], nMstElapsed[index]);
			nAbsOfDelta[index]		= abs(nMeanOfDelta[index]);

			if (pwm_context.new_adjust_skip > 0)
				pwm_context.new_adjust_skip--;

			if (o_idx >= PWM_SUM_WINDOW)
			{
				nSignOfDelta  = ((nMeanOfDelta[index] >= 0) ? 1 : -1);
				nSignOfSlope  = ((nAbsOfDelta[index-3] > nAbsOfDelta[index-4]) ? 0x1000 : 0);
				nSignOfSlope += ((nAbsOfDelta[index-2] > nAbsOfDelta[index-3]) ? 0x0100 : 0);
				nSignOfSlope += ((nAbsOfDelta[index-1] > nAbsOfDelta[index-2]) ? 0x0010 : 0);
				nSignOfSlope += ((nAbsOfDelta[index-0] > nAbsOfDelta[index-1]) ? 0x0001 : 0);

				changeMask	  = ((nMeanOfDelta[index-3] > 0) ? 0x1000 : 0);
				changeMask	 |= ((nMeanOfDelta[index-2] > 0) ? 0x0100 : 0);
				changeMask	 |= ((nMeanOfDelta[index-1] > 0) ? 0x0010 : 0);
				changeMask	 |= ((nMeanOfDelta[index-0] > 0) ? 0x0001 : 0);

				nDiffSlope = ((int)nAbsOfDelta[index] - (int)nAbsOfDelta[index-4])/PWM_SUM_WINDOW;
#if 0
				SDEC_DTV_SOC_Message( SDEC_PCR, "nMeanOfDelta[%lld, %lld, %lld, %lld]",
					nMeanOfDelta[index-3],	nMeanOfDelta[index-2],	nMeanOfDelta[index-1],	nMeanOfDelta[index-0]);
				SDEC_DTV_SOC_Message( SDEC_PCR, "new_adjust_skip[%lu] changeMask[0x%04x] nAbsOfDelta[%d], nSignOfSlope[0x%04x] nOvershoot[%d]",
					pwm_context.new_adjust_skip, changeMask, nAbsOfDelta[index], nSignOfSlope, nOvershoot);
#endif
				if (pwm_context.skip_chk_overshoot == 0)
				{
					for (i = 0; i < PWM_SUM_WINDOW; i++)
					{
					  if (nAbsOfDelta[index-i] > 2700) nOvershoot++;
					}
				}

				if (pwm_context.new_adjust_skip > 0)
				{
					/* *	Skip 3 times just after last adjustment	 */
				}
				else if ((changeMask == 0x1110) || (changeMask == 0x1))
				{
					/* * If difference sign is changed (+ -> - or - -> +), adjust Center Frequency */
					int diff = (nMeanOfDelta[index] - nMeanOfDelta[index-4])/PWM_SUM_WINDOW;

					pwm_context.new_adjust_skip = PWM_SUM_WINDOW-1;
					pwm_context.skip_chk_overshoot = 0;
					adjust = (diff * 103 * pwm_fcw_sz_step) / (250 * 100);

					SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[32m" "CH[%d][ChangeMASK] adjust(%3d) diff[%4d]" "\x1b[0m",
									cur_ch,  adjust, diff);
					SDEC_DTV_SOC_Message( SDEC_PCR, "[ch%d] nMeanOfDelta[%d]%lld] nMeanOfDelta[%d][%lld]",
									cur_ch,  index, nMeanOfDelta[index], index-4, nMeanOfDelta[index-4]);
				}
				else if (nAbsOfDelta[index] < 250)
				{
					/* *	System Clock has been almost converged, 	Nothing to do.	 */
					pwm_context.skip_chk_overshoot = 0;
				}
				else if (nSignOfSlope == 0x1111)
				{
					/* *	System Clock is going to diverge. MUST adjust FCW value to slope	 */
					pwm_context.new_adjust_skip = PWM_SUM_WINDOW-1;
					adjust = nSignOfDelta * (nDiffSlope * 120)  * pwm_fcw_sz_step / (250 * 100);

					SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[32m" "CH[%d][Divergence] adjust(%3d) nSignOfDelta[0x%04x] nDiffSlope[%4d]" "\x1b[0m",
									cur_ch,  adjust, nSignOfDelta, nDiffSlope);
					SDEC_DTV_SOC_Message( SDEC_PCR, "[ch%d] nMeanOfDelta[%d]%lld] nMeanOfDelta[%d][%lld]",
									cur_ch,  index, nMeanOfDelta[index], index-4, nMeanOfDelta[index-4]);
				}
				else if (nOvershoot >= 2)
				{
					/* *	Jitter is too big, speed up more. MUST adjust FCW value to slope 	 */
					int diff1 = (nMeanOfDelta[index] - nMeanOfDelta[index-4])/PWM_SUM_WINDOW;
					int diff2 = (nMeanOfDelta[index] / 8);

					if      ((diff1 ^ diff2) &    0x8000000) diff1 = 0;
					else if ((diff1 < 0) && (diff2 < diff1)) diff1 = diff2;
					else if ((diff1 > 0) && (diff2 > diff1)) diff1 = diff2;

					pwm_context.new_adjust_skip = PWM_SUM_WINDOW-1;
					pwm_context.skip_chk_overshoot = PWM_SUM_WINDOW;
					adjust = (diff1 * 120  * pwm_fcw_sz_step) / (250*100);

					SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[32m" "CH[%d][JITTER][nOvershoot >= 2] adjust(%3d) diff1[%4d] diff2[%4d]" "\x1b[0m",
									cur_ch,  adjust, diff1, diff2);
					SDEC_DTV_SOC_Message( SDEC_PCR, "[ch%d] nMeanOfDelta[%d]%lld] nMeanOfDelta[%d][%lld]",
									cur_ch,  index, nMeanOfDelta[index], index-4, nMeanOfDelta[index-4]);
				}
				else if ((changeMask != 0x1111) && (changeMask != 0x0000))
				{
					if (pwm_context.skip_chk_overshoot > 0)
						pwm_context.skip_chk_overshoot--;
				}
#if	0
				{
				int	idx_3 = ((index + PWM_SUM_WINDOW - 3) % PWM_SUM_WINDOW);
				int	idx_2 = ((index + PWM_SUM_WINDOW - 2) % PWM_SUM_WINDOW);
				int	idx_1 = ((index + PWM_SUM_WINDOW - 1) % PWM_SUM_WINDOW);

				dbgprint(", %4d, %4d, %4d, %4d,  %5d, %5d, %5d,  %5d,  %2d, ss, %04x, cm, %04x, %d(%d), %d",
							nMstElapsed [idx_3], nMstElapsed [idx_2], nMstElapsed [idx_1], nMstElapsed [index],
							nMeanOfDelta[idx_3], nMeanOfDelta[idx_2], nMeanOfDelta[idx_1], nMeanOfDelta[index],
							nSignOfDelta, nSignOfSlope, changeMask, nDiffSlope,
							adjust, pwm_context.skip_chk_overshoot);
				}
#endif
				if (adjust == 0)
				{
					/* No action needed if adjust == 0 */
				}
				else
				{
					new_fcw_value += adjust;

					/* PWM Register Setting */
					SDEC_IO_SettingFCW(stpSdecParam, new_fcw_value);
					pwm_context.fcw_value = new_fcw_value;

					SDEC_DTV_SOC_Message( SDEC_PCR, "\x1b[34m" "CH[%d] NEW FCW, 0x%06x==>0x%06x, adjust(%3d) PCR[%08x]-STC[%08x]=%d" "\x1b[0m",
								cur_ch,	cur_fcw_value, new_fcw_value, adjust, cur_pcr,cur_stc , pcr_jitt );
				}
			}

			if (index == (PWM_SUM_WINDOW-1))
			{
				nTimeStamps[-1] = nTimeStamps[index];
			}

			nAbsOfDelta[index-4]  = nAbsOfDelta[index];
			nMeanOfDelta[index-4] = nMeanOfDelta[index];
			nSumOfDelta[(index+1)%PWM_SUM_WINDOW] = 0;
		}
	}

	prev_pcr = cur_pcr;
	prev_stc = cur_stc;
	prev_jit = pcr_jitt;

}

void SDEC_PCRRecovery(struct work_struct *work)
{
	S_SDEC_PARAM_T 	*stpSdecParam;
	UINT32 			ui32Curr_stc 				= 0x0;
	UINT32 			ui32Curr_pcr 				= 0x0;
	UINT8 			cur_ch 						= 0x0;

	stpSdecParam =	container_of (work, S_SDEC_PARAM_T, PcrRecovery);

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return, "Invalid parameters" );

	cur_ch			= stpSdecParam->stPCR_STC.ui8Channel;
	ui32Curr_stc 	= stpSdecParam->stPCR_STC.STC_hi_value;
	ui32Curr_pcr 	= stpSdecParam->stPCR_STC.PCR_hi_value;

	action_pcr(stpSdecParam, cur_ch, ui32Curr_pcr,ui32Curr_stc);
}


void SDEC_TPIIntr(struct work_struct *work)
{
	S_SDEC_PARAM_T 	*stpSdecParam;
	UINT8 	ui8Ch 		= 0x0;
	UINT8	ui8PIDIdx	= 0x0;

	stpSdecParam =	container_of (work, S_SDEC_PARAM_T, TPIIntr);

	LX_SDEC_CHECK_CODE( stpSdecParam == NULL, return, "Invalid parameters" );

	ui8Ch			= stpSdecParam->stTPI_Intr.ui8Channel;
	ui8PIDIdx 		= stpSdecParam->stTPI_Intr.ui8PIDIdx;

	SDEC_SetPidf_TPI_IEN_Enable(stpSdecParam, ui8Ch, ui8PIDIdx, SDEC_HAL_DISABLE);
	SDEC_HAL_TPISetIntrPayloadUnitStartIndicator(ui8Ch, SDEC_HAL_DISABLE);
}

