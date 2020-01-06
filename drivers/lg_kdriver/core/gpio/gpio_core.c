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
 *  main driver implementation for gpio device.
 *
 *  author		ks.hyun (ks.hyun@lge.com)
 *  				jun.kong (jun.kong@lge.com)
 *  version		1.0
 *  date		2012.05.03
 *  note		Additional information.
 *
 *  @addtogroup lg115x_gpio
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
#include <asm/io.h>

#include "os_util.h"
#include "gpio_drv.h"
#include "gpio_reg.h"
#include "sys_regs.h"
#include "sys_io.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define GPIO_LOCK_INIT(dev)		OS_InitMutex(&dev->lock, OS_SEM_ATTR_DEFAULT)
#define GPIO_LOCK(dev)			OS_LockMutex(&dev->lock)
#define GPIO_UNLOCK(dev)		OS_UnlockMutex(&dev->lock)

#define GPIO_EX_LOCK_INIT(dev)	OS_InitMutex(&dev->ex_lock, OS_SEM_ATTR_DEFAULT)
#define GPIO_EX_LOCK(dev)		OS_LockMutex(&dev->ex_lock)
#define GPIO_EX_UNLOCK(dev)		OS_UnlockMutex(&dev->ex_lock)


#if 0
#define GPIO_CORE_DEBUG(format, args...)	GPIO_ERROR(format, ##args)
#else
#define GPIO_CORE_DEBUG(format, args...)	do{}while(0)
#endif

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/
typedef struct
{
	UINT8	direction;
	UINT8	data;
} GPIO_PM_DATA_T;


typedef struct GPIO_DEV
{
	OS_SEM_T	lock;
	UINT32		max_num;
	UINT32 		intr_num[GPIO_IRQ_NUM_NR];

	int 		(*SetPinMux)(UINT32 port, BOOLEAN enable);
	int 		(*GetPinMux)(UINT32 port, BOOLEAN *enable);
	int 		(*SetValue)	(UINT32 port, LX_GPIO_VALUE_T value);
	int 		(*GetValue)	(UINT32 port, LX_GPIO_VALUE_T *value);
	int 		(*SetMode)	(UINT32 port, LX_GPIO_MODE_T mode);
	int 		(*GetMode)	(UINT32 port, LX_GPIO_MODE_T *mode);
	int 		(*SetIntrCB)	(UINT32 port, void (*pfnGPIO_CB)(UINT32 value), UINT32 enable);
	int 		(*GetIntrValue)	(UINT32 *port, LX_GPIO_VALUE_T *value);

	/* To access gpio pins in LG115xAN(ACE) */
	OS_SEM_T	ex_lock;
	UINT32		max_ex_num;
	int 		(*ExSetPinMux)(UINT32 port, BOOLEAN enable);
	int 		(*ExGetPinMux)(UINT32 port, BOOLEAN *enable);
	int 		(*ExSetValue) (UINT32 port, LX_GPIO_VALUE_T value);
	int 		(*ExGetValue) (UINT32 port, LX_GPIO_VALUE_T *value);
	int 		(*ExSetMode)  (UINT32 port, LX_GPIO_MODE_T mode);
	int 		(*ExGetMode)  (UINT32 port, LX_GPIO_MODE_T *mode);

#ifdef KDRV_CONFIG_PM
	int			(*Resume)	(void);
	int			(*Suspend)	(void);
	GPIO_PM_DATA_T*	pmdata;
#endif
} GPIO_DEV_T;


/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

static GPIO_DEV_T *_pGpioDev;
 UINT32*	_gpioBaseAddr;

/*************************************************************************
* Set Pinmux
*************************************************************************/

#ifdef INCLUDE_M14_CHIP_KDRV

static int _GPIO_SetPinMux_M14Ax(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask = 0;

#if 0
#define GPIO_SET_PINMUX_M14A0(_reg, _mask, _en)		\
	do{												\
		UINT32 _value;								\
		CTOP_CTRL_M14A0_RdFL(_reg);					\
		_value = CTOP_CTRL_M14A0_Rd(_reg);			\
		if(_en)	_value |= _mask;					\
		else	_value &= (~_mask);					\
		CTOP_CTRL_M14A0_Wr(_reg, _value);			\
		CTOP_CTRL_M14A0_WrFL(_reg);					\
	} while(0)
#endif

	if(port < 32)
	{
		// Do nothing...
	}
	else 	/* 32 ~ 144 */
	{
		UINT32 offset, value;

		//
		/* CTOP_CTRL_BASE + 0x84 = CTR33(GPIO[39:32] enable) */
		offset = 0x84 + ((port - 32)/8)*4;
		value = CTOP_CTRL_M14_READ(offset);
		mask = 1 << ((port % 8)* 4 + 3);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14_WRITE(offset, value);
	}

	return rc;
}

static int _GPIO_GetPinMux_M14Ax(UINT32 port, BOOLEAN *enable)
{
	if(port < 32)
	{
		*enable = 1;
	}
	else 	/* 32 ~ 144 */
	{
		UINT32 mask;
		UINT32 offset, value;

		/* CTOP_CTRL_BASE + 0x84 = CTR33(GPIO[39:32] enable) */
		offset = 0x84 + ((port - 32)/8)*4;
		value = CTOP_CTRL_M14_READ(offset);
		mask = 1 << ((port % 8)* 4 + 3);
		*enable = (value & mask) ? 1 : 0;
	}

	return 0;
}

static int _GPIO_SetPinMux_M14Bx(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask = 0;
	UINT32 offset, value;

	if(port < 32)
	{
		// Do nothing...
		if( port == 31 )
		{
			mask = 1 << 28; //CPU monitor[15:8] enable for GPIO31  1:gpio31 0:cpu monitor
			value = 		CTOP_CTRL_M14B0_Rd(RIGHT,ctr122);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(RIGHT,ctr122,value);
			return 0;
		}
	}
	else if(port <= 39)	/* 32 ~ 39 */
	{
		// Do nothing...
	}
	else if(port <= 70)	/* 40 ~ 70 */
	{
		//0xC0019020
		//offset = 0x20;

		if( port <= 47)  /* 40 ~ 47 */
		{
			mask = 1 << ((40 - port)+ 12);
		}
		else if( port <= 55)  /* 48 ~ 55 */
		{
			mask = 1 << ((48 - port)+ 20);
		}
		else if( port <= 57)  /* 56 ~ 57 */
		{
			mask = 1 << ((port - 56 )*4 + 3);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr97);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(LEFT,ctr97,value);
			return 0;
		}
		else if( port <= 63)  /* 58 ~ 63 */
		{
			mask = 1 << ((58 - port)+ 26);
		}
		else if( port <= 66)  /* 64 ~ 66 */
		{
			mask = 1 << ((port - 64 )+ 21);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr100);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(LEFT,ctr100,value);
			return 0;
		}
		else if( port <= 70)  /* 67 ~ 70 */
		{
			mask = 1 << ((67 - port)+ 30);
		}
		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr08);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14B0_Wr(TOP,ctr08,value);
	}
	else if(port <= 95)	/* 71 ~ 95 */
	{

		//offset = 0x24;
		if( port <= 71)  /* 71 ~ 71 */
		{
			mask = 1 <<  31;

			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr97);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(LEFT,ctr97,value);
			return 0;
		}
		else if( port <= 76)  /* 72 ~ 76 */
		{
			mask = 1 << (( port - 72)+ 23);
		}
		else if( port <= 79)  /* 77 ~ 79 */
		{
			mask = 1 << ((port - 77 )+ 18);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr100);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(LEFT,ctr100,value);
			return 0;
		}
		else if( port <= 87)  /* 80 ~ 87 */
		{
			mask = 1 << (( port - 80)+ 15);
		}
		else if( port <= 95)  /* 88 ~ 95 */
		{
			mask = 1 << (( port - 88)+ 7);
		}
		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr09);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14B0_Wr(TOP,ctr09,value);
	}
	else if(port <= 119)	/* 96 ~ 119 */
	{

		//offset = 0x28;
		if( port <= 103)  /* 96 ~ 103 */
		{
			mask = 1 << (( port - 96)+ 24);
		}
		else if( port <= 111)  /* 104 ~ 111 */
		{
			mask = 1 << (( port - 104)+ 16);
		}
		else if( port <= 113)  /* 112 ~ 113 */
		{
			mask = 1 << ((port - 112 )*4 + 3);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr98);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(LEFT,ctr98,value);
			return 0;
		}
		else if( port <= 119)  /* 114 ~ 119 */
		{
			mask = 1 << (( port - 114)+ 10);
		}

		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr10);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14B0_Wr(TOP,ctr10,value);
	}
	else if(port <= 135)	/* 120 ~ 135 */
	{

		//offset = 0x2c;
		if( port <= 125)  /* 120 ~ 125 */
		{
			mask = 1 << (( port - 120)+ 16);
		}
		else if( port <= 127)  /* 126 ~ 127 */
		{
			mask = 1 << ((port - 112 )*4 + 27);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr98);
			if(enable)	value |= mask;
			else		value &= (~mask);
			CTOP_CTRL_M14B0_Wr(LEFT,ctr98,value);
			return 0;
		}
		else if( port <= 133)  /* 128 ~ 133 */ // Do nothing...
		{
			return 0;
		}
		else if( port <= 135)  /* 134 ~ 135 */
		{
			mask = 1 << (( port - 134)+ 22);
		}

		//offset = 0x2c;
		mask = 1 << (( port - 128)+ 16);
		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr11);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14B0_Wr(TOP,ctr11,value);
	}
	else 	/* 136 ~ 144 */
	{

	}

	return rc;
}

static int _GPIO_GetPinMux_M14Bx(UINT32 port, BOOLEAN *enable)
{

	UINT32 mask;
	UINT32 value;
	if(port < 32)
	{
		// Do nothing...
	}
	else if(port <= 39)	/* 32 ~ 39 */
	{
		// Do nothing...
	}
	else if(port <= 70)	/* 40 ~ 70 */
	{
		//offset = 0x20;

		if( port <= 47)  /* 40 ~ 47 */
		{
			mask = 1 << ((40 - port)+ 12);
		}
		else if( port <= 55)  /* 48 ~ 55 */
		{
			mask = 1 << ((48 - port)+ 20);
		}
		else if( port <= 57)  /* 56 ~ 57 */
		{
			mask = 1 << ((port - 56 )*4 + 3);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr97);
			*enable = (value & mask) ? 1 : 0;
			return 0;
		}
		else if( port <= 63)  /* 58 ~ 63 */
		{
			mask = 1 << ((58 - port)+ 26);
		}
		else if( port <= 66)  /* 64 ~ 66 */
		{
			mask = 1 << ((port - 64 )+ 21);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr100);
			*enable = (value & mask) ? 1 : 0;
			return 0;
		}
		else if( port <= 70)  /* 67 ~ 70 */
		{
			mask = 1 << ((67 - port)+ 30);
		}
		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr08);
		*enable = (value & mask) ? 1 : 0;

	}
	else if(port <= 95)	/* 64 ~ 95 */
	{

		//offset = 0x24;
		if( port <= 71)  /* 64 ~ 71 */
		{
			mask = 1 <<  31;

			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr97);
			*enable = (value & mask) ? 1 : 0;
			return 0;
		}
		else if( port <= 76)  /* 72 ~ 76 */
		{
			mask = 1 << (( port - 72)+ 23);
		}
		else if( port <= 79)  /* 77 ~ 79 */
		{
			mask = 1 << ((port - 77 )+ 18);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr100);
			*enable = (value & mask) ? 1 : 0;
			return 0;
		}
		else if( port <= 87)  /* 80 ~ 87 */
		{
			mask = 1 << (( port - 80)+ 15);
		}
		else if( port <= 95)  /* 88 ~ 95 */
		{
			mask = 1 << (( port - 88)+ 7);
		}

		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr09);
		*enable = (value & mask) ? 1 : 0;

	}
	else if(port <= 119)	/* 96 ~ 119 */
	{

		//offset = 0x28;
		if( port <= 103)  /* 96 ~ 103 */
		{
			mask = 1 << (( port - 96)+ 24);
		}
		else if( port <= 111)  /* 104 ~ 111 */
		{
			mask = 1 << (( port - 104)+ 16);
		}
		else if( port <= 113)  /* 112 ~ 113 */
		{
			mask = 1 << ((port - 112 )*4 + 3);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr98);
			*enable = (value & mask) ? 1 : 0;
			return 0;
		}
		else if( port <= 119)  /* 114 ~ 119 */
		{
			mask = 1 << (( port - 114)+ 10);
		}
		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr10);
		*enable = (value & mask) ? 1 : 0;
	}
	else if(port <= 135)	/* 120 ~ 135 */
	{
		//offset = 0x2c;

		if( port <= 125)  /* 120 ~ 125 */
		{
			mask = 1 << (( port - 120)+ 16);
		}
		else if( port <= 127)  /* 126 ~ 127 */
		{
			mask = 1 << ((port - 112 )*4 + 27);
			value = 		CTOP_CTRL_M14B0_Rd(LEFT,ctr98);
			*enable = (value & mask) ? 1 : 0;
			return 0;
		}
		else if( port <= 133)  /* 128 ~ 133 */ // Do nothing...
		{
			return 0;
		}
		else if( port <= 135)  /* 134 ~ 135 */
		{
			mask = 1 << (( port - 134)+ 22);
		}

		value = 		CTOP_CTRL_M14B0_Rd(TOP,ctr11);
		*enable = (value & mask) ? 1 : 0;
	}
	else 	/* 135 ~  */
	{

	}
	return 0;
}

#endif //INCLUDE_M14_CHIP_KDRV


#ifdef INCLUDE_H14_CHIP_KDRV
static int _GPIO_SetPinMux_H14Ax(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask = 0;
	UINT32 value;
	UINT32 offset =0;

	if(port < 32)
	{
		// Do nothing...
	}
	else if(port <= 35)	/* 32 ~ 35 */ //Drive strength control //h14bringup
	{

		#if 0
		offset = 0xa0 + ((port - 32)/8)*4;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port % 8)* 4 + 3);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14_WRITE(offset, value);
		#endif
	}
	else if(port <= 63)	/* 36 ~ 63 */
	{

		offset = 0x84;

		if( port <= 39)  /* 36 ~ 39 */
		{
			mask = 1 << ((port - 36)+ 28);
		}
		else if( port <= 47)  /* 40 ~ 47 */
		{
			mask = 1 << ((port - 40)+ 16);
		}
		else if( port <= 55)  /* 48 ~ 55 */
		{
			mask = 1 << ((port - 48)+ 8);
		}
		else if( port <= 63)  /* 56 ~ 63 */
		{
			mask = 1 << ((port - 56)+ 0);
		}

		value = CTOP_CTRL_H14_READ(offset);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_H14_WRITE(offset, value);

	}
	else if(port <= 95)	/* 64 ~ 95 */
	{

		offset = 0x94;

		if( port <= 71)/* 64 ~ 71*/
		{
			mask = 1 << ((port - 64)+ 24);
		}
		else if( port <= 79)  /* 72~ 79*/
		{
			mask = 1 << ((port - 72)+ 16);
		}
		else if( port <= 87)  /* 80 ~ 87 */
		{
			mask = 1 << ((port - 80)+ 8);
		}
		else if( port <= 95)  /* 88 ~ 95 */
		{
			mask = 1 << ((port - 88)+ 0);
		}

		value = CTOP_CTRL_H14_READ(offset);

		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_H14_WRITE(offset, value);
	}
	else if(port <= 103)	/* 96 ~ 103 */
	{
		offset = 0xa4;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port - 96) + 18);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_H14_WRITE(offset, value);
	}

	else if(port <= 135)	/* 104 ~ 135 */
	{
		offset = 0xa8 + 4 *( (port - 104)/8 ) ;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port % 8)*4+ 3);

		// consider ctop pin mux enable bit inversion
		if(( port >= 120 && port <= 125 ) || ( port >= 134 && port <= 135 ))
		{
			if(enable)	value &= (~mask);
			else		value |= mask;
		}
		else
		{
			if(enable)	value |= mask;
			else		value &= (~mask);
		}
		CTOP_CTRL_H14_WRITE(offset, value);
	}

	else if(port <= 139)	/* 136 ~ 139 */
	{

		offset = 0x84;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port - 136) + 24);

		// consider ctop pin mux enable bit inversion
		if(enable)	value &= (~mask);
		else		value |= mask;

		CTOP_CTRL_H14_WRITE(offset, value);
	}

	else
	{
		//port num error
	}

	return rc;
}

static int _GPIO_GetPinMux_H14Ax(UINT32 port, BOOLEAN *enable)
{
	UINT32 mask = 0;
	UINT32 value;
	UINT32 offset =0;

	if(port < 32)
	{
		*enable = 1;
		// Do nothing...
	}
	else if(port <= 35)	/* 32 ~ 35 */ //Drive strength control //jun.kong
	{

		*enable = 0;

		#if 0
		offset = 0xa0 + ((port - 32)/8)*4;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port % 8)* 4 + 3);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_M14_WRITE(offset, value);
		#endif
	}
	else if(port <= 63)	/* 36 ~ 63 */
	{

		offset = 0x84;

		if( port <= 39)  /* 36 ~ 39 */
		{
			mask = 1 << ((port - 36)+ 28);
		}
		else if( port <= 47)  /* 40 ~ 47 */
		{
			mask = 1 << ((port - 40)+ 16);
		}
		else if( port <= 55)  /* 48 ~ 55 */
		{
			mask = 1 << ((port - 48)+ 8);
		}
		else if( port <= 63)  /* 56 ~ 63 */
		{
			mask = 1 << ((port - 56)+ 0);
		}

		value = CTOP_CTRL_H14_READ(offset);
		*enable = (value & mask) ? 1 : 0;

	}
	else if(port <= 95)	/* 64 ~ 95 */
	{

		offset = 0x94;

		if( port <= 71)/* 64 ~ 71*/
		{
			mask = 1 << ((port - 64)+ 24);
		}
		else if( port <= 79)  /* 72~ 79*/
		{
			mask = 1 << ((port - 72)+ 16);
		}
		else if( port <= 87)  /* 80 ~ 87 */
		{
			mask = 1 << ((port - 80)+ 8);
		}
		else if( port <= 95)  /* 88 ~ 95 */
		{
			mask = 1 << ((port - 88)+ 0);
		}

		value = CTOP_CTRL_H14_READ(offset);
		*enable = (value & mask) ? 1 : 0;
	}
	else if(port <= 103)	/* 96 ~ 103 */
	{
		offset = 0xa4;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port - 96) + 18);
		*enable = (value & mask) ? 1 : 0;
	}

	else if(port <= 135)	/* 104 ~ 135 */
	{

		offset = 0xa8 + ((port - 104)/8)*4;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port % 8)*4+ 3);
		*enable = (value & mask) ? 1 : 0;
	}

	else if(port <= 139)	/* 136 ~ 139 */
	{

		offset = 0x84;

		value = CTOP_CTRL_H14_READ(offset);
		mask = 1 << ((port - 136) + 24);
		*enable = (value & mask) ? 1 : 0;

	}
	else
	{
		//port num error
	}
	return 0;
}
#endif //INCLUDE_H14_CHIP_KDRV



#ifdef INCLUDE_H13_CHIP_KDRV
static int _GPIO_SetPinMux_H13Ax(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask = 0;

#if 0
#define GPIO_SET_PINMUX_H13A0(_reg, _mask, _en)		\
	do{												\
		UINT32 _value;								\
		CTOP_CTRL_H13A0_RdFL(_reg);					\
		_value = CTOP_CTRL_H13A0_Rd(_reg);			\
		if(_en)	_value |= _mask;					\
		else	_value &= (~_mask);					\
		CTOP_CTRL_H13A0_Wr(_reg, _value);			\
		CTOP_CTRL_H13A0_WrFL(_reg);					\
	} while(0)
#endif

	if(port < 32)
	{
		// Do nothing...
	}
	else if (port >=32 && port <136) 	/* 32 ~ 135 */
	{
		UINT32 offset, value;

		/* CTOP_CTRL_BASE + 0x84 = CTR33(GPIO[39:32] enable) */
		offset = 0x84 + ((port - 32)/8)*4;
		value = CTOP_CTRL_H13_READ(offset);
		mask = 1 << ((port % 8)* 4 + 3);
		if(enable)	value |= mask;
		else		value &= (~mask);
		CTOP_CTRL_H13_WRITE(offset, value);
	}
	else/* 136 ~ 143 */
	{

	}


	return rc;
}

static int _GPIO_GetPinMux_H13Ax(UINT32 port, BOOLEAN *enable)
{
	if(port < 32)
	{
		*enable = 1;
	}
	else if (port >=32 && port <136) 	/* 32 ~ 135 */
	{
		UINT32 mask;
		UINT32 offset, value;

		/* CTOP_CTRL_BASE + 0x84 = CTR33(GPIO[39:32] enable) */
		offset = 0x84 + ((port - 32)/8)*4;
		value = CTOP_CTRL_H13_READ(offset);
		mask = 1 << ((port % 8)* 4 + 3);
		*enable = (value & mask) ? 1 : 0;
	}
	else/* 136 ~ 144 */
	{

	}

	return 0;
}

#endif //INCLUDE_H13_CHIP_KDRV

#ifdef INCLUDE_L9_CHIP_KDRV
static int _GPIO_SetPinMux_L9Bx(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask = 0;

#define GPIO_SET_PINMUX_L9BX(_reg, _mask, _en)			\
		do{ 											\
			UINT32 _value;								\
			CTOP_CTRL_L9B_RdFL(_reg); 					\
			_value = CTOP_CTRL_L9B_Rd(_reg);			\
			if(_en) _value |= _mask;					\
			else	_value &= (~_mask); 				\
			CTOP_CTRL_L9B_Wr(_reg, _value);				\
			CTOP_CTRL_L9B_WrFL(_reg); 					\
		} while(0)

	if(port < 32)
	{
		// Do nothing...
	}
	else if(port < 40)	/* 32 ~ 39 */
	{
		mask = 1 << ((port % 8) + 24);
		GPIO_SET_PINMUX_L9BX(ctr32_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 48)	/* 40 ~ 47 */
	{
		mask = 1 << ((port % 8)*4+3);
		GPIO_SET_PINMUX_L9BX(ctr53_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 51)	/* 48 ~ 50 */
	{
		mask = 1 << ((port % 8)*4+23);
		GPIO_SET_PINMUX_L9BX(ctr35_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 56)	/* 51 ~ 55 */
	{
		if(port == 51)		mask=1<<0;
		else if(port == 52)	mask=1<<7;
		else if(port == 53)	mask=1<<15;
		else if(port == 54)	mask=1<<23;
		else if(port == 55)	mask=1<<31;

		GPIO_SET_PINMUX_L9BX(ctr34_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 61)	/* 56 ~ 60 */
	{
		if(port == 56)		mask=1<<15;
		else if(port == 57) mask=1<<19;
		else if(port == 58) mask=1<<23;
		else if(port == 59) mask=1<<27;
		else if(port == 60) mask=1<<31;

		GPIO_SET_PINMUX_L9BX(ctr37_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 64)	/* 61 ~ 63 */
	{
		if(port == 61)		mask=1<<11;
		else if(port == 62)	mask=1<<15;
		else if(port == 63)	mask=1<<19;

		GPIO_SET_PINMUX_L9BX(ctr35_reg_gpio_mux_enable, mask, enable);
	}
	else if(port == 64)	/* 64 */
	{
		mask=1<<31;
		GPIO_SET_PINMUX_L9BX(ctr39_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 69)	/* 65 ~ 68 */
	{
		if(port == 65)		mask=1<<19;
		else if(port == 66)	mask=1<<23;
		else if(port == 67)	mask=1<<27;
		else if(port == 68)	mask=1<<31;

		GPIO_SET_PINMUX_L9BX(ctr38_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 72)	/* 69 ~ 71 */
	{
		if(port == 69)		mask=1<<3;
		else if(port == 70) mask=1<<7;
		else if(port == 71) mask=1<<11;

		GPIO_SET_PINMUX_L9BX(ctr37_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 80)	/* 72 ~ 79 */
	{
		mask = 1 << ((port % 8) + 23);
		GPIO_SET_PINMUX_L9BX(ctr39_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 82)	/* 80 ~ 81 */
	{
		if(port == 80)		mask=1<<27;
		else if(port == 81) mask=1<<31;

		GPIO_SET_PINMUX_L9BX(ctr51_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 88)	/* 82 ~ 87 */
	{
		mask = 1 << ((port % 8) + 15);
		GPIO_SET_PINMUX_L9BX(ctr39_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 94)	/* 88 ~ 93 */
	{
		mask = 1 << ((port % 8)*4 + 11);
		GPIO_SET_PINMUX_L9BX(ctr52_reg_gpio_mux_enable, mask, enable);
	}
	else if(port < 96)	/* 94 ~ 95 */
	{
		if(port == 94)		mask=1<<19;
		else if(port == 95)	mask=1<<23;
		GPIO_SET_PINMUX_L9BX(ctr52_reg_gpio_mux_enable, mask, enable);
	}

	return rc;
}

static int _GPIO_SetPinMux_L9Ax(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask = 0;

#define GPIO_SET_PINMUX_L9AX(_reg, _mask, _en)		\
	do{ 											\
		UINT32 _value;								\
		CTOP_CTRL_L9_RdFL(_reg);					\
		_value = CTOP_CTRL_L9_Rd(_reg);				\
		if(_en) _value |= _mask;					\
		else	_value &= (~_mask); 				\
		CTOP_CTRL_L9_Wr(_reg, _value); 				\
		CTOP_CTRL_L9_WrFL(_reg);					\
	} while(0)

	if(port < 32)
	{
		//Do nothing... dedicated ports
	}
	else if(port < 40)	/* 32 ~ 39 */
	{
		mask = 1 << ((port % 8) + 24);
		GPIO_SET_PINMUX_L9AX(ctr32, mask, enable);
	}
	else if(port < 48)	/* 40 ~ 47 */
	{
		mask = 1 << ((port % 8)*4+3);
		GPIO_SET_PINMUX_L9AX(ctr53, mask, enable);
	}
	else if(port < 51)	/* 48 ~ 50 */
	{
		mask = 1 << ((port % 8)*4+23);
		GPIO_SET_PINMUX_L9AX(ctr35, mask, enable);
	}
	else if(port < 56)	/* 51 ~ 55 */
	{
		if(port == 51)		mask=1<<0;
		else if(port == 52) mask=1<<7;
		else if(port == 53) mask=1<<15;
		else if(port == 54) mask=1<<23;
		else if(port == 55) mask=1<<31;

		GPIO_SET_PINMUX_L9AX(ctr34, mask, enable);
	}
	else if(port < 61)	/* 56 ~ 60 */
	{
		if(port == 56)		mask=1<<15;
		else if(port == 57) mask=1<<19;
		else if(port == 58) mask=1<<23;
		else if(port == 59) mask=1<<27;
		else if(port == 60) mask=1<<31;

		GPIO_SET_PINMUX_L9AX(ctr37, mask, enable);
	}
	else if(port < 64)	/* 61 ~ 63 */
	{
		if(port == 61)		mask=1<<11;
		else if(port == 62) mask=1<<15;
		else if(port == 63) mask=1<<19;

		GPIO_SET_PINMUX_L9AX(ctr35, mask, enable);
	}
	else if(port < 65)
	{
		mask=1<<31;
		GPIO_SET_PINMUX_L9AX(ctr39, mask, enable);
	}
	else if(port < 69)	/* 65 ~ 68 */
	{
		if(port == 65)		mask=1<<19;
		else if(port == 66) mask=1<<23;
		else if(port == 67) mask=1<<27;
		else if(port == 68) mask=1<<31;

		GPIO_SET_PINMUX_L9AX(ctr38, mask, enable);
	}
	else if(port < 72)	/* 69 ~ 71 */
	{
		if(port == 69)		mask=1<<3;
		else if(port == 70) mask=1<<7;
		else if(port == 71) mask=1<<11;

		GPIO_SET_PINMUX_L9AX(ctr37, mask, enable);
	}
	else if(port < 79)	/* 72 ~ 79 */
	{
		mask = 1 << ((port % 8) + 23);
		GPIO_SET_PINMUX_L9AX(ctr39, mask, enable);
	}
	else if(port < 82)	/* 80 ~ 81 */
	{
		if(port == 80)		mask=1<<27;
		else if(port == 81) mask=1<<31;

		GPIO_SET_PINMUX_L9AX(ctr51, mask, enable);
	}
	else if(port < 88)	/* 82 ~ 87 */
	{
		mask = 1 << ((port % 8) + 15);
		GPIO_SET_PINMUX_L9AX(ctr39, mask, enable);
	}
	else if(port < 94)	/* 88 ~ 93 */
	{
		mask = 1 << ((port % 8)*4 + 11);
		GPIO_SET_PINMUX_L9AX(ctr52, mask, enable);
	}
	else if(port < 96)	/* 94 ~ 95 */
	{
		if(port == 94)		mask=1<<19;
		else if(port == 95) mask=1<<23;
		GPIO_SET_PINMUX_L9AX(ctr52, mask, enable);
	}

	return rc;
}
#endif


#ifdef INCLUDE_L8_CHIP_KDRV
static int _GPIO_SetPinMux_L8(UINT32 port, BOOLEAN enable)
{
	int rc = 0;
	UINT32 mask, value;

	if(port < 32)
	{
		// Do nothing...
	}
	else if(port < 32 * 2)
	{
		mask = 1 << ( 31 - (port % 32));

		CTOP_CTRL_RdFL(ctr24);
		value = CTOP_CTRL_Rd(ctr24) | mask;
		CTOP_CTRL_Wr(ctr24, value);
		CTOP_CTRL_WrFL(ctr24);
	}
	else if(port < 32 * 3)
	{
		if(lx_chip_rev() < LX_CHIP_REV(L8,B0))
		{
			// for work-around : Top control register connection error - start
			if(port == 81) { mask = 0x4000; }
			else if(port == 82) { mask = 0x400; }
			else if(port == 83) { mask = 0x1000; }
			else if(port == 84) { mask = 0x2000; }
			else if(port == 85) { mask = 0x800; }
			// for work-around : Top control register connection error - end
			else { mask = 1 << ( 31 - (port % 32)); }
		}
		else
		{
			mask = 1 << ( 31 - (port % 32));
		}

		CTOP_CTRL_RdFL(ctr25);
		value = CTOP_CTRL_Rd(ctr25) | mask;
		CTOP_CTRL_Wr(ctr25, value);
		CTOP_CTRL_WrFL(ctr25);
	}

	return rc;
}
#endif


/*************************************************************************
* Get Value
*************************************************************************/
#ifdef INCLUDE_L8_CHIP_KDRV
static int _GPIO_GetValue_L8Ax(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT32 data = GPIONDATA(port/8);

	if(port < 32) 		{ data = (data >> (port % 8)) & 0x1; }
	// for work-around : Top control register connection error - start
	else if(port == 81)	{ data = (data >> 6) & 0x1; } // 0x40
	else if(port == 82)	{ data = (data >> 2) & 0x1; } // 0x4
	else if(port == 83)	{ data = (data >> 4) & 0x1; } // 0x10
	else if(port == 84)	{ data = (data >> 5) & 0x1; } // 0x20
	else if(port == 85)	{ data = (data >> 3) & 0x1; } // 0x8
	// for work-around : Top control register connection error - end
	else				{ data = (data >> (7 - (port % 8))) & 0x1; }

	*value = data ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}

static int _GPIO_GetValue_L8Bx(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT32 data = GPIONDATA(port/8);

	if(port < 32)	{ data = (data >> (port % 8)) & 0x1; }
	else			{ data = (data >> (7 - (port % 8))) & 0x1; }

	*value = data ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}
#endif

int _GPIO_GetValue(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT32 data = GPIONDATA(port/8);
	*value = ((data >> (port % 8)) & 0x1) ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}



/*************************************************************************
* Set Value
*************************************************************************/
#ifdef INCLUDE_L8_CHIP_KDRV
static int _GPIO_SetValue_L8Ax(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT32 mask, data;

	if(port < 32) 		{ mask = 1 << (2 + (port % 8)); }
	// for work-around : Top control register connection error - start
	else if(port == 81)	{ mask = 0x40 << 2; }
	else if(port == 82)	{ mask = 0x4	<< 2; }
	else if(port == 83)	{ mask = 0x10 << 2; }
	else if(port == 84)	{ mask = 0x20 << 2; }
	else if(port == 85)	{ mask = 0x8	<< 2; }
	// for work-around : Top control register connection error - end
	else				{ mask = 1 << (2 + (7-(port % 8))); }

	data = (value == LX_GPIO_VALUE_HIGH) ? 0xff : 0x0;
	SYS_REG_WRITE32(GPION_BASE(port/8) + mask, data);
	return 0;
}

static int _GPIO_SetValue_L8Bx(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT32 mask = 0, data = 0;

	if(port < 32)	{ mask = 1 << (2 + (port % 8)); }
	else			{ mask = 1 << (2 + (7-(port % 8))); }

	data = (value == LX_GPIO_VALUE_HIGH) ? 0xff : 0x0;
	SYS_REG_WRITE32(GPION_BASE(port/8) + mask, data);
	return 0;
}
#endif

static int _GPIO_SetValue(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT32 mask = 0, data = 0;

	mask = 1 << (2 + (port % 8));

	data = (value == LX_GPIO_VALUE_HIGH) ? 0xff : 0x0;
	SYS_WRITE32(GPION_BASE(port/8) + mask, data);
	return 0;
}



/*************************************************************************
* Set Mode
*************************************************************************/
static int _GPIO_SetMode(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT32 direction, mask;

	direction = GPIONDIR(port/8);

	mask = 1 << (port % 8);
	direction = (mode == LX_GPIO_MODE_INPUT) ?
				direction & (~mask) : direction | mask;

	GPIONDIR(port/8) = direction;
	return 0;
}

static int _GPIO_SetIntr(UINT32 port, UINT32 enable)
{
	UINT32 enable_array=0, sense_array=0,both_edge_array=0,high_array=0, mask=0;

	sense_array = GPIONIS(port/8);
	both_edge_array = GPIONIBE(port/8);
	high_array  = GPIONIEV(port/8);
	enable_array = GPIONIE(port/8);

	mask = 1 << (port % 8);

	sense_array = (enable == 1) ?
				  sense_array & (~mask):(sense_array | mask) ;

	both_edge_array = (enable == 1) ?
				  both_edge_array & (~mask):(both_edge_array | mask) ;

	high_array  = (enable == 1) ?
				  (high_array | mask) : high_array & (~mask);

	enable_array = (enable == 1) ?
				  (enable_array | mask) : enable_array & (~mask);

	GPIONIS(port/8) = sense_array;
	GPIONIBE(port/8) = both_edge_array;
	GPIONIEV(port/8) = high_array;
	GPIONIE(port/8) = enable_array;

	return 0;
}




#ifdef INCLUDE_L8_CHIP_KDRV
static int _GPIO_SetMode_L8Ax(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT32 direction, mask;

	direction = GPIONDIR(port/8);

	if(port < 32) 		{ mask = 1 << (port % 8); }
	// for work-around : Top control register connection error - start
	else if(port == 81)	{ mask = 0x40; }
	else if(port == 82)	{ mask = 0x4; }
	else if(port == 83)	{ mask = 0x10; }
	else if(port == 84)	{ mask = 0x20; }
	else if(port == 85)	{ mask = 0x8; }
	// for work-around : Top control register connection error - end
	else				{ mask = 1 << (7 - (port % 8)); }

	direction = (mode == LX_GPIO_MODE_INPUT) ?
					direction & (~mask) : direction | mask;

	GPIONDIR(port/8) = direction;
	return 0;
}

static int _GPIO_SetMode_L8Bx(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT32 direction, mask;

	direction = GPIONDIR(port/8);
	if(port < 32)	{ mask = 1 << (port % 8); }
	else			{ mask = 1 << (7 - (port % 8)); }
	direction = (mode == LX_GPIO_MODE_INPUT) ?
				direction & (~mask) : direction | mask;

	GPIONDIR(port/8) = direction;
	return 0;
}
#endif

#ifdef INCLUDE_H13_CHIP_KDRV

static int _GPIO_SetMode_H13Bx(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT32 direction=0, mask=0;

	direction = GPIONDIR(port/8);

	if(port < 136)	{ mask = 1 << (port % 8); }
	else	 if(port >= 136 && port < 144)		{ mask = 1 << (7 - (port % 8)); }


	direction = (mode == LX_GPIO_MODE_INPUT) ?
				direction & (~mask) : direction | mask;

	GPIONDIR(port/8) = direction;
	return 0;
}

static int _GPIO_GetMode_H13Bx(UINT32 port, LX_GPIO_MODE_T *mode)
{
	UINT32 direction=0, mask = 0;

	direction = GPIONDIR(port/8);
	if(port < 136)	{ mask = 1 << (port % 8); }
	else	 if(port >= 136 && port < 144)		{ mask = 1 << (7 - (port % 8)); }
	*mode = (direction & mask) ? LX_GPIO_MODE_OUTPUT : LX_GPIO_MODE_INPUT;
	return 0;
}

static int _GPIO_SetValue_H13Bx(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT32 mask=0, data=0;

	if(port < 136)	{ mask = 1 << (2 + (port % 8)); }
	else	 if(port >= 136 && port < 144)		{ mask = 1 << (2 + (7-(port % 8))); }

	data = (value == LX_GPIO_VALUE_HIGH) ? 0xff : 0x0;
	SYS_WRITE32(GPION_BASE(port/8) + mask, data);
	return 0;
}

int _GPIO_GetValue_H13Bx(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT32 data = GPIONDATA(port/8);

	if(port < 136)	{ data = (data >> (port % 8)) & 0x1; }
	else	 if(port >= 136 && port < 144)		{ data = (data >> (7 - (port % 8))) & 0x1; }

	*value = data ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}

static int _GPIO_SetIntr_H13Bx(UINT32 port, UINT32 enable)
{
	UINT32 enable_array=0, sense_array=0,both_edge_array=0,high_array=0, mask=0;

	sense_array = GPIONIS(port/8);
	both_edge_array = GPIONIBE(port/8);
	high_array  = GPIONIEV(port/8);
	enable_array = GPIONIE(port/8);

	if(port < 136)	{ mask = 1 << (port % 8); }
	else	 if(port >= 136 && port < 144)		{ mask = 1 << (7 - (port % 8)); }

	sense_array = (enable == 1) ?
				  sense_array & (~mask):(sense_array | mask) ;

	both_edge_array = (enable == 1) ?
				  both_edge_array & (~mask):(both_edge_array | mask) ;

	high_array  = (enable == 1) ?
				  (high_array | mask) : high_array & (~mask);

	enable_array = (enable == 1) ?
				  (enable_array | mask) : enable_array & (~mask);

	GPIONIS(port/8) = sense_array;
	GPIONIBE(port/8) = both_edge_array;
	GPIONIEV(port/8) = high_array;
	GPIONIE(port/8) = enable_array;

	return 0;
}

static int _GPIO_SetIntrCB_H13Bx(UINT32 port, void (*pfnGPIO_CB)(UINT32 value), UINT32 enable)
{
	_GPIO_SetMode( port, LX_GPIO_MODE_INPUT );
	_GPIO_SetIntr_H13Bx( port, enable );
	return 0;
}
#endif


static int _GPIO_GetMode(UINT32 port, LX_GPIO_MODE_T *mode)
{
	UINT32 direction, mask;

	direction = GPIONDIR(port/8);
	mask = 1 << (port % 8);
	*mode = (direction & mask) ? LX_GPIO_MODE_OUTPUT : LX_GPIO_MODE_INPUT;
	return 0;
}

static int _GPIO_SetIntrCB(UINT32 port, void (*pfnGPIO_CB)(UINT32 value), UINT32 enable)
{
	_GPIO_SetMode( port, LX_GPIO_MODE_INPUT );
	_GPIO_SetIntr( port, enable );
	return 0;
}

static int _GPIO_GetIntrValue(UINT32 * port, LX_GPIO_VALUE_T *value)
{
	ULONG flags = 0;

	if( gpio_intr_pin != 0)
	{
		*port =  (UINT32)gpio_intr_pin;
		*value = (LX_GPIO_VALUE_T)gpio_intr_pin_value;

		spin_lock_irqsave(&gpioPoll_lock, flags);
		gpio_intr_pin = 0;
		gpio_intr_pin_value = 0;
		spin_unlock_irqrestore(&gpioPoll_lock, flags);
	}
	else
	{
		*port =  (UINT32)0;
		*value = (LX_GPIO_VALUE_T)0;
	}
	return 0;
}


#ifdef KDRV_CONFIG_PM

#ifdef INCLUDE_L8_CHIP_KDRV
static int _GPIO_Resume_L8(void)
{
	int i;

#if 0	// It's not used any longer, so I don't care this code
	CTOP_CTRL_Wr(ctr24, drv_data->top_control_24);
	CTOP_CTRL_WrFL(ctr24);
	CTOP_CTRL_Wr(ctr25, drv_data->top_control_25);
	CTOP_CTRL_WrFL(ctr25);
#endif

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		GPIONDIR(i) = _pGpioDev->pmdata[i].direction;
	}

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		GPIONDATA(i) = _pGpioDev->pmdata[i].data;
	}
}

static int _GPIO_Suspend_L8(void)
{
	int i;

#if 0	// It's not used any longer, so I don't care this code
	CTOP_CTRL_RdFL(ctr24);
	drv_data->top_control_24 = CTOP_CTRL_Rd(ctr24);
	CTOP_CTRL_RdFL(ctr25);
	drv_data->top_control_25 = CTOP_CTRL_Rd(ctr25);
#endif

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		_pGpioDev->pmdata[i].direction = GPIONDIR(i);
	}

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		_pGpioDev->pmdata[i].data = GPIONDATA(i);
	}

}
#endif

int GPIO_DevResume(void)
{
	if(_pGpioDev->Resume) _pGpioDev->Resume();
	return 0;
}

int GPIO_DevSuspend(void)
{
	if(_pGpioDev->Suspend) _pGpioDev->Suspend();
	return 0;
}
#endif




int GPIO_DevSetPinMux(UINT32 port, BOOLEAN enable)
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->SetPinMux(port, enable);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevGetPinMux(UINT32 port, BOOLEAN *enable)
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->GetPinMux(port, enable);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevSetValue(UINT32 port, LX_GPIO_VALUE_T value)
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->SetValue(port, value);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevGetValue(UINT32 port, LX_GPIO_VALUE_T *value)
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->GetValue(port, value);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}


int GPIO_DevSetMode(UINT32 port, LX_GPIO_MODE_T mode)
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->SetMode(port, mode);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevGetMode(UINT32 port, LX_GPIO_MODE_T *mode)
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->GetMode(port, mode);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevSetISR(UINT32 port, void (*pfnGPIO_CB)	(UINT32 ), UINT32 enable)
//int GPIO_DevSetISR(UINT32 port, GPIO_INTR_CALLBACK_T )
{
	int rc = -1;
	if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->SetIntrCB(port, pfnGPIO_CB, enable);
		GPIO_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevGetIntrValue(UINT32 *port, LX_GPIO_VALUE_T *value)
{
	int rc = -1;

	//if(port < _pGpioDev->max_num)
	{
		GPIO_LOCK(_pGpioDev);
		rc = _pGpioDev->GetIntrValue(port, value);
		GPIO_UNLOCK(_pGpioDev);
	}

	return rc;
}



/*****************************************
 * ACCESS GPIOs in LG115xAN(ACE)         *
 *****************************************/
static int _GPIO_ExSetPinMux(UINT32 port, BOOLEAN enable)
{
	// do nothing
	return 0;
}

static int _GPIO_ExGetPinMux(UINT32 port, BOOLEAN *enable)
{
	*enable = 1;
	return 0;
}

#ifdef INCLUDE_M14_CHIP_KDRV
static int _GPIO_ExGetValue_M14Ax(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT8 mask, data;

	GPIO_CORE_DEBUG("_GPIO_ExGetValue_M14Ax. port:%d\n", port);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_M14A0_RdFL(gpio_16);
		data = ACE_REG_M14A0_Rd(gpio_16);
	}
	else
	{
		ACE_REG_M14A0_RdFL(gpio_17);
		data = ACE_REG_M14A0_Rd(gpio_17);
	}
	GPIO_CORE_DEBUG("DATA:0x%02x\n", data);

	/*
	(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
	(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
	*/
	*value = (data&mask) ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}

static int _GPIO_ExSetValue_M14Ax(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT8 mask, data;

	GPIO_CORE_DEBUG("_GPIO_ExSetValue_M14Ax. port:%d, value:%d\n", port, value);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_M14A0_RdFL(gpio_0);
		data = ACE_REG_M14A0_Rd(gpio_0);
	}
	else
	{
		ACE_REG_M14A0_RdFL(gpio_1);
		data = ACE_REG_M14A0_Rd(gpio_1);
	}
	data = (value == LX_GPIO_VALUE_LOW) ?
				data & (~mask) : data | mask;

	if(port < 8)
	{
		ACE_REG_M14A0_Wr(gpio_0, data);
		ACE_REG_M14A0_WrFL(gpio_0);
	}
	else
	{
		ACE_REG_M14A0_Wr(gpio_1, data);
		ACE_REG_M14A0_WrFL(gpio_1);
	}
	return 0;
}

static int _GPIO_ExSetMode_M14Ax(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT8 direction, mask;

	GPIO_CORE_DEBUG("_GPIO_ExSetMode_M14Ax. port:%d, mode:%d\n", port, mode);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_M14A0_RdFL(gpio_2);
		direction = ACE_REG_M14A0_Rd(gpio_2);
	}
	else
	{
		ACE_REG_M14A0_RdFL(gpio_3);
		direction = ACE_REG_M14A0_Rd(gpio_3);
	}
	GPIO_CORE_DEBUG("DIR:0x%02x\n", direction);

	direction = (mode == LX_GPIO_MODE_INPUT) ?
				direction & (~mask) : direction | mask;

	if(port < 8)
	{
		ACE_REG_M14A0_Wr(gpio_2, direction);
		ACE_REG_M14A0_WrFL(gpio_2);
	}
	else
	{
		ACE_REG_M14A0_Wr(gpio_3, direction);
		ACE_REG_M14A0_WrFL(gpio_3);
	}
	return 0;
}

static int _GPIO_ExGetMode_M14Ax(UINT32 port, LX_GPIO_MODE_T *mode)
{
	UINT8 direction, mask;

	GPIO_CORE_DEBUG("_GPIO_ExGetMode_M14Ax. port:%d\n", port);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_M14A0_RdFL(gpio_2);
		direction = ACE_REG_M14A0_Rd(gpio_2);
	}
	else
	{
		ACE_REG_M14A0_RdFL(gpio_3);
		direction = ACE_REG_M14A0_Rd(gpio_3);
	}
	GPIO_CORE_DEBUG("DIR:0x%02x\n", direction);
	*mode = (direction & mask) ? LX_GPIO_MODE_OUTPUT : LX_GPIO_MODE_INPUT;

	return 0;
}



#ifdef KDRV_CONFIG_PM
static int _GPIO_Resume_M14(void)
{
	int i;

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		GPIONDIR(i) = _pGpioDev->pmdata[i].direction;
	}

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		GPIONDATA(i) = _pGpioDev->pmdata[i].data;
	}
	return 0;
}

static int _GPIO_Suspend_M14(void)
{
	int i;

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		_pGpioDev->pmdata[i].direction = GPIONDIR(i);
	}

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		_pGpioDev->pmdata[i].data = GPIONDATA(i);
	}
	return 0;
}
#endif //
#endif



#ifdef INCLUDE_H14_CHIP_KDRV
static int _GPIO_ExGetValue_H14Ax(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT8 mask, data;

	GPIO_CORE_DEBUG("_GPIO_ExGetValue_H14Ax. port:%d\n", port);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H14A0_RdFL(gpio_16);
		data = ACE_REG_H14A0_Rd(gpio_16);
	}
	else
	{
		ACE_REG_H14A0_RdFL(gpio_17);
		data = ACE_REG_H14A0_Rd(gpio_17);
	}
	GPIO_CORE_DEBUG("DATA:0x%02x\n", data);

	/*
	(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
	(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
	*/
	*value = (data&mask) ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}

static int _GPIO_ExSetValue_H14Ax(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT8 mask, data;

	GPIO_CORE_DEBUG("_GPIO_ExSetValue_H14Ax. port:%d, value:%d\n", port, value);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H14A0_RdFL(gpio_0);
		data = ACE_REG_H14A0_Rd(gpio_0);
	}
	else
	{
		ACE_REG_H14A0_RdFL(gpio_1);
		data = ACE_REG_H14A0_Rd(gpio_1);
	}
	data = (value == LX_GPIO_VALUE_LOW) ?
				data & (~mask) : data | mask;

	if(port < 8)
	{
		ACE_REG_H14A0_Wr(gpio_0, data);
		ACE_REG_H14A0_WrFL(gpio_0);
	}
	else
	{
		ACE_REG_H14A0_Wr(gpio_1, data);
		ACE_REG_H14A0_WrFL(gpio_1);
	}
	return 0;
}

static int _GPIO_ExSetMode_H14Ax(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT8 direction, mask;

	GPIO_CORE_DEBUG("_GPIO_ExSetMode_H14Ax. port:%d, mode:%d\n", port, mode);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H14A0_RdFL(gpio_2);
		direction = ACE_REG_H14A0_Rd(gpio_2);
	}
	else
	{
		ACE_REG_H14A0_RdFL(gpio_3);
		direction = ACE_REG_H14A0_Rd(gpio_3);
	}
	GPIO_CORE_DEBUG("DIR:0x%02x\n", direction);

	direction = (mode == LX_GPIO_MODE_INPUT) ?
				direction & (~mask) : direction | mask;

	if(port < 8)
	{
		ACE_REG_H14A0_Wr(gpio_2, direction);
		ACE_REG_H14A0_WrFL(gpio_2);
	}
	else
	{
		ACE_REG_H14A0_Wr(gpio_3, direction);
		ACE_REG_H14A0_WrFL(gpio_3);
	}
	return 0;
}

static int _GPIO_ExGetMode_H14Ax(UINT32 port, LX_GPIO_MODE_T *mode)
{
	UINT8 direction, mask;

	GPIO_CORE_DEBUG("_GPIO_ExGetMode_H14Ax. port:%d\n", port);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H14A0_RdFL(gpio_2);
		direction = ACE_REG_H14A0_Rd(gpio_2);
	}
	else
	{
		ACE_REG_H14A0_RdFL(gpio_3);
		direction = ACE_REG_H14A0_Rd(gpio_3);
	}
	GPIO_CORE_DEBUG("DIR:0x%02x\n", direction);
	*mode = (direction & mask) ? LX_GPIO_MODE_OUTPUT : LX_GPIO_MODE_INPUT;

	return 0;
}
#ifdef KDRV_CONFIG_PM
static int _GPIO_Resume_H14(void)
{
	int i;

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		GPIONDIR(i) = _pGpioDev->pmdata[i].direction;
	}

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		GPIONDATA(i) = _pGpioDev->pmdata[i].data;
	}
	return 0;
}

static int _GPIO_Suspend_H14(void)
{
	int i;

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		_pGpioDev->pmdata[i].direction = GPIONDIR(i);
	}

	for(i = 0; i < _pGpioDev->max_num/8; i++)
	{
		_pGpioDev->pmdata[i].data = GPIONDATA(i);
	}
	return 0;

}
#endif //KDRV_CONFIG_PM
#endif

#ifdef INCLUDE_H13_CHIP_KDRV
static int _GPIO_ExGetValue_H13Ax(UINT32 port, LX_GPIO_VALUE_T *value)
{
	UINT8 mask, data;

	GPIO_CORE_DEBUG("_GPIO_ExGetValue_H13Ax. port:%d\n", port);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H13A0_RdFL(gpio_16);
		data = ACE_REG_H13A0_Rd(gpio_16);
	}
	else
	{
		ACE_REG_H13A0_RdFL(gpio_17);
		data = ACE_REG_H13A0_Rd(gpio_17);
	}
	GPIO_CORE_DEBUG("DATA:0x%02x\n", data);

	/*
	(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
	(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
	*/
	*value = (data&mask) ? LX_GPIO_VALUE_HIGH : LX_GPIO_VALUE_LOW;
	return 0;
}

static int _GPIO_ExSetValue_H13Ax(UINT32 port, LX_GPIO_VALUE_T value)
{
	UINT8 mask, data;

	GPIO_CORE_DEBUG("_GPIO_ExSetValue_H13Ax. port:%d, value:%d\n", port, value);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H13A0_RdFL(gpio_0);
		data = ACE_REG_H13A0_Rd(gpio_0);
	}
	else
	{
		ACE_REG_H13A0_RdFL(gpio_1);
		data = ACE_REG_H13A0_Rd(gpio_1);
	}
	data = (value == LX_GPIO_VALUE_LOW) ?
				data & (~mask) : data | mask;

	if(port < 8)
	{
		ACE_REG_H13A0_Wr(gpio_0, data);
		ACE_REG_H13A0_WrFL(gpio_0);
	}
	else
	{
		ACE_REG_H13A0_Wr(gpio_1, data);
		ACE_REG_H13A0_WrFL(gpio_1);
	}
	return 0;
}

static int _GPIO_ExSetMode_H13Ax(UINT32 port, LX_GPIO_MODE_T mode)
{
	UINT8 direction, mask;

	GPIO_CORE_DEBUG("_GPIO_ExSetMode_H13Ax. port:%d, mode:%d\n", port, mode);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H13A0_RdFL(gpio_2);
		direction = ACE_REG_H13A0_Rd(gpio_2);
	}
	else
	{
		ACE_REG_H13A0_RdFL(gpio_3);
		direction = ACE_REG_H13A0_Rd(gpio_3);
	}
	GPIO_CORE_DEBUG("DIR:0x%02x\n", direction);

	direction = (mode == LX_GPIO_MODE_INPUT) ?
				direction & (~mask) : direction | mask;

	if(port < 8)
	{
		ACE_REG_H13A0_Wr(gpio_2, direction);
		ACE_REG_H13A0_WrFL(gpio_2);
	}
	else
	{
		ACE_REG_H13A0_Wr(gpio_3, direction);
		ACE_REG_H13A0_WrFL(gpio_3);
	}
	return 0;
}

static int _GPIO_ExGetMode_H13Ax(UINT32 port, LX_GPIO_MODE_T *mode)
{
	UINT8 direction, mask;

	GPIO_CORE_DEBUG("_GPIO_ExGetMode_H13Ax. port:%d\n", port);

	mask = 1 << (port % 8);
	if(port < 8)
	{
		ACE_REG_H13A0_RdFL(gpio_2);
		direction = ACE_REG_H13A0_Rd(gpio_2);
	}
	else
	{
		ACE_REG_H13A0_RdFL(gpio_3);
		direction = ACE_REG_H13A0_Rd(gpio_3);
	}
	GPIO_CORE_DEBUG("DIR:0x%02x\n", direction);
	*mode = (direction & mask) ? LX_GPIO_MODE_OUTPUT : LX_GPIO_MODE_INPUT;

	return 0;
}
#endif


int GPIO_DevExSetPinMux(UINT32 port, BOOLEAN enable)
{
	int rc = -1;
	if(port < _pGpioDev->max_ex_num)
	{
		GPIO_EX_LOCK(_pGpioDev);
		rc = _pGpioDev->ExSetPinMux(port, enable);
		GPIO_EX_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevExGetPinMux(UINT32 port, BOOLEAN *enable)
{
	int rc = -1;
	if(port < _pGpioDev->max_ex_num)
	{
		GPIO_EX_LOCK(_pGpioDev);
		rc = _pGpioDev->ExGetPinMux(port, enable);
		GPIO_EX_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevExSetValue(UINT32 port, LX_GPIO_VALUE_T value)
{
	int rc = -1;
	if(port < _pGpioDev->max_ex_num)
	{
		GPIO_EX_LOCK(_pGpioDev);
		rc = _pGpioDev->ExSetValue(port, value);
		GPIO_EX_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevExGetValue(UINT32 port, LX_GPIO_VALUE_T *value)
{
	int rc = -1;
	if(port < _pGpioDev->max_ex_num)
	{
		GPIO_EX_LOCK(_pGpioDev);
		rc = _pGpioDev->ExGetValue(port, value);
		GPIO_EX_UNLOCK(_pGpioDev);
	}
	return rc;
}


int GPIO_DevExSetMode(UINT32 port, LX_GPIO_MODE_T mode)
{
	int rc = -1;
	if(port < _pGpioDev->max_ex_num)
	{
		GPIO_EX_LOCK(_pGpioDev);
		rc = _pGpioDev->ExSetMode(port, mode);
		GPIO_EX_UNLOCK(_pGpioDev);
	}
	return rc;
}

int GPIO_DevExGetMode(UINT32 port, LX_GPIO_MODE_T *mode)
{
	int rc = -1;
	if(port < _pGpioDev->max_ex_num)
	{
		GPIO_EX_LOCK(_pGpioDev);
		rc = _pGpioDev->ExGetMode(port, mode);
		GPIO_EX_UNLOCK(_pGpioDev);
	}
	return rc;
}



int GPIO_DevInit(void)
{
	UINT32 i, num_blocks;
	UINT32 phys_base, addr_gap;

	_pGpioDev = (GPIO_DEV_T*)OS_Malloc(sizeof(GPIO_DEV_T));
	memset(_pGpioDev, 0, sizeof(GPIO_DEV_T));


	GPIO_LOCK_INIT(_pGpioDev);
	GPIO_EX_LOCK_INIT(_pGpioDev);

	if(0)
	{}
	else if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{

		#ifdef INCLUDE_H14_CHIP_KDRV
		phys_base		= H14_GPIO0_BASE;
		addr_gap		= 0x10000;

		_pGpioDev->max_num	= 144;		// 144, Not supported GPIO(17) in CTOP // added 141,142,143 original gpio input(no need to set CTOP)
		num_blocks = (_pGpioDev->max_num+7)/8;

		_pGpioDev->GetValue 	= _GPIO_GetValue;
		_pGpioDev->SetValue 	= _GPIO_SetValue;
		_pGpioDev->SetMode		= _GPIO_SetMode;
		_pGpioDev->GetMode		= _GPIO_GetMode;
		_pGpioDev->SetPinMux	= _GPIO_SetPinMux_H14Ax;
		_pGpioDev->GetPinMux	= _GPIO_GetPinMux_H14Ax;
		_pGpioDev->SetIntrCB	= _GPIO_SetIntrCB;
		_pGpioDev->GetIntrValue	= _GPIO_GetIntrValue;

		_pGpioDev->max_ex_num	= 16;
		_pGpioDev->ExGetValue 	= _GPIO_ExGetValue_H14Ax;
		_pGpioDev->ExSetValue 	= _GPIO_ExSetValue_H14Ax;
		_pGpioDev->ExSetMode	= _GPIO_ExSetMode_H14Ax;
		_pGpioDev->ExGetMode	= _GPIO_ExGetMode_H14Ax;
		_pGpioDev->ExSetPinMux	= _GPIO_ExSetPinMux;
		_pGpioDev->ExGetPinMux	= _GPIO_ExGetPinMux;
#ifdef KDRV_CONFIG_PM
		_pGpioDev->pmdata = (GPIO_PM_DATA_T*)OS_Malloc(sizeof(GPIO_PM_DATA_T)*_pGpioDev->max_num );
		_pGpioDev->Resume	= _GPIO_Resume_H14;
		_pGpioDev->Suspend	= _GPIO_Suspend_H14;
#endif //
		/* Set Interrupt Sense to get the value */
		ACE_REG_H14A0_Wr(gpio_4, 0xFF);
		ACE_REG_H14A0_WrFL(gpio_4);
		ACE_REG_H14A0_Wr(gpio_5, 0xFF);
		ACE_REG_H14A0_WrFL(gpio_5);

		/*
		(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
		(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
		*/
		ACE_REG_H14A0_Wr(gpio_8, 0xFF);
		ACE_REG_H14A0_WrFL(gpio_8);
		ACE_REG_H14A0_Wr(gpio_9, 0xFF);
		ACE_REG_H14A0_WrFL(gpio_9);
		#endif

	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		phys_base		= M14_GPIO__BASE;
		addr_gap		= 0x10000;

		_pGpioDev->max_num	= 136;		// 144, Not supported GPIO(17) in CTOP
		num_blocks = (_pGpioDev->max_num+7)/8;

		_pGpioDev->GetValue 	= _GPIO_GetValue;
		_pGpioDev->SetValue 	= _GPIO_SetValue;
		_pGpioDev->SetMode		= _GPIO_SetMode;
		_pGpioDev->GetMode		= _GPIO_GetMode;
		_pGpioDev->SetPinMux	= _GPIO_SetPinMux_M14Bx;
		_pGpioDev->GetPinMux	= _GPIO_GetPinMux_M14Bx;
		_pGpioDev->SetIntrCB	= _GPIO_SetIntrCB;
		_pGpioDev->GetIntrValue	= _GPIO_GetIntrValue;

		_pGpioDev->max_ex_num	= 16;
		_pGpioDev->ExGetValue 	= _GPIO_ExGetValue_M14Ax;
		_pGpioDev->ExSetValue 	= _GPIO_ExSetValue_M14Ax;
		_pGpioDev->ExSetMode	= _GPIO_ExSetMode_M14Ax;
		_pGpioDev->ExGetMode	= _GPIO_ExGetMode_M14Ax;
		_pGpioDev->ExSetPinMux	= _GPIO_ExSetPinMux;
		_pGpioDev->ExGetPinMux	= _GPIO_ExGetPinMux;
#ifdef KDRV_CONFIG_PM
		_pGpioDev->pmdata = (GPIO_PM_DATA_T*)OS_Malloc(sizeof(GPIO_PM_DATA_T)*_pGpioDev->max_num );
		_pGpioDev->Resume	= _GPIO_Resume_M14;
		_pGpioDev->Suspend	= _GPIO_Suspend_M14;
#endif //
		#if 0 // code cause errro in M14 - temperal remove
		/* Set Interrupt Sense to get the value */
		ACE_REG_M14A0_Wr(gpio_4, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_4);
		ACE_REG_M14A0_Wr(gpio_5, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_5);

		/*
		(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
		(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
		*/
		ACE_REG_M14A0_Wr(gpio_8, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_8);
		ACE_REG_M14A0_Wr(gpio_9, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_9);
		#endif
		#endif

	}
	else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		#ifdef INCLUDE_M14_CHIP_KDRV
		phys_base		= M14_GPIO__BASE;
		addr_gap		= 0x10000;

		_pGpioDev->max_num	= 136;		// 144, Not supported GPIO(17) in CTOP
		num_blocks = (_pGpioDev->max_num+7)/8;

		_pGpioDev->GetValue 	= _GPIO_GetValue;
		_pGpioDev->SetValue 	= _GPIO_SetValue;
		_pGpioDev->SetMode		= _GPIO_SetMode;
		_pGpioDev->GetMode		= _GPIO_GetMode;
		_pGpioDev->SetPinMux	= _GPIO_SetPinMux_M14Ax;
		_pGpioDev->GetPinMux	= _GPIO_GetPinMux_M14Ax;
		_pGpioDev->SetIntrCB	= _GPIO_SetIntrCB;
		_pGpioDev->GetIntrValue	= _GPIO_GetIntrValue;

		_pGpioDev->max_ex_num	= 16;
		_pGpioDev->ExGetValue 	= _GPIO_ExGetValue_M14Ax;
		_pGpioDev->ExSetValue 	= _GPIO_ExSetValue_M14Ax;
		_pGpioDev->ExSetMode	= _GPIO_ExSetMode_M14Ax;
		_pGpioDev->ExGetMode	= _GPIO_ExGetMode_M14Ax;
		_pGpioDev->ExSetPinMux	= _GPIO_ExSetPinMux;
		_pGpioDev->ExGetPinMux	= _GPIO_ExGetPinMux;
#ifdef KDRV_CONFIG_PM
		_pGpioDev->pmdata = (GPIO_PM_DATA_T*)OS_Malloc(sizeof(GPIO_PM_DATA_T)*_pGpioDev->max_num );
		_pGpioDev->Resume	= _GPIO_Resume_M14;
		_pGpioDev->Suspend	= _GPIO_Suspend_M14;
#endif //
		#if 0 // code cause errro in M14 - temperal remove
		/* Set Interrupt Sense to get the value */
		ACE_REG_M14A0_Wr(gpio_4, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_4);
		ACE_REG_M14A0_Wr(gpio_5, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_5);

		/*
		(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
		(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
		*/
		ACE_REG_M14A0_Wr(gpio_8, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_8);
		ACE_REG_M14A0_Wr(gpio_9, 0xFF);
		ACE_REG_M14A0_WrFL(gpio_9);
		#endif
		#endif

	}

	else if(lx_chip_rev() >= LX_CHIP_REV(H13, B0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		phys_base		= H13_GPIO__BASE;
		addr_gap		= 0x10000;

		_pGpioDev->max_num	= 144;		// 144, Not supported GPIO(17) in CTOP
		num_blocks = (_pGpioDev->max_num+7)/8;

		_pGpioDev->GetValue 	= _GPIO_GetValue_H13Bx;
		_pGpioDev->SetValue 	= _GPIO_SetValue_H13Bx;
		_pGpioDev->SetMode		= _GPIO_SetMode_H13Bx;
		_pGpioDev->GetMode		= _GPIO_GetMode_H13Bx;
		_pGpioDev->SetPinMux	= _GPIO_SetPinMux_H13Ax;
		_pGpioDev->GetPinMux	= _GPIO_GetPinMux_H13Ax;
		_pGpioDev->SetIntrCB	= _GPIO_SetIntrCB_H13Bx;
		_pGpioDev->GetIntrValue	= _GPIO_GetIntrValue;

		_pGpioDev->max_ex_num	= 16;
		_pGpioDev->ExGetValue 	= _GPIO_ExGetValue_H13Ax;
		_pGpioDev->ExSetValue 	= _GPIO_ExSetValue_H13Ax;
		_pGpioDev->ExSetMode	= _GPIO_ExSetMode_H13Ax;
		_pGpioDev->ExGetMode	= _GPIO_ExGetMode_H13Ax;
		_pGpioDev->ExSetPinMux	= _GPIO_ExSetPinMux;
		_pGpioDev->ExGetPinMux	= _GPIO_ExGetPinMux;

		/* Set Interrupt Sense to get the value */
		ACE_REG_H13A0_Wr(gpio_4, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_4);
		ACE_REG_H13A0_Wr(gpio_5, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_5);

		/*
		(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
		(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
		*/
		ACE_REG_H13A0_Wr(gpio_8, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_8);
		ACE_REG_H13A0_Wr(gpio_9, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_9);

		GPIO_ERROR("gpionaxnum[%d] numblox[%d]\n",_pGpioDev->max_num,num_blocks );
		#endif

	}
	else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		#ifdef INCLUDE_H13_CHIP_KDRV
		phys_base		= H13_GPIO__BASE;
		addr_gap		= 0x10000;

		_pGpioDev->max_num	= 144;		// 144, Not supported GPIO(17) in CTOP
		num_blocks = (_pGpioDev->max_num+7)/8;

		_pGpioDev->GetValue 	= _GPIO_GetValue;
		_pGpioDev->SetValue 	= _GPIO_SetValue;
		_pGpioDev->SetMode		= _GPIO_SetMode;
		_pGpioDev->GetMode		= _GPIO_GetMode;
		_pGpioDev->SetPinMux	= _GPIO_SetPinMux_H13Ax;
		_pGpioDev->GetPinMux	= _GPIO_GetPinMux_H13Ax;
		_pGpioDev->SetIntrCB	= _GPIO_SetIntrCB;
		_pGpioDev->GetIntrValue	= _GPIO_GetIntrValue;

		_pGpioDev->max_ex_num	= 16;
		_pGpioDev->ExGetValue 	= _GPIO_ExGetValue_H13Ax;
		_pGpioDev->ExSetValue 	= _GPIO_ExSetValue_H13Ax;
		_pGpioDev->ExSetMode	= _GPIO_ExSetMode_H13Ax;
		_pGpioDev->ExGetMode	= _GPIO_ExGetMode_H13Ax;
		_pGpioDev->ExSetPinMux	= _GPIO_ExSetPinMux;
		_pGpioDev->ExGetPinMux	= _GPIO_ExGetPinMux;

		/* Set Interrupt Sense to get the value */
		ACE_REG_H13A0_Wr(gpio_4, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_4);
		ACE_REG_H13A0_Wr(gpio_5, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_5);

		/*
		(3) GPIO_IEV 가 1일면 input level인 ‘1’ 인 경우 High, ‘0’이면 Low
		(4) GPIO_IEV 가 0일면 input level인 ‘0’ 인 경우 High, ‘1’이면 Low
		*/
		ACE_REG_H13A0_Wr(gpio_8, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_8);
		ACE_REG_H13A0_Wr(gpio_9, 0xFF);
		ACE_REG_H13A0_WrFL(gpio_9);

		#endif

	}
	else
	{

		GPIO_ERROR("AUD : LX_CHIP_REV => Unknown(0x%X) : ERROR\n", lx_chip_rev());
		GPIO_ERROR("Unsupported chip !!!\n");
		return -1;
	}

	_gpioBaseAddr = (UINT32*)OS_Malloc(num_blocks * sizeof(UINT32));
	for(i=0; i<num_blocks; i++)
	{
		_gpioBaseAddr[i] = (UINT32)ioremap(phys_base + i*addr_gap , 0x10000);
		GPIO_PRINT("_gpioBaseAddr[%d]=[%8x] phy[%8x] \n",i,_gpioBaseAddr[i],phys_base + i*addr_gap );
	}
	return 0;
}



/** @} */
