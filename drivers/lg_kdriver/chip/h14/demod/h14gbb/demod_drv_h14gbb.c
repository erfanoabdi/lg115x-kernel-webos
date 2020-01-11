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
#include "demod_reg_h14gbb.h"

#include "demod_common_h14gbb.h"
#include "demod_dvb_h14gbb.h"
#include "demod_vqi_h14gbb.h"

#if 0
#endif

LX_DEMOD_COMMON_REG_T 	gpRealH14GBBCOMMON_Ax;
LX_DEMOD_SYNC_REG_T 	gpRealH14GBBSYNC_Ax;
LX_DEMOD_EQ_V_REG_T 	gpRealH14GBBEQ_V_Ax;
LX_DEMOD_EQ_DI_REG_T 	gpRealH14GBBEQ_DI_Ax;
LX_DEMOD_EQ_CQS_REG_T 	gpRealH14GBBCQS_Ax;
LX_DEMOD_FEC_REG_T	 	gpRealH14GBBFEC_Ax;


LX_DEMOD_COMMON_REG_T 	*gpRegH14GBBCOMMON_Ax 	=  &gpRealH14GBBCOMMON_Ax;
LX_DEMOD_SYNC_REG_T 	*gpRegH14GBBSYNC_Ax	=  &gpRealH14GBBSYNC_Ax;
LX_DEMOD_EQ_V_REG_T 	*gpRegH14GBBEQ_V_Ax 	=  &gpRealH14GBBEQ_V_Ax;
LX_DEMOD_EQ_DI_REG_T 	*gpRegH14GBBEQ_DI_Ax	=  &gpRealH14GBBEQ_DI_Ax;
LX_DEMOD_EQ_CQS_REG_T 	*gpRegH14GBBCQS_Ax 	=  &gpRealH14GBBCQS_Ax;
LX_DEMOD_FEC_REG_T	 	*gpRegH14GBBFEC_Ax 	=  &gpRealH14GBBFEC_Ax;


const LX_DEMOD_CFG_T*   DEMOD_H14GBB_GetCfg(void);



/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
const static	DEMOD_HAL_T		g_demod_hal_h14gbb =
{
		.GetCfg	   					=  DEMOD_H14GBB_GetCfg,


/***************************************************************************************
* DVBT/ DVBC/ VSB/ QAM/ ISDBT  common  function
****************************************************************************************/

		.ResetHW					= DEMOD_H14GBB_ResetHW,	
		.SetI2C						= DEMOD_H14GBB_SetI2C,	
		.SetInclk 					= DEMOD_H14GBB_SetInclk,	
		.AdcInit					= NULL,


		.Get_Id						= DEMOD_H14GBB_Get_Id,
		.RepeaterEnable 	  		= DEMOD_H14GBB_RepeaterEnable,
		.SoftwareReset		 		= DEMOD_H14GBB_SoftwareReset,
		.Serial_Control 	 		= DEMOD_H14GBB_Serial_Control,
		.Power_Save 	  			= DEMOD_H14GBB_Power_Save,
		.StdOperModeContrl		 	= DEMOD_H14GBB_StdOperModeContrl,
		.NeverlockScan				= DEMOD_H14GBB_NeverlockScan,
		.TPOutCLKEnable 	  		= DEMOD_H14GBB_TPOutEnable,
		.Set_IF_Frq 	  			= DEMOD_H14GBB_Set_IF_Frq,
		.Set_AGCPolarity	   		= DEMOD_H14GBB_Set_AGCPolarity,
		.Set_SpectrumCtrl			= DEMOD_H14GBB_Set_SpectrumCtrl,
		.Get_SpectrumStatus 	  	= DEMOD_H14GBB_Get_SpectrumStatus,
		.Get_IFAGC		 			= DEMOD_H14GBB_Get_IFAGC,
		.Get_OperMode				= DEMOD_H14GBB_Get_OperMode,
		.Get_NeverLockStatus	   	= DEMOD_H14GBB_Get_NeverLockStatus,
		.Get_CarrierFreqOffset		= DEMOD_H14GBB_Get_CarrierFreqOffset,
		.Get_TPIFStatus 	  		= DEMOD_H14GBB_Get_TPIFStatus,
		.Get_VABER		 			= DEMOD_H14GBB_Get_VABER,
		.Get_Packet_Error			= DEMOD_H14GBB_Get_Packet_Error,
		.Get_BandWidthMode			= DEMOD_H14GBB_Get_BandWidthMode,
		.Get_QAMMode				= DEMOD_H14GBB_Get_QAMMode,
		.Get_MseInfo 				= DEMOD_H14GBB_Get_MseInfo,
		.Get_Lock					= DEMOD_H14GBB_Get_Lock,
		.Get_SysLockTime			= DEMOD_H14GBB_VSB_Get_SysLockTime,
		.Get_FecLockTime			= DEMOD_H14GBB_VSB_Get_FecLockTime,
/***************************************************************************************
* DVBT/ DVBC common  function
****************************************************************************************/


/***************************************************************************************
* DVBC function
****************************************************************************************/

		.DVBC_AutoSymbolRateDet 	  		= DEMOD_H14GBB_DVBC_AutoSymbolRateDet,
		.DVBC_IsSymbolRateAuto		 		= DEMOD_H14GBB_DVBC_IsSymbolRateAuto,
		.DVBC_IsQammodeAutoDet		 		= DEMOD_H14GBB_DVBC_IsQammodeAutoDet,
		.DVBC_Set_DefaultRegisterValue		= DEMOD_H14GBB_DVBC_Set_DefaultRegisterValue,
		.DVBC_Set_Config_auto				= DEMOD_H14GBB_DVBC_Set_Config_auto,
		.DVBC_Set_QamMode 					= DEMOD_H14GBB_DVBC_Set_QamMode,
		.DVBC_Set_NeverLockWaitTime 	  	= DEMOD_H14GBB_DVBC_Set_NeverLockWaitTime,
		.DVBC_Get_SymbolRateDetect		 	= DEMOD_H14GBB_DVBC_Get_SymbolRateDetect,
		.DVBC_Get_SymbolRateStatus		 	= DEMOD_H14GBB_DVBC_Get_SymbolRateStatus,
		.DVBC_Get_QamModeDetectStatus		= DEMOD_H14GBB_DVBC_Get_QamModeDetectStatus,
		.DVBC_Get_DvbInfo					= DEMOD_H14GBB_DVBC_Get_DvbInfo,

		.DVBC_Obtaining_Signal_Lock			= DEMOD_H14GBB_DVBC_Obtaining_Signal_Lock,
		.DVBC_Monitoring_Signal_Lock 		= DEMOD_H14GBB_DVBC_Monitoring_Signal_Lock,

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

		.DVBT_Set_DefaultRegisterValue		= DEMOD_H14GBB_DVBT_Set_DefaultRegisterValue,
		.DVBT_Set_Config_auto				= DEMOD_H14GBB_DVBT_Set_Config_auto,
		.DVBT_Set_PartialConfig 	  		= DEMOD_H14GBB_DVBT_Set_PartialConfig,
		.DVBT_Set_FullConfig	   			= DEMOD_H14GBB_DVBT_Set_FullConfig,
		.DVBT_Set_NeverLockWaitTime 	  	= DEMOD_H14GBB_DVBT_Set_NeverLockWaitTime,
		.DVBT_Get_DelaySpreadStatus 	  	= DEMOD_H14GBB_DVBT_Get_DelaySpreadStatus,
		.DVBT_Get_Hierach_HPSel 	  		= DEMOD_H14GBB_DVBT_Get_Hierach_HPSel,
		.DVBT_Get_FFTMode					= DEMOD_H14GBB_DVBT_Get_FFTMode,
		.DVBT_Get_GuradIntervalMode 	  	= DEMOD_H14GBB_DVBT_Get_GuradIntervalMode,
		.DVBT_Get_HierachyMode				= DEMOD_H14GBB_DVBT_Get_HierachyMode,
		.DVBT_Get_LpCoderRate				= DEMOD_H14GBB_DVBT_Get_LpCoderRate,
		.DVBT_Get_HpCoderRate				= DEMOD_H14GBB_DVBT_Get_HpCoderRate,
		.DVBT_Get_CellId	   				= DEMOD_H14GBB_DVBT_Get_CellId,
		.DVBT_Get_TpsInfo					= DEMOD_H14GBB_DVBT_Get_TpsInfo,
		.DVBT_Get_TotalInfo 	  			= DEMOD_H14GBB_DVBT_Get_TotalInfo,
		.DVBT_Get_IFO_LOCK		 			= DEMOD_H14GBB_DVBT_Get_IFO_LOCK,
		.DVBT_Get_CochanDetIndicator	   	= DEMOD_H14GBB_DVBT_Get_CochanDetIndicator,
		.DVBT_EqualizereReset				= DEMOD_H14GBB_DVBT_EqualizereReset,

		.DVBT_Obtaining_Signal_Lock			= DEMOD_H14GBB_DVBT_Obtaining_Signal_Lock,
		.DVBT_Monitoring_Signal_Lock 		= DEMOD_H14GBB_DVBT_Monitoring_Signal_Lock,


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

		.VSB_Set_DefaultRegisterValue		= DEMOD_H14GBB_VSB_Set_DefaultRegisterValue,
		.VSB_Set_NeverLockWaitTime			= DEMOD_H14GBB_VSB_Set_NeverLockWaitTime,
		.VSB_CochannelExist 	  			= DEMOD_H14GBB_VSB_CochannelExist,
		.VSB_PreMonitor 	  				= DEMOD_H14GBB_VSB_PreMonitor,
		.VSB_Monitor	   					= DEMOD_H14GBB_VSB_Monitor,
		.VSB_Get_MSEdynStatus				= DEMOD_H14GBB_VSB_Get_MSEdynStatus,
		.VSB_Get_TotalInfo		 			= DEMOD_H14GBB_VSB_Get_TotalInfo,

		.VSB_Obtaining_Signal_Lock			= DEMOD_H14GBB_VSB_Obtaining_Signal_Lock,
		.VSB_Monitoring_Signal_Lock 		= DEMOD_H14GBB_VSB_Monitoring_Signal_Lock,


/***************************************************************************************
* QAM function
****************************************************************************************/

		.QAM_SoftwareResetFEC				= DEMOD_H14GBB_QAM_SoftwareResetFEC,
		.QAM_Monitor	   					= DEMOD_H14GBB_QAM_Monitor,
		.QAM_ModeAutoDetection		 		= DEMOD_H14GBB_QAM_ModeAutoDetection,
		.QAM_64Mode 	  					= DEMOD_H14GBB_QAM_64Mode,
		.QAM_256Mode	   					= DEMOD_H14GBB_QAM_256Mode,
		.QAM_Set_DefaultRegisterValue		= DEMOD_H14GBB_QAM_Set_DefaultRegisterValue,
		.QAM_Set_NeverLockWaitTime			= DEMOD_H14GBB_QAM_Set_NeverLockWaitTime,

		.QAM_Get_TotalInfo		 			= DEMOD_H14GBB_QAM_Get_TotalInfo,

		.QAM_Obtaining_Signal_Lock			= DEMOD_H14GBB_QAM_Obtaining_Signal_Lock,
		.QAM_Monitoring_Signal_Lock 		= DEMOD_H14GBB_QAM_Monitoring_Signal_Lock,
		.QAM_EQ_Signal_Detector   			= DEMOD_H14GBB_QAM_EQ_Signal_Detector,



/***************************************************************************************
* ISDBT function
****************************************************************************************/

		.ISDBT_Set_DefaultRegisterValue 	= DEMOD_H14GBB_ISDBT_Set_DefaultRegisterValue,
		.ISDBT_Set_Config_auto		 		= DEMOD_H14GBB_ISDBT_Set_Config_auto,
		.ISDBT_Set_PartialConfig	   		= DEMOD_H14GBB_ISDBT_Set_PartialConfig,
		.ISDBT_Set_FullConfig				= DEMOD_H14GBB_ISDBT_Set_FullConfig,
		.ISDBT_Set_NeverLockWaitTime	   	= DEMOD_H14GBB_ISDBT_Set_NeverLockWaitTime,
		.ISDBT_Get_DelaySpreadStatus	   	= DEMOD_H14GBB_ISDBT_Get_DelaySpreadStatus,
		.ISDBT_Get_FFTMode		 			= DEMOD_H14GBB_ISDBT_Get_FFTMode,
		.ISDBT_Get_GuradIntervalMode	   	= DEMOD_H14GBB_ISDBT_Get_GuradIntervalMode,
		.ISDBT_Get_TMCCInfo 	  			= DEMOD_H14GBB_ISDBT_Get_TMCCInfo,
		.ISDBT_Get_TotalInfo	   			= DEMOD_H14GBB_ISDBT_Get_TotalInfo,
		.ISDBT_Get_IFO_LOCK 	  			= DEMOD_H14GBB_ISDBT_Get_IFO_LOCK,
		.ISDBT_Get_CochanDetIndicator		= DEMOD_H14GBB_ISDBT_Get_CochanDetIndicator,
		.ISDBT_EqualizereReset		 		= DEMOD_H14GBB_ISDBT_EqualizereReset,

		.ISDBT_Obtaining_Signal_Lock		= DEMOD_H14GBB_ISDBT_Obtaining_Signal_Lock,
		.ISDBT_Monitoring_Signal_Lock 		= DEMOD_H14GBB_ISDBT_Monitoring_Signal_Lock,


/***************************************************************************************
* ABB  function
****************************************************************************************/

		.ADEMOD_Demod_Open				 	= NULL,
		.ADEMOD_Init						= NULL,
		.ADEMOD_Set_IF_Frq					= NULL,
		.ADEMOD_SoftwareReset				= NULL,
		.ADEMOD_Set_AftRange 				= NULL,

		.ADEMOD_ResetHW						= NULL,	
		.ADEMOD_SetI2C						= NULL,	
		.ADEMOD_Set_AbbMode 				= NULL,	
		.ADEMOD_Set_WorkAround	 			= NULL,
		.ADEMOD_Set_CvbsRateConversion		= NULL,
		.ADEMOD_Set_SifCtrl 				= NULL,
		
		.ADEMOD_Obtaining_Signal_Lock		= NULL,
		.ADEMOD_Monitoring_Signal_Lock		= NULL,
			
		.ADEMOD_Dbg_Get_RegDump 				= NULL,
		.ADEMOD_Dbg_Set_RegValue				= NULL,
		.ADEMOD_Dbg_Get_RegValue				= NULL,



};

const static	LX_DEMOD_CFG_T 		g_demod_cfg_h14gbb =
{

};

/*========================================================================================
    Implementation Group
========================================================================================*/

/** get L9 specific configuration
 *
 *  @return LX_DEMOD_CFG_T
 */
const LX_DEMOD_CFG_T*   DEMOD_H14GBB_GetCfg(void)
{
    return &g_demod_cfg_h14gbb;
}

void	DEMOD_H14GBB_InitHAL( DEMOD_HAL_T*	hal )
{
	memcpy( hal, &g_demod_hal_h14gbb, sizeof(DEMOD_HAL_T));
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


