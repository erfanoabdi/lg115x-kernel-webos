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


/** @file ovi_kapi.h
 *
 *  	application interface header for ovi device
 *
 *	@author	    	dj911.kim	
 *	@version		1.0
 *	@note		
 *	@date		2012.09.09
 *	@see		
 */

#ifndef	_OVI_KAPI_H_
#define	_OVI_KAPI_H_

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
#define	LX_OVI_IOC_MAGIC		'o'

/**
@name OVI IOCTL List
ioctl list for ovi device.

@{

@def LX_OVI_IOW_SET_LVDS
Set LVDS parameter

*/

#define LX_OVI_MAX_NUM_OF_MODE_INDEX_CHAR	50

/*----------------------------------------------------------------------------------------
	IO comand naming rule  : MODULE_IO[R][W]_COMMAND
----------------------------------------------------------------------------------------*/
#define LX_OVI_IOW_SET_COMBINATION				_IOW(LX_OVI_IOC_MAGIC, 0, UINT32)
#define LX_OVI_IOW_SET_SPREAD_SPECTRUM			_IOW(LX_OVI_IOC_MAGIC, 1, LX_OVI_SPREAD_SPECTRUM_T)
#define LX_OVI_IOW_SET_FRAMEODC_ENABLE			_IOW(LX_OVI_IOC_MAGIC, 2, BOOLEAN)
#define LX_OVI_IOW_SET_COLORTEMP_ENABLE			_IOW(LX_OVI_IOC_MAGIC, 3, BOOLEAN)
#define LX_OVI_IOW_SET_GAMMA_ENABLE				_IOW(LX_OVI_IOC_MAGIC, 4, BOOLEAN)
#define LX_OVI_IOW_SET_DITHER_ENABLE				_IOW(LX_OVI_IOC_MAGIC, 5, BOOLEAN)
#define LX_OVI_IOW_SET_LINEODC_MODE				_IOW(LX_OVI_IOC_MAGIC, 6, LX_OVI_TCON_LINEOD_MODE_T)
#define LX_OVI_IOW_SET_OD_LUT						_IOW(LX_OVI_IOC_MAGIC, 7, LX_OVI_TCON_OD_DWLD_T)
#define LX_OVI_IOW_SET_DITHER_MODE				_IOW(LX_OVI_IOC_MAGIC, 8, LX_OVI_TCON_DITHER_T)
#define LX_OVI_IOW_SET_COLOR_TEMP				_IOW(LX_OVI_IOC_MAGIC, 9, LX_OVI_TCON_COLOR_TEMP_T)
#define LX_OVI_IOW_SET_GAMMA						_IOW(LX_OVI_IOC_MAGIC, 10, LX_OVI_TCON_GAMMA_T)
#define LX_OVI_IORW_GET_TCON_OD_LUT				_IOWR(LX_OVI_IOC_MAGIC, 11, LX_OVI_TCON_OD_DWLD_T)
#define LX_OVI_IOW_SET_TCON_BYTE_OD_WRITE		_IOW(LX_OVI_IOC_MAGIC, 12, LX_OVI_TCON_OD_LUT_DEBUG_T)
#define LX_OVI_IOW_SET_TCON_REVERSE				_IOW(LX_OVI_IOC_MAGIC, 13, BOOLEAN)
#define LX_OVI_IOW_SET_LVDS_REVERSE				_IOW(LX_OVI_IOC_MAGIC, 14, BOOLEAN)
#define LX_OVI_IOW_SET_LVDS_BLACK					_IOW(LX_OVI_IOC_MAGIC, 15, BOOLEAN)
#define LX_OVI_IOW_SET_TCON_TPG					_IOW(LX_OVI_IOC_MAGIC, 16, LX_OVI_TCON_TPG_MODE_T)
#define LX_OVI_IOW_SET_CHANNEL_POWER			_IOW(LX_OVI_IOC_MAGIC, 17, BOOLEAN)
#define LX_OVI_IOW_SET_LVDS_FORMAT				_IOW(LX_OVI_IOC_MAGIC, 18, LX_OVI_LVDS_OUT_STD_T)
#define LX_OVI_IOW_SET_LVDS_BIT					_IOW(LX_OVI_IOC_MAGIC, 19, LX_OVI_BIT_DEPTH_T)
#define LX_OVI_IOR_GET_DATE_INFO					_IO(LX_OVI_IOC_MAGIC, 20)
#define LX_OVI_IOW_SET_TCON_CLOCK					_IOW(LX_OVI_IOC_MAGIC, 21, BOOLEAN)

#define LX_OVI_IOC_MAXNR				(_IOC_NR(LX_OVI_IOW_SET_TCON_CLOCK) +1 )

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    Type Definitions (Common)
----------------------------------------------------------------------------------------*/
	
typedef enum
{
	LX_OVI_8BIT,					///< 8bit depth
	LX_OVI_10BIT					///< 10bit depth
}
LX_OVI_BIT_DEPTH_T;

typedef enum {
	LX_OVI_H_720 = 720,
	LX_OVI_H_1920 = 1920,
	LX_OVI_H_3840 = 3840
}
LX_OVI_H_WIDTH_T;

typedef enum
{
	LX_OVI_LVDS_MODE_NORMAL,				///< Normal LVDS
	LX_OVI_LVDS_MODE_HS					///< HS(High Speed) LVDS
}
LX_OVI_LVDS_MODE_T;

typedef enum
{
	LX_OVI_LVDS_OUT_JEIDA,				///< JEIDA LVDS
	LX_OVI_LVDS_OUT_VESA					///< VESA LVDS
}
LX_OVI_LVDS_OUT_STD_T;

typedef enum {
	LX_OVI_EPI_6LANE,
	LX_OVI_EPI_12LANE,
	LX_OVI_EPI_16LANE
}
LX_OVI_EPI_LANE_T;

typedef enum {
	LX_OVI_SPREAD_RATIO_1,	// 0.75%
	LX_OVI_SPREAD_RATIO_2,	// 1.00%
	LX_OVI_SPREAD_RATIO_3,	// 1.25%
	LX_OVI_SPREAD_RATIO_4,	// 1.50%
	LX_OVI_SPREAD_RATIO_DEFAULT = LX_OVI_SPREAD_RATIO_3
}
LX_OVI_SPREAD_RATIO_T;

#define LX_OVI_SPREAD_SPECTRUM_DEFAULT_PERIOD	70

typedef enum {
	LX_OVI_TCON_FRAMEOD,
	LX_OVI_TCON_LINEOD,
}
LX_OVI_TCON_OD_MODE_T;

typedef enum {
	LX_OVI_TCON_LODC_V1_0,
	LX_OVI_TCON_LODC_V2_0
}
LX_OVI_TCON_LODC_VERSION_T;

typedef enum {
	LX_OVI_TCON_LODC_LEFT_REF,
	LX_OVI_TCON_LODC_RIGHT_REF
}
LX_OVI_TCON_LODC_ODDLINE_REF_T;

typedef enum {
	LX_OVI_TCON_LODC_NORMAL_REF,
	LX_OVI_TCON_LODC_REV_REF
}
LX_OVI_TCON_LODC_LASTSUBPIX_REF_T;

typedef enum {
	LX_OVI_TCON_DITHER_TRUNC,
	LX_OVI_TCON_DITHER_ROUND,
	LX_OVI_TCON_DITHER_RANDOM,
}
LX_OVI_TCON_DITHER_T;

typedef enum {
	LX_OVI_TCON_TPG_OFF,
	LX_OVI_TCON_TPG_BLACK,
	LX_OVI_TCON_TPG_WHITE,
	LX_OVI_TCON_TPG_HVBAR,
	LX_OVI_TCON_TPG_HOR16GREY,
	LX_OVI_TCON_TPG_HOR64GREY,
	LX_OVI_TCON_TPG_HOR256GREY,
	LX_OVI_TCON_TPG_VER16GREY
}
LX_OVI_TCON_TPG_MODE_T;

/* IOCTL parameter */
typedef struct
{
	UINT32 version;
} LX_OVI_CONFIG_VER_T;

typedef struct
{
	BOOLEAN 				bEnable;
	LX_OVI_SPREAD_RATIO_T 	eSpreadRatio;
	UINT16 					u16Period;		// default value : LX_OVI_SPREAD_SPECTRUM_DEFAULT_PERIOD
} LX_OVI_SPREAD_SPECTRUM_T;

typedef struct
{
	UINT32 sizeNum;
	UINT32 *pData;
} LX_OVI_TCON_GAMMA_LUT_T;

typedef struct
{
	UINT32 sizeNum;
	UINT8 *pData;
} LX_OVI_TCON_OD_LUT_T;

typedef struct
{
	LX_OVI_TCON_OD_LUT_T odLut;
	LX_OVI_TCON_OD_MODE_T eMode;
} LX_OVI_TCON_OD_DWLD_T;

typedef struct
{
	BOOLEAN bEnable;
	LX_OVI_TCON_LODC_VERSION_T eVersion;
	LX_OVI_TCON_LODC_ODDLINE_REF_T eRefPrevLine;
	LX_OVI_TCON_LODC_LASTSUBPIX_REF_T eRefLastSubPix;
} LX_OVI_TCON_LINEOD_MODE_T;

typedef struct
{
	UINT32 r_gain;						// red gain	0~255 (192= 1.0gain)
	UINT32 g_gain;						// green gain	0~255 (192= 1.0gain)
	UINT32 b_gain;						// blue gain	0~255 (192= 1.0gain)
	UINT32 r_offset;						// red offset	0~255 (128= zero offset)
	UINT32 g_offset;						// green offset	0~255 (128= zero offset)
	UINT32 b_offset;						// blue offset	0~255 (128= zero offset)
} LX_OVI_TCON_COLOR_TEMP_T;

typedef struct
{
	LX_OVI_TCON_GAMMA_LUT_T rGammaLut;
	LX_OVI_TCON_GAMMA_LUT_T gGammaLut;
	LX_OVI_TCON_GAMMA_LUT_T bGammaLut;
} LX_OVI_TCON_GAMMA_T;

typedef struct
{
	unsigned int addr;
	unsigned int value;
}	LX_OVI_REG_T;

typedef struct
{
	LX_OVI_TCON_OD_MODE_T mode;
	UINT32 index;
	UINT32 value;
} LX_OVI_TCON_OD_LUT_DEBUG_T;

typedef struct
{
	char index[LX_OVI_MAX_NUM_OF_MODE_INDEX_CHAR];
	LX_OVI_REG_T* regs;
	UINT32 count;
} LX_OVI_OUTPUT_MODE_T;

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _OVI_DRV_H_ */

/** @} */

