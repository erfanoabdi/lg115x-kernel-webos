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


#ifndef _SDEC_IO_h
#define _SDEC_IO_h

#ifdef __cplusplus
extern "C" {
#endif

#include "os_util.h"
#include "sdec_kapi.h"
#include "sdec_reg.h"

#ifdef USE_QEMU_SYSTEM
extern SDTOP_REG_T  pSDEC_TOP_Reg;
extern SDIO_REG_T pSDEC_IO_Reg;
extern MPG_REG_T pSDEC_MPG_Reg0;
extern MPG_REG_T pSDEC_MPG_Reg1;
#endif

#define CLEAR_ALL_MODE			0xFFFFFFFF
#define SDEC_BUFFER_BASE                         0x09A0C000

//log level
#if 0 // before '13 logm use
#define SDEC_DRV		0	/* 0x00000001 - kernel driver command trace */
#define SDEC_TRACE		1	/* 0x00000002 - io function in/out trace */
#define SDEC_NORMAL		2	/* 0x00000004 - generic print */
#define SDEC_READ		3	/* 0x00000008 - in SDEC_Read() */
#define SDEC_ISR		4	/* 0x00000010 - in ISR */
#define SDEC_IO			5	/* 0x00000020 - input / output port */
#define SDEC_PIDSEC		6	/* 0x00000040 - PID & Section handling */
#define SDEC_RESET		7	/* 0x00000080 - IO/MWC Reset Print */
#define SDEC_PCR		8	/* 0x00000100 - PCR Recover */
#define SDEC_ERROR		9	/* 0x00000200 - Error */
#define SDEC_CIA		10	/* 0x00000400 - Special Force for debug */
#define SDEC_PES		11	/* 0x00000800 - For PES Packet */
#define SDEC_SWPARSER	12	/* 0x00001000 - For CH_C SW Parser */
#define SDEC_DESC		13  /* 0x00002000 - Descrambler Key */
//#define SDEC_			13	/* 0x00002000 - Reserved */
//#define SDEC_			14	/* 0x00004000 - Reserved */
//#define SDEC_			15	/* 0x00008000 - Reserved */
#else
#define SDEC_ERROR		LX_LOGM_LEVEL_ERROR			/* 0 */
#define SDEC_WARNING	LX_LOGM_LEVEL_WARNING		/* 1 */
#define SDEC_NOTI		LX_LOGM_LEVEL_NOTI			/* 2 */
#define SDEC_INFO		LX_LOGM_LEVEL_INFO			/* 3 */
#define SDEC_DEBUG		LX_LOGM_LEVEL_DEBUG			/* 4 */
#define SDEC_TRACE		LX_LOGM_LEVEL_TRACE			/* 5 */
#define SDEC_DRV		(LX_LOGM_LEVEL_TRACE + 1)	/* 6 */
#define SDEC_NORMAL		(LX_LOGM_LEVEL_TRACE + 2)	/* 7 */
#define SDEC_READ		(LX_LOGM_LEVEL_TRACE + 3)	/* 8 */
#define SDEC_ISR		(LX_LOGM_LEVEL_TRACE + 4)	/* 9 */
#define SDEC_IO			(LX_LOGM_LEVEL_TRACE + 5)	/* 10 */
#define SDEC_PIDSEC		(LX_LOGM_LEVEL_TRACE + 6)	/* 11 */
#define SDEC_RESET		(LX_LOGM_LEVEL_TRACE + 7)	/* 12 */
#define SDEC_PCR		(LX_LOGM_LEVEL_TRACE + 8)	/* 13 */
#define SDEC_CIA		(LX_LOGM_LEVEL_TRACE + 9)	/* 14 */
#define SDEC_PES		(LX_LOGM_LEVEL_TRACE + 10)	/* 15 */
#define SDEC_SWPARSER	(LX_LOGM_LEVEL_TRACE + 11)	/* 16 */
#define SDEC_DESC		(LX_LOGM_LEVEL_TRACE + 12)	/* 17 */
#define SDEC_RFOUT		(LX_LOGM_LEVEL_TRACE + 13)	/* 18 */
#define SDEC_ISR_SEC_ERR	(LX_LOGM_LEVEL_TRACE + 14)	/* 19 */
//#define SDEC_			(LX_LOGM_LEVEL_TRACE + 15)	/* 20 */
#endif

#define SDEC_DTV_SOC_Message(_idx, format, args...) 	DBG_PRINT( g_sdec_debug_fd, 	_idx, 		format "\n", ##args)
#define SDEC_DEBUG_Print(format,args...)  				DBG_PRINT( g_sdec_debug_fd,		SDEC_ERROR,	format "\n", ##args)

#define DEC_EN				(1 << 31)
#define DL_EN				(1 << 30)
#define TSO_EN				(1 << 29)
#define PID(x)				(x << 16)
#define TPI_IEN			(1 << 15)
#define NO_DSC 				(1 << 14)
#define PAYLOAD_PES		(1 << 12)
#define SF_MAN_EN			(1 << 11)
#define GPB_IRQ_CONF		(1 << 10)
#define TS_DN				(1 << 9)
#define DEST				0x7F

#define AUD_DEV0	0x40
#define AUD_DEV1	0x41
#define VID_DEV0	0x44
#define VID_DEV1	0x45

#define DEST_RESERVED	0x4F

#define MAX_KEY_WORD_IDX	8

#define TVCT_PID	0x1FFB

/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/
// #define SDEC_IO_CH_NUM	4
#define SDEC_IO_CH_NUM	8

#define SDEC_IO_FLT_NUM	64
#define SDEC_CORE_CH_NUM	4

#define _SDEC_CORE_0	0
#define _SDEC_CORE_1	1

#define LX_SDEC_MAX_PESINFO_DATA		16

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/
#define MIN( a, b )  ( (a) < (b) ? (a) : (b) )
#define MAX( a, b )  ( (a) > (b) ? (a) : (b) )
#define ABS( a )     ( (a) < 0 ? -(a) : (a) )
#define LX_IS_ERR(err)  ((err) != (0) ? (1) : (0))		// conflict include/linux/err.h
#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x[0])) //array length
#define BYTEALIGN(x, y) ((x + (y-1))/ y * y) //byte align
#define IS_WRAP(x, y) ((x == y) ? (0) : (x))
//for bit perator
//one bit clear
#define clear__bit(data, loc)		((data) &= ~(0x1<<(loc)))
//serious many bit clear
#define clear_bits(data, area, loc)		((data) &= ~((area)<<(loc)))
//one bit set
#define set__bit(data, loc)				((data) |= (0x1<<(loc)))
//series many bit set
#define set_bits(data, area, loc)		((data) |= ((area)<<(loc)))
//one bit inversion
#define invert_bit(data, loc)			((data) ^= (0x1<<(loc)))
//series many bit inversion
#define inver_bits(data, area, loc)		((data) ^= ((area)<<(loc)))
//bit inspection
#define check_bit(data, loc)			((data) & (0x1<<(loc)))
#define check_one_bit(data, loc)			((0x1) & (data >> (loc)))
//bit extraction
#define extract_bits(data, area, loc)	((data) >> (loc)) & (area)

#define NORMAL_COLOR           "\033[0m"
#define GRAY_COLOR             "\033[1;30m"
#define RED_COLOR              "\033[1;31m"
#define GREEN_COLOR            "\033[1;32m"
#define MAGENTA_COLOR          "\033[1;35m"
#define YELLOW_COLOR           "\033[1;33m"
#define BLUE_COLOR             "\033[1;34m"
#define DBLUE_COLOR            "\033[0;34m"
#define WHITE_COLOR            "\033[1;37m"
#define COLORERR_COLOR         "\033[1;37;41m"
#define COLORWRN_COLOR         "\033[0;31m"
#define BROWN_COLOR            "\033[0;40;33m"
#define CYAN_COLOR             "\033[0;40;36m"
#define LIGHTGRAY_COLOR        "\033[1;40;37m"
#define BRIGHTRED_COLOR        "\033[0;40;31m"
#define BRIGHTBLUE_COLOR       "\033[0;40;34m"
#define BRIGHTMAGENTA_COLOR    "\033[0;40;35m"
#define BRIGHTCYAN_COLOR       "\033[0;40;36m"

#define REG_WRITE32( addr, value )	( *( volatile UINT32 * )( addr ) ) = ( volatile UINT32 )( value )
#define REG_READ32( addr )			( *( volatile UINT32 * )( addr ) )

#define LX_SDEC_CHECK_CODE(__checker,__if_action,fmt,args...)   \
						__CHECK_IF_ERROR(__checker, SDEC_DEBUG_Print, __if_action , fmt, ##args )

#define SDEC_GPB_LOCK(h, ch_idx, flt_idx)		spin_lock_irqsave(&h->stSdecGpbSpinlock[ch_idx][flt_idx], flags);
#define SDEC_GPB_UNLOCK(h, ch_idx, flt_idx)		spin_unlock_irqrestore(&h->stSdecGpbSpinlock[ch_idx][flt_idx], flags);

#define SDEC_CONVERT_CORE_CH(core, ch)  do{ \
											if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))	\
											{											\
												if(ch >= SDEC_CORE_CH_NUM)				\
												{										\
													core = 1;							\
													ch -= SDEC_CORE_CH_NUM;			\
												}										\
											}											\
										}while(0)

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------
   enum
----------------------------------------------------------------------------------------*/

typedef enum{
	OK = 0,
	NOT_OK = -1,
	INVALID_PARAMS = -2,
	NOT_ENOUGH_RESOURCE = -3,
	NOT_SUPPORTED = -4,
	NOT_PERMITTED = -5,
	TIMEOUT = -6,
	NO_DATA_RECEIVED = -7,
	DN_BUF_OVERFLOW = -8,
	INVALID_ARGS = -9,
}DTV_STATUS_T;

typedef enum{
	PCR = 0x00000001,
	TB_DCOUNT 				= 0x00000002,
	BDRC_0 					= 0x00000010,
	BDRC_1 					= 0x00000020,
	BDRC_2 					= 0x00000040,
	BDRC_3 					= 0x00000080,
	ERR_RPT				= 0x00001000,
	GPB_DATA_CHA_GPL 		= 0x00010000,
	GPB_DATA_CHA_GPH 		= 0x00020000,
	GPB_DATA_CHB_GPL 		= 0x00040000,
	GPB_DATA_CHB_GPH 		= 0x00080000,
	GPB_FULL_CHA_GPL 		= 0x00100000,
	GPB_FULL_CHA_GPH 		= 0x00200000,
	GPB_FULL_CHB_GPL 		= 0x00400000,
	GPB_FULL_CHB_GPH 		= 0x00800000,
	TP_INFO_CHA 			= 0x01000000,
	TP_INFO_CHB 			= 0x02000000,
	SEC_ERR_CHA 			= 0x04000000,
	SEC_ERR_CHB 			= 0x08000000,
}E_SDEC_INTR_T;


typedef enum{
	MPG_SD,
	MPG_CC,
	MPG_DUP,
	MPG_TS,
	MPG_PD,
	STCC_DCONT,
	CDIF_RPAGE,
	CDIF_WPAGE,
	CDIF_OVFLOW,
	SB_DROPPED,
	SYNC_LOST,
	TEST_DCONT,
}E_SDEC_ERRINTR_T;

typedef enum{
	 /*00*/E_SDEC_GPB_SIZE_4K,
	 /*01*/E_SDEC_GPB_SIZE_8K,
	 /*02*/E_SDEC_GPB_SIZE_16K,
	 /*03*/E_SDEC_GPB_SIZE_32K,
	 /*04*/E_SDEC_GPB_SIZE_64K,
	 /*05*/E_SDEC_GPB_SIZE_128K,
	 /*06*/E_SDEC_GPB_SIZE_256K,
	 /*07*/E_SDEC_GPB_SIZE_512K,
	 /*08*/E_SDEC_GPB_SIZE_1024K
 }E_SDEC_GPB_SIZE_T;

typedef enum{
	 /*00*/E_SDEC_CHA,
	 /*01*/E_SDEC_CHB,
	 /*03*/E_SDEC_CHA_CHB
 }E_SDEC_CH_T;

typedef union{
	struct{
		UINT16 SDEC_FLTSTATE_FREE			:1;
		UINT16 SDEC_FLTSTATE_ALLOC			:1;
		UINT16 SDEC_FLTSTATE_ENABLE			:1;
		UINT16 SDEC_FLTSTATE_SCRAMBLED		:1;
		UINT16 SDEC_FLTSTATE_DATAREADY		:1;
		UINT16 SDEC_FLTSTATE_SCRMREADY		:1;
		UINT16  							:6;
		UINT16 SDEC_FLTSTATE_OVERFLOW		:1;
		UINT16 								:3;
	}f;
		UINT16 w;
}S_STATUS_INFO_T;

typedef struct{

	UINT32  flag : 1, //enable or disable
			mode : 8,
			used : 1,
			reserved : 22;
	S_STATUS_INFO_T stStatusInfo;
}S_FILTER_MAP_T;

//for cdic set
typedef enum{
	/*00*/E_SDEC_CDIC_SRC_CDIN0,
	/*01*/E_SDEC_CDIC_SRC_CDIN1,
	/*02*/E_SDEC_CDIC_SRC_CDIN2,
	/*03*/E_SDEC_CDIC_SRC_CDIN3,
	/*04*/E_SDEC_CDIC_SRC_CDIN4,
	/*05*/E_SDEC_CDIC_SRC_CDIN5,
	/*06*/E_SDEC_CDIC_SRC_UPLOAD0,
	/*07*/E_SDEC_CDIC_SRC_UPLOAD1,
	/*08*/E_SDEC_CDIC_SRC_GPB,
	/*10*/E_SDEC_CDIC_SRC_CDINA = 10,	/* From L9B0. Parallel1 */
 }E_SDEC_CDIC_SRC_T;

//for CDIP set
typedef enum{
	 /*00*/E_SDEC_CDIP_0,
	 /*01*/E_SDEC_CDIP_1,
	 /*02*/E_SDEC_CDIP_2,
	 /*03*/E_SDEC_CDIP_3,
 }E_SDEC_CDIP_IDX_T;

typedef enum{
	 /*00*/E_SDEC_CDIOP_0,
	 /*01*/E_SDEC_CDIOP_1
 }E_SDEC_CDIOP_IDX_T;

typedef enum{
	 /*00*/E_SDEC_CDIOP_VAL_SEL_0,
	 /*01*/E_SDEC_CDIOP_VAL_SEL_1,
	 /*02*/E_SDEC_CDIOP_VAL_SEL_2,
	 /*03*/E_SDEC_CDIOP_VAL_SEL_3
 }E_SDEC_CDIOP_VAL_SEL_T;

typedef enum{
	 /*00*/E_SDEC_CDIOP_SERIAL_0,
	 /*01*/E_SDEC_CDIOP_SERIAL_1
 }E_SDEC_CDIOP_PCONF_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_DISABLE,
	 /*01*/E_SDEC_CDIP_ENABLE
 }E_SDEC_CDIP_EN_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_TEST_DISABLE,
	 /*01*/E_SDEC_CDIP_TEST_ENABLE
 }E_SDEC_CDIP_TEST_EN_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_ERR_ACT_LOW,
	 /*01*/E_SDEC_CDIP_ERR_ACT_HIGH
 }E_SDEC_CDIP_ERR_POL_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_CLK_ACT_HIGH,
	 /*01*/E_SDEC_CDIP_CLK_ACT_LOW
 }E_SDEC_CDIP_CLK_POL_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_VAL_ACT_HIGH,
	 /*01*/E_SDEC_CDIP_VAL_ACT_LOW
 }E_SDEC_CDIP_VAL_POL_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_REQ_ACT_HIGH,
	 /*01*/E_SDEC_CDIP_REQ_ACT_LOW
 }E_SDEC_CDIP_REQ_POL_T;

typedef enum {
	 /*00*/E_SDEC_CDIP_ERR_DISABLE,
	 /*01*/E_SDEC_CDIP_ERR_ENABLE
 }E_SDEC_CDIP_ERR_EN_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_VAL_DISABLE,
	 /*01*/E_SDEC_CDIP_VAL_ENABLE
 }E_SDEC_CDIP_VAL_EN_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_REQ_DISABLE,
	 /*01*/E_SDEC_CDIP_REQ_ENABLE
 }E_SDEC_CDIP_REQ_EN_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_SERIAL_0,
	 /*01*/E_SDEC_CDIP_SERIAL_1,
	 /*02*/E_SDEC_CDIP_PARALLEL_0,
	 /*03*/E_SDEC_CDIP_PARALLEL_1,
 }E_SDEC_CDIP_PCONF_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_BA_CLK_ENABLE,
	 /*01*/E_SDEC_CDIP_BA_CLK_DISABLE
 }E_SDEC_CDIP_BA_CLK_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_BA_VAL_ENABLE,
	 /*01*/E_SDEC_CDIP_BA_VAL_DISABLE
 }E_SDEC_CDIP_BA_VAL_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_BA_SOP_ENABLE,
	 /*01*/E_SDEC_CDIP_BA_SOP_DISABLE
 }E_SDEC_CDIP_BA_SOP_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_47DETECTION,
	 /*01*/E_SDEC_CDIP_NEG_SOP,
	 /*02*/E_SDEC_CDIP_POS_SOP,
	 /*03*/E_SDEC_CDIP_BOTH_SOP
 }E_SDEC_CDIP_SYNC_TYPE_T;

typedef enum{
	 /*00*/E_SDEC_CDIP_ATSC,
	 /*01*/E_SDEC_CDIP_OC,
	 /*02*/E_SDEC_CDIP_ARIB,
	 /*03*/E_SDEC_CDIP_DVB,
	 /*10*/E_SDEC_CDIP_PES = 10,
	 /*04*/E_SDEC_CDIP_MPES
 }E_SDEC_CDIP_DTYPE_T;
//end of CDIP set

//CDIP IN set

typedef struct{
	E_SDEC_CDIP_IDX_T eSdecCdipIdx;
	E_SDEC_CDIP_EN_T eSdecCdipEn;
	E_SDEC_CDIP_TEST_EN_T eSdecCdipTestEn;
	E_SDEC_CDIP_ERR_POL_T eSdecCdipErrPol;
	E_SDEC_CDIP_CLK_POL_T eSdecCdipClkPol;
	E_SDEC_CDIP_VAL_POL_T  eSdecCdipValPol;
	E_SDEC_CDIP_REQ_POL_T  eSdecCdipReqPol;
	E_SDEC_CDIP_ERR_EN_T eSdecCdipErrEn;
	E_SDEC_CDIP_VAL_EN_T eSdecCdipValEn;
	E_SDEC_CDIP_REQ_EN_T eSdecCdipReqEn;
	E_SDEC_CDIP_PCONF_T eSdecCdipPconf;
	E_SDEC_CDIP_BA_CLK_T eSdecCdipBaClk;
	UINT32 ui32ClkDiv;
	E_SDEC_CDIP_BA_VAL_T eSdecCdipBaVal;
	E_SDEC_CDIP_BA_SOP_T eSdecCdipBaSop;
	E_SDEC_CDIP_SYNC_TYPE_T eSdecCdipSyncType;
	E_SDEC_CDIP_DTYPE_T eSdecCdipDtype;
}S_SDEC_CDIP_CONF_SET_T;

typedef struct{
	E_SDEC_CDIP_IDX_T eSdecCdipIdx;
	E_SDEC_CDIP_EN_T eSdecCdipEn;
	E_SDEC_CDIP_TEST_EN_T eSdecCdipTestEn;
	E_SDEC_CDIP_ERR_POL_T eSdecCdipErrPol;
	E_SDEC_CDIP_CLK_POL_T eSdecCdipClkPol;
	E_SDEC_CDIP_VAL_POL_T  eSdecCdipValPol;
	E_SDEC_CDIP_REQ_POL_T  eSdecCdipReqPol;
	E_SDEC_CDIP_ERR_EN_T eSdecCdipErrEn;
	E_SDEC_CDIP_VAL_EN_T eSdecCdipValEn;
	E_SDEC_CDIP_REQ_EN_T eSdecCdipReqEn;
	E_SDEC_CDIOP_VAL_SEL_T eSdecCdiopValSel;
	E_SDEC_CDIP_PCONF_T eSdecCdipPconf;
	E_SDEC_CDIP_BA_CLK_T eSdecCdipBaClk;
	UINT32 ui32ClkDiv;
	E_SDEC_CDIP_BA_VAL_T eSdecCdipBaVal;
	E_SDEC_CDIP_BA_SOP_T eSdecCdipBaSop;
	E_SDEC_CDIP_SYNC_TYPE_T eSdecCdipSyncType;
	E_SDEC_CDIP_DTYPE_T eSdecCdipDtype;
}S_SDEC_CDIP_CONF_IN_SET_T;

typedef struct{
	UINT32 ui32Baseptr;
	UINT32 ui32Endptr;
	UINT32 ui32Readptr;
	UINT32 ui32UsrReadptr;
	GPB_BND stGpbBnd;
	UINT8 ui8PidFltIdx;
}S_SDEC_MEMORY_INFO_T;

#if 0
typedef struct{
	UINT8 ui8Channel;
	UINT16 ui16StatusInfo;
	UINT8 ui8index;
}S_GPB_DATA_T;
#endif

typedef struct{
	UINT32 	ui8Channel : 8,
			ui16StatusInfo : 16,
		 	ui8index : 8;
	UINT32 	ui32ReadPtr;
	UINT32 	ui32WritePtr;
}S_GPB_INFO_T;


typedef union{
	struct{
		UINT32 SDEC_OPEN			:1;
		UINT32 SDEC_CLOSE			:1;
		UINT32 						:30;
	}f;
		UINT32 w;
}S_SDEC_STATUS_T;

typedef struct {
	UINT8 	ui8Channel;
	UINT32 	STC_hi_value;
	UINT32 	STC_lo_value;
	UINT32 	PCR_hi_value;
	UINT32 	PCR_lo_value;
}S_PCR_STC_T;

typedef struct {
	UINT8 ui8Channel;
	UINT8 ui8PIDIdx;
}S_TPI_INTR_T;

typedef enum{
	/* SD IO */
	E_SDEC_REGBACKUP_IO_CDIP0 = 0,		/* 0x00 */
	E_SDEC_REGBACKUP_IO_CDIP1,			/* 0x04 */
	E_SDEC_REGBACKUP_IO_CDIP2, 			/* 0x08 */
	E_SDEC_REGBACKUP_IO_CDIP3, 			/* 0x0C */
	E_SDEC_REGBACKUP_IO_CDIOP0, 			/* 0x10 */
	E_SDEC_REGBACKUP_IO_CDIOP1, 			/* 0x14 */
	E_SDEC_REGBACKUP_IO_CDIC_DSC0,		/* 0x28 */
	E_SDEC_REGBACKUP_IO_CDIC_DSC1, 		/* 0x2C */
	E_SDEC_REGBACKUP_IO_INTR_EN,			/* 0x100 */
	E_SDEC_REGBACKUP_IO_ERR_INTR_EN,	/* 0x110 */
	E_SDEC_REGBACKUP_IO_GPB_BASE,		/* 0x120 */
	E_SDEC_REGBACKUP_IO_CDIC2,			/* 0x12C */

	E_SDEC_REGBACKUP_IO_NUM,
}E_SDEC_REG_IO_BACKUP_T;

typedef enum{
	/* SD CORE */
	E_SDEC_REGBACKUP_CORE_EXT_CONF,			/* 0x10 */
	E_SDEC_REGBACKUP_CORE_TPI_ICONF, 		/* 0x40 */
	E_SDEC_REGBACKUP_CORE_KMEM_ADDR, 		/* 0x98 */
	E_SDEC_REGBACKUP_CORE_KMEM_DATA,		/* 0x9C */
	E_SDEC_REGBACKUP_CORE_SECF_EN_L, 		/* 0xB0 */
	E_SDEC_REGBACKUP_CORE_SECF_EN_H,		/* 0xB4 */
	E_SDEC_REGBACKUP_CORE_SECF_MTYPE_L,		/* 0xB8 */
	E_SDEC_REGBACKUP_CORE_SECF_MTYPE_H,		/* 0xBC */
	E_SDEC_REGBACKUP_CORE_SECFB_VALID_L, 	/* 0xC0 */
	E_SDEC_REGBACKUP_CORE_SECFB_VALID_H, 	/* 0xC4 */

	E_SDEC_REGBACKUP_CORE_NUM,
}E_SDEC_REG_CORE_BACKUP_T;

typedef struct {
	BOOLEAN	bInit;
	UINT32 	regIO[E_SDEC_REGBACKUP_IO_NUM];
	UINT32 	regCORE[E_SDEC_REGBACKUP_CORE_NUM];
}S_REG_BACKUP_LIST_T;

typedef enum{
	E_SDEC_RESET_MODE_ENABLE = 0,			/* enable reset */
	E_SDEC_RESET_MODE_DISABLE,				/* disable reset */
	E_SDEC_RESET_MODE_ONETIME,				/* 1 time reset mode */
}E_SDEC_RESET_MODE_T;

typedef enum{
	E_SDEC_DN_OUT_FROM_CORE_0 = 0,			/* Download of CORE0 */
	E_SDEC_DN_OUT_FROM_SENC,				/* Download of SENC */
	E_SDEC_DN_OUT_FROM_CORE_1,				/* Download of CORE1 */
}E_SDEC_TOP_DN_SEL_T;


/*----------------------------------------------------------------------------------------
   structre
----------------------------------------------------------------------------------------*/

typedef struct
{
	//releated OS
	OS_SEM_T		stSdecMutex;
	spinlock_t 		stSdecNotiSpinlock;
	spinlock_t 		stSdecGpbSpinlock[SDEC_IO_CH_NUM][SDEC_IO_FLT_NUM];
	spinlock_t 		stSdecPesSpinlock;
	spinlock_t 		stSdecSecSpinlock;
	spinlock_t 		stSdecResetSpinlock;
	spinlock_t		stSdecPidfSpinlock;

	// Event for SW Parser
	OS_EVENT_T		stSdecSWPEvent;
	OS_EVENT_T		stSdecRFOUTEvent; /* LIVE_HEVC */

	wait_queue_head_t wq;
	int				  wq_condition;

	//work queue
	struct workqueue_struct *workqueue;
	struct work_struct Notify;
	struct work_struct PcrRecovery;
	struct work_struct PesProc;
	struct work_struct SecProc;
	struct work_struct TPIIntr;

	S_GPB_INFO_T	stPesGPBInfo[LX_SDEC_MAX_PESINFO_DATA];
	UINT8 			ui8PesGpbInfoWIdx;
	UINT8 			ui8PesGpbInfoRIdx;

	S_PCR_STC_T stPCR_STC;
	S_TPI_INTR_T stTPI_Intr;

	// 2011.11.04 gaius.lee - moved to HAL
//	volatile  SDTOP_REG_T *stSDEC_TOP_Reg;
//	volatile  SDIO_REG_T *stSDEC_IO_Reg;
//	volatile  MPG_REG_T *stSDEC_MPG_Reg[2];

	S_FILTER_MAP_T stPIDMap[SDEC_IO_CH_NUM][SDEC_IO_FLT_NUM];
	S_FILTER_MAP_T stSecMap[SDEC_IO_CH_NUM][SDEC_IO_FLT_NUM];

	S_SDEC_MEMORY_INFO_T stSdecMeminfo[SDEC_IO_CH_NUM][SDEC_IO_FLT_NUM];

//	S_GPB_DATA_T stGPBData[MAX_GPB_DATA];
	LX_SDEC_NOTIFY_PARAM_T stGPBInfo[LX_SDEC_MAX_GPB_DATA];

	S_SDEC_STATUS_T stSdecStatus;

	UINT8 ui8CurrentCh;
	UINT8 ui8CurrentMode;

	UINT32 ui32MsgMask;

	UINT8 bPcrRecoveryFlag[SDEC_IO_CH_NUM];

	UINT8 ui8GpbInfoWIdx;
	UINT8 ui8GpbInfoRIdx;

	LX_SDEC_INPUT_T eInputPath[SDEC_IO_CH_NUM];

	UINT8 ui8CDICResetMode;
	UINT8 ui8SDMWCResetMode;
	UINT8 ui8CDICResetNum;
	UINT8 ui8SDMWCResetNum;

	/* for JP MCU Descrambler Control Mode in H13 */
	UINT8 ui8McuDescramblerCtrlMode;

	/* for using original external demod clock in serial input, ex> Columbia */
	UINT8 ui8UseOrgExtDemodClk;

	/* for UHD bypass channel */
	UINT8 ui8RFBypassChannel;

	/* for UHD SDT channel in H13 */
	UINT8 ui8SDTChannel;

	/* for TVCT Problem */
	UINT8 ui8ClearTVCTGathering;
}S_SDEC_PARAM_T;


/*----------------------------------------------------------------------------------------
   API
----------------------------------------------------------------------------------------*/
DTV_STATUS_T SDEC_RegInit(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_MutexInitialize(S_SDEC_PARAM_T *stpSdecParam);
//DTV_STATUS_T SDEC_SpinLockInitialize(S_SDEC_PARAM_T *stpSdecParam);
//DTV_STATUS_T SDEC_SpinLockDestroy(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_WorkQueueInitialize(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_WorkQueueInitialize(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_WorkQueueDestroy(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_WaitQueueInitialize(S_SDEC_PARAM_T *stpSdecParam);

//Init
DTV_STATUS_T SDEC_IO_InitialaizeModule(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CAP_T*	stpLXSdecCap);
DTV_STATUS_T SDEC_IO_GetChipCfg(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CHIP_CFG_T *stpLXSdecGetChipCfg);
DTV_STATUS_T SDEC_IO_DebugCommand(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_DBG_CMD_T *stpLXSdecDbgCmd);
DTV_STATUS_T SDEC_IO_GetCurrentSTCPCR(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_STC_PCR_T *stpLXSdecGetSTCPCR);
DTV_STATUS_T SDEC_GetCurrentSTCPCR(unsigned long *pcr, unsigned long *stc);
DTV_STATUS_T SDEC_IO_GetCurrentGSTC(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_GSTC_T *stpLXSdecGetGSTC);
DTV_STATUS_T SDEC_IO_GetCurrentGSTC32(S_SDEC_PARAM_T *stpSdecParam,	LX_SDEC_GET_GSTC32_T *stpLXSdecGetGSTC);
DTV_STATUS_T SDEC_IO_SetCurrentGSTC(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SET_GSTC_T *stpLXSdecSetGSTC);
DTV_STATUS_T SDEC_IO_SelInputPort(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SEL_INPUT_T *stpLXSdecSelPort);
DTV_STATUS_T SDEC_IO_CfgInputPort(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CFG_INPUT_PARAM_T *stpLXSdecCfgPortParam);
DTV_STATUS_T SDEC_IO_SelParInput(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SEL_PAR_INPUT_T *stpLXSdecParInput);
DTV_STATUS_T SDEC_IO_SelCiInput(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SEL_CI_INPUT_T *stpLXSdecParInput);
DTV_STATUS_T SDEC_IO_GetParInput(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_PAR_INPUT_T *stpLXSdecParInput);
DTV_STATUS_T SDEC_IO_GetCiInput(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_CI_INPUT_T *stpLXSdecCiInput);
DTV_STATUS_T SDEC_IO_SetCipherEnable(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CIPHER_ENABLE_T *stLXSdecCipherEnable);
DTV_STATUS_T SDEC_IO_SetCipherMode(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CIPHER_MODE_T *stpLXSdecCipherMode);
DTV_STATUS_T SDEC_IO_SetCipherKey(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CIPHERKEY_T *stpLXSdecCipherKey);
DTV_STATUS_T SDEC_IO_GetCipherKey(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_CIPHERKEY_T *stpLXSdecCipherKey);
DTV_STATUS_T SDEC_IO_SetPCRPID(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_SET_PCRPID_T *stpLXSdecPIDFltSetPID);
DTV_STATUS_T SDEC_IO_SetPcrRecovery(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SET_PCR_RECOVERY_T *stpLXSdecSetPCRRecovery);
DTV_STATUS_T SDEC_IO_GetInputPort(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_INPUT_T *stpLXSdecGetInputPort);
DTV_STATUS_T SDEC_IO_SetVidOutport(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SET_VDEC_PORT_T *stpLXSdecSetVidOutort);
DTV_STATUS_T SDEC_IO_InputPortEnable(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_ENABLE_INPUT_T *stpLXSdecEnableInput);
DTV_STATUS_T SDEC_IO_SelectPVRSource(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_DL_SEL_T *stpLXSdecDlSel);

/// PID filter
DTV_STATUS_T SDEC_IO_PIDFilterAlloc(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_ALLOC_T *stpLXSdecPIDFltAlloc);
DTV_STATUS_T SDEC_IO_PIDFilterFree(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_FREE_T *stpLXSdecPIDFltFree);
DTV_STATUS_T SDEC_IO_PIDFilterSetPID(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_SET_PID_T *stpLXSdecPIDFltSetPID);
DTV_STATUS_T SDEC_IO_PIDFilterMapSelect(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_SELSECFLT_T *stpLXSdecPIDFltSelect);
DTV_STATUS_T SDEC_IO_PIDFilterEnable(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltEnable);
DTV_STATUS_T SDEC_IO_PIDFilterCRCEnable(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltPESCRCEnable);
DTV_STATUS_T SDEC_IO_PIDFilterGetState(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_STATE_T *stpLXSdecPIDFltState);
DTV_STATUS_T SDEC_IO_PIDFilterEnableSCMBCHK(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltEnable);
DTV_STATUS_T SDEC_IO_PIDFilterEnableDownload(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_PIDFLT_ENABLE_T *stpLXSdecPIDFltEnableDownload);

/// Section filter
DTV_STATUS_T SDEC_IO_SectionFilterAlloc(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_ALLOC_T *stpLXSdecSecFltAlloc);
DTV_STATUS_T SDEC_IO_SectionFilterFree(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_FREE_T *stpLXSecPIDFltFree);
DTV_STATUS_T SDEC_IO_PESFilterFree(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_FREE_T *stpLXSecPIDFltFree);
DTV_STATUS_T SDEC_IO_SectionFilterPattern(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_PATTERN_T *stpLXSecFltPattern);
DTV_STATUS_T SDEC_IO_SectionFilterBufferReset(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_BUFFER_RESET *stpLXSdecSecfltBufferReset);
DTV_STATUS_T SDEC_IO_SectionFilterBufferSet(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_BUFFER_SET_T *stpLXSdecSecfltBufferSet);
DTV_STATUS_T SDEC_IO_SectionFilterDummyBufferSet(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_BUFFER_SET_T *stpLXSdecSecfltBufferSet);
DTV_STATUS_T SDEC_IO_SectionFilterGetInfo(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_BUFFER_GET_INFO_T *stpLXSdecSecfltBufferGetInfo);
DTV_STATUS_T SDEC_IO_SectionFilterSetReadPtr(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_READPTR_SET_T *stpLXSdecSecfltReadPtrSet);
DTV_STATUS_T SDEC_IO_SectionFilterGetState(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_STATE_T *stpLXSdecSecfltState);
DTV_STATUS_T SDEC_IO_SectionFilterGetAvailableNumber(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_GET_AVAIL_NUMBER_T *stpLXSdecSecFltAvailNum);
DTV_STATUS_T SDEC_IO_EnableLog(S_SDEC_PARAM_T *stpSdecParam, UINT32 *pulArg);
DTV_STATUS_T SDEC_IO_GetRegister(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_REG_T *stpLXSdecReadRegisters);
DTV_STATUS_T SDEC_IO_SetRegister(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_REG_T *stpLXSdecWriteRegisters);
DTV_STATUS_T SDEC_IO_SetMCUDescramblerCtrlMode(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SET_MCUMODE_T *stpLXSdecSetMcumode);
DTV_STATUS_T SDEC_IO_SetTVCTMode(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SET_TVCTMODE_T *stpLXSdecClearTVCTGathering);
DTV_STATUS_T SDEC_IO_UseOrgExtDemodCLKinSerialInput(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SET_SERIALCLKMODE_T *stpLXSdecSetSerialClkMode);
DTV_STATUS_T SDEC_IO_GetGPBBaseOffset(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GPB_BASE_OFFSET_T *stpLXSdecGPBBaseOffset);

//SDEC inner API
DTV_STATUS_T SDEC_TPI_Set(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch);
DTV_STATUS_T SDEC_ERR_Intr_Set(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch);
DTV_STATUS_T SDEC_Pidf_Clear(S_SDEC_PARAM_T *stpSdecParam,  UINT8 ui8ch,UINT32 ui8PidfIdx);
DTV_STATUS_T SDEC_Secf_Clear(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8ch, UINT32 ui8PidfIdx);
DTV_STATUS_T SDEC_Gpb_Init(void);
DTV_STATUS_T SDEC_GpbSet(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8ch,UINT32 ui32GpbSize, UINT32 ui32GpbBaseAddr, UINT32 ui32GpbIdx);
DTV_STATUS_T SDEC_ParamInit(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_PIDIdxCheck(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8ch,UINT8 *ui8pPidIdx, LX_SDEC_PIDFLT_MODE_T ePidFltMode, UINT16 ui16PidValue);
DTV_STATUS_T SDEC_SecIdxCheck(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8ch,UINT8 *ui8pSecIdx);
DTV_STATUS_T SDEC_InputSet(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SEL_INPUT_T *stpLXSdecSelPort);
DTV_STATUS_T SDEC_DisablePIDF(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8PidFltIdx, UINT8 ui8SecFltIdx, UINT8 ui8Ch);
DTV_STATUS_T SDEC_CdipConfSet(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8CdipIdx,  LX_SDEC_STREAMMODE_T eSdecStreamMode);
DTV_STATUS_T SDEC_CdiopConfInSet(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8CdipIdx, LX_SDEC_STREAMMODE_T eSdecStreamMode);
DTV_STATUS_T SDEC_Copy2SharedMem(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_IntrEnable(S_SDEC_PARAM_T *stpSdecParam, E_SDEC_INTR_T eSdecIntrSrc);
DTV_STATUS_T SDEC_IntrDisable(S_SDEC_PARAM_T *stpSdecParam, E_SDEC_INTR_T eSdecIntrSrc);
DTV_STATUS_T SDEC_Intialize(S_SDEC_PARAM_T *stpSdecParam);
DTV_STATUS_T SDEC_InputPortEnable(UINT8 core, LX_SDEC_INPUT_T eInputPath, UINT8 en);
DTV_STATUS_T SDEC_InputPortReset(UINT8 ui8Ch);
DTV_STATUS_T SDEC_SDMWCReset(UINT8 ui8Ch);
DTV_STATUS_T SDEC_TEAH_Reset(UINT8 core);
DTV_STATUS_T SDEC_IO_SetFCW(S_SDEC_PARAM_T *stpSdecParam, UINT32 ulArg);
DTV_STATUS_T SDEC_IO_SetSTCMultiply(S_SDEC_PARAM_T *stpSdecParam, UINT32 *pui32Arg);
void SDEC_Notify(struct work_struct *work);
void SDEC_PCRRecovery(struct work_struct *work);
void SDEC_TPIIntr(struct work_struct *work);
UINT32 SDEC_IO_SettingFCW(S_SDEC_PARAM_T *stpSdecParam, UINT32 new_fcw_value);
void SDEC_PWM_Init(S_SDEC_PARAM_T	*stpSdecParam );
BOOLEAN SDEC_SetNotifyParam(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_NOTIFY_PARAM_T notiParam);
DTV_STATUS_T SDEC_InputPortReset(UINT8 ui8Ch);
void SDEC_CheckNotifyOvf(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_NOTIFY_PARAM_T *pNotiParam);
void SDEC_DeleteInNotify(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch, UINT8 ui32SecFltId);

int SDEC_SetPidfData(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch,	UINT8 ui8PIDIdx, UINT32 val);
UINT32 SDEC_GetPidfData(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch, UINT8 ui8PIDIdx);
int SDEC_SetPidf_Enable(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch, UINT8 ui8PIDIdx, UINT8 en);
int SDEC_SetPidf_TPI_IEN_Enable(S_SDEC_PARAM_T *stpSdecParam,	UINT8 ui8Ch, UINT8 ui8PIDIdx, UINT8 en);
int SDEC_SetPidf_Disc_Enable(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch,	UINT8 ui8PIDIdx, UINT8 en);
int SDEC_SetPidf_DN_Enable(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8Ch, UINT8 ui8PIDIdx, UINT8 en);

// STCC Related
DTV_STATUS_T SDEC_IO_GetSTCCASGStatus(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_STCCASG_T *stpLXSdecGetSTCCASG);
DTV_STATUS_T SDEC_IO_SetSTCCASGStatus(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_GET_STCCASG_T *stpLXSdecSetSTCCASG);
DTV_STATUS_T SDEC_IO_GetSTCCStatus(S_SDEC_PARAM_T *stpSdecParam,	LX_SDEC_GET_STCC_STATUS_T *stpLXSdecGetSTCCStatus);


//void  SDEC_DTV_SOC_Message(S_SDEC_PARAM_T *stpSdecParam, UINT32 level, const UINT8 *fmt, ...);

#define __NEW_PWM_RESET_COND__	1
#ifdef __NEW_PWM_RESET_COND__
void SDEC_IO_SetPWMResetCondition(S_SDEC_PARAM_T *stpSdecParam, UINT8 ch, BOOLEAN reset);
#endif

#if 1 /* TS Out Stub */
DTV_STATUS_T SDEC_IO_SerialTSOUT_SetSrc(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SERIALTSOUT_SET_SOURCE_T *stpLXSdecSTSOutSetSrc);
DTV_STATUS_T SDEC_IO_BDRC_SetData(S_SDEC_PARAM_T *stpSdecParam,	LX_SDEC_BDRC_T *stpLXSdecBDRCSetData);
DTV_STATUS_T SDEC_IO_BDRC_Enable(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_BDRC_ENABLE_T *stpLXSdecBDRCEnable);
DTV_STATUS_T SDEC_IO_SerialTSOUT_SetBufELevel(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SERIALTSOUT_SET_BUFELEV_T *stpLXSdecSTSOutSetBufELev);
DTV_STATUS_T SDEC_IO_CfgOutputPort(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CFG_OUTPUT_T *stpLXSdecCfgPort);
DTV_STATUS_T SDEC_IO_SectionFilterSetWritePtr(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_WRITEPTR_SET_T *stpLXSdecSecfltWritePtrSet);
DTV_STATUS_T SDEC_IO_BDRC_SetWritePtrUpdated(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_BDRC_WPTRUPD_T *stpLXSdecBDRCSetWptrUpd);
DTV_STATUS_T SDEC_IO_SectionFilterGetReadPtr(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_GET_READPTR_T *stpLXSdecSecfltReadPtrGet);
DTV_STATUS_T SDEC_IO_SectionFilterGetWritePtr(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_SECFLT_GET_WRITEPTR_T *stpLXSdecSecfltWritePtrGet);
DTV_STATUS_T SDEC_IO_CfgIOPort(S_SDEC_PARAM_T *stpSdecParam, LX_SDEC_CFG_IO_T *stpLXSdecCfgIOPort);
#endif /* End of TS Out Stub */


DTV_STATUS_T SDEC_Pidf_AllClear(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8ch);
DTV_STATUS_T SDEC_Secf_AllClear(S_SDEC_PARAM_T *stpSdecParam, UINT8 ui8ch);


/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif

