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

/** @file pe_cvi_reg_m14.h
 *
 * main driver implementation for de device.
 * de device will teach you how to make device driver with new platform.
 *
 * author	  seungjun.youm (sj.youm@lge.com)
 * version	  1.0
 * date		  2011.02.17
 * note		  Additional information.
 *
 * @addtogroup lg1152_de
 * @{
 */
#ifndef _PE_CVI_REG_M14_H_
#define _PE_CVI_REG_M14_H_
/*----------------------------------------------------------------------------------------
 *	 Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *	 File Inclusions
 *---------------------------------------------------------------------------------------*/
#include "pe_reg_mac.h"
#include "pe_cvi_reg_m14a0.h"
#include "pe_cvi_reg_m14b0.h"

/*----------------------------------------------------------------------------------------
 *	 Constant Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *	 Macro Definitions
 *---------------------------------------------------------------------------------------*/
// Ax
#define PE_CVA_M14A0_RdFL(_r)			PE_REG_PHYS_RdFL(gPE_CVA_M14A,data,_r)
#define PE_CVA_M14A0_WrFL(_r)			PE_REG_PHYS_WrFL(gPE_CVA_M14A,data,_r)
#define PE_CVA_M14A0_Rd(_r)				PE_REG_SHDW_Rd32(gPE_CVA_M14A,data,_r)
#define PE_CVA_M14A0_Wr(_r,_d1)			PE_REG_SHDW_Wr32(gPE_CVA_M14A,data,_r,_d1)
#define PE_CVA_M14A0_Rd01(_r,_f1,_d1)	PE_REG_SHDW_Rd01(gPE_CVA_M14A,data,_r,_f1,_d1)
#define PE_CVA_M14A0_Wr01(_r,_f1,_d1)	PE_REG_SHDW_Wr01(gPE_CVA_M14A,data,_r,_f1,_d1)

#define PE_CVB_M14A0_RdFL(_r)			PE_REG_PHYS_RdFL(gPE_CVB_M14A,data,_r)
#define PE_CVB_M14A0_WrFL(_r)			PE_REG_PHYS_WrFL(gPE_CVB_M14A,data,_r)
#define PE_CVB_M14A0_Rd(_r)				PE_REG_SHDW_Rd32(gPE_CVB_M14A,data,_r)
#define PE_CVB_M14A0_Wr(_r,_d1)			PE_REG_SHDW_Wr32(gPE_CVB_M14A,data,_r,_d1)
#define PE_CVB_M14A0_Rd01(_r,_f1,_d1)	PE_REG_SHDW_Rd01(gPE_CVB_M14A,data,_r,_f1,_d1)
#define PE_CVB_M14A0_Wr01(_r,_f1,_d1)	PE_REG_SHDW_Wr01(gPE_CVB_M14A,data,_r,_f1,_d1)

// Bx
#define PE_CVA_M14B0_RdFL(_r)			PE_REG_PHYS_RdFL(gPE_CVA_M14B,data,_r)
#define PE_CVA_M14B0_WrFL(_r)			PE_REG_PHYS_WrFL(gPE_CVA_M14B,data,_r)
#define PE_CVA_M14B0_Rd(_r)				PE_REG_SHDW_Rd32(gPE_CVA_M14B,data,_r)
#define PE_CVA_M14B0_Wr(_r,_d1)			PE_REG_SHDW_Wr32(gPE_CVA_M14B,data,_r,_d1)
#define PE_CVA_M14B0_Rd01(_r,_f1,_d1)	PE_REG_SHDW_Rd01(gPE_CVA_M14B,data,_r,_f1,_d1)
#define PE_CVA_M14B0_Wr01(_r,_f1,_d1)	PE_REG_SHDW_Wr01(gPE_CVA_M14B,data,_r,_f1,_d1)

#define PE_CVB_M14B0_RdFL(_r)			PE_REG_PHYS_RdFL(gPE_CVB_M14B,data,_r)
#define PE_CVB_M14B0_WrFL(_r)			PE_REG_PHYS_WrFL(gPE_CVB_M14B,data,_r)
#define PE_CVB_M14B0_Rd(_r)				PE_REG_SHDW_Rd32(gPE_CVB_M14B,data,_r)
#define PE_CVB_M14B0_Wr(_r,_d1)			PE_REG_SHDW_Wr32(gPE_CVB_M14B,data,_r,_d1)
#define PE_CVB_M14B0_Rd01(_r,_f1,_d1)	PE_REG_SHDW_Rd01(gPE_CVB_M14B,data,_r,_f1,_d1)
#define PE_CVB_M14B0_Wr01(_r,_f1,_d1)	PE_REG_SHDW_Wr01(gPE_CVB_M14B,data,_r,_f1,_d1)

/* m14a0 cvi */
#define PE_CVA_M14A0_QWr01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVA_M14A,data,_r);\
		PE_REG_SHDW_Wr01(gPE_CVA_M14A,data,_r,_f1,_d1);\
		PE_REG_PHYS_WrFL(gPE_CVA_M14A,data,_r);}
#define PE_CVA_M14A0_QRd01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVA_M14A,data,_r);\
		PE_REG_SHDW_Rd01(gPE_CVA_M14A,data,_r,_f1,_d1);}
#define PE_CVA_M14A0_QRd02(_r,_f1,_d1,_f2,_d2)	\
	{	PE_REG_PHYS_RdFL(gPE_CVA_M14A,data,_r);\
		PE_REG_SHDW_Rd02(gPE_CVA_M14A,data,_r,_f1,_d1,_f2,_d2);}
#define PE_CVB_M14A0_QWr01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVB_M14A,data,_r);\
		PE_REG_SHDW_Wr01(gPE_CVB_M14A,data,_r,_f1,_d1);\
		PE_REG_PHYS_WrFL(gPE_CVB_M14A,data,_r);}
#define PE_CVB_M14A0_QRd01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVB_M14A,data,_r);\
		PE_REG_SHDW_Rd01(gPE_CVB_M14A,data,_r,_f1,_d1);}
#define PE_CVB_M14A0_QRd02(_r,_f1,_d1,_f2,_d2)	\
	{	PE_REG_PHYS_RdFL(gPE_CVB_M14A,data,_r);\
		PE_REG_SHDW_Rd02(gPE_CVB_M14A,data,_r,_f1,_d1,_f2,_d2);}

/* m14b0 cvi */
#define PE_CVA_M14B0_QWr01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVA_M14B,data,_r);\
		PE_REG_SHDW_Wr01(gPE_CVA_M14B,data,_r,_f1,_d1);\
		PE_REG_PHYS_WrFL(gPE_CVA_M14B,data,_r);}
#define PE_CVA_M14B0_QRd01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVA_M14B,data,_r);\
		PE_REG_SHDW_Rd01(gPE_CVA_M14B,data,_r,_f1,_d1);}
#define PE_CVA_M14B0_QRd02(_r,_f1,_d1,_f2,_d2)	\
	{	PE_REG_PHYS_RdFL(gPE_CVA_M14B,data,_r);\
		PE_REG_SHDW_Rd02(gPE_CVA_M14B,data,_r,_f1,_d1,_f2,_d2);}
#define PE_CVB_M14B0_QWr01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVB_M14B,data,_r);\
		PE_REG_SHDW_Wr01(gPE_CVB_M14B,data,_r,_f1,_d1);\
		PE_REG_PHYS_WrFL(gPE_CVB_M14B,data,_r);}
#define PE_CVB_M14B0_QRd01(_r,_f1,_d1)	\
	{	PE_REG_PHYS_RdFL(gPE_CVB_M14B,data,_r);\
		PE_REG_SHDW_Rd01(gPE_CVB_M14B,data,_r,_f1,_d1);}
#define PE_CVB_M14B0_QRd02(_r,_f1,_d1,_f2,_d2)	\
	{	PE_REG_PHYS_RdFL(gPE_CVB_M14B,data,_r);\
		PE_REG_SHDW_Rd02(gPE_CVB_M14B,data,_r,_f1,_d1,_f2,_d2);}

/*----------------------------------------------------------------------------------------
 *	 Type Definitions
 *---------------------------------------------------------------------------------------*/
typedef struct {
	union {
		UINT32			*addr;
		PE_CVA_REG_M14A0_T *data;
	} shdw;

	union {
		volatile UINT32			 *addr;
		volatile PE_CVA_REG_M14A0_T *data;
	} phys;
} PE_CVA_REG_M14A_T;

typedef struct {
	union {
		UINT32			*addr;
		PE_CVB_REG_M14A0_T *data;
	} shdw;

	union {
		volatile UINT32			 *addr;
		volatile PE_CVB_REG_M14A0_T *data;
	} phys;
} PE_CVB_REG_M14A_T;

typedef struct {
	union {
		UINT32			*addr;
		PE_CVA_REG_M14B0_T *data;
	} shdw;

	union {
		volatile UINT32			 *addr;
		volatile PE_CVA_REG_M14B0_T *data;
	} phys;
} PE_CVA_REG_M14B_T;

typedef struct {
	union {
		UINT32			*addr;
		PE_CVB_REG_M14B0_T *data;
	} shdw;

	union {
		volatile UINT32			 *addr;
		volatile PE_CVB_REG_M14B0_T *data;
	} phys;
} PE_CVB_REG_M14B_T;

/*----------------------------------------------------------------------------------------
 *	 External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *	 External Variables
 *---------------------------------------------------------------------------------------*/

#endif	 /* _PE_CVI_REG_M14_H_ */

