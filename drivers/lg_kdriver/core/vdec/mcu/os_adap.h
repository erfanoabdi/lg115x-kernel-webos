/*
   SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
   Copyright(c) 2010 by LG Electronics Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
 */

#ifndef __OS_ADAP_H__
#define __OS_ADAP_H__

#include "vdec_type_defs.h"

#ifdef __XTENSA__
#include <xtensa/xtruntime.h>
#include <stdio.h>
#include "../hal/lg1152/mcu_hal_api.h"

#define EXTINT0         0
#define EXTINT1         1
#define EXTINT2         2
#define EXTINT0_MASK    0x1
#define EXTINT1_MASK    0x2
#define EXTINT2_MASK    0x4

#define OS_INTS_ON(x)	_xtos_ints_on(x)
#define OS_INTS_OFF(x)	_xtos_ints_off(x)

#define	VDEC_TranselateVirualAddr(x, y)		MCU_HAL_TranslateAddrforMCU(x, y)
#define	VDEC_ClearVirualAddr(x)

#else //!__XTENSA__
#include <linux/io.h>

#define OS_INTS_ON(x)
#define OS_INTS_OFF(x)

#define	VDEC_TranselateVirualAddr(x, y)		ioremap(x, y)
#define	VDEC_ClearVirualAddr(x)				iounmap(x)

#endif //~__XTENSA__

#endif //~__OS_ADAP_H__
