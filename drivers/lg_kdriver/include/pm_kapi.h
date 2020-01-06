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
/** @file pm_kapi.h
 *
 *  application interface header for pm device
 *
 *  @author		ingyu.yang (ingyu.yang@lge.com)
 *  @version		1.0
 *  @date		2009.01.06
 *
 *  @addtogroup lg1150_pm
 *	@{
 */

#ifndef	_PM_KAPI_H_
#define	_PM_KAPI_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#include "base_types.h"

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define PM_IOC_MAGIC               'g'
/**
@name PM IOCTL List
ioctl list for pm device.

@{
@def PM_IOW_SET_PIN_MUX
Set pin to pm.
When pm device receives above IOCTL with pin index, then set the pin to pm.
In this IOCTL, just use the u8Pinnumber parameter in Pm_Pinset_Param_t structure.

@def PM_IOW_SET_MODE
Set pin to input or output.
When pm device receives above IOCTL with Pm_Pinset_Param_t, then set the pin to input or output.
If u8flag is "1" then pin is set to the output port.

@def PM_IORW_GET_VALUE
Get information from external chip.
When pm device receives above IOCTL with Pm_Pinset_Param_t, then get the information from external chip.
In this IOCTL, just use the u8Pinnumber parameter in Pm_Pinset_Param_t structure.

@def PM_IOW_SET_VALUE
Set pin to pull high or low.
When pm device receives above IOCTL with Pm_Pinset_Param_t, then set the pin to pull high or low.
If u8flag is "1" then pin is set to the pull high.
*/

/*----------------------------------------------------------------------------------------
	IO comand naming rule  : MODULE_IO[R][W]_COMMAND
----------------------------------------------------------------------------------------*/
#define PM_IOW_COMMAND_SET		_IOW(PM_IOC_MAGIC,	0, LX_PM_PARAM_T)
#define PM_IORW_COMMAND_GET		_IOW(PM_IOC_MAGIC,	1, LX_PM_PARAM_T)

#define PM_IOW_EX_COMMAND_SET		_IOW(PM_IOC_MAGIC,	10, LX_PM_PARAM_T)
#define PM_IORW_EX_COMMAND_GET	_IOW(PM_IOC_MAGIC,	11, LX_PM_PARAM_T)


#define PM_IOC_MAXNR               20
#define PM_NUM_IN_ARRAY			8


#define PM_ARRAY_NUM 	6

#define PM_IRQ_NUM_NR 6
#define PM_PIN_MAX 150
/** @} */


#define PMINFO_HEADER						0x504d4657
#define PMINFO_VERSION_1					1

#define PMINFO_SIZE							0x4000
#define PMINFO_OFFSET						(0x40000 - PMINFO_SIZE)

#define PMS_OFF								0
#define PMS_ON								1

#define	GOV_MODE_NORMAL						0x0
#define	GOV_MODE_JUSTDEMAND					0x1

#define	GOV_HOTPLUG_ENABLE					0x0
#define	GOV_HOTPLUG_DISABLE_N_CPU1_OFF		0x1
#define	GOV_HOTPLUG_DISABLE_N_CPU1_ON		0x2

#define	TEST_FULL							0x0
#define	TEST_DVFS							0x1
#define	TEST_HOTPLUG						0x2
#define	TEST_FIXED							0x3

#define	PM_THERMAL_LOG_OFF			0x0
#define	PM_THERMAL_LOG_ON			0x1
#define	IPC_PACKET_BASE (0xF7083E00)  /* Limit ~0x2000_3FFF 256 bytes CA15:0xF7083E00~ */
#define	IPC_PACKET_SIZE (4*8)       /* 4Byte unit */
#define 	IPC_STATUS_BASE         ((IPC_PACKET_BASE) + (IPC_PACKET_SIZE)*(7) )    /* 0x20003EE0 */
#define	IPC_TS_BASE            ((IPC_STATUS_BASE) + (0x1C))    /* offset: 0x1c */
#define	IPC_TS_VAL(NUM)        (*( int *) (IPC_TS_BASE + 4*NUM)) /* NUM: 0~3  range : -40 ~ +125 Celcius */
#define	IPC_TS_ADDR(NUM)        ( (IPC_TS_BASE) + 4*(NUM)) /* NUM: 0~3  range : -40 ~ +125 Celcius */
#define	TS_CELCIUS_MAX		(125)
#define	TS_CELCIUS_MIN		(-40)
#define	TS_NUM	3
#define	TS_HISTORY_NUM		10	/* should be even number */

#define	STATUS_TEMPERATURE_STABLE			0
#define	STATUS_TEMPERATURE_INCREASING		1
#define	STATUS_TEMPERATURE_DECREASING		2
#define	STATUS_TEMPERATURE_LOW_WARNING		3
#define	STATUS_TEMPERATURE_HIGH_WARNING		4
#define	STATUS_TEMPERATURE_LOW_CRITICAL		5
#define	STATUS_TEMPERATURE_HIGH_CRITICAL		6

#define	TEMPERATURE_LOW_WARNING		(0)
#define	TEMPERATURE_HIGH_WARNING		(100)
#define	TEMPERATURE_LOW_CRITICAL		(-10)
#define	TEMPERATURE_HIGH_CRITICAL		(120)




/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/
typedef enum
{
	LX_PM_COMMAND_PIN_MUX = 0,
	LX_PM_COMMAND_MODE,
	LX_PM_COMMAND_VALUE,
	LX_PM_COMMAND_INTR_VALUE,
	LX_PM_COMMAND_ISR,

} LX_PM_COMMAND_T;


typedef enum
{
	LX_PM_MODE_INPUT				= 0,
	LX_PM_MODE_OUTPUT				= 1,
	LX_PM_MODE_OUTPUT_OPENDRAIN	= 2,

} LX_PM_MODE_T;

typedef enum
{
	LX_PM_MODE_INTR_DISABLE				= 0,
	LX_PM_MODE_INTR_ENABLE				= 1,


} LX_PM_INTR_T;

typedef enum
{
	LX_PM_VALUE_LOW				= 0,
	LX_PM_VALUE_HIGH				= 1,
	LX_PM_VALUE_INVALID			= 0xFF,
} LX_PM_VALUE_T;

typedef struct PM_INTR_CALLBACK{
	void (*pfnPM_CB)	(UINT32 value);		///< The audio event message callback.
} PM_INTR_CALLBACK_T;

typedef struct
{
	UINT32	command;
    UINT32	pin_number;
    UINT32	data;
    UINT32	pin_value;
    UINT32	intr_enable;
	PM_INTR_CALLBACK_T	cb;
} LX_PM_PARAM_T;


typedef struct
{
	unsigned int	header;
	unsigned int	ver;
	unsigned int	g_enable;
	unsigned int	g_mode;
	unsigned int 	g_start_timer;
	unsigned int	g_sampling_rate;
	unsigned int	g_up_threshold;
	unsigned int	g_down_threshold;
	unsigned int	g_micro_up_threshold;
	unsigned int	g_micro_down_threshold;
	unsigned int	g_hotplug_state;
	unsigned int	g_thermal_enable;
	unsigned int	g_thermal_mode;
	unsigned int	g_thermal_sampling_rate;
	unsigned int	g_thermal_threshold;
	unsigned int	g_down_delay;
	unsigned int	g_reserved[16];
	unsigned int	t_enable;
	unsigned int	t_mode;
	unsigned int	t_start_timer;
	unsigned int	t_loop_rate;
	unsigned int	t_target_freq;
	unsigned int	t_target_vol;
	unsigned int	t_fixed_freq;
	unsigned int	t_fixed_vol;
	unsigned int	t_reserved[16];
	unsigned int	drv_print;
	unsigned int	gov_print;
	unsigned int	crc;

}LX_PM_CONFIG_T;


/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PM_DRV_H_ */

/** @} */
