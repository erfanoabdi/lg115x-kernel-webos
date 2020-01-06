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
#include <linux/rmap.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>

#include <asm/uaccess.h>
#include "proc_util.h"
#include "debug_util.h"

#include "base_types.h"
#include "base_device.h"

#include "os_util.h"

#include "audio_kapi_master.h"
#include "audio_cfg.h"
#include "audio_drv.h"

#ifdef INCLUDE_H14_CHIP_KDRV
#include "../../chip/h13/audio/mixedIP/mixedIP_drv_h13_b0.h"
#endif	//#ifdef INCLUDE_H14_CHIP_KDRV

#ifdef INCLUDE_M14_CHIP_KDRV
#include "../../chip/m14/audio/mixedIP/mixedIP_drv_m14_a0.h"
#include "../../chip/m14/audio/mixedIP/mixedIP_drv_m14_b0.h"
#endif	//#ifdef INCLUDE_M14_CHIP_KDRV

#ifdef INCLUDE_H13_CHIP_KDRV
#include "../../chip/h14/audio/mixedIP/mixedIP_drv_h14_a0.h"
#endif	//#ifdef INCLUDE_H13_CHIP_KDRV

#include "audio_buffer.h"
#include "audio_imc_func.h"
#include "common/adec_common_version.h"


/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/


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
UINT8	g_AdcPortNum = 0xFF;	//Not exist ADC Port Number.

LX_AUD_CLOCK_SRC_T			g_setClockSrc = LX_AUD_CLOCK_UNKNOWN;
LX_AUD_SPDIF_DTO_RATE_T		g_prevDtoDRate[SPDIF_ES_OUT_DTO_CLOCK_TYPE][SPDIF_ES_OUT_FREQ_NUMBER] = { \
										{DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE}, \
										{DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE}};

LX_AUD_SPDIF_DTO_RATE_T		g_prevDtoARate[SPDIF_ES_OUT_DTO_CLOCK_TYPE][SPDIF_ES_OUT_FREQ_NUMBER] = { \
										{DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE}, \
										{DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE, DTO_A_CLOCK_CENTER_TAP_VALUE}};


/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/


/*========================================================================================
	Implementation Group
========================================================================================*/
/**
 *  ANALOG_ReadRegister
*/
int ANALOG_ReadRegister(UINT8 slaveAddr, UINT8 regAddr, UINT8 *value)
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_ANALOG_ReadRegister(slaveAddr, regAddr, value);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_ANALOG_ReadRegister(slaveAddr, regAddr, value);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_ANALOG_ReadRegister(slaveAddr, regAddr, value);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_ANALOG_ReadRegister(slaveAddr, regAddr, value);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 *  ANALOG_WriteRegister
*/
int ANALOG_WriteRegister(UINT8 slaveAddr, UINT8 regAddr, UINT8 value)
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_ANALOG_WriteRegister(slaveAddr, regAddr, value);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_ANALOG_WriteRegister(slaveAddr, regAddr, value);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_ANALOG_WriteRegister(slaveAddr, regAddr, value);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_ANALOG_WriteRegister(slaveAddr, regAddr, value);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Initializes the clock and reset module register value.
 * @see ADEC_Init
*/
int MIXED_IP_AUDIO_InitClockRegisters( void )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_InitClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_InitClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_InitClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_InitClockRegisters();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Resets the clock and reset module register value.
 * @see ADEC_Init
*/
int MIXED_IP_AUDIO_ResetClockRegisters( void )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_ResetClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_ResetClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_ResetClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_ResetClockRegisters();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Resets the AAD clock and CVD PLL clock.
 * @see ADEC_Init
*/
int MIXED_IP_AUDIO_ResetAADClockRegisters( void )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_ResetAADClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_ResetAADClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_ResetAADClockRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_ResetAADClockRegisters();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Initializes the Mixed IP ADC, MIC ADC, DAC and HP module register value.
 * @see ADEC_L9_InitModule
*/
int MIXED_IP_AUDIO_SetClockSource( LX_AUD_SAMPLING_FREQ_T ui32SrcFreq, LX_AUD_CLOCK_SRC_T ui8AdecClockSrc, UINT32 spdifOutput )
{
	int 	ret = RET_OK;

	g_setClockSrc = ui8AdecClockSrc;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_SetClockSource(ui32SrcFreq, ui8AdecClockSrc, spdifOutput);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetClockSource(ui32SrcFreq, ui8AdecClockSrc, spdifOutput);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetClockSource(ui32SrcFreq, ui8AdecClockSrc, spdifOutput);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_SetClockSource(ui32SrcFreq, ui8AdecClockSrc, spdifOutput);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}


/**
 * Sets a PCM Clock frequency and Rate to sync with SPDIF clock for SPDIF output.
 * @see ADEC_L9_InitModule
*/
int MIXED_IP_AUDIO_SetPcmClockRate ( LX_AUD_SAMPLING_FREQ_T ui32SrcFreq, UINT32 ui32PcmClock )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_SetPcmClockRate(ui32SrcFreq, ui32PcmClock);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetPcmClockRate(ui32SrcFreq, ui32PcmClock);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetPcmClockRate(ui32SrcFreq, ui32PcmClock);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_SetPcmClockRate(ui32SrcFreq, ui32PcmClock);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

int MIXED_IP_AUDIO_SetSPDIFSamplingFreq ( LX_AUD_SAMPLING_FREQ_T ui32SamplingFreq, LX_AUD_SPDIF_DTO_RATE_T stSpdifDtoRate, AUD_DTO_T dto )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_SetSPDIFSamplingFreq(ui32SamplingFreq, stSpdifDtoRate, dto);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetSPDIFSamplingFreq(ui32SamplingFreq, stSpdifDtoRate, dto);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetSPDIFSamplingFreq(ui32SamplingFreq, stSpdifDtoRate, dto);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_SetSPDIFSamplingFreq(ui32SamplingFreq, stSpdifDtoRate, dto);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Initializes the Mixed IP ADC, MIC ADC, DAC and HP module register value.
 * @see ADEC_L9_InitModule
*/
int MIXED_IP_AUDIO_InitCodecRegisters ( void )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_InitCodecRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_InitCodecRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_InitCodecRegisters();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_InitCodecRegisters();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Control the Mixed IP AAD, ADC, MIC ADC, DAC and HPDRV module to power on and mute off.
 * @see ADEC_L9_SetSource
*/
int MIXED_IP_AUDIO_PowerControl ( LX_AUD_POWER_BLOCK_T block, LX_AUD_POWER_MODE_T mode )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_PowerControl(block, mode);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_PowerControl(block, mode);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_PowerControl(block, mode);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_PowerControl(block, mode);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Sets the Mixed IP AAD SIF Input Port.
 * @see .
*/
int MIXED_IP_AUDIO_SetAADSIFInputPort(LX_AUD_SIF_INPUT_T sifSource)
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		AUD_KDRV_DEBUG("H14 is only support external SIF Input.\n");
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetAADSIFInputPort(sifSource);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetAADSIFInputPort(sifSource);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		AUD_KDRV_DEBUG("H13 is only support external SIF Input.\n");
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	AUD_KDRV_DEBUG("MIXED_IP_AUDIO_SetAADSIFInputPort(sifSource:%d)\n", sifSource);
	return ret;
}

/**
 * Start the Mixed IP ADC, MIC ADC, DAC and HP module to mute off.
 * @see ADEC_L9_InitModule
*/
int MIXED_IP_AUDIO_StartCodec ( void )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_StartCodec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_StartCodec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_StartCodec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_StartCodec();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Stop the Mixed IP ADC, MIC ADC, DAC and HP module to mute off.(only upper L8 B0 Chip)
 * @see ADEC_L9_InitModule
*/
int MIXED_IP_AUDIO_StopCodec ( void )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_StopCodec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_StopCodec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_StopCodec();
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_StopCodec();
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Sets a ADC port number for ADEC I2S input.
 * @see ADEC_L9_SetSource
*/
int MIXED_IP_AUDIO_SetADCPortNumber ( UINT8 ui8PortNum )
{
	int 	ret = RET_OK;

	//Check previous clock source
	if(g_AdcPortNum == ui8PortNum)
	{
		AUD_KDRV_MIP_DEBUG("MIXED_IP_AUDIO_SetADCPortNumber : same port = %d\n", ui8PortNum);
		return RET_OK;
	}

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_SetADCPortNumber(ui8PortNum);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetADCPortNumber(ui8PortNum);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetADCPortNumber(ui8PortNum);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_SetADCPortNumber(ui8PortNum);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	//Save current adc port number.
	g_AdcPortNum = ui8PortNum;

	AUD_KDRV_DEBUG("SetADCPortNumber : ADC Port = %d\n", g_AdcPortNum);
	return ret;
}

/**
 * Sets the volume of DAC output.
 * @see ADEC_L9_SetDACOutVolume
*/
int MIXED_IP_AUDIO_SetDACOutVolume ( LX_AUD_DAC_VOL_T stDACOutVol )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_SetDACOutVolume(stDACOutVol);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetDACOutVolume(stDACOutVol);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetDACOutVolume(stDACOutVol);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_SetDACOutVolume(stDACOutVol);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

/**
 * Sets the mute of DAC output.
 * @see ADEC_L9_SetOutputPortMute
*/
int MIXED_IP_AUDIO_SetDACOutMute ( LX_AUD_OUTPUT_PORT_MUTE_T stOutPortMute )
{
	int 	ret = RET_OK;

	//Check a chip revision
	if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		#ifdef INCLUDE_H14_CHIP_KDRV
		ret = H14A0_MIXED_IP_ADEC_SetDACOutMute(stOutPortMute);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14B0_MIXED_IP_ADEC_SetDACOutMute(stOutPortMute);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		ret = M14A0_MIXED_IP_ADEC_SetDACOutMute(stOutPortMute);
		#endif
	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		ret = H13B0_MIXED_IP_ADEC_SetDACOutMute(stOutPortMute);
		#endif
	}
	else
	{
		AUD_KDRV_ERROR("<<< %s : LX_CHIP_REV => Unknown(0x%X)\n", __F__, lx_chip_rev());
	}

	return ret;
}

LX_AUD_SPDIF_DTO_RATE_T MIXED_IP_AUDIO_GetDTORate(LX_AUD_CLOCK_SRC_T clockSrc, LX_AUD_SAMPLING_FREQ_T samplingFreq, AUD_DTO_T dto)
{
	UINT32	spdifDTOClockType;
	UINT32	spdifDTOFreq;

	LX_AUD_SPDIF_DTO_RATE_T	spdifDTORate;

	if(samplingFreq == LX_AUD_SAMPLING_FREQ_32_KHZ)
		spdifDTOFreq = SPDIF_ES_OUT_FREQ_32KHZ;
	else if(samplingFreq == LX_AUD_SAMPLING_FREQ_44_1KHZ)
		spdifDTOFreq = SPDIF_ES_OUT_FREQ_44_1KHZ;
	else
		spdifDTOFreq = SPDIF_ES_OUT_FREQ_48KHZ;

	if(clockSrc == LX_AUD_CLOCK_HDMI)
		spdifDTOClockType = SPDIF_ES_OUT_DTO_CLOCK_HDMI;
	else
		spdifDTOClockType = SPDIF_ES_OUT_DTO_CLOCK_NON_HDMI;

	if(dto == AUD_DTO_D)
	{
		spdifDTORate = g_prevDtoDRate[spdifDTOClockType][spdifDTOFreq];
	}
	else
	{
		spdifDTORate = g_prevDtoARate[spdifDTOClockType][spdifDTOFreq];
	}

	return spdifDTORate;
}

void MIXED_IP_AUDIO_SetDTORate(LX_AUD_CLOCK_SRC_T clockSrc, LX_AUD_SAMPLING_FREQ_T samplingFreq, AUD_DTO_T dto, LX_AUD_SPDIF_DTO_RATE_T dtoRate)
{
	UINT32	spdifDTOClockType;
	UINT32	spdifDTOFreq;

	if(samplingFreq == LX_AUD_SAMPLING_FREQ_32_KHZ)
		spdifDTOFreq = SPDIF_ES_OUT_FREQ_32KHZ;
	else if(samplingFreq == LX_AUD_SAMPLING_FREQ_44_1KHZ)
		spdifDTOFreq = SPDIF_ES_OUT_FREQ_44_1KHZ;
	else
		spdifDTOFreq = SPDIF_ES_OUT_FREQ_48KHZ;

	if(clockSrc == LX_AUD_CLOCK_HDMI)
		spdifDTOClockType = SPDIF_ES_OUT_DTO_CLOCK_HDMI;
	else
		spdifDTOClockType = SPDIF_ES_OUT_DTO_CLOCK_NON_HDMI;

	if(dto == AUD_DTO_D)
	{
		g_prevDtoDRate[spdifDTOClockType][spdifDTOFreq] = dtoRate;
	}
	else
	{
		g_prevDtoARate[spdifDTOClockType][spdifDTOFreq] = dtoRate;
	}

	return;
}

/**
 * Sets DTO Interrupt Source.
 * @see
*/
void MIXED_IP_AUDIO_SetDTOIntSrc(LX_AUD_CLOCK_SRC_T clockSrc, LX_AUD_MASTER_SPDIF_OUTPUT_T spdifOutput)
{
	DtoCtrlCmdSetIntNum	setIntSrc;

	if(clockSrc == LX_AUD_CLOCK_HDMI && spdifOutput == LX_AUD_MASTER_SPDIF_ES)
	{
		setIntSrc.RefInt	= AUD_INT_HDMIIN;
		setIntSrc.TarInt	= AUD_INT_SPDIF;
	}
	else
	{
		setIntSrc.RefInt	= AUD_INT_PCM;
		setIntSrc.TarInt	= AUD_INT_SPDIF;
	}

	AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_INTNUM, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetIntNum), &setIntSrc);
	return;
}

/**
 * Set DTO Int Sampling Frequency
 * @see
*/
void MIXED_IP_AUDIO_SetDTOIntSamplingFreq(LX_AUD_CLOCK_SRC_T clockSrc, UINT32 spdifOutput, LX_AUD_SAMPLING_FREQ_T samplingFreq)
{
	DtoCtrlCmdSetIntFs		setDtoIntFs;

	if(clockSrc == LX_AUD_CLOCK_DTV)
	{
		if(spdifOutput == LX_AUD_MASTER_SPDIF_ES)
		{
			setDtoIntFs.RefIntFs	= LX_AUD_SAMPLING_FREQ_48_KHZ;
			setDtoIntFs.TarIntFs	= samplingFreq;
			AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_INTFS, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetIntFs), &setDtoIntFs);
		}
		else
		{
			setDtoIntFs.RefIntFs	= LX_AUD_SAMPLING_FREQ_48_KHZ;
			setDtoIntFs.TarIntFs	= LX_AUD_SAMPLING_FREQ_48_KHZ;
			AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_INTFS, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetIntFs), &setDtoIntFs);
		}
	}
	else if(clockSrc == LX_AUD_CLOCK_HDMI)
	{
		if(spdifOutput == LX_AUD_MASTER_SPDIF_ES)
		{
			setDtoIntFs.RefIntFs	= samplingFreq;
			setDtoIntFs.TarIntFs	= samplingFreq;
			AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_INTFS, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetIntFs), &setDtoIntFs);
		}
		else
		{
			setDtoIntFs.RefIntFs	= LX_AUD_SAMPLING_FREQ_48_KHZ;
			setDtoIntFs.TarIntFs	= LX_AUD_SAMPLING_FREQ_48_KHZ;
			AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_INTFS, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetIntFs), &setDtoIntFs);
		}
	}
	else
	{
		setDtoIntFs.RefIntFs		= LX_AUD_SAMPLING_FREQ_48_KHZ;
		setDtoIntFs.TarIntFs		= LX_AUD_SAMPLING_FREQ_48_KHZ;
		AUDIO_IMC_SendCmdParam(DTOCTRL_CMD_SET_INTFS, ADEC_MODULE_DTO_0, sizeof(DtoCtrlCmdSetIntFs), &setDtoIntFs);
	}

	AUD_KDRV_DEBUG("AUDIO_SetSPDIFSamplingFreq(src:%d, freq:%d)\n", clockSrc, samplingFreq);
	return;
}

/**
 * Start DTO.
 * @see
*/
void MIXED_IP_AUDIO_StartDTO(LX_AUD_CLOCK_SRC_T clockSrc, UINT32 spdifOutput)
{
	// Start/Stop DTO
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_DTO_0);
	AUDIO_IMC_SendCmd(ADEC_CMD_START, ADEC_MODULE_DTO_0);
	AUDIO_IMC_SendCmd(ADEC_CMD_STOP, ADEC_MODULE_DTO_1);

	return;
}



