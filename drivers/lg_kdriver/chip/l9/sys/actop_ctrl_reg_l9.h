/*
 * SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2013 by LG Electronics Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.

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
 * author     ks.hyun (ks.hyun@lge.com)
 * version    1.0
 * date       2011.04.13
 * note       Additional information.
 *
 * @addtogroup lg1152_sys
 * @{
 */
#ifndef  _ACTRL_REG_L9_H_
#define  _ACTRL_REG_L9_H_
/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include "l9/afe/vport_i2c_l9a0.h"
#include "l9/afe/vport_i2c_l9b0.h"


/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/

#define ACTRL_RdFL(_r)		do{	VPORT_I2C_Read((UINT32*)&_r);	} while(0)
#define ACTRL_WrFL(_r)		do{	VPORT_I2C_Write((UINT32*)&_r);	} while(0)


#define ACTRL_Rd(_r)		((AI2C_DATA_T*)(&_r))->data
#define ACTRL_Wr(_r,_v)		(((AI2C_DATA_T*)(&_r))->data = _v)


#define ACTRL_Rd01(_r,_f01,_v01)											\
								do {											\
									(_v01) = _r._f01;							\
								} while(0)

#define ACTRL_Rd02(_r,_f01,_v01,_f02,_v02)									\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
								} while(0)

#define ACTRL_Rd03(_r,_f01,_v01,_f02,_v02,_f03,_v03)						\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
								} while(0)

#define ACTRL_Rd04(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04)				\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
								} while(0)

#define ACTRL_Rd05(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05)													\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
								} while(0)

#define ACTRL_Rd06(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06)										\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
								} while(0)

#define ACTRL_Rd07(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07)								\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
								} while(0)

#define ACTRL_Rd08(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08)					\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
								} while(0)

#define ACTRL_Rd09(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09)													\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
								} while(0)

#define ACTRL_Rd10(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10)										\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
								} while(0)

#define ACTRL_Rd11(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11)								\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
									(_v11) = (_r._f11);				\
								} while(0)

#define ACTRL_Rd12(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12)					\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
									(_v11) = (_r._f11);				\
									(_v12) = (_r._f12);				\
								} while(0)

#define ACTRL_Rd13(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13)													\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
									(_v11) = (_r._f11);				\
									(_v12) = (_r._f12);				\
									(_v13) = (_r._f13);				\
								} while(0)

#define ACTRL_Rd14(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13,_f14,_v14)										\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
									(_v11) = (_r._f11);				\
									(_v12) = (_r._f12);				\
									(_v13) = (_r._f13);				\
									(_v14) = (_r._f14);				\
								} while(0)

#define ACTRL_Rd15(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13,_f14,_v14,_f15,_v15)								\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
									(_v11) = (_r._f11);				\
									(_v12) = (_r._f12);				\
									(_v13) = (_r._f13);				\
									(_v14) = (_r._f14);				\
									(_v15) = (_r._f15);				\
								} while(0)

#define ACTRL_Rd16(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13,_f14,_v14,_f15,_v15,_f16,_v16)					\
								do {											\
									(_v01) = (_r._f01);				\
									(_v02) = (_r._f02);				\
									(_v03) = (_r._f03);				\
									(_v04) = (_r._f04);				\
									(_v05) = (_r._f05);				\
									(_v06) = (_r._f06);				\
									(_v07) = (_r._f07);				\
									(_v08) = (_r._f08);				\
									(_v09) = (_r._f09);				\
									(_v10) = (_r._f10);				\
									(_v11) = (_r._f11);				\
									(_v12) = (_r._f12);				\
									(_v13) = (_r._f13);				\
									(_v14) = (_r._f14);				\
									(_v15) = (_r._f15);				\
									(_v16) = (_r._f16);				\
								} while(0)


#define ACTRL_Wr01(_r,_f01,_v01)								\
								do {								\
									(_r._f01) = (_v01);				\
								} while(0)

#define ACTRL_Wr02(_r,_f01,_v01,_f02,_v02)						\
								do {								\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
								} while(0)

#define ACTRL_Wr03(_r,_f01,_v01,_f02,_v02,_f03,_v03)						\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
								} while(0)

#define ACTRL_Wr04(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04)				\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
								} while(0)

#define ACTRL_Wr05(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05)													\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
								} while(0)

#define ACTRL_Wr06(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06)										\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
								} while(0)

#define ACTRL_Wr07(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07)								\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
								} while(0)

#define ACTRL_Wr08(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08)					\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
								} while(0)

#define ACTRL_Wr09(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09)													\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
								} while(0)

#define ACTRL_Wr10(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10)										\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
								} while(0)

#define ACTRL_Wr11(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11)								\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
									(_r._f11) = (_v11);				\
								} while(0)

#define ACTRL_Wr12(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12)					\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
									(_r._f11) = (_v11);				\
									(_r._f12) = (_v12);				\
								} while(0)

#define ACTRL_Wr13(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13)													\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
									(_r._f11) = (_v11);				\
									(_r._f12) = (_v12);				\
									(_r._f13) = (_v13);				\
								} while(0)

#define ACTRL_Wr14(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13,_f14,_v14)										\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
									(_r._f11) = (_v11);				\
									(_r._f12) = (_v12);				\
									(_r._f13) = (_v13);				\
									(_r._f14) = (_v14);				\
								} while(0)

#define ACTRL_Wr15(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13,_f14,_v14,_f15,_v15)								\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
									(_r._f11) = (_v11);				\
									(_r._f12) = (_v12);				\
									(_r._f13) = (_v13);				\
									(_r._f14) = (_v14);				\
									(_r._f15) = (_v15);				\
								} while(0)

#define ACTRL_Wr16(_r,_f01,_v01,_f02,_v02,_f03,_v03,_f04,_v04,				\
					_f05,_v05,_f06,_v06,_f07,_v07,_f08,_v08,					\
					_f09,_v09,_f10,_v10,_f11,_v11,_f12,_v12,					\
					_f13,_v13,_f14,_v14,_f15,_v15,_f16,_v16)					\
								do {											\
									(_r._f01) = (_v01);				\
									(_r._f02) = (_v02);				\
									(_r._f03) = (_v03);				\
									(_r._f04) = (_v04);				\
									(_r._f05) = (_v05);				\
									(_r._f06) = (_v06);				\
									(_r._f07) = (_v07);				\
									(_r._f08) = (_v08);				\
									(_r._f09) = (_v09);				\
									(_r._f10) = (_v10);				\
									(_r._f11) = (_v11);				\
									(_r._f12) = (_v12);				\
									(_r._f13) = (_v13);				\
									(_r._f14) = (_v14);				\
									(_r._f15) = (_v15);				\
									(_r._f16) = (_v16);				\
								} while(0)




/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/
typedef struct
{
	UINT32
	data:8,
	rsvd8:8,
	slaveAddr:8,
	regAddr:8;
} AI2C_DATA_T;


static inline UINT32 ACTRL_READ(UINT32 slave, UINT32 offset)
{
	AI2C_DATA_T r;

	r.slaveAddr = (UINT8)slave;
	r.regAddr = (UINT8)offset;
	VPORT_I2C_Read((UINT32*)&r);

	return r.data;
}

static inline void ACTRL_WRITE(UINT32 slave, UINT32 offset, UINT32 value)
{
	AI2C_DATA_T r;

	r.slaveAddr = (UINT8)slave;
	r.regAddr = (UINT8)offset;
	r.data = (UINT8)value;
	VPORT_I2C_Write((UINT32*)&r);
}

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/


#endif   /* ----- #ifndef _ACTRL_REG_L9_H_  ----- */
/**  @} */
