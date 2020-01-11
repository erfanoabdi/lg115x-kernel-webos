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

/*****************************************************************************
**
**  Name:demod_drv_m14a0.c
**
**  Description:    ABB/GBB point functions interface.
**
**  Functions
**  Implemented:   LX_DEMOD_CFG_T*   DEMOD_M14A0_GetCfg
**                 void	DEMOD_M14A0_InitHAL
**
**  References:    
**
**  Exports:       
**
**  Dependencies:   demod_impl.h for system configuration data.
**				
**
**  Revision History:
**
**     Date        Author          Description
**  -------------------------------------------------------------------------
**   30-12-2009  Jeongpil Yun    Initial draft.
**   31-07-2013   Jeongpil Yun    
**
*****************************************************************************/



#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>			/**< printk() */
#include <linux/slab.h>			 	/**< kmalloc() */
#include <linux/fs.h> 				/**< everything\ldots{} */
#include <linux/types.h>		 	/**< size_t */
#include <linux/fcntl.h>			/**< O_ACCMODE */
#include <asm/uaccess.h>
#include <linux/ioport.h>			/**< For request_region, check_region etc */
#include <asm/io.h>					/**< For ioremap_nocache */
#include <linux/workqueue.h>		/**< For working queue */
#include <linux/interrupt.h>
#include <linux/irq.h>

#include "demod_impl.h"
#include "demod_reg_m14a0.h"

#include "demod_common_m14a0.h"
#include "demod_dvb_m14a0.h"
#include "demod_vqi_m14a0.h"
#include "demod_analog_m14a0.h"

#if 0
#endif

LX_DEMOD_COMMON_REG_T 	gpRealM14A0COMMON_Ax;
LX_DEMOD_SYNC_REG_T 	gpRealM14A0SYNC_Ax;
LX_DEMOD_EQ_V_REG_T 	gpRealM14A0EQ_V_Ax;
LX_DEMOD_EQ_DI_REG_T 	gpRealM14A0EQ_DI_Ax;
LX_DEMOD_EQ_CQS_REG_T 	gpRealM14A0CQS_Ax;
LX_DEMOD_FEC_REG_T	 	gpRealM14A0FEC_Ax;


LX_DEMOD_COMMON_REG_T 	*gpRegM14A0COMMON_Ax 	=  &gpRealM14A0COMMON_Ax;
LX_DEMOD_SYNC_REG_T 	*gpRegM14A0SYNC_Ax	=  &gpRealM14A0SYNC_Ax;
LX_DEMOD_EQ_V_REG_T 	*gpRegM14A0EQ_V_Ax 	=  &gpRealM14A0EQ_V_Ax;
LX_DEMOD_EQ_DI_REG_T 	*gpRegM14A0EQ_DI_Ax	=  &gpRealM14A0EQ_DI_Ax;
LX_DEMOD_EQ_CQS_REG_T 	*gpRegM14A0CQS_Ax 	=  &gpRealM14A0CQS_Ax;
LX_DEMOD_FEC_REG_T	 	*gpRegM14A0FEC_Ax 	=  &gpRealM14A0FEC_Ax;


const LX_DEMOD_CFG_T*   DEMOD_M14A0_GetCfg(void);



/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
const static	DEMOD_HAL_T		g_demod_hal_m14a0 =
{
		.GetCfg	   					=  DEMOD_M14A0_GetCfg,


/***************************************************************************************
* DVBT/ DVBC/ VSB/ QAM/ ISDBT  common  function
****************************************************************************************/

		.ResetHW					= DEMOD_M14A0_ResetHW,	
		.SetI2C						= DEMOD_M14A0_SetI2C,	
		.SetInclk 					= DEMOD_M14A0_SetInclk,	
		.AdcInit					= DEMOD_M14A0_AdcInit,

		.Get_Id						= DEMOD_M14A0_Get_Id,
		.RepeaterEnable 	  		= DEMOD_M14A0_RepeaterEnable,
		.SoftwareReset		 		= DEMOD_M14A0_SoftwareReset,
		.Serial_Control 	 		= DEMOD_M14A0_Serial_Control,
		.Power_Save 	  			= DEMOD_M14A0_Power_Save,
		.StdOperModeContrl		 	= DEMOD_M14A0_StdOperModeContrl,
		.NeverlockScan				= DEMOD_M14A0_NeverlockScan,
		.TPOutCLKEnable 	  		= DEMOD_M14A0_TPOutEnable,
		.Set_IF_Frq 	  			= DEMOD_M14A0_Set_IF_Frq,
		.Set_AGCPolarity	   		= DEMOD_M14A0_Set_AGCPolarity,
		.Set_SpectrumCtrl			= DEMOD_M14A0_Set_SpectrumCtrl,
		.Get_SpectrumStatus 	  	= DEMOD_M14A0_Get_SpectrumStatus,
		.Get_IFAGC		 			= DEMOD_M14A0_Get_IFAGC,
		.Get_OperMode				= DEMOD_M14A0_Get_OperMode,
		.Get_NeverLockStatus	   	= DEMOD_M14A0_Get_NeverLockStatus,
		.Get_CarrierFreqOffset		= DEMOD_M14A0_Get_CarrierFreqOffset,
		.Get_TPIFStatus 	  		= DEMOD_M14A0_Get_TPIFStatus,
		.Get_VABER		 			= DEMOD_M14A0_Get_VABER,
		.Get_Packet_Error			= DEMOD_M14A0_Get_Packet_Error,
		.Get_BandWidthMode			= DEMOD_M14A0_Get_BandWidthMode,
		.Get_QAMMode				= DEMOD_M14A0_Get_QAMMode,
		.Get_MseInfo 				= DEMOD_M14A0_Get_MseInfo,
		.Get_Lock					= DEMOD_M14A0_Get_Lock,
		.Get_SysLockTime			= DEMOD_M14A0_VSB_Get_SysLockTime,
		.Get_FecLockTime			= DEMOD_M14A0_VSB_Get_FecLockTime,
		

/***************************************************************************************
* DVBT/ DVBC common  function
****************************************************************************************/


/***************************************************************************************
* DVBC function
****************************************************************************************/

		.DVBC_AutoSymbolRateDet 	  		= DEMOD_M14A0_DVBC_AutoSymbolRateDet,
		.DVBC_IsSymbolRateAuto		 		= DEMOD_M14A0_DVBC_IsSymbolRateAuto,
		.DVBC_IsQammodeAutoDet		 		= DEMOD_M14A0_DVBC_IsQammodeAutoDet,
		.DVBC_Set_DefaultRegisterValue		= DEMOD_M14A0_DVBC_Set_DefaultRegisterValue,
		.DVBC_Set_Config_auto				= DEMOD_M14A0_DVBC_Set_Config_auto,
		.DVBC_Set_QamMode 					= DEMOD_M14A0_DVBC_Set_QamMode,
		.DVBC_Set_NeverLockWaitTime 	  	= DEMOD_M14A0_DVBC_Set_NeverLockWaitTime,
		.DVBC_Get_SymbolRateDetect		 	= DEMOD_M14A0_DVBC_Get_SymbolRateDetect,
		.DVBC_Get_SymbolRateStatus		 	= DEMOD_M14A0_DVBC_Get_SymbolRateStatus,
		.DVBC_Get_QamModeDetectStatus		= DEMOD_M14A0_DVBC_Get_QamModeDetectStatus,
		.DVBC_Get_DvbInfo					= DEMOD_M14A0_DVBC_Get_DvbInfo,

		.DVBC_Obtaining_Signal_Lock			= DEMOD_M14A0_DVBC_Obtaining_Signal_Lock,
		.DVBC_Monitoring_Signal_Lock 		= DEMOD_M14A0_DVBC_Monitoring_Signal_Lock,

/***************************************************************************************
* DVBC2 function
****************************************************************************************/

		.DVBC2_IsQammodeAutoDet 			= NULL,
		.DVBC2_Set_DefaultRegisterValue 	= NULL,
		.DVBC2_Set_Config_auto				= NULL,
		.DVBC2_Set_PartialConfig			= NULL,
		.DVBC2_Set_FullConfig				= NULL,
		.DVBC2_Set_QamMode					= NULL,
		.DVBC2_Set_NeverLockWaitTime		= NULL,
		.DVBC2_Set_StartFrequency			= NULL,
		.DVBC2_Get_QamModeDetectStatus		= NULL,
		.DVBC2_Get_multiPLP_ID				= NULL,
		.DVBC2_Get_DvbInfo					= NULL,

		.DVBC2_Obtaining_Signal_Lock		= NULL,
		.DVBC2_Monitoring_Signal_Lock		= NULL,


/***************************************************************************************
* DVBT function
****************************************************************************************/

		.DVBT_Set_DefaultRegisterValue		= DEMOD_M14A0_DVBT_Set_DefaultRegisterValue,
		.DVBT_Set_Config_auto				= DEMOD_M14A0_DVBT_Set_Config_auto,
		.DVBT_Set_PartialConfig 	  		= DEMOD_M14A0_DVBT_Set_PartialConfig,
		.DVBT_Set_FullConfig	   			= DEMOD_M14A0_DVBT_Set_FullConfig,
		.DVBT_Set_NeverLockWaitTime 	  	= DEMOD_M14A0_DVBT_Set_NeverLockWaitTime,
		.DVBT_Get_DelaySpreadStatus 	  	= DEMOD_M14A0_DVBT_Get_DelaySpreadStatus,
		.DVBT_Get_Hierach_HPSel 	  		= DEMOD_M14A0_DVBT_Get_Hierach_HPSel,
		.DVBT_Get_FFTMode					= DEMOD_M14A0_DVBT_Get_FFTMode,
		.DVBT_Get_GuradIntervalMode 	  	= DEMOD_M14A0_DVBT_Get_GuradIntervalMode,
		.DVBT_Get_HierachyMode				= DEMOD_M14A0_DVBT_Get_HierachyMode,
		.DVBT_Get_LpCoderRate				= DEMOD_M14A0_DVBT_Get_LpCoderRate,
		.DVBT_Get_HpCoderRate				= DEMOD_M14A0_DVBT_Get_HpCoderRate,
		.DVBT_Get_CellId	   				= DEMOD_M14A0_DVBT_Get_CellId,
		.DVBT_Get_TpsInfo					= DEMOD_M14A0_DVBT_Get_TpsInfo,
		.DVBT_Get_TotalInfo 	  			= DEMOD_M14A0_DVBT_Get_TotalInfo,
		.DVBT_Get_IFO_LOCK		 			= DEMOD_M14A0_DVBT_Get_IFO_LOCK,
		.DVBT_Get_CochanDetIndicator	   	= DEMOD_M14A0_DVBT_Get_CochanDetIndicator,
		.DVBT_EqualizereReset				= DEMOD_M14A0_DVBT_EqualizereReset,

		.DVBT_Obtaining_Signal_Lock			= DEMOD_M14A0_DVBT_Obtaining_Signal_Lock,
		.DVBT_Monitoring_Signal_Lock 		= DEMOD_M14A0_DVBT_Monitoring_Signal_Lock,

/***************************************************************************************
* DVBT2 function
****************************************************************************************/

		.DVBT2_Set_DefaultRegisterValue 	= NULL,
		.DVBT2_Set_Config_auto				= NULL,
		.DVBT2_Set_PartialConfig			= NULL,
		.DVBT2_Set_FullConfig				= NULL,
		.DVBT2_Set_NeverLockWaitTime		= NULL,
		.DVBT2_Get_DelaySpreadStatus		= NULL,
		.DVBT2_Get_FFTMode					= NULL,
		.DVBT2_Get_GuradIntervalMode		= NULL,
		.DVBT2_Get_CoderRate				= NULL,
		.DVBT2_Get_PlpInfo					= NULL,
		.DVBT2_Get_multiPLP_ID				= NULL,
		.DVBT2_Get_TotalInfo				= NULL,

		.DVBT2_Obtaining_Signal_Lock		= NULL,
		.DVBT2_Monitoring_Signal_Lock		= NULL,



/***************************************************************************************
* VSB/ QAM/ ISDBT common  function
****************************************************************************************/



/***************************************************************************************
* VSB function
****************************************************************************************/

		.VSB_Set_DefaultRegisterValue		= DEMOD_M14A0_VSB_Set_DefaultRegisterValue,
		.VSB_Set_NeverLockWaitTime			= DEMOD_M14A0_VSB_Set_NeverLockWaitTime,
		.VSB_CochannelExist 	  			= DEMOD_M14A0_VSB_CochannelExist,
		.VSB_PreMonitor 	  				= DEMOD_M14A0_VSB_PreMonitor,
		.VSB_Monitor	   					= DEMOD_M14A0_VSB_Monitor,
		.VSB_Get_MSEdynStatus				= DEMOD_M14A0_VSB_Get_MSEdynStatus,
		.VSB_Get_TotalInfo		 			= DEMOD_M14A0_VSB_Get_TotalInfo,

		.VSB_Obtaining_Signal_Lock			= DEMOD_M14A0_VSB_Obtaining_Signal_Lock,
		.VSB_Monitoring_Signal_Lock 		= DEMOD_M14A0_VSB_Monitoring_Signal_Lock,


/***************************************************************************************
* QAM function
****************************************************************************************/

		.QAM_SoftwareResetFEC				= DEMOD_M14A0_QAM_SoftwareResetFEC,
		.QAM_Monitor	   					= DEMOD_M14A0_QAM_Monitor,
		.QAM_ModeAutoDetection		 		= DEMOD_M14A0_QAM_ModeAutoDetection,
		.QAM_64Mode 	  					= DEMOD_M14A0_QAM_64Mode,
		.QAM_256Mode	   					= DEMOD_M14A0_QAM_256Mode,
		.QAM_Set_DefaultRegisterValue		= DEMOD_M14A0_QAM_Set_DefaultRegisterValue,
		.QAM_Set_NeverLockWaitTime			= DEMOD_M14A0_QAM_Set_NeverLockWaitTime,

		.QAM_Get_TotalInfo		 			= DEMOD_M14A0_QAM_Get_TotalInfo,

		.QAM_Obtaining_Signal_Lock			= DEMOD_M14A0_QAM_Obtaining_Signal_Lock,
		.QAM_Monitoring_Signal_Lock 		= DEMOD_M14A0_QAM_Monitoring_Signal_Lock,
		.QAM_EQ_Signal_Detector   			= DEMOD_M14A0_QAM_EQ_Signal_Detector,



/***************************************************************************************
* ISDBT function
****************************************************************************************/

		.ISDBT_Set_DefaultRegisterValue 	= DEMOD_M14A0_ISDBT_Set_DefaultRegisterValue,
		.ISDBT_Set_Config_auto		 		= DEMOD_M14A0_ISDBT_Set_Config_auto,
		.ISDBT_Set_PartialConfig	   		= DEMOD_M14A0_ISDBT_Set_PartialConfig,
		.ISDBT_Set_FullConfig				= DEMOD_M14A0_ISDBT_Set_FullConfig,
		.ISDBT_Set_NeverLockWaitTime	   	= DEMOD_M14A0_ISDBT_Set_NeverLockWaitTime,
		.ISDBT_Get_DelaySpreadStatus	   	= DEMOD_M14A0_ISDBT_Get_DelaySpreadStatus,
		.ISDBT_Get_FFTMode		 			= DEMOD_M14A0_ISDBT_Get_FFTMode,
		.ISDBT_Get_GuradIntervalMode	   	= DEMOD_M14A0_ISDBT_Get_GuradIntervalMode,
		.ISDBT_Get_TMCCInfo 	  			= DEMOD_M14A0_ISDBT_Get_TMCCInfo,
		.ISDBT_Get_TotalInfo	   			= DEMOD_M14A0_ISDBT_Get_TotalInfo,
		.ISDBT_Get_IFO_LOCK 	  			= DEMOD_M14A0_ISDBT_Get_IFO_LOCK,
		.ISDBT_Get_CochanDetIndicator		= DEMOD_M14A0_ISDBT_Get_CochanDetIndicator,
		.ISDBT_EqualizereReset		 		= DEMOD_M14A0_ISDBT_EqualizereReset,

		.ISDBT_Obtaining_Signal_Lock		= DEMOD_M14A0_ISDBT_Obtaining_Signal_Lock,
		.ISDBT_Monitoring_Signal_Lock 		= DEMOD_M14A0_ISDBT_Monitoring_Signal_Lock,




/***************************************************************************************
* ABB  function
****************************************************************************************/

		.ADEMOD_Demod_Open				 	= DEMOD_M14A0_ANALOG_Demod_Open,
		.ADEMOD_Init						= DEMOD_M14A0_ANALOG_Init,
		.ADEMOD_Set_IF_Frq					= DEMOD_M14A0_ANALOG_Set_IF_Frq,
		.ADEMOD_SoftwareReset				= DEMOD_M14A0_ANALOG_SoftReset,
		.ADEMOD_Set_AftRange 				= DEMOD_M14A0_ANALOG_Set_AftRange,

		.ADEMOD_ResetHW						= DEMOD_M14A0_ANALOG_ResetHW,	
		.ADEMOD_SetI2C						= DEMOD_M14A0_ANALOG_SetI2C,	
		.ADEMOD_Set_AbbMode 				= DEMOD_M14A0_ANALOG_Set_AbbMode,
		.ADEMOD_Set_WorkAround	 			= DEMOD_M14A0_ANALOG_Set_WorkAround,
		.ADEMOD_Set_CvbsRateConversion		= DEMOD_M14A0_ANALOG_Set_CvbsRateConversion,
		.ADEMOD_Set_HighCvbsRateOffset		= DEMOD_M14A0_ANALOG_Set_HighCvbsRateOffset,
		.ADEMOD_Set_ClampingCtrl			= NULL,
		.ADEMOD_Set_SifCtrl					= DEMOD_M14A0_ANALOG_Set_SifCtrl,
		.ADEMOD_Set_CvbsDecCtrl				= DEMOD_M14A0_ANALOG_Set_CvbsDecCtrl,
		.ADEMOD_Set_SifPathCtrl				= DEMOD_M14A0_ANALOG_Set_SifPathCtrl,
		.ADEMOD_Set_SpectrumInv				= DEMOD_M14A0_ANALOG_Set_SpectrumInv,
		.ADEMOD_Set_SpecialSifData 			= DEMOD_M14A0_ANALOG_ChangeFilter4SIF,

		
		.ADEMOD_Obtaining_Signal_Lock		= DEMOD_M14A0_ANALOG_Obtaining_Signal_Lock,
		.ADEMOD_Monitoring_Signal_Lock 		= DEMOD_M14A0_ANALOG_Monitoring_Signal_Lock,
			
		.ADEMOD_Dbg_Get_RegDump					= DEMOD_M14A0_ANALOG_Dbg_Get_RegDump,
		.ADEMOD_Dbg_Set_RegValue 				= DEMOD_M14A0_ANALOG_Dbg_Set_RegisterValue,
		.ADEMOD_Dbg_Set_SmartTune 				= DEMOD_M14A0_ANALOG_Dbg_SmartTune,
		.ADEMOD_Dbg_Get_RegValue				= DEMOD_M14A0_ANALOG_Dbg_Get_RegisterValue,
		.ADEMOD_Dbg_TestFunctions 				= DEMOD_M14A0_ANALOG_Dbg_TestFunctions,
			

};
const static	LX_DEMOD_CFG_T 		g_demod_cfg_m14a0 =
{

};

/*========================================================================================
    Implementation Group
========================================================================================*/

/** get L9 specific configuration
 *
 *  @return LX_DEMOD_CFG_T
 */
const LX_DEMOD_CFG_T*   DEMOD_M14A0_GetCfg(void)
{
    return &g_demod_cfg_m14a0;
}

void	DEMOD_M14A0_InitHAL( DEMOD_HAL_T*	hal )
{
	memcpy( hal, &g_demod_hal_m14a0, sizeof(DEMOD_HAL_T));
}

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/


/*========================================================================================
	Implementation Group
========================================================================================*/


