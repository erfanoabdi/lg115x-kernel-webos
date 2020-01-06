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
 * main driver implementation for HDMI device.
 * HDMI device will teach you how to make device driver with new platform.
 *
 * author     sunghyun.myoung (sh.myoung@lge.com)
 * version    1.0
 * date       2010.02.19
 * note       Additional information.
 *
 * @addtogroup lg115x_hdmi
 * @{
 */

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>	/**< printk() */
#include <linux/slab.h> 	/**< kmalloc() */
#include <linux/fs.h> 		/**< everything\ldots{} */
#include <linux/types.h> 	/**< size_t */
#include <linux/fcntl.h>	/**< O_ACCMODE */
#include <asm/uaccess.h>
#include <linux/ioport.h>	/**< For request_region, check_region etc */
#include <asm/io.h>			/**< For ioremap_nocache */
#include <linux/workqueue.h>		/**< For working queue */
#include <linux/interrupt.h>
#include <linux/semaphore.h>


#include "os_util.h"

#include "hdmi_drv.h"
#include "hdmi_kapi.h"
#include "hdmi_module.h"
#include "hdmi_ver_def.h"

#ifdef USE_HDMI_KDRV_FOR_H13
#include "../../chip/h13/hdmi/hdmi_hw_h13.h"
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
#include "../../chip/h14/hdmi/hdmi_hw_h14.h"
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
#include "../../chip/m14/hdmi/hdmi_hw_m14b0.h"
#include "../../chip/m14/hdmi/hdmi_hw_m14a0.h"
#endif
/******************************************************************************
 *				DEFINES
 *****************************************************************************/

/**
 *	Global variables of the driver
 */
extern OS_SEM_T	g_HDMI_Sema;
/******************************************************************************
 *				DATA STRUCTURES
 *****************************************************************************/
/**
 *	Structure that declares the usual file
 *	access functions
 */


/******************************************************************************
 *				Local function
 *****************************************************************************/
/**
 *	Structure that declares the usual file
 *	access functions
 */

/**
* HMDI Module exit
*
* @parm void
* @return int
*/
int HDMI_exit(void)
{
	int ret = 0;

	// power down

	HDMI_DisableInterrupt();
	
	return ret;
}

/**
* HDMI Module initialize
*
* @parm void
* @return int
*/
int HDMI_Initialize(LX_HDMI_INIT_T *param)
{
	int ret = RET_OK;

	//HDMI_ERROR("HDMI initializ = %d \n", param->bHdmiSW);
	//if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	//LX_COMP_CHIP_REV(lx_chip_rev(), LX_CHIP_REV(H13,A0))

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		HDMI_H13_HWInitial();
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		HDMI_H14_HWInitial();
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			HDMI_M14B0_HWInitial();
		else
			HDMI_M14A0_HWInitial();
	} else
#endif
		ret = RET_ERROR;	// Unkown chip revision
	
	return ret;
}

int HDMI_SetPort(UINT32 *port)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
//		ret = HDMI_H13_SetPort(port);
	 } else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
//		ret = HDMI_H14_SetPort(port);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_SetPort(port);
		else
			ret = HDMI_M14A0_SetPort(port);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision


	return ret;
}


int HDMI_GetMode(LX_HDMI_MODE_T *mode)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetMode(mode);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetMode(mode);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetMode(mode);
		else
			ret = HDMI_M14A0_GetMode(mode);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}
int HDMI_GetAspectRatio(LX_HDMI_ASPECTRATIO_T *ratio)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetAspectRatio(ratio);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetAspectRatio(ratio);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetAspectRatio(ratio);
		else
			ret = HDMI_M14A0_GetAspectRatio(ratio);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}
int HDMI_GetColorDomain(LX_HDMI_COLOR_DOMAIN_T *color)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetColorDomain(color);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetColorDomain(color);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetColorDomain(color);
		else
			ret = HDMI_M14A0_GetColorDomain(color);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;

}
int HDMI_GetTimingInfo(LX_HDMI_TIMING_INFO_T *info)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetTimingInfo(info);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetTimingInfo(info);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetTimingInfo(info);
		else
			ret = HDMI_M14A0_GetTimingInfo(info);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetAviPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetAviPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetAviPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetAviPacket(packet);
		else
			ret = HDMI_M14A0_GetAviPacket(packet);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetVsiPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{	
		ret = HDMI_H13_GetVsiPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetVsiPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetVsiPacket(packet);
		else
			ret = HDMI_M14A0_GetVsiPacket(packet);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetSpdPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetSpdPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetSpdPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetSpdPacket(packet);
		else
			ret = HDMI_M14A0_GetSpdPacket(packet);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetInfoPacket(LX_HDMI_INFO_PACKET_T *packet)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetInfoPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{ 
		ret = HDMI_H14_GetInfoPacket(packet);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetInfoPacket(packet);
		else
			ret = HDMI_M14A0_GetInfoPacket(packet);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetStatus(LX_HDMI_STATUS_T *status)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{	
		ret = HDMI_H13_GetStatus(status);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetStatus(status);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetStatus(status);
		else
			ret = HDMI_M14A0_GetStatus(status);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}
int HDMI_SetHPD(LX_HDMI_HPD_T *hpd)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_SetHPD(hpd);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_SetHPD(hpd);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_SetHPD(hpd);
		else
			ret = HDMI_M14A0_SetHPD(hpd);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_PowerConsumption(UINT32 power)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{	
		ret = HDMI_H13_SetPowerControl(power);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_SetPowerControl(power);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_SetPowerControl(power);
		else
			ret = HDMI_M14A0_SetPowerControl(power);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

//audio related
int HDMI_GetAudioInfo(LX_HDMI_AUDIO_INFO_T *pAudioInfo)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetAudioInfo(pAudioInfo);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetAudioInfo(pAudioInfo);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetAudioInfo(pAudioInfo);
		else
			ret = HDMI_M14A0_GetAudioInfo(pAudioInfo);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetAudioCopyInfo(LX_HDMI_AUDIO_COPY_T *pCopyInfo)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetAudioCopyInfo(pCopyInfo);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetAudioCopyInfo(pCopyInfo);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetAudioCopyInfo(pCopyInfo);
		else
			ret = HDMI_M14A0_GetAudioCopyInfo(pCopyInfo);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_SetArc(LX_HDMI_ARC_CTRL_T *pArcCtrl)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_SetArc(pArcCtrl);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_SetArc(pArcCtrl);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_SetArc(pArcCtrl);
		else
			ret = HDMI_M14A0_SetArc(pArcCtrl);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_SetMute(LX_HDMI_MUTE_CTRL_T *pMuteCtrl)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_SetMute(pMuteCtrl);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_SetMute(pMuteCtrl);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_SetMute(pMuteCtrl);
		else
			ret = HDMI_M14A0_SetMute(pMuteCtrl);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_GetAudioDebugInfo(LX_HDMI_DEBUG_AUDIO_INFO_T *pAudioDebugInfo)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_GetAudioDebugInfo(pAudioDebugInfo);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_GetAudioDebugInfo(pAudioDebugInfo);
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetAudioDebugInfo(pAudioDebugInfo);
		else
			ret = HDMI_M14A0_GetAudioDebugInfo(pAudioDebugInfo);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}


int HDMI_DisableInterrupt(void)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
	{
		ret = HDMI_H13_DisableInterrupt();
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
	{
		ret = HDMI_H14_DisableInterrupt();
	} else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_DisableInterrupt();
		else
			ret = HDMI_M14A0_DisableInterrupt();
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}


int HDMI_GetRegister(UINT32 addr , UINT32 *value)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
		ret = HDMI_H13_GetRegister(addr , value);
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
		ret = HDMI_H14_GetRegister(addr , value);
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_GetRegister(addr , value);
		else
			ret = HDMI_M14A0_GetRegister(addr , value);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_SetRegister(UINT32 addr , UINT32 value)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
		ret = HDMI_H13_SetRegister(addr, value);
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
		ret = HDMI_H14_SetRegister(addr, value);
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_SetRegister(addr, value);
		else
			ret = HDMI_M14A0_SetRegister(addr, value);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Read_EDID_Data(int port_num , UINT8 *pedid_data)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Read_EDID_Data(port_num , pedid_data);
		else
			ret = HDMI_M14A0_Read_EDID_Data(port_num , pedid_data);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Write_EDID_Data(int port_num , UINT8 *pedid_data)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Write_EDID_Data(port_num , pedid_data);
		else
			ret = HDMI_M14A0_Write_EDID_Data(port_num , pedid_data);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Get_Phy_Status(LX_HDMI_PHY_INFORM_T *sp_hdmi_phy_status)
{
	int ret = RET_OK;

	if(sp_hdmi_phy_status == NULL)	
		return RET_ERROR;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Get_Phy_Status(sp_hdmi_phy_status);
		else
			ret = HDMI_M14A0_Get_Phy_Status(sp_hdmi_phy_status);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

/*
int HDMI_Get_Phy_Status(LX_HDMI_PHY_INFORM_T *sp_hdmi_phy_status)
{
	int ret = RET_OK;

	if(sp_hdmi_phy_status == NULL)	
		return RET_ERROR;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		ret = HDMI_M14A0_Get_Phy_Status(sp_hdmi_phy_status);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}
*/

#ifdef	KDRV_CONFIG_PM
int HDMI_RunSuspend(void)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
		ret = HDMI_H13_RunSuspend( );
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
		ret = HDMI_H14_RunSuspend( );
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_RunSuspend( );
		else
			ret = HDMI_M14A0_RunSuspend( );
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}
int HDMI_RunResume(void)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
		ret = HDMI_H13_RunResume( );
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
		ret = HDMI_H14_RunResume( );
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_RunResume( );
		else
			ret = HDMI_M14A0_RunResume( );
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}
#endif

int HDMI_Read_HDCP_Key(UINT8 *phdcp_key)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Read_HDCP_Key(phdcp_key);
		else
			ret = HDMI_M14A0_Read_HDCP_Key(phdcp_key);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Write_HDCP_Key(UINT8 *phdcp_key)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Write_HDCP_Key(phdcp_key);
		else
			ret = HDMI_M14A0_Write_HDCP_Key(phdcp_key);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Thread_Control(int sleep_enable)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_H13
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H13))
		ret = HDMI_H13_Thread_Control(sleep_enable);
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_H14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_H14))
		ret = HDMI_H14_Thread_Control(sleep_enable);
	else
#endif

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Thread_Control(sleep_enable);
		else
			ret = HDMI_M14A0_Thread_Control(sleep_enable);
	}
	else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}


int HDMI_Get_Aksv_Data(int port_num, UINT8 *pAksv_Data)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Get_Aksv_Data(port_num, pAksv_Data);
		else
			ret = HDMI_M14A0_Get_Aksv_Data(port_num, pAksv_Data);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_MHL_Send_RCP(UINT8 rcp_data)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_MHL_Send_RCP(rcp_data);
			else
			ret = HDMI_M14A0_MHL_Send_RCP(rcp_data);
	}
	else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_MHL_Send_WriteBurst(LX_HDMI_MHL_WRITEBURST_DATA_T *spWriteburst_data)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_MHL_Send_WriteBurst(spWriteburst_data);
			else
			ret = HDMI_M14A0_MHL_Send_WriteBurst(spWriteburst_data);
	}
	else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_MHL_Read_WriteBurst(LX_HDMI_MHL_WRITEBURST_DATA_T *spWriteburst_data)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_MHL_Read_WriteBurst(spWriteburst_data);
		else
			ret = HDMI_M14A0_MHL_Read_WriteBurst(spWriteburst_data);
	}
	else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Module_Call_Type(LX_HDMI_CALL_TYPE_T	hdmi_call_type)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Module_Call_Type(hdmi_call_type);
		else
			ret = HDMI_M14A0_Module_Call_Type(hdmi_call_type);
	}
	else
#endif
		ret = RET_OK;		// Unkown chip revision

	return ret;
}

int HDMI_Reset_Internal_Edid(int port_num, int edid_resetn)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Reset_Internal_Edid(port_num, edid_resetn);
		else
			ret = HDMI_M14A0_Reset_Internal_Edid(port_num, edid_resetn);
	}
	else
#endif
		ret = RET_OK;		// Unkown chip revision

	return ret;
}

int HDMI_Enable_External_DDC_Access(int port_num, int enable)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Enable_External_DDC_Access(port_num, enable);
	}
	else
#endif
		ret = RET_OK;		// Unkown chip revision

	return ret;
}

int HDMI_MHL_Receive_RCP(LX_HDMI_RCP_RECEIVE_T *sp_MHL_RCP_rcv_msg)
{
	int ret = RET_OK;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_MHL_Receive_RCP(sp_MHL_RCP_rcv_msg);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

int HDMI_Get_Driver_Status(LX_HDMI_DRIVER_STATUS_T *sp_hdmi_driver_status)
{
	int ret = RET_OK;

	if(sp_hdmi_driver_status == NULL)	
		return RET_ERROR;

#ifdef USE_HDMI_KDRV_FOR_M14
	if(!LX_COMP_CHIP(lx_chip_rev(), LX_CHIP_M14))
	{
		if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
			ret = HDMI_M14B0_Get_Driver_Status(sp_hdmi_driver_status);
		else
			ret = HDMI_M14A0_Get_Driver_Status(sp_hdmi_driver_status);
	} else
#endif
		ret = RET_ERROR;		// Unkown chip revision

	return ret;
}

