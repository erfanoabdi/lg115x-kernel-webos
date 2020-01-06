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
 
/** @file pe_kapi.h
 *
 *  application interface header for picture enhance modules.
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2011.06.11
 *	@see		
 */

#ifndef	_PE_KAPI_H_
#define	_PE_KAPI_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/
#ifndef USE_XTENSA
#include "base_types.h"
#endif

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */


/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define	PE_IOC_MAGIC		'a'
#define PE_IOC_MAXNR		100

#define LX_PE_CMG_TBLPOINT			8
#define LX_PE_CMG_REGION_MAX		15
#define LX_PE_CMG_REGION_NUM		(LX_PE_CMG_REGION_MAX+1)
#define LX_PE_CMG_DELTANUM			6
#define LX_PE_CMG_DELTA_SETNUM		2

#define PE_NUM_OF_CSC_COEF	9
#define PE_NUM_OF_CSC_OFST	6

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define PE_IOWR_PKT					_IOWR(PE_IOC_MAGIC, 0, LX_PE_PKT_T)

#define PE_ITEM_PKTRW(_str)			PE_PKTRW_##_str
#define PE_ITEM_PKTMODL(_str)		PE_PKTMODL_##_str
#define PE_ITEM_PKTFUNC(_str)		PE_PKTFUNC_##_str

#define PE_PKTINFO_RWTYPE_POS		24
#define PE_PKTINFO_RWTYPE_BITS		0x000000ff
#define PE_PKTINFO_MODLTYPE_POS		16
#define PE_PKTINFO_MODLTYPE_BITS	0x000000ff
#define PE_PKTINFO_FUNCTYPE_POS		0
#define PE_PKTINFO_FUNCTYPE_BITS	0x0000ffff

#define PE_PKTINFO_RWTYPE_MAXNUM	PE_PKTINFO_RWTYPE_BITS
#define PE_PKTINFO_MODLTYPE_MAXNUM	PE_PKTINFO_MODLTYPE_BITS
#define PE_PKTINFO_FUNCTYPE_MAXNUM	PE_PKTINFO_FUNCTYPE_BITS

#define PE_LSHIFT_DATA(_val, _bits, _pos)	(((_val)&(_bits))<<(_pos))
#define PE_DATA_MASK(_bits, _pos)			((_bits)<<(_pos))
#define PE_RSHIFT_DATA(_val, _bits, _pos)	(((_val)&PE_DATA_MASK((_bits),(_pos)))>>(_pos))

#define PE_SET_PKTINFO_RWTYPE(_info,_type)	\
	_info = (( (_info) & ~(PE_DATA_MASK(PE_PKTINFO_RWTYPE_BITS,PE_PKTINFO_RWTYPE_POS)) ) \
			| (PE_LSHIFT_DATA(_type,PE_PKTINFO_RWTYPE_BITS,PE_PKTINFO_RWTYPE_POS)))
#define PE_GET_PKTINFO_RWTYPE(_info)	PE_RSHIFT_DATA(_info, PE_PKTINFO_RWTYPE_BITS, PE_PKTINFO_RWTYPE_POS)

#define PE_SET_PKTINFO_MODLTYPE(_info,_type)	\
	_info = (( (_info) & ~(PE_DATA_MASK(PE_PKTINFO_MODLTYPE_BITS,PE_PKTINFO_MODLTYPE_POS)) ) \
			| (PE_LSHIFT_DATA(_type,PE_PKTINFO_MODLTYPE_BITS,PE_PKTINFO_MODLTYPE_POS)))
#define PE_GET_PKTINFO_MODLTYPE(_info)	PE_RSHIFT_DATA(_info, PE_PKTINFO_MODLTYPE_BITS, PE_PKTINFO_MODLTYPE_POS)

#define PE_SET_PKTINFO_FUNCTYPE(_info,_type)	\
	_info = (( (_info) & ~(PE_DATA_MASK(PE_PKTINFO_FUNCTYPE_BITS,PE_PKTINFO_FUNCTYPE_POS)) ) \
			| (PE_LSHIFT_DATA(_type,PE_PKTINFO_FUNCTYPE_BITS,PE_PKTINFO_FUNCTYPE_POS)))
#define PE_GET_PKTINFO_FUNCTYPE(_info)	PE_RSHIFT_DATA(_info, PE_PKTINFO_FUNCTYPE_BITS, PE_PKTINFO_FUNCTYPE_POS)

/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

/**
 *	pe window index enumeration
 */
typedef enum
{
	LX_PE_WIN_0 = 0,				///< window 0
	LX_PE_WIN_1,					///< window 1

	LX_PE_WIN_ALL,					///< all window
	LX_PE_WIN_NUM = LX_PE_WIN_ALL,	///< number of windows
}
LX_PE_WIN_ID;

/**
 *	pe debug type enumeration
 */
typedef enum
{
	LX_PE_DBG_NONE		= 0x0,	///< set nothing, reserved
	LX_PE_DBG_LV		= 0x1,	///< debug print level on, off
	LX_PE_DBG_BY		= 0x2,	///< bypass mode on, off
	LX_PE_DBG_FW		= 0x4,	///< fw ctrl on, off
	LX_PE_DBG_ALL		= LX_PE_DBG_LV|LX_PE_DBG_BY|LX_PE_DBG_FW	///< set all
}
LX_PE_DBG_TYPE;

/**
 *	pe level type enumeration
 */
typedef enum
{
	LX_PE_LEVEL_MOTION		= 0x1,	///< select only motion level
	LX_PE_LEVEL_NOISE		= 0x2,	///< select only noise level
	LX_PE_LEVEL_PEAKPOS		= 0x4,	///< select only peak pos level
	LX_PE_LEVEL_ALL			= 0x7	///< set all
}
LX_PE_LEVEL_TYPE;

/**
 *	pe source type enumeration
 */
typedef enum
{
	LX_PE_SRC_DTV = 0,		///< digital
	LX_PE_SRC_ATV,			///< atv
	LX_PE_SRC_CVBS,			///< cvbs, s-video
	LX_PE_SRC_SCART,		///< scart
	LX_PE_SRC_COMP,			///< component
	LX_PE_SRC_RGBPC,		///< rgb-pc
	LX_PE_SRC_HDMI,			///< hdmi
	LX_PE_SRC_NUM			///< max num
}
LX_PE_SRC_TYPE;

/**
 *	pe format type enumeration
 */
typedef enum
{
	LX_PE_FMT_SD = 0,		///< sd  input, hor size ~1023
	LX_PE_FMT_HD,			///< hd  input, hor size 1024~2000
	LX_PE_FMT_UHD,			///< uhd input, hor size 2001 ~
	LX_PE_FMT_NUM			///< max num
}
LX_PE_FMT_TYPE;

/**
 *	pe color standard type enumeration
 */
typedef enum
{
	LX_PE_CSTD_NTSC,		///< ntsc
	LX_PE_CSTD_PAL,			///< pal
	LX_PE_CSTD_SECAM,		///< secam
	LX_PE_CSTD_NUM			///< max num
}
LX_PE_CSTD_TYPE;

/**
 *	pe hdmi type enumeration
 */
typedef enum
{
	LX_PE_HDMI_TV = 0,		///< hdmi tv
	LX_PE_HDMI_PC,			///< hdmi pc
	LX_PE_HDMI_NUM			///< max num
}
LX_PE_HDMI_TYPE;

/**
 *	pe scart type enumeration
 */
typedef enum
{
	LX_PE_SCART_AV = 0,		///< scart av
	LX_PE_SCART_RGB,		///< scart rgb
	LX_PE_SCART_NUM			///< max num
}
LX_PE_SCART_TYPE;

/**
 *	pe frame rate type enumeration
 */
typedef enum
{
	LX_PE_FR_60HZ = 0,		///< frame rate 60hz,30hz,24hz
	LX_PE_FR_50HZ,			///< frame rate 50hz,25hz
	LX_PE_FR_NUM			///< max num
}
LX_PE_FR_TYPE;

/**
 *	pe output type enumeration
 */
typedef enum
{
	LX_PE_OUT_2D = 0,		///< single normal
	LX_PE_OUT_3D_2D,		///< 3d to 2d
	LX_PE_OUT_2D_3D,		///< 2d to 3d
	LX_PE_OUT_UD,			///< ud
	LX_PE_OUT_TB,			///< top and bottom
	LX_PE_OUT_SS,			///< side by side
	LX_PE_OUT_FS,			///< frame sequential
	LX_PE_OUT_DUAL_TB,		///< dual screen Top n Bottom
	LX_PE_OUT_DUAL_SS,		///< dual screen Side by Side
	LX_PE_OUT_DUAL_FULL,	///< dual screen Full
	LX_PE_OUT_PIP,			///< pip screen
	LX_PE_OUT_NUM			///< max num
}
LX_PE_OUT_TYPE;

/**
 *	pe scan type enumeration
 */
typedef enum
{
	LX_PE_SCAN_INTERLACE = 0,	///< scan type interlace
	LX_PE_SCAN_PROGRESS,		///< scan type progress
	LX_PE_SCAN_NUM				///< max num
}
LX_PE_SCAN_TYPE;

/**
 *	pe dtv play type enumeration
 */
typedef enum
{
	LX_PE_DTV_NORMAL = 0,	///< dtv type normal
	LX_PE_DTV_FILEPLAY,		///< dtv type fileplay
	LX_PE_DTV_HDDPLAY,		///< dtv type hddplay
	LX_PE_DTV_PHOTOPLAY,	///< dtv type photoplay
	LX_PE_DTV_TESTPIC,		///< dtv type test picture
	LX_PE_DTV_CAMERA,		///< dtv type camera
	LX_PE_DTV_INVALID,		///< dtv type invalid, not dtv
	LX_PE_DTV_NUM			///< max num
}
LX_PE_DTV_TYPE;

/**
 *	pe hdd src type enumeration
 */
typedef enum
{
	LX_PE_HDD_SRC_DTV = 0,		///< hdd src dtv
	LX_PE_HDD_SRC_ATV60,		///< hdd src atv 60
	LX_PE_HDD_SRC_ATV50,		///< hdd src atv 50
	LX_PE_HDD_SRC_AV60,			///< hdd src av 60
	LX_PE_HDD_SRC_AV50,			///< hdd src av 50
	LX_PE_HDD_SRC_SCARTRGB,		///< hdd src scart rgb
	LX_PE_HDD_SRC_INVALID,		///< hdd src invalid, not hdd
	LX_PE_HDD_SRC_NUM			///< max num
}
LX_PE_HDD_SRC_TYPE;

/**
 * pe 3D formatter image format parameter.
 * see LX_3D_IMG_FMT_IN_T
 */
typedef enum
{
	LX_PE_3D_IN_TB = 0,		///< top and bottom
	LX_PE_3D_IN_SS,			///< side by side
	LX_PE_3D_IN_QC,			///< quincunx
	LX_PE_3D_IN_CB,			///< check board
	LX_PE_3D_IN_FP,			///< frame packing
	LX_PE_3D_IN_FPI,		///< frame packing interlace
	LX_PE_3D_IN_FA,			///< field alternate
	LX_PE_3D_IN_FS,			///< frame sequence
	LX_PE_3D_IN_LA,			///< line alternate
	LX_PE_3D_IN_SSF,		///< side by side full
	LX_PE_3D_IN_DUAL,		///< dual HD
	LX_PE_3D_IN_CA,			///< column alternate
	LX_PE_3D_IN_LAH,		///< line alternate half
	LX_PE_3D_IN_NUM			///< max number
}
LX_PE_3D_IN_TYPE;

/**
 *	pe dynamic contrast enhancement color domain enumeration
 */
typedef enum
{
	LX_PE_YC_DOMAIN = 0,	///< yc domain
	LX_PE_HSV_DOMAIN,		///< hsv domain
	LX_PE_KTD_DOMAIN,		///< ktd domain
	LX_PE_DOMAIN_NUM		///< max num
}
LX_PE_COLOR_DOMAIN;

/**
 *	packet type descripter for read, write, init
 */
typedef enum
{
	PE_ITEM_PKTRW(INIT)	= 0,	///< init type
	PE_ITEM_PKTRW(SET)	= 1,	///< set type
	PE_ITEM_PKTRW(GET)	= 2,	///< get type
	PE_ITEM_PKTRW(NUM),			///< rw type number
	PE_ITEM_PKTRW(MAX)	= PE_PKTINFO_RWTYPE_MAXNUM	///< max
}
LX_PE_PKT_RWTYPE;

/**
 *	packet type descripter for pe modules
 */
typedef enum
{
	PE_ITEM_PKTMODL(INIT)		= 0,	///< using init module
	PE_ITEM_PKTMODL(DEFAULT)	= 1,	///< set default settings on each module
	PE_ITEM_PKTMODL(DBG)		= 2,	///< set debug settings on each module
	PE_ITEM_PKTMODL(CMN)		= 3,	///< using common module
	PE_ITEM_PKTMODL(CSC)		= 4,	///< using csc module
	PE_ITEM_PKTMODL(CMG)		= 5,	///< using color manege module
	PE_ITEM_PKTMODL(NRD)		= 6,	///< using noise reduction module
	PE_ITEM_PKTMODL(DNT)		= 7,	///< using deinterlacer module
	PE_ITEM_PKTMODL(SHP)		= 8,	///< using sharpness module
	PE_ITEM_PKTMODL(CCM)		= 9,	///< using color correction module
	PE_ITEM_PKTMODL(DCM)		= 10,	///< using dynamic contrast module
	PE_ITEM_PKTMODL(WIN)		= 11,	///< using window control module
	PE_ITEM_PKTMODL(ETC)		= 12,	///< using etc(misc) control module
	PE_ITEM_PKTMODL(HST)		= 13,	///< using histogram module
	PE_ITEM_PKTMODL(NUM),				///< module type number
	PE_ITEM_PKTMODL(MAX)		= PE_PKTINFO_MODLTYPE_MAXNUM	///< max
}
LX_PE_PKT_MODLTYPE;

/**
 *	packet type descripter for functions
 */
typedef enum
{
	PE_ITEM_PKTFUNC(LX_PE_INIT_SETTINS_T)		= 10000,	///< using struct LX_PE_INIT_SETTINS_T
	PE_ITEM_PKTFUNC(LX_PE_DEFAULT_SETTINGS_T)	= 10001,	///< using struct LX_PE_DEFAULT_SETTINGS_T
	PE_ITEM_PKTFUNC(LX_PE_DBG_SETTINGS_T)		= 10002,	///< using struct LX_PE_DBG_SETTINGS_T

	PE_ITEM_PKTFUNC(LX_PE_CMN_CONTRAST_T)		= 11000,	///< using struct LX_PE_CMN_CONTRAST_T
	PE_ITEM_PKTFUNC(LX_PE_CMN_BRIGHTNESS_T)		= 11001,	///< using struct LX_PE_CMN_BRIGHTNESS_T
	PE_ITEM_PKTFUNC(LX_PE_CMN_SATURATION_T)		= 11002,	///< using struct LX_PE_CMN_SATURATION_T
	PE_ITEM_PKTFUNC(LX_PE_CMN_HUE_T)			= 11003,	///< using struct LX_PE_CMN_HUE_T
	PE_ITEM_PKTFUNC(LX_PE_CMN_LEVEL_CTRL_T)		= 11004,	///< using struct LX_PE_CMN_LEVEL_CTRL_T

	PE_ITEM_PKTFUNC(LX_PE_CSC_XVYCC_T)			= 12000,	///< using struct LX_PE_CSC_XVYCC_T
	PE_ITEM_PKTFUNC(LX_PE_CSC_GAMUT_T)			= 12001,	///< using struct LX_PE_CSC_GAMUT_T
	PE_ITEM_PKTFUNC(LX_PE_CSC_POST_T)			= 12002,	///< using struct LX_PE_CSC_POST_T
	PE_ITEM_PKTFUNC(LX_PE_CSC_INPUT_T)			= 12003,	///< using struct LX_PE_CSC_INPUT_T

	PE_ITEM_PKTFUNC(LX_PE_CMG_ENABLE_T)			= 13000,	///< using struct LX_PE_CMG_ENABLE_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_REGION_ENABLE_T)	= 13001,	///< using struct LX_PE_CMG_REGION_ENABLE_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_REGION_T)			= 13002,	///< using struct LX_PE_CMG_REGION_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_REGION_CTRL_T)	= 13003,	///< using struct LX_PE_CMG_REGION_CTRL_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_GLOBAL_CTRL_T)	= 13004,	///< using struct LX_PE_CMG_GLOBAL_CTRL_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_COLOR_CTRL_T)		= 13005,	///< using struct LX_PE_CMG_COLOR_CTRL_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_CW_CTRL_T)		= 13006,	///< using struct LX_PE_CCM_CW_T
	PE_ITEM_PKTFUNC(LX_PE_CMG_CW_GAIN_CTRL_T)	= 13007,	///< using struct LX_PE_CMG_CW_GAIN_CTRL_T

	PE_ITEM_PKTFUNC(LX_PE_NRD_DNR0_CMN_T)		= 14001,	///< using struct LX_PE_NRD_DNR0_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_TNR1_CMN_T)		= 14002,	///< using struct LX_PE_NRD_TNR1_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_DNR2_CMN_T)		= 14003,	///< using struct LX_PE_NRD_DNR2_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_DNR2_DETAIL_T)	= 14004,	///< using struct LX_PE_NRD_DNR2_DETAIL_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_TNR2_CMN_T)		= 14005,	///< using struct LX_PE_NRD_TNR2_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_TNR2_DETAIL_T)	= 14006,	///< using struct LX_PE_NRD_TNR2_DETAIL_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_DNR3_CMN_T)		= 14007,	///< using struct LX_PE_NRD_DNR3_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_TNR3_CMN_T)		= 14008,	///< using struct LX_PE_NRD_TNR3_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_DNR4_CMN_T)		= 14009,	///< using struct LX_PE_NRD_DNR4_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_DNR4_DETAIL_T)	= 14010,	///< using struct LX_PE_NRD_DNR4_DETAIL_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_TNR4_CMN_T)		= 14011,	///< using struct LX_PE_NRD_TNR4_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_NRD_TNR4_DETAIL_T)	= 14012,	///< using struct LX_PE_NRD_TNR4_DETAIL_T

	PE_ITEM_PKTFUNC(LX_PE_DNT_FILMMODE_T)		= 15000,	///< using struct LX_PE_DNT_FILMMODE_T
	PE_ITEM_PKTFUNC(LX_PE_DNT_LD_MODE_T)		= 15001,	///< using struct LX_PE_DNT_LD_MODE_T

	PE_ITEM_PKTFUNC(LX_PE_SHP_SCLFILTER_T)		= 16000,	///< using struct LX_PE_SHP_SCLFILTER_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE1_CMN_T)		= 16001,	///< using struct LX_PE_SHP_RE1_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE1_HOR_T)		= 16002,	///< using struct LX_PE_SHP_RE1_HOR_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE1_VER_T)		= 16003,	///< using struct LX_PE_SHP_RE1_VER_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE1_MISC_T)		= 16004,	///< using struct LX_PE_SHP_RE1_MISC_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_EE1_T)			= 16005,	///< using struct LX_PE_SHP_EE1_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_DE1_T)			= 16006,	///< using struct LX_PE_SHP_DE1_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE2_CMN_T)		= 16007,	///< using struct LX_PE_SHP_RE2_CMN_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE2_HOR_T)		= 16008,	///< using struct LX_PE_SHP_RE2_HOR_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE2_VER_T)		= 16009,	///< using struct LX_PE_SHP_RE2_VER_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE2_MISC_T)		= 16010,	///< using struct LX_PE_SHP_RE2_MISC_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_RE3_MISC_T)		= 16011,	///< using struct LX_PE_SHP_RE3_MISC_T
	PE_ITEM_PKTFUNC(LX_PE_SHP_SR0_T)			= 16012,	///< using struct LX_PE_SHP_SR0_T

	PE_ITEM_PKTFUNC(LX_PE_CCM_GAMMALUT_T)		= 17000,	///< using struct LX_PE_CCM_GAMMALUT_T
	PE_ITEM_PKTFUNC(LX_PE_CCM_PIXEL_REPLACE_T)	= 17001,	///< using struct LX_PE_CCM_PIXEL_REPLACE_T
	PE_ITEM_PKTFUNC(LX_PE_CCM_WB_T)				= 17002,	///< using struct LX_PE_CCM_WB_T
	PE_ITEM_PKTFUNC(LX_PE_CCM_AUTO_CR_T)		= 17003,	///< using struct LX_PE_CCM_AUTO_CR_T

	PE_ITEM_PKTFUNC(LX_PE_DCM_DCE_CONF_T)		= 18000,	///< using struct LX_PE_DCM_DCE_CONF_T
	PE_ITEM_PKTFUNC(LX_PE_DCM_DCE_LUT_T)		= 18001,	///< using struct LX_PE_DCM_DCE_LUT_T
	PE_ITEM_PKTFUNC(LX_PE_DCM_DSE_LUT_T)		= 18002,	///< using struct LX_PE_DCM_DSE_LUT_T
	PE_ITEM_PKTFUNC(LX_PE_DCM_BLENDING_T)		= 18003,	///< using struct LX_PE_DCM_BLENDING_T
	PE_ITEM_PKTFUNC(LX_PE_DCM_DCE_SMOOTH0_T)	= 18004,	///< using struct LX_PE_DCM_DCE_SMOOTH0_T
	PE_ITEM_PKTFUNC(LX_PE_DCM_DCE_SMOOTH1_T)	= 18005,	///< using struct LX_PE_DCM_DCE_SMOOTH1_T

	PE_ITEM_PKTFUNC(LX_PE_ETC_TBL_T)			= 20000,	///< using struct LX_PE_ETC_TBL_T
	PE_ITEM_PKTFUNC(LX_PE_INF_DISPLAY_T)		= 20001,	///< using struct LX_PE_INF_DISPLAY_T
	PE_ITEM_PKTFUNC(LX_PE_INF_LEVEL_T)			= 20002,	///< using struct LX_PE_INF_LEVEL_T

	PE_ITEM_PKTFUNC(LX_PE_HST_HISTO_INFO_T)		= 21000,	///< using struct LX_PE_HST_HISTO_INFO_T
	PE_ITEM_PKTFUNC(LX_PE_HST_HISTO_CFG_T)		= 21001,	///< using struct LX_PE_HST_HISTO_CFG_T

	PE_ITEM_PKTFUNC(MAX)						= PE_PKTINFO_FUNCTYPE_MAXNUM	///< max
}
LX_PE_PKT_FUNCTYPE;

/**
 *	pe packet
 */
typedef struct
{
	UINT32 info;	///< [31:24]LX_PE_PKT_RWTYPE | [23:16]LX_PE_PKT_MODLTYPE | [15:0]LX_PE_PKT_FUNCTYPE
	UINT32 size;	///< size of data
	void *data;		///< packet data
}
LX_PE_PKT_T;

/**
 *	pe firmware control parameter type
 */
typedef struct
{
	UINT32 ctrl_en;		//< fw control enable or not
	UINT32 dbg_en;		///< dbg print enable or not
}
LX_PE_DBG_FWI_CTRL_T;

/**
 *	pe debug settings control parameter type (only for debug)
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	LX_PE_DBG_TYPE type;		///< debug type
	UINT32 print_lvl;			///< pe debug print level
	UINT32 bypass;				///< bypass each module
	LX_PE_DBG_FWI_CTRL_T fwc;	///< fw ctrl
}
LX_PE_DBG_SETTINGS_T;

/**
 *	pe default settings control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID	win_id;		///< window id
}
LX_PE_DEFAULT_SETTINGS_T;

/**
 *	pe initial settings control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 suspend_mode;		///< suspend mode, 0:off(default), 1:on
}
LX_PE_INIT_SETTINS_T;

/***************************************************************************/
/* CMN : Common */
/***************************************************************************/
/**
 *	pe contrast control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT16 contrast;		///< contrast value
}
LX_PE_CMN_CONTRAST_T;

/**
 *	pe brightness control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
    UINT16 brightness;		///< brightness value
}
LX_PE_CMN_BRIGHTNESS_T;

/**
 *	pe saturation control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
    UINT16 saturation;		///< saturation value
}
LX_PE_CMN_SATURATION_T;

/**
 *	pe hue control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
    UINT16 hue;				///< hue value
}
LX_PE_CMN_HUE_T;

/**
 *	pe level control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
    UINT16 offset;			///< display input y offset, 0~512~1023
    UINT16 center;			///< center position for y gain control, 0~128~255
    UINT16 gain;			///< display input y gain, 0~128~255
	UINT16 tp_on;			///< test pattern on,off
}
LX_PE_CMN_LEVEL_CTRL_T;

/***************************************************************************/
/* CCM : Color Correctin Module */
/***************************************************************************/
/**
 *	pe gamma lut control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT32 size;			///< size of lut_data
	UINT32 *data;			///< [29:20]R | [19:10]B | [9:0]G
}
LX_PE_CCM_GAMMALUT_T;

/**
 *	pe gamma pixel replacement control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 pxl_rep_r;		///< R value for pixel replacement, 1:on, 0:off, default:0
	UINT8 pxl_rep_g;		///< G value for pixel replacement, 1:on, 0:off, default:0
	UINT8 pxl_rep_b;		///< B value for pixel replacement, 1:on, 0:off, default:0
}
LX_PE_CCM_PIXEL_REPLACE_T;

/**
 *	pe white balance control parameter type
 */
typedef struct {
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 r_gain;			///< red gain, 		0~255(192= 1.0 gain)
	UINT8 g_gain;			///< green gain, 	0~255(192= 1.0 gain)
	UINT8 b_gain;			///< blue gain, 	0~255(192= 1.0 gain)
	UINT8 r_offset; 		///< red offset,	0~255(128= zero offset)
	UINT8 g_offset; 		///< green offset,	0~255(128= zero offset)
	UINT8 b_offset; 		///< blue offset,	0~255(128= zero offset)
}
LX_PE_CCM_WB_T;

/**
 *	pe left right mismatch correction control parameter type
 */
typedef struct {
	UINT8 enable;			///< enable LRCR, 1:on, 0:off
	UINT8 th_max_hist;		///< if(max_hist > th_max_hist), auto cr does not apply
	UINT8 th_valid_bins;	///< if(n_valid_bins > th_valid_bins), auto cr does not apply
	UINT8 adj_th0;			///< adjustment threshold 0
	UINT8 adj_th1;			///< adjustment threshold 1
}
LX_PE_CCM_AUTO_CR_T;

/***************************************************************************/
/* DCM : Dynamic Contrast Module */
/***************************************************************************/
/**
 *	pe dynamic contrast enhancement configuration parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;			///< window id
	LX_PE_COLOR_DOMAIN domain;		///< color domain.
    UINT32 min_pos;					///< the min position, default "26"(0~255(0%~100%))
	UINT32 max_pos;					///< the max position, default "220"(0~255(0%~100%))
}
LX_PE_DCM_DCE_CONF_T;

/**
 *	pe dynamic contrast lut parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 size;				///< lut step, size of data, sync with the lut step on dce config
	UINT32 *data;				///< [9:0] y data, [25:16] x data
}
LX_PE_DCM_DCE_LUT_T;

/**
 *	pe dynamic saturation lut parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 size;				///< lut step, size of data, sync with the lut step on dce config
	UINT32 *data;				///< [9:0] y data, [25:16] x data
}
LX_PE_DCM_DSE_LUT_T;


/**
 *	pe dynamic contrast bypass blending control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 color_out_gain;		///< dce out gain, 1(0%)~128(200%)~255(400%) (forbidden 0)
	UINT32 region_sel;			///< LSB : select region 0
	UINT32 color_region_en;		///< color region apply for dce y
	UINT32 y_grad_gain;			///< Gradient of Y  signal,0x0:128to0, 0x1:64to0, 0x2:32to0, 0x3:16to0
	UINT32 cb_grad_gain;		///< Gradient of Cb signal,0x0:128to0, 0x1:64to0, 0x2:32to0, 0x3:16to0
	UINT32 cr_grad_gain;		///< Gradient of Cr signal,0x0:128to0, 0x1:64to0, 0x2:32to0, 0x3:16to0
	UINT32 y_range_min;			///< y_range_min : 0~1023
	UINT32 y_range_max;			///< y_range_max : 0~1023
	UINT32 cb_range_min;		///< cb_range_min : 0~1023
	UINT32 cb_range_max;		///< cb_range_max : 0~1023
	UINT32 cr_range_min;		///< cr_range_min : 0~1023
	UINT32 cr_range_max;		///< cr_range_max : 0~1023
}
LX_PE_DCM_BLENDING_T;

/**
 *	pe dynamic blur control parameter type
 *	- use for dce on hsv domain
 *	ver.0, for H13Bx,M14Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 avg_filter_tap;		///< luma blur for DCE(0:3x3,1:5x5,2:7x7)
	UINT32 edge_map_tap;		///< edge-map tap size for blending(0:7x5,1:5x5,2:3x3)
	UINT32 e_to_w_th_x0;		///< edge to weight : x0(0~255)
	UINT32 e_to_w_th_x1;		///< edge to weight : x1(0~255)
	UINT32 e_to_w_th_y0;		///< edge to weight : y0(0~255)
	UINT32 e_to_w_th_y1;		///< edge to weight : y2(0~255)
	UINT32 chroma_blur_mode;	///< chroma blur for DCE(0:blur off,1:blur only)
	UINT32 blur_v_gain;			///< 0x0 : blur V <-> 0xF : original V
}
LX_PE_DCM_DCE_SMOOTH0_T;

/**
 *	pe dynamic blur control parameter type
 *	- use for dce on hsv domain
 *	ver.1, for H14x,M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 sc_map_sel;			///< 0x0:MAX(e,t map),0x1:t map,0x2:e map
	UINT32 sc_amap_filter_tap;	///< amap, 0x0~0x3 : 5,7,9,15-tap
	UINT32 amap_gain;			///< e map gain:(2.3u)0x0=0 ~ 0x8=1.0 ~ 0x1F=3.99x
	UINT32 tmap_gain;			///< t map gain:(2.3u)0x0=0 ~ 0x8=1.0 ~ 0x1F=3.99x
	UINT32 e_to_w_th_x0;		///< edge to weight : x0
	UINT32 e_to_w_th_x1;		///< edge to weight : x1
	UINT32 e_to_w_th_y0;		///< edge to weight : y0
	UINT32 e_to_w_th_y1;		///< edge to weight : y1
	UINT32 blur_v_gain;			///< 0x0 : blur V <-> 0xF : original V
}
LX_PE_DCM_DCE_SMOOTH1_T;

/***************************************************************************/
/* HST : Histogram Module */
/***************************************************************************/
/**
 *	pe histogram information parameter type
 *	- status[0] : [25:16]hist_bin_max, [09:00]hist_bin_min
 *	- status[1] : [25:16]hist_v_max, [09:00]hist_v_min
 *	- status[2] : [28:08]hist_max_bin_val, [05:00]hist_max_bin_num
 *	- status[3] : [21:00]hist_diff_sum
 *	- status[4] : [20:00]hist_detect_rgn_num
 *	- status[5] : [07:00]saturation status
 *	- status[6] : [07:00]motion status(from tnr block)
 *	- status[7] : [17:00]texture measure
 *	- status[8] : researved
 *	- status[9] : researved
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	LX_PE_COLOR_DOMAIN domain;	///< color domain
	UINT32 histo_ready;			///< histogram is valid, if 1, invalid otherwise.
	UINT32 average[3];			///< the average of all luma in the specified region Y or RGB
    UINT32 min;					///< the min of all luma in the specified region
	UINT32 max;					///< the max of all luma in the specified region
    UINT32 histogram[96];		///< histogram data of luma(histo[0~31]1st,[32~63]2nd,[64~95]3rd)
    UINT32 status[10];			///< histogram status
}
LX_PE_HST_HISTO_INFO_T;

/**
 *	pe histogram operation parameter type
 */
typedef struct
{
	UINT32 src_apl_op;	///< src apl loading operation, 0:run, 1:stop
	UINT32 src_hist_op;	///< src histogarm loading operation, 0:run, 1:stop
	UINT32 lrc_hist_op;	///< lrcr histogarm loading operation, 0:run, 1:stop
}
LX_PE_HST_HISTO_OPR_T;

/**
 *	pe histogram configuration parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	LX_PE_HST_HISTO_OPR_T opr;	///< histo operation
}
LX_PE_HST_HISTO_CFG_T;

/***************************************************************************/
/* WIN : Window control */
/***************************************************************************/

/***************************************************************************/
/* CSC : Color Space Conversion */
/***************************************************************************/
/**
 *	pe xvYCC control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 enable;			///< enable xvYCC, 1:on, 0:off
	UINT8 scaler;			///< scaling factor, 0~255(255 = x1.0)
}
LX_PE_CSC_XVYCC_T;

/**
 *	pe color gamut control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT16 matrix[PE_NUM_OF_CSC_COEF];	///< 3x3 matrix, primary color correction
}
LX_PE_CSC_GAMUT_T;

/**
 *	pe post csc control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT16 matrix[PE_NUM_OF_CSC_COEF];	///< 3x3 matrix
	UINT16 offset[PE_NUM_OF_CSC_OFST];	///< in[3],out[3] offset
}
LX_PE_CSC_POST_T;

/**
 *	input csc control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT32 enable;						///< input csc enable, 0:off,1:on
	UINT16 matrix[PE_NUM_OF_CSC_COEF];	///< 3x3 matrix
	UINT16 offset[PE_NUM_OF_CSC_OFST];	///< in[3],out[3] offset
}
LX_PE_CSC_INPUT_T;

/***************************************************************************/
/* CMG : Color Management */
/***************************************************************************/

/**
 *	pe color enhancement enable parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 enable;			///< cen enable, 0:off,1:on
}
LX_PE_CMG_ENABLE_T;

/**
 *	pe color enhancement region enable parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;						///< window id
	UINT8 enable[LX_PE_CMG_REGION_NUM];			///< enable region 0 ~ 15
	UINT8 show_region[LX_PE_CMG_REGION_NUM];	///< show region 0 ~ 15, for debug
}
LX_PE_CMG_REGION_ENABLE_T;

/**
 *	pe color enhancement region parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;					///< window id
	UINT32 region_num;						///< region number, 0~15
	UINT16 hue_x[LX_PE_CMG_TBLPOINT];		///< hue input, 0~512~1023(0~360~720 degree)
	UINT8 hue_g[LX_PE_CMG_TBLPOINT];		///< hue gain, 0~127
	UINT8 sat_x[LX_PE_CMG_TBLPOINT];		///< saturation input, 0~100
	UINT8 sat_g[LX_PE_CMG_TBLPOINT];		///< saturation gain, 0~127
	UINT8 val_x[LX_PE_CMG_TBLPOINT];		///< value input, 0~255
	UINT8 val_g[LX_PE_CMG_TBLPOINT];		///< value gain, 0~127
}
LX_PE_CMG_REGION_T;

/**
 *	pe color enhancement region control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;						///< window id
	UINT32 region_num;							///< region number, 0~15
	SINT8 region_delta[LX_PE_CMG_DELTANUM];		///< region delta(offset), -128 ~ 127, [0]h [1]s [2]v [3]g [4]b [5]r
	UINT8 master_gain;							///< region master gain, 0~128~255
}
LX_PE_CMG_REGION_CTRL_T;

/**
 *	pe color enhancement global control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;						///< window id
	SINT16 global_delta[LX_PE_CMG_DELTANUM];	///< global gain,-512 ~ 511, [0]h [1]s [2]v [3]g [4]b [5]r
}
LX_PE_CMG_GLOBAL_CTRL_T;

/**
 *	pe color enhancement color control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT16 saturation;		///< saturation gain
}
LX_PE_CMG_COLOR_CTRL_T;

/** 
 *  pe clear white control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT32 cw_en;			///< clear white enable
	UINT32 gain_sel;		///< Y-CR gain sel, '0' = use Y level gain, '1' = use CR region gain
	UINT32 gain_x[5];		///< gain table x0~4
	UINT32 gain_y[5];		///< gain table y0~4
	UINT32 region_sel;		///< color region sel 1=enable, bit control.(0x0 ~ 0xff)
	UINT32 region_gain;		///< color region gain, default : "64"( 0 ~ 255(1~400%))
}
LX_PE_CMG_CW_CTRL_T;

/** 
 *  pe clear white rgb gain control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT32 g_gain;			///< User Control, G_Gain  (resolution = 2^(-9), 0~192~255)
	UINT32 b_gain;			///< User Control, B_Gain  (resolution = 2^(-9), 0~192~255)
	UINT32 r_gain;			///< User Control, R_Gain  (resolution = 2^(-9), 0~192~255)
}
LX_PE_CMG_CW_GAIN_CTRL_T;

/***************************************************************************/
/* NRD : Noise Reduction*/
/***************************************************************************/
/**
 *	pe digital noise reduction control parameter type
 *	ver.0, for H13Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT32 mnr_enable;		///< mosquito nr enable
	UINT32 ac_bnr_h_en;		///< horizontal ac block nr enable
	UINT32 ac_bnr_v_en;		///< vertical ac block nr enable
	UINT32 dc_bnr_en;		///< dc block nr enable
	UINT32 mnr_gain;		///< mosquito nr gain, filter threshold
	UINT32 ac_bnr_h_gain;	///< ac block nr gain, strength h max
	UINT32 ac_bnr_v_gain;	///< ac block nr gain, strength v max
	UINT32 dc_bnr_gain;		///< dc block nr gain
}
LX_PE_NRD_DNR0_CMN_T;

/**
 *	pe digital noise reduction common control parameter type
 *	ver.2, for H13Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/* dnr main */
	UINT8 enable_ac_bnr;				///< enable_ac_bnr
	UINT8 enable_mnr;					///< enable_mnr
	UINT8 enable_dc_bnr;				///< enable_dc_bnr
	/* mnr */
	UINT8 reg_mnr_enable;				///< reg_mnr_enable
	UINT8 reg_mnr_master_gain;			///< reg_mnr_master_gain
	UINT8 reg_chroma_master_gain;		///< reg_chroma_master_gain
	UINT8 reg_mnr_h_gain;				///< reg_mnr_h_gain
	UINT8 reg_mnr_v_gain;				///< reg_mnr_v_gain
	/* ac_bnr */
	UINT8 bnr_h_en;						///< bnr_h_en
	UINT8 bnr_v_en;						///< bnr_v_en
	UINT8 ac_bnr_protect_motion_max;	///< ac_bnr_protect_motion_max
	UINT8 ac_bnr_protect_motion_min;	///< ac_bnr_protect_motion_min
	UINT8 hbmax_gain;					///< gain for maximum h-blockiness (0~7) d:4,(hmax * hbmaxgain) - hothers = hb
	UINT8 vbmax_gain;					///< gain for maximum v-blockiness (0~7) d:4,(vmax * vbmaxgain) - vothers = vb
	UINT8 strength_h_max;				///< strength_h_max
	UINT8 strength_v_max;				///< strength_v_max
	/* dc_bnr */
	UINT8 reg_dc_bnr_enable;			///< reg_dc_bnr_enable
	UINT8 reg_dc_bnr_mastergain;		///< reg_dc_bnr_mastergain
	UINT16 reg_dc_bnr_chromagain;		///< reg_dc_bnr_chromagain	
}
LX_PE_NRD_DNR2_CMN_T;

/**
 *	pe digital noise reduction detailed control parameter type
 *	ver.2, for H14Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 enable_ac_bnr;
	UINT8 enable_mnr;
	UINT8 enable_dc_bnr;
	UINT8 dnr_motion_scale;				///< dnr_motion_scale
	UINT8 reg_ac_manual_gain;			///< ac_manual_gain
}
LX_PE_NRD_DNR3_CMN_T;

/**
 *	pe digital noise reduction common control parameter type
 *	ver.4, for M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/* dnr max ctrl 1 */
	UINT8 reg_dnr_max_enable;			///< p0d_dnr_max_ctrl, 0, dnr top enable
	/* dc bnr 3 */
	UINT8 reg_dc_bnr_enable;			///< p0d_dc_bnr_ctrl_0, 0, DC BNR enable,0:= off (output debug mode),1  = on
	UINT8 reg_dc_bnr_mastergain;		///< p0d_dc_bnr_ctrl_2, 15:10, mastergain
	UINT8 reg_dc_bnr_chromagain;		///< p0d_dc_bnr_ctrl_2, 23:16, chromagain
	/* ac bnr 7 */
	UINT8 reg_bnr_ac_h_en;				///< p0d_ac_bnr_ctrl_0, 0, ac bnr h en
	UINT8 reg_bnr_ac_v_en;				///< p0d_ac_bnr_ctrl_0, 1, ac bnr v en
	UINT8 reg_bnr_ac_h_chroma_en;		///< p0d_ac_bnr_ctrl_0, 2, ac bnr h ch en
	UINT8 reg_bnr_ac_v_chroma_en;		///< p0d_ac_bnr_ctrl_0, 3, ac bnr v ch en
	UINT8 reg_bnr_ac_acness_resol_h;	///< p0d_ac_bnr_ctrl_0,  5: 4, 0x0 is original, 1: /2, 2: /4, 3:/ 8
	UINT8 reg_bnr_ac_acness_resol_v;	///< p0d_ac_bnr_ctrl_4,  7: 6, 0x0 is original, 1: /2, 2: /4, 3:/ 8
	UINT8 reg_ac_master_gain;			///< p0d_ac_bnr_ctrl_9, 15:10, ac_master_gain
	/* mnr 5 */
	UINT8 reg_mnr_enable;				///< p0d_mnr_ctrl_0, 0, mnr_enable
	UINT8 reg_mnr_master_gain;			///< p0d_mnr_ctrl_0, 15: 8, mnr_master_gain
	UINT8 reg_chroma_master_gain;		///< p0d_mnr_ctrl_0, 23:16, chroma_master_gain
	UINT8 reg_mnr_h_gain;				///< p0d_mnr_ctrl_2, 31:24, mnr_h_gain
	UINT8 reg_mnr_v_gain;				///< p0d_mnr_ctrl_2, 23:16, mnr_v_gain
}
LX_PE_NRD_DNR4_CMN_T;

/**
 *	pe digital noise reduction detailed control parameter type
 *	ver.2, for H13Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/* mnr */
	UINT8 reg_h_expend;					///< reg_h_expend
	UINT8 reg_gain_flt_size;			///< reg_gain_flt_size
	UINT8 reg_mnr_s1_mmd_min;			///< reg_mnr_s1_mmd_min
	UINT8 reg_mnr_s2_ratio_min;			///< reg_mnr_s2_ratio_min
	UINT8 reg_mnr_s2_ratio_max;			///< reg_mnr_s2_ratio_max
	UINT8 reg_mnr_s2_mmd_min;			///< reg_mnr_s2_mmd_min
	UINT8 reg_filter_x0;				///< reg_filter_x0
	UINT8 reg_filter_x1;				///< reg_filter_x1
	UINT8 reg_filter_y0;				///< reg_filter_y0
	UINT8 reg_filter_y1;				///< reg_filter_y1
	UINT8 reg_motion_mnr_en;			///< reg_motion_mnr_en
	UINT8 reg_motion_mnr_filter;		///< reg_motion_mnr_filter
	UINT8 reg_mnr_motion_min;			///< reg_mnr_motion_min
	UINT8 reg_mnr_motion_max;			///< reg_mnr_motion_max
	UINT8 reg_motion_mnr_x0;			///< reg_motion_mnr_x0
	UINT8 reg_motion_mnr_x1;			///< reg_motion_mnr_x1
	UINT8 reg_motion_mnr_y0;			///< reg_motion_mnr_y0
	UINT8 reg_motion_mnr_y1;			///< reg_motion_mnr_y1
	/* ac_bnr */
	UINT8 motion_protect_enable;		///< motion_protect_enable
	UINT8 source_type;					///< 0 := sd (9 lm),1 = hd (5 lm)
	UINT8 fiter_type;					///< 0  :=  normal 3-tap avg. 1 1 1, 1  =  center-weighted 3-tap avg : 1 2 1
	UINT8 strength_h_x0;				///< ( hb - offset ) * gain => graph
	UINT8 strength_h_x1;				///< strength_h_x1
	UINT8 detect_min_th;				///< minimun threshold of block line detection
	UINT8 strength_v_x0;				///< ( vb - offset ) * gain => graph
	UINT8 strength_v_x1;				///< strength_v_x1
	/* dc_bnr */
	UINT8 reg_dc_bnr_var_en;			///< reg_dc_bnr_var_en
	UINT8 reg_dc_bnr_motion_en;			///< reg_dc_bnr_motion_en
	UINT8 reg_dc_bnr_acadaptive;		///< reg_dc_bnr_acadaptive
	UINT8 reg_dc_bnr_sdhd_sel;			///< reg_dc_bnr_sdhd_sel
	UINT8 reg_dc_blur_sel;				///< reg_dc_blur_sel
	UINT8 reg_dc_protection_en;			///< reg_dc_protection_en
	UINT8 reg_dc_bnr_var_th0;			///< reg_dc_bnr_var_th0
	UINT8 reg_dc_bnr_var_th1;			///< reg_dc_bnr_var_th1
	UINT8 reg_dc_bnr_var_th2;			///< reg_dc_bnr_var_th2
	UINT8 reg_dc_bnr_var_th3;			///< reg_dc_bnr_var_th3
	UINT8 reg_dc_bnr_motion_th0;		///< reg_dc_bnr_motion_th0
	UINT8 reg_dc_bnr_motion_th1;		///< reg_dc_bnr_motion_th1
	UINT8 reg_dc_bnr_motion_th2;		///< reg_dc_bnr_motion_th2
	UINT8 reg_dc_bnr_motion_th3;		///< reg_dc_bnr_motion_th3
	UINT8 reg_dc_chroma_variance;		///< reg_dc_chroma_variance
	UINT8 reg_dc_var_h_gain;			///< reg_dc_var_h_gain
	UINT8 reg_dc_protection_th;			///< reg_dc_protection_th
}
LX_PE_NRD_DNR2_DETAIL_T;

/**
 *	pe digital noise reduction detailed control parameter type
 *	ver.4, for M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/* dc bnr 18 */
	UINT8 reg_dc_var_en;				///< p0d_dc_bnr_ctrl_2, 0, dc_var_en
	UINT8 reg_dc_motion_en;				///< p0d_dc_bnr_ctrl_2, 1, dc_motion_en
	UINT8 reg_dc_protection_en;			///< p0d_dc_bnr_ctrl_2, 2, dc_protection_en
	UINT8 reg_dc_detail_en;				///< p0d_dc_bnr_ctrl_2, 3, dc_detail_en
	UINT8 reg_dc_blur_sel;				///< p0d_dc_bnr_ctrl_0, 2, dc_blur_sel
	UINT8 reg_dc_motion_max;			///< p0d_dc_bnr_ctrl_0, 23:16, dc_motion_max
	UINT8 reg_dc_motion_min;			///< p0d_dc_bnr_ctrl_0, 31:24, dc_motion_min
	UINT8 reg_dc_detail_max;			///< p0d_dc_bnr_ctrl_1,  7: 0, dc_detail_max
	UINT8 reg_dc_detail_min;			///< p0d_dc_bnr_ctrl_1, 15: 8, dc_detail_min
	UINT8 reg_var_v_gain;				///< p0d_dc_bnr_ctrl_1, 23:20, var_v_gain
	UINT8 reg_var_h_gain;				///< p0d_dc_bnr_ctrl_1, 27:24, var_h_gain
	UINT8 reg_var_cut_resolution;		///< p0d_dc_bnr_ctrl_1, 31:28, var_cut_resolution
	UINT8 reg_dc_global_motion_th;		///< p0d_dc_bnr_ctrl_2,  7: 4, dc_global_motion_th
	UINT8 reg_dc_protection_th;			///< p0d_dc_bnr_ctrl_2, 31:24, dc_protection_th
	UINT8 reg_dc_bnr_var_th0;			///< p0d_dc_bnr_ctrl_3, 31:24, dc_bnr_var_th0
	UINT8 reg_dc_bnr_var_th1;			///< p0d_dc_bnr_ctrl_3, 23:16, dc_bnr_var_th1
	UINT8 reg_dc_bnr_var_th2;			///< p0d_dc_bnr_ctrl_3, 15: 8, dc_bnr_var_th2
	UINT8 reg_dc_bnr_var_th3;			///< p0d_dc_bnr_ctrl_3,  7: 0, dc_bnr_var_th3
	/* ac bnr 23 */
	UINT8 reg_bnr_ac_diff_min_v_th;		///< p0d_ac_bnr_ctrl_0, 15: 8, bnr_ac_diff_min_v_th
	UINT8 reg_bnr_ac_diff_min_h_th;		///< p0d_ac_bnr_ctrl_0, 23:16, bnr_ac_diff_min_h_th
	UINT8 reg_bnr_ac_global_motion_th;	///< p0d_ac_bnr_ctrl_1,  7: 0, bnr_ac_global_motion_th
	UINT8 reg_bnr_ac_acness_max;		///< p0d_ac_bnr_ctrl_1, 15: 8, bnr_ac_acness_max
	UINT8 reg_bnr_ac_acness_min;		///< p0d_ac_bnr_ctrl_1, 23:16, bnr_ac_acness_min
	UINT8 reg_bnr_ac_motion_max;		///< p0d_ac_bnr_ctrl_4, 23:16, bnr_ac_motion_max
	UINT8 reg_bnr_ac_motion_min;		///< p0d_ac_bnr_ctrl_4, 31:24, bnr_ac_motion_min
	UINT8 reg_bnr_ac_detail_th1;		///< p0d_ac_bnr_ctrl_5, 31:24, bnr_ac_detail_th1
	UINT8 reg_bnr_ac_detail_th2;		///< p0d_ac_bnr_ctrl_5, 23:16, bnr_ac_detail_th2
	UINT8 reg_bnr_ac_detail_th3;		///< p0d_ac_bnr_ctrl_5, 15: 8, bnr_ac_detail_th3
	UINT8 reg_bnr_ac_detail_th4;		///< p0d_ac_bnr_ctrl_5,  7: 0, bnr_ac_detail_th4
	UINT8 reg_bnr_ac_pos_gain_h0;		///< p0d_ac_bnr_ctrl_6, 23:16, bnr_ac_pos_gain_h0
	UINT8 reg_bnr_ac_pos_gain_h1;		///< p0d_ac_bnr_ctrl_6, 15: 8, bnr_ac_pos_gain_h1
	UINT8 reg_bnr_ac_pos_gain_h2;		///< p0d_ac_bnr_ctrl_6,  7: 0, bnr_ac_pos_gain_h2
	UINT8 reg_bnr_ac_pos_gain_h3;		///< p0d_ac_bnr_ctrl_7, 31:24, bnr_ac_pos_gain_h3
	UINT8 reg_bnr_ac_pos_gain_l0;		///< p0d_ac_bnr_ctrl_7, 23:16, bnr_ac_pos_gain_l0
	UINT8 reg_bnr_ac_pos_gain_l1;		///< p0d_ac_bnr_ctrl_7, 15: 8, bnr_ac_pos_gain_l1
	UINT8 reg_bnr_ac_pos_gain_l2;		///< p0d_ac_bnr_ctrl_7,  7: 0, bnr_ac_pos_gain_l2
	UINT8 reg_bnr_ac_pos_gain_l3;		///< p0d_ac_bnr_ctrl_8, 31:24, bnr_ac_pos_gain_l3
	UINT8 reg_bnr_ac_detail_max;		///< p0d_detail_ctrl,  7: 0, bnr_ac_detail_max
	UINT8 reg_bnr_ac_detail_min;		///< p0d_detail_ctrl, 15: 8, bnr_ac_detail_min
	UINT8 reg_bnr_diff_l;				///< p0d_detail_ctrl, 23:16, bnr_diff_l
	UINT8 reg_bnr_diff_p;				///< p0d_detail_ctrl, 31:24, bnr_diff_p
	/* mnr 18 */
	UINT8 reg_h_expend;					///< p0d_mnr_ctrl_0, 4, h_expend
	UINT8 reg_gain_flt_size;			///< p0d_mnr_ctrl_0, 5, gain_flt_size
	UINT8 reg_mnr_s1_mmd_min;			///< p0d_mnr_ctrl_1,  7: 0, mnr_s1_mmd_min
	UINT8 reg_mnr_s2_ratio_min;			///< p0d_mnr_ctrl_1, 15: 8, mnr_s2_ratio_min
	UINT8 reg_mnr_s2_ratio_max;			///< p0d_mnr_ctrl_1, 23:16, mnr_s2_ratio_max
	UINT8 reg_mnr_s2_mmd_min;			///< p0d_mnr_ctrl_1, 31:24, mnr_s2_mmd_min
	UINT8 reg_filter_x0;				///< p0d_mnr_ctrl_3,  7: 0, filter_x0
	UINT8 reg_filter_x1;				///< p0d_mnr_ctrl_3, 15: 8, filter_x1
	UINT8 reg_filter_y0;				///< p0d_mnr_ctrl_3, 23:16, filter_y0
	UINT8 reg_filter_y1;				///< p0d_mnr_ctrl_3, 31:24, filter_y1
	UINT8 reg_motion_mnr_en;			///< p0d_mnr_ctrl_4, 0, motion_mnr_en
	UINT8 reg_motion_mnr_filter;		///< p0d_mnr_ctrl_4, 1, motion_mnr_filter
	UINT8 reg_mnr_motion_min;			///< p0d_mnr_ctrl_4, 23:16, mnr_motion_min
	UINT8 reg_mnr_motion_max;			///< p0d_mnr_ctrl_4, 31:24, mnr_motion_max
	UINT8 reg_motion_mnr_x0;			///< p0d_mnr_ctrl_5,  7: 0, motion_mnr_x0
	UINT8 reg_motion_mnr_x1;			///< p0d_mnr_ctrl_5, 15: 8, motion_mnr_x1
	UINT8 reg_motion_mnr_y0;			///< p0d_mnr_ctrl_5, 23:16, motion_mnr_y0
	UINT8 reg_motion_mnr_y1;			///< p0d_mnr_ctrl_5, 31:24, motion_mnr_y1
}
LX_PE_NRD_DNR4_DETAIL_T;

/**
 *	pe temporal noise reduction common control parameter type
 *	ver.1, for H13Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	/* auto mode */
	UINT8 auto_mode;			///< auto nr mode, on(1),off(0)
	/* noise level gaining */
	UINT8 adjust_nt_lvl;		///< Adjust NT_Lvl enable
	UINT8 adjust_nt_lvl_val;	///< Adjust NT_Lvl value (8u, "this-128" will be added to Calculated NT_Lvl)
	/* alpha gain */
	UINT8 y_gain;				///< Y_TNR_Gain (3.5u)
	UINT8 c_gain;				///< C_TNR_Gain (3.5u)
}
LX_PE_NRD_TNR1_CMN_T;

/**
 *	pe temporal noise reduction control parameter type
 *	ver.2, for H13Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 auto_mode_en;					///< auto nr mode en, on(1),off(0)
	UINT8 adjust_nt_lvl;				///< dbg_ctrl. adjust nt_lvl enable
	UINT8 adjust_nt_lvl_val;			///< dbg_ctrl. adjust nt_lvl value (8u, "this-128" will be added to calculated nt_lvl),bypass 0x80
	UINT8 y_gain;						///< 0x00 ~ 0xfe = strongest ~ weakest,0x20 := normal operation,0xff = special case. luma-tnr off (alphay=1.0)
	UINT8 c_gain;						///< 0x00 ~ 0xfe = strongest ~ weakest,0x20 := normal operation,0xff = special case. chroma-tnr off (alphac=1.0)
}
LX_PE_NRD_TNR2_CMN_T;

/**
 *	pe temporal noise reduction control parameter type
 *	ver.3, for H14Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 tnr_enable;					///< tnr_en, on(1),off(0)
}
LX_PE_NRD_TNR3_CMN_T;

/**
 *	pe temporal noise reduction control parameter type
 *	ver.4, for M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 tnr_master_gain;				///< master TNR gain, 0~255
}
LX_PE_NRD_TNR4_CMN_T;

/**
 *	pe temporal noise reduction detailed control parameter type
 *	ver.2, for H13Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	/*ma: motion 16*/
	UINT8 m_coring_en_c;				///< gain accordint to result of ma, mc blending 
	UINT8 m_coring_en_y;				///< motion coring enable y
	UINT8 m_gain_c;						///< motion gain c: 2.2u
	UINT8 m_gain_y;						///< max = 3.9999
	UINT8 m_coring_th;					///< m_coring_th
	UINT8 m_offset_mode_en;				///< 0 := motion gain mode,1 = motion offset mode
	UINT8 m_gain_en_var;				///< motion gain enable by variance
	UINT8 a_gain_en_var;				///< alpha gain enable by variance
	UINT8 wa_y0;						///< variance-adaptive motion/alpha gain : 0.8u
	UINT8 wa_y1;						///< variance-adaptive motion/alpha gain : 0.8u
	UINT8 wa_y2;						///< variance-adaptive motion/alpha gain : 0.8u
	UINT8 wa_y3;						///< variance-adaptive motion/alpha gain : 0.8u
	UINT8 wa_x0;						///< spatial variance threshold 0
	UINT8 wa_x1;						///< spatial variance threshold 1
	UINT8 wa_x2;						///< spatial variance threshold 2
	UINT8 wa_x3;						///< spatial variance threshold 3
	/*ma: alpha gain 8*/
	UINT8 luma_gain_p0_x;				///< luma_gain_p0_x
	UINT8 luma_gain_p1_x;				///< luma_gain_p1_x
	UINT8 luma_gain_p2_x;				///< luma_gain_p2_x
	UINT8 luma_gain_p3_x;				///< luma_gain_p3_x
	UINT8 luma_gain_p0_y;				///< luma_gain_p0_y
	UINT8 luma_gain_p1_y;				///< luma_gain_p1_y
	UINT8 luma_gain_p2_y;				///< luma_gain_p2_y
	UINT8 luma_gain_p3_y;				///< luma_gain_p3_y
	/*ma: alhpa remapping 19*/
	UINT8 alpha_avg_en;					///< alpha spatial avg enable
	UINT8 alpha_mapping_y_en;			///< alpha mapping for y enable
	UINT8 alpha_mapping_c_en;			///< alpha mapping for c enable
	UINT8 alpha_mapping_y_x0;			///< alpha_mapping_y_x0
	UINT8 alpha_mapping_y_x1;			///< alpha_mapping_y_x1
	UINT8 alpha_mapping_y_x2;			///< alpha_mapping_y_x2
	UINT8 alpha_mapping_y_x3;			///< alpha_mapping_y_x3
	UINT8 alpha_mapping_y_y0;			///< alpha_mapping_y_y0
	UINT8 alpha_mapping_y_y1;			///< alpha_mapping_y_y1
	UINT8 alpha_mapping_y_y2;			///< alpha_mapping_y_y2
	UINT8 alpha_mapping_y_y3;			///< alpha_mapping_y_y3
	UINT8 alpha_mapping_c_x0;			///< alpha_mapping_c_x0
	UINT8 alpha_mapping_c_x1;			///< alpha_mapping_c_x1
	UINT8 alpha_mapping_c_x2;			///< alpha_mapping_c_x2
	UINT8 alpha_mapping_c_x3;			///< alpha_mapping_c_x3
	UINT8 alpha_mapping_c_y0;			///< alpha_mapping_c_y0
	UINT8 alpha_mapping_c_y1;			///< alpha_mapping_c_y1
	UINT8 alpha_mapping_c_y2;			///< alpha_mapping_c_y2
	UINT8 alpha_mapping_c_y3;			///< alpha_mapping_c_y3
	/*ma: noise level adjust gain 54*/
	UINT8 nt_lvl_adjust_gm_enable;		///< noise level adjust (by the global motion amount) enable
	UINT8 nt_lvl_adjust_lpct_enable;	///< noise level adjust (by the percent of smooth area) enable
	UINT8 nt_lvl_adjust_avg_ts_enable;	///< (by the (avg_t - avg_s))  enable,avg_t : frame average of temporal difference,avg_s : frame average of spatial variance
	UINT8 nt_lvl_adjust_lpct_sel;		///< 0 := the percent of smooth area in a picture
	UINT8 nt_lvl_gain_gm_x0;			///< nt_lvl_gain_gm_x0
	UINT8 nt_lvl_gain_gm_x1;			///< nt_lvl_gain_gm_x1
	UINT8 nt_lvl_gain_gm_x2;			///< nt_lvl_gain_gm_x2
	UINT8 nt_lvl_gain_gm_x3;			///< nt_lvl_gain_gm_x3
	UINT8 nt_lvl_gain_gm_y0;			///< nt_lvl_gain_gm_y0
	UINT8 nt_lvl_gain_gm_y1;			///< nt_lvl_gain_gm_y1
	UINT8 nt_lvl_gain_gm_y2;			///< nt_lvl_gain_gm_y2
	UINT8 nt_lvl_gain_gm_y3;			///< nt_lvl_gain_gm_y3
	UINT8 nt_lvl_gain_st_x0;			///< nt_lvl_gain_st_x0
	UINT8 nt_lvl_gain_st_x1;			///< nt_lvl_gain_st_x1
	UINT8 nt_lvl_gain_st_x2;			///< nt_lvl_gain_st_x2
	UINT8 nt_lvl_gain_st_x3;			///< nt_lvl_gain_st_x3
	UINT8 nt_lvl_gain_st_y0;			///< nt_lvl_gain_st_y0
	UINT8 nt_lvl_gain_st_y1;			///< nt_lvl_gain_st_y1
	UINT8 nt_lvl_gain_st_y2;			///< nt_lvl_gain_st_y2
	UINT8 nt_lvl_gain_st_y3;			///< nt_lvl_gain_st_y3
	UINT8 nt_lvl_gain_lpct_x0;			///< nt_lvl_gain_lpct_x0
	UINT8 nt_lvl_gain_lpct_x1;			///< nt_lvl_gain_lpct_x1
	UINT8 nt_lvl_gain_lpct_x2;			///< nt_lvl_gain_lpct_x2
	UINT8 nt_lvl_gain_lpct_x3;			///< nt_lvl_gain_lpct_x3
	UINT8 nt_lvl_gain_lpct_y0;			///< nt_lvl_gain_lpct_y0
	UINT8 nt_lvl_gain_lpct_y1;			///< nt_lvl_gain_lpct_y1
	UINT8 nt_lvl_gain_lpct_y2;			///< nt_lvl_gain_lpct_y2
	UINT8 nt_lvl_gain_lpct_y3;			///< nt_lvl_gain_lpct_y3
	/* map*/
	UINT8 m_gain_en_var2;				///< motion gain enable by variance. this motion goes to motion-adaptive spatial blur
	UINT8 m_gain_ctrl2_x0;				///< m_gain_ctrl2_x0
	UINT8 m_gain_ctrl2_x1;				///< m_gain_ctrl2_x1
	UINT8 m_gain_ctrl2_y0;				///< m_gain_ctrl2_y0
	UINT8 m_gain_ctrl2_y1;				///< m_gain_ctrl2_y1
	UINT8 sf_map_flt_en;				///< spatial var. map filtering enable,spatial variance filtering,spatial variance adaptive
	UINT8 sf_map_tap;					///< "01" : 7-tap 7x3,"10" : 5-tap  5x3,"11" : 3-tap  3x3
	UINT8 sf_map_gain;					///< spatial var. map gain(3.3u)
	UINT8 sf_th0;						///< spatial var-to-filter gain 
	UINT8 sf_th1;						///< lager than th0, smaller than th1, 3-tap recommand
	UINT8 sf_th2;						///< th2 5-tap
	UINT8 sf_th3;						///< th3 7-tap
	UINT8 sf_th4;						///< th4 9-tap
	UINT8 sf_mb_x0;						///< x0 x motion
	UINT8 sf_mb_x1;						///< sf_mb_x1
	UINT8 sf_mb_x2;						///< sf_mb_x2
	UINT8 sf_mb_x3;						///< sf_mb_x3
	UINT8 sf_mb_y0;						///< y0  y gain : bigger gain cause blurer
	UINT8 sf_mb_y1;						///< sf_mb_y1
	UINT8 sf_mb_y2;						///< sf_mb_y2
	UINT8 sf_mb_y3;						///< sf_mb_y3
	/* bluring gain */
	UINT8 sf_gain_mode;					///< 0' : disable gain control,'1' : enable gain control,master gain
	UINT8 sf_gain;						///< sf gain(0.4u)
	UINT8 sf_blend_en_y;				///< motion-adaptive blending enable of blurred data and bypass data: y
	UINT8 sf_blend_en_c;				///< motion-adaptive blending enable of blurred data and bypass data: c
	UINT8 sf_blend_motion_exp_mode;		///< 0:= no expand,1 = 11-tap expand,2 = 9-tap expand,3 = 5-tap expand
	/*mc*/
	UINT8 sad_sel_pels;					///< 0 := sad_ma,1 = alpha_blend(sad_mc, sad_ma)
	UINT8 sad_mamc_blend;				///< 0xff -> sad of mc,0x00 -> sad of ma
	UINT8 mamc_blend_sel;				///< 0:= spatial variance, 1 = motion vector consistency
	UINT8 mamc_blend_x0;				///< spatial variance threshold
	UINT8 mamc_blend_x1;				///< mamc_blend_x1
	UINT8 mamc_blend_y0;				///< 0xff: mc filter, 0x00: ma filter
	UINT8 mamc_blend_y1;				///< mamc_blend_y1
	/*me*/
	UINT8 hme_half_pel_en;				///< half pel search enable for motion estimation
	UINT8 mv_cost_smooth_gain;			///< 0.5u
	UINT8 mv_cost_smooth_en;			///< enable for temporal smoothness constraint for block matching cost
	UINT8 mv_cost_gmv_smooth_gain;		///< mv_cost_gmv_smooth_gain
	UINT8 mv_cost_gmv_smooth_en;		///< mv_cost_gmv_smooth_en
	UINT8 mv0_protect_th;				///< sad threshold for zero-motion vector protection
	UINT8 mv0_protect_en;				///< zero-motion vector protection enable
	UINT8 sad_protect_en;				///< sad-based zero-motion vector protection enable 
	UINT8 sad_protect_gm_en;			///< zero-motion vector protection enable by global motion
	UINT8 mv_protect_control_x0;		///< variance threshold for 0-motion vector protection
	UINT8 mv_protect_control_x1;		///< mv_protect_control_x1
	UINT8 mv_protect_control_y0;		///< sad threshold for 0-motion vector protection
	UINT8 mv_protect_control_y1;		///< mv_protect_control_y1
}
LX_PE_NRD_TNR2_DETAIL_T;

/**
 *	pe temporal noise reduction detailed control parameter type
 *	ver.4, for M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/*ma 26*/
	UINT8 reg_g_blend_a;				///< p0m_tnr_ctrl_25, 31:24, reg_g_blend_a, , set to fw
	UINT8 motion_sc_reset_en;			///< p0m_tnr_ctrl_31,     2, motion_sc_reset_en
	UINT8 luma_gain_ctrl_x0;			///< p0m_tnr_ctrl_33, 31:24, luma_gain_ctrl
	UINT8 luma_gain_ctrl_x1;			///< p0m_tnr_ctrl_33, 23:16, luma_gain_ctrl
	UINT8 luma_gain_ctrl_x2;			///< p0m_tnr_ctrl_33, 15: 8, luma_gain_ctrl
	UINT8 luma_gain_ctrl_x3;			///< p0m_tnr_ctrl_33,  7: 0, luma_gain_ctrl
	UINT8 luma_gain_ctrl_y0;			///< p0m_tnr_ctrl_34, 31:24, luma_gain_ctrl
	UINT8 luma_gain_ctrl_y1;			///< p0m_tnr_ctrl_34, 23:16, luma_gain_ctrl
	UINT8 luma_gain_ctrl_y2;			///< p0m_tnr_ctrl_34, 15: 8, luma_gain_ctrl
	UINT8 luma_gain_ctrl_y3;			///< p0m_tnr_ctrl_34,  7: 0, luma_gain_ctrl
	UINT8 skingain_motioncontrol_x0;	///< p0m_tnr_ctrl_32, 15: 8, skingain_motioncontrol
	UINT8 skingain_motioncontrol_x1;	///< p0m_tnr_ctrl_32, 31:24, skingain_motioncontrol
	UINT8 skingain_motioncontrol_y0;	///< p0m_tnr_ctrl_32,  7: 3, skingain_motioncontrol
	UINT8 skingain_motioncontrol_y1;	///< p0m_tnr_ctrl_32, 23:19, skingain_motioncontrol
	UINT8 skin_lut_y0;					///< p0m_tnr_ctrl_35,  7: 0, skin_lut
	UINT8 skin_lut_y1;					///< p0m_tnr_ctrl_35, 15: 8, skin_lut
	UINT8 skin_lut_y2;					///< p0m_tnr_ctrl_35, 23:16, skin_lut
	UINT8 skin_lut_y3;					///< p0m_tnr_ctrl_35, 31:24, skin_lut
	UINT8 skin_lut_cb0;					///< p0m_tnr_ctrl_36,  7: 0, skin_lut
	UINT8 skin_lut_cb1;					///< p0m_tnr_ctrl_36, 15: 8, skin_lut
	UINT8 skin_lut_cb2;					///< p0m_tnr_ctrl_36, 23:16, skin_lut
	UINT8 skin_lut_cb3;					///< p0m_tnr_ctrl_36, 31:24, skin_lut
	UINT8 skin_lut_cr0;					///< p0m_tnr_ctrl_37,  7: 0, skin_lut
	UINT8 skin_lut_cr1;					///< p0m_tnr_ctrl_37, 15: 8, skin_lut
	UINT8 skin_lut_cr2;					///< p0m_tnr_ctrl_37, 23:16, skin_lut
	UINT8 skin_lut_cr3;					///< p0m_tnr_ctrl_37, 31:24, skin_lut
	/*me 22*/
	UINT8 past_refine_ratio;			///< p0m_tnr_ctrl_07,  7: 4, past_refine_ratio, , set to fw
	UINT8 global_past_refine_ratio;		///< p0m_tnr_ctrl_07, 11: 8, global_past_refine_ratio, , set to fw
	UINT8 mv_refine_ratio;				///< p0m_tnr_ctrl_08, 27:24, spatial vector refine, , set to fw
	UINT8 temporal_refine_adj1;			///< p0m_tnr_ctrl_09,  2: 0, temporal refine vector adjustment
	UINT8 temporal_refine_adj2;			///< p0m_tnr_ctrl_09,  5: 3, temporal refine vector adjustment
	UINT8 zero_refine_ratio;			///< p0m_tnr_ctrl_09, 30:27, zero vector refine
	UINT8 me_conf_scale_ma;				///< p0m_tnr_ctrl_12,  2: 0, me_conf_scale_ma
	UINT8 me_conf_scale_mc;				///< p0m_tnr_ctrl_12,  6: 4, me_conf_scale_mc
	UINT8 past_x0;						///< p0m_tnr_ctrl_39,  7: 0, ME auto temporal refine ratio setting
	UINT8 past_x1;						///< p0m_tnr_ctrl_39, 15: 8, ME auto temporal refine ratio setting
	UINT8 past_x2;						///< p0m_tnr_ctrl_39, 23:16, ME auto temporal refine ratio setting
	UINT8 past_y0;						///< p0m_tnr_ctrl_39, 27:24, ME auto temporal refine ratio setting
	UINT8 past_y1;						///< p0m_tnr_ctrl_39, 31:28, ME auto temporal refine ratio setting
	UINT8 past_y2;						///< p0m_tnr_ctrl_38,  3: 0, ME auto temporal refine ratio setting
	UINT8 past_y3;						///< p0m_tnr_ctrl_38,  7: 4, ME auto temporal refine ratio setting
	UINT8 global_x0;					///< p0m_tnr_ctrl_04,  7: 0, ME auto global refine ratio setting
	UINT8 global_x1;					///< p0m_tnr_ctrl_04, 15: 8, ME auto global refine ratio setting
	UINT8 global_x2;					///< p0m_tnr_ctrl_04, 23:16, ME auto global refine ratio setting
	UINT8 global_y0;					///< p0m_tnr_ctrl_04, 27:24, ME auto global refine ratio setting
	UINT8 global_y1;					///< p0m_tnr_ctrl_04, 31:28, ME auto global refine ratio setting
	UINT8 global_y2;					///< p0m_tnr_ctrl_05,  3: 0, ME auto global refine ratio setting
	UINT8 global_y3;					///< p0m_tnr_ctrl_05,  7: 4, ME auto global refine ratio setting
	/*mc 5*/
	UINT8 am_th_mode;					///< p0m_tnr_ctrl_16,     4, A-mean th mode
	UINT8 am_th_val;					///< p0m_tnr_ctrl_16, 15: 8, A-mean th value(manaul)
	UINT8 mcblnd_th_x0;					///< p0m_tnr_ctrl_19,  7: 0, alpha(normal) +alpha(strong) blending
	UINT8 mcblnd_th_x1;					///< p0m_tnr_ctrl_19, 15: 8, alpha(normal) +alpha(strong) blending
	UINT8 sc_alpha_mode;				///< p0m_tnr_ctrl_20, 23:22, alpha in case of scene-change
	/*ma post 4*/
	UINT8 reg_ma_conf_x0;				///< p0m_tnr_ctrl_43,  7: 3, reg_ma_conf
	UINT8 reg_ma_conf_x1;				///< p0m_tnr_ctrl_43, 15:11, reg_ma_conf
	UINT8 reg_ma_conf_y0;				///< p0m_tnr_ctrl_43, 23:16, reg_ma_conf
	UINT8 reg_ma_conf_y1;				///< p0m_tnr_ctrl_43, 31:24, reg_ma_conf
	/*mc post 4*/
	UINT8 reg_mc_conf_x0;				///< p0m_tnr_ctrl_42, 31:24, reg_mc_conf
	UINT8 reg_mc_conf_x1;				///< p0m_tnr_ctrl_42, 23:16, reg_mc_conf
	UINT8 reg_mc_conf_y0;				///< p0m_tnr_ctrl_42, 15:11, reg_mc_conf
	UINT8 reg_mc_conf_y1;				///< p0m_tnr_ctrl_42,  7: 3, reg_mc_conf
	/*snr 7*/
	UINT8 reg_snr_master_gain;			///< p0m_tnr_ctrl_48,  7: 0, reg_snr_master_gain
	UINT8 reg_snr_c_blur_gain;			///< p0m_tnr_ctrl_48, 23:16, reg_snr_c_blur_gain, , set to fw
	UINT8 reg_snr_y_blur_gain;			///< p0m_tnr_ctrl_48, 31:24, reg_snr_y_blur_gain, , set to fw
	UINT8 snr_blendgain_x0;				///< p0m_tnr_ctrl_50, 23:16, snr_blendgain
	UINT8 snr_blendgain_x1;				///< p0m_tnr_ctrl_50, 31:24, snr_blendgain
	UINT8 snr_blendgain_y0;				///< p0m_tnr_ctrl_50,  7: 0, snr_blendgain
	UINT8 snr_blendgain_y1;				///< p0m_tnr_ctrl_50, 15: 8, snr_blendgain
}
LX_PE_NRD_TNR4_DETAIL_T;

/***************************************************************************/
/* DNT : Deinterlace */
/***************************************************************************/

/**
 *	pe deinterlacer, film(3:2,2:2 pull down) mode control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT32 enable;			///< enable film mode, 1:on, 0:off
}
LX_PE_DNT_FILMMODE_T;

/**
 *	pe deinterlacer, low delay mode control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT32 enable;			///< enable low delay mode, 1:on, 0:off
}
LX_PE_DNT_LD_MODE_T;

/***************************************************************************/
/* SHP : Sharpness */
/***************************************************************************/
/**
 *	scaler filter coefficient control parameter type
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 h_luma_id;			///< horizontal luma index
	UINT32 v_luma_id;			///< vertical luma index
	UINT32 h_chrm_id;			///< horizontal chroma index
	UINT32 v_chrm_id;			///< vertical chroma index
}
LX_PE_SHP_SCLFILTER_T;

/**
 *	pe resolution enhancement(RE) normal control(ver.1) parameter type
 *	ver.1, for H13Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 mp_white_gain;	///< white gain
	UINT8 mp_black_gain;	///< black gain
	UINT8 sp_white_gain;	///< white gain
	UINT8 sp_black_gain;	///< black gain
	/* cti */
	UINT8 cti_en;			///< cti enable
	UINT8 tap_size;			///< "000" 21 ta
	UINT8 cti_gain;			///< gain (3.5u)
	UINT8 ycm_y_gain;		///< ycm_y_gain
	UINT8 ycm_c_gain;		///< ycm_c_gain
}
LX_PE_SHP_RE1_CMN_T;

/**
 *	pe resolution enhancement(RE) horizontal control(ver.1) parameter type
 *	ver.1, for H13Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;			///< window id
	UINT8 edge_y_filter_en;			///< edge_Y_filter_en
	UINT8 reg_csft_gain;			///< reg_csft_gain: center shift gain(1.5u)
	UINT8 edge_filter_white_gain;	///< edge filter white gain
	UINT8 edge_filter_black_gain;	///< edge filter black gain
	UINT8 a_gen_width;				///< a_gen_widt
	UINT8 mp_horizontal_gain;		///< horizontal gain(3.5u)
	UINT8 sp_horizontal_gain;		///< horizontal gain(3.5u)
}
LX_PE_SHP_RE1_HOR_T;

/**
 *	pe resolution enhancement(RE) vertical control(ver.1) parameter type
 *	ver.1, for H13Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 der_v_en;			///< der-v enable
	UINT8 bif_manual_th;	///< bilateral filter threshold
	UINT8 csft_gain;		///< center-shift gain (1.5u)
	UINT8 gain_b;			///< gain(b) : 2.5u
	UINT8 gain_w;			///< gain(w): 2.5u
	UINT8 der_gain_mapping;	///< weight selection for weighted average of bif and inpu
	UINT8 mmd_sel;			///< a-gen. siz
	UINT8 mp_vertical_gain;	///< vertical gain(3.5u)
	UINT8 sp_vertical_gain;	///< vertical gain(3.5u)
}
LX_PE_SHP_RE1_VER_T;

/**
 *	pe resolution enhancement(RE) misc. control parameter type
 *	ver.1, for H13Ax
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	/* e-map */
	UINT8 amap2_sel;			///< A-map generation
	UINT8 ga_max;				///< A to Edge weight control
	UINT8 ga_th0;				///< th0
	UINT8 ga_th1;				///< th1
	/* t-map */
	UINT8 amap1_sel;			///< A-map generatio for texture enahcer
	UINT8 max_sel;				///< A-map expansion for texture-enhance
	UINT8 tmap_gain;			///< T-to-texture weight
	UINT8 gt_th0;				///< th0
	UINT8 gt_th0a;				///< th0a
	UINT8 gt_th0b;				///< th0b
	UINT8 gt_th1;				///< th1
	UINT8 gt_gain0a;			///< gain(th0a) : 1.5u
	UINT8 gt_gain0b;			///< gain(th0b) : 1.5u
	UINT8 gt_max;				///< gain(th1) :1.5u
	UINT8 coring_mode1;			///< t-map(H/V) coring mode
	UINT16 var_th;				///< flat region rejection threshold,11bit
	/* d-jag */
	UINT8 center_blur_en;		///< center-pixel averaging for edge-direction calculatio
	UINT8 level_th;				///< g0: level threshold
	UINT8 protect_th;			///< G1: protect threshold
	UINT8 n_avg_gain;			///< neighborhood pixel averaing : gain
}
LX_PE_SHP_RE1_MISC_T;

/**
 *	pe edge enhancement(EE) control parameter type
 *	ver.1, for H13
 */
typedef struct
{
	LX_PE_WIN_ID win_id;	///< window id
	UINT8 mp_edge_gain_b;	///< MP: edge gain(b) : 2.5u
	UINT8 mp_edge_gain_w;	///< MP: edge gain(w)
	UINT8 sp_edge_gain_b;	///< SP: edge gain(b) : 2.5u
	UINT8 sp_edge_gain_w;	///< SP: edge gain(w)
}
LX_PE_SHP_EE1_T;

/**
 *	pe detail enhancement(DE) control parameter type
 *	ver.1, for H13
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT8 mp_texture_gain_b;	///< MP: texture gain(b)
	UINT8 mp_texture_gain_w;	///< MP: texture gain(w) 
	UINT8 sp_texture_gain_b;	///< SP: texture gain(b)
	UINT8 sp_texture_gain_w;	///< SP: texture gain(w) 
}
LX_PE_SHP_DE1_T;

/**
 *	pe resolution enhancement(RE) normal control(ver.2) parameter type
 *	ver.2, for H13Bx, M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 mp_white_gain;				///< white gain
	UINT8 mp_black_gain;				///< black gain
	UINT8 sp_white_gain;				///< white gain
	UINT8 sp_black_gain;				///< black gain
	/* cti */
	UINT8 tap_size;						///< "000" 21 tap,"001" 19 tap,"010" 17 tap,"011" 15 tap,"100" 13 tap,"101" 11 tap,"110"  9 tap
	UINT8 cti_gain;						///< gain (3.5u)
	UINT8 ycm_y_gain;					///< ycm_y_gain
	UINT8 ycm_c_gain;					///< ycm_c_gain
}
LX_PE_SHP_RE2_CMN_T;

/**
 *	pe resolution enhancement(RE) horizontal control(ver.1) parameter type
 *	ver.2, for H13Bx, M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 reg_csft_gain;				///< reg_csft_gain: center shift gain(1.5u)
	UINT8 edge_filter_white_gain;		///< edge filter white gain upper 2bit: int, lower 4 bit: float, max: 63(dec) 3.9999
	UINT8 edge_filter_black_gain;		///< edge filter black gain 
	UINT8 a_gen_width;					///< width size for dynamic range
	UINT8 mp_horizontal_gain;			///< horizontal gain(3.5u)
	UINT8 sp_horizontal_gain;			///< horizontal gain(3.5u)
	UINT8 e_gain_th1;					///< a to edge gain: th1
	UINT8 e_gain_th2;					///< a to edge gain: th2
	UINT8 f_gain_th1;					///< a to flat gain: th1
	UINT8 f_gain_th2;					///< a to flat gain: th2
	UINT8 coring_th;					///< coring_th
	UINT8 y_gain;						///< y_gain
	UINT8 c_gain;						///< c_gain
}
LX_PE_SHP_RE2_HOR_T;

/**
 *	pe resolution enhancement(RE) vertical control(ver.1) parameter type
 *	ver.2, for H13Bx, M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	UINT8 bif_manual_th;				///< bilateral filter threshold
	UINT8 csft_gain;					///< center-shift gain (1.5u)
	UINT8 gain_b;						///< gain(b) : 2.5u
	UINT8 gain_w;						///< gain(w): 2.5u
	UINT8 mmd_sel;						///< "000" : 11 tap,"001" : 9-tap,"010" : 7-tap,"011" : 5-tap,"100" : 3-tap ,o.w   : 11-tap
	UINT8 mp_vertical_gain;				///< vertical gain(3.5u)
	UINT8 sp_vertical_gain;				///< vertical gain(3.5u)
	UINT8 gain_th1;						///< a-to-weight: th1
	UINT8 gain_th2;						///< a-to-weight: th2
}
LX_PE_SHP_RE2_VER_T;

/**
 *	pe resolution enhancement(RE) misc. control parameter type
 *	ver.2, for H13Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/* cti */
	UINT8 cti_en;						///< cti enable
	UINT8 coring_th0;					///< th with coring th0 mmd
	UINT8 coring_th1;					///< coring_th1
	UINT8 coring_map_filter;			///< "000" : no ,"001" : 5-tap ,"010" : 7-tap ,"011" : 9-tap ,100: : 11-tap ,"101" : 13-tap 
	UINT8 coring_tap_size;				///< "000" 21 tap,"001" 19 tap,"010" 17 tap,"011" 15 tap,"100" 13 tap,"101" 11 tap,"110"  9 tap
	UINT8 ycm_en1;						///< y c miss matching.
	UINT8 ycm_band_sel;					///< ycm_band_sel
	UINT8 ycm_diff_th;					///< ycm_diff_th
	/* h */
	UINT8 edge_y_filter_en;				///< edge_y_filter_en
	UINT8 e_gain_max;					///< a to edge weight: gain
	UINT8 f_gain_max;					///< a to flat weight: gain
	UINT8 mp_lap_h_mode;				///< "000" : h3,"001" : h4,"010" : h5,"011" : h6,"100" : h7
	UINT8 sp_lap_h_mode;				///< "00" : h3,"01" : h5,"10" : h7,"11" : not used
	/* v */
	UINT8 der_v_en;						///< der-v enable
	UINT8 der_gain_mapping;				///< "00" : a-to weight,"01" : a-map,"10  : a-to weight,"11" : a-map
	UINT8 max_sel;						///< ,"000" : do not expand,"001" : 3-tap,"010" : 5-tap,"011" : 7-tap,"100" : 9-tap,"101" : 11-tap,o.w   : do not expand
	UINT8 mp_lap_v_mode;				///< laplacian v mode,"000" : v3,"001" : v4,"010" : v5,"011" : v6,"100" : v7
	UINT8 sp_lap_v_mode;				///< 0' : v3,'1' : v5
	/* cmm */
	UINT8 mp_sobel_weight;				///< weight for sobel operator output
	UINT8 mp_laplacian_weight;			///< weight for laplacian operator output
	UINT8 sp_sobel_weight;				///< weight for sobel operator output
	UINT8 sp_laplacian_weight;			///< weight for laplacian operator output	
	UINT8 flat_en;						///< flat-filter enable
	UINT8 flat_filter_type;				///< '0' : bi-lateral filter,'1': average filter	
	/*d_jag*/
	UINT8 edf_en;						///< edge-directional de-jagging enable
	UINT8 center_blur_en;				///< '0' : use original pixel,'1' : use avg. pixel
	UINT8 count_diff_th;				///< matchness threshod for edge-direction decision
	UINT8 n_avg_mode;					///< mode for dual edges,'0' : use 12 direction results,'1' : use 36 direction results
	UINT8 line_variation_diff_th;		///< line-variation threshold for edge-direction decision
	UINT8 level_th;						///< g0: level threshold
	UINT8 protect_th;					///< g1: protect threshold
	UINT8 n_avg_gain;					///< neighborhood pixel averaing : gain
	UINT8 edf_count_min;				///< g0 : edf_count_min
	UINT8 edf_count_max;				///< g0 : edf_count_max
	UINT8 dj_h_count_min;				///< g0 : dj_h_count_min
	UINT8 dj_h_count_max;				///< g0 : dj_h_count_max
	UINT8 dj_v_count_min;				///< g0 : dj_v_count_min
	UINT8 dj_v_count_max;				///< g0 : dj_v_count_max
	/*e_map*/
	UINT8 amap2_sel;					///< "00" : 5-tap,"01" : 7-tap,"10" : 9-tap,"11" : 15-tap
	UINT8 amap_gain;					///< edge-map gain
	UINT8 ga_max;						///< <a to edge weight control>
	UINT8 ga_th0;						///< th0
	UINT8 ga_th1;						///< th1
	/*t_map*/
	UINT8 amap1_sel;					///< "00" : 15-tap,"01" : 9-tap,"10" : 7-tap,"11" : 5-tap
	UINT8 tmap_max_sel;					///< "000" : bypass,"001" : 5-tap,"010" : 7-tap,"011" : 9-tap,"100" : 11-tap,"101" : 13-tap,"110" : 15-tap,"111" : 17-tap
	UINT8 avg_sel;						///< "00" : bypass,"01" : 3x1 average [1 2 1],"10" : 5x1 average [1 2 2 2 1 ],"11" : bypass
	UINT8 tmap_gain;					///< texture-map gain(2.5u)
	UINT8 gt_th0;						///< th0
	UINT8 gt_th0a;						///< th0a
	UINT8 gt_th0b;						///< th0b
	UINT8 gt_th1;						///< th1
	UINT8 gt_gain0a;					///< gain(th0a) : 1.5u
	UINT8 gt_gain0b;					///< gain(th0b) : 1.5u
	UINT8 gt_max;						///< gain(th1) :1.5u
	UINT8 a2tw_en;						///< strong edge protection ,'0': disable,'1': enable
	UINT8 a2tw_th0;						///< strong edge: th0
	UINT8 a2tw_th1;						///< strong edge: th1
	UINT8 exp_mode;						///< "00" : bypass,"01" : 3-tap expansion,"10" : 5-tap expansion,"11" : bypass
	UINT8 coring_mode1;					///< coring mode "00" : remove 0,"01" : remove 0/1,"10" : remove 0/1/2,"11" : remove 0/1/2/3
	UINT8 coring_mode2;					///< coring mode,"00" : remove 0,"01" : remove 0/1,"10" : remove 0/1/2,"11" : remove 0/1/2/3
	UINT8 g_th0;						///< <edge/texture blending> th0
	UINT8 g_th1;						///< th1
	UINT16 var_th;						///< flat region rejection threshold if variance < var_th, reject the region
	/*ti-h*/
	UINT8 enable;						///< ti-h enable
	UINT8 coring_step;					///< n : 2^n (transition width)
	UINT8 gain0_en;						///< gain0 enable
	UINT8 gain1_en;						///< gain1 enable
	UINT8 gain0_th0;					///< gain0_th0
	UINT8 gain1_th1;					///< gain1_th1
	UINT8 gain1_div_mode;				///< gain1: div mode,"00" : div by 32,"01" : div by 16,"10" : div by 8,"11" : div by 64
}
LX_PE_SHP_RE2_MISC_T;

/**
 *	pe resolution enhancement(RE) misc. control parameter type
 *	ver.3, for M14Bx
 */
typedef struct
{
	LX_PE_WIN_ID win_id;				///< window id
	/* h 5 */
	UINT8 edge_y_filter_en;				///< edge_y_filter_en
	UINT8 e_gain_max;					///< a to edge weight: gain
	UINT8 f_gain_max;					///< a to flat weight: gain
	UINT8 mp_lap_h_mode;				///< "000" : h3,"001" : h4,"010" : h5,"011" : h6,"100" : h7
	UINT8 sp_lap_h_mode;				///< "00" : h3,"01" : h5,"10" : h7,"11" : not used
	/* v 5 */
	UINT8 der_v_en;						///< der-v enable
	UINT8 der_gain_mapping;				///< "00" : a-to weight,"01" : a-map,"10  : a-to weight,"11" : a-map
	UINT8 max_sel;						///< ,"000" : do not expand,"001" : 3-tap,"010" : 5-tap,"011" : 7-tap,"100" : 9-tap,"101" : 11-tap,o.w   : do not expand
	UINT8 mp_lap_v_mode;				///< laplacian v mode,"000" : v3,"001" : v4,"010" : v5,"011" : v6,"100" : v7
	UINT8 sp_lap_v_mode;				///< 0' : v3,'1' : v5
	/* cmm 6 */
	UINT8 mp_sobel_weight;				///< weight for sobel operator output
	UINT8 mp_laplacian_weight;			///< weight for laplacian operator output
	UINT8 sp_sobel_weight;				///< weight for sobel operator output
	UINT8 sp_laplacian_weight;			///< weight for laplacian operator output	
	UINT8 flat_en;						///< flat-filter enable
	UINT8 flat_filter_type;				///< '0' : bi-lateral filter,'1': average filter	
	/*d_jag 12 */
	UINT8 edf_en;						///< edge-directional de-jagging enable
	UINT8 center_blur_mode;				///< [m14b]center-pixel averaging for edge-direction calculation,0:use original pixel, 1:use avg. pixel
	UINT8 count_diff_th;				///< matchness threshod for edge-direction decision
	UINT8 n_avg_mode;					///< mode for dual edges,'0' : use 12 direction results,'1' : use 36 direction results
	UINT8 line_variation_diff_th;		///< line-variation threshold for edge-direction decision
	UINT8 level_th;						///< g0: level threshold
	UINT8 protect_th;					///< g1: protect threshold
	UINT8 n_avg_gain;					///< neighborhood pixel averaing : gain
	UINT8 reg_g0_cnt_min;				///< [m14b]G0 : edf_count_min
	UINT8 reg_g0_mul;					///< [m14b]G0 : n x ( Count - min_cnt_th )
	UINT8 reg_g1_protect_min;			///< [m14b]G1 : Min diff th - LR Diff
	UINT8 reg_g1_mul;					///< [m14b]G1 : n x ( th - LR_Diff )
	/*e_map 5 */
	UINT8 amap2_sel;					///< "00" : 5-tap,"01" : 7-tap,"10" : 9-tap,"11" : 15-tap
	UINT8 amap_gain;					///< edge-map gain
	UINT8 ga_max;						///< <a to edge weight control>
	UINT8 ga_th0;						///< th0
	UINT8 ga_th1;						///< th1
	/*t_map 22*/
	UINT8 amap1_sel;					///< "00" : 15-tap,"01" : 9-tap,"10" : 7-tap,"11" : 5-tap
	UINT8 tmap_max_sel;					///< "000" : bypass,"001" : 5-tap,"010" : 7-tap,"011" : 9-tap,"100" : 11-tap,"101" : 13-tap,"110" : 15-tap,"111" : 17-tap
	UINT8 avg_sel;						///< "00" : bypass,"01" : 3x1 average [1 2 1],"10" : 5x1 average [1 2 2 2 1 ],"11" : bypass
	UINT8 tmap_gain;					///< texture-map gain(2.5u)
	UINT8 gt_th0;						///< th0
	UINT8 gt_th0a;						///< th0a
	UINT8 gt_th0b;						///< th0b
	UINT8 gt_th1;						///< th1
	UINT8 gt_gain0a;					///< gain(th0a) : 1.5u
	UINT8 gt_gain0b;					///< gain(th0b) : 1.5u
	UINT8 gt_max;						///< gain(th1) :1.5u
	UINT8 a2tw_en;						///< strong edge protection ,'0': disable,'1': enable
	UINT8 a2tw_th0;						///< strong edge: th0
	UINT8 a2tw_th1;						///< strong edge: th1
	UINT8 exp_mode;						///< "00" : bypass,"01" : 3-tap expansion,"10" : 5-tap expansion,"11" : bypass
	UINT8 coring_mode1;					///< coring mode "00" : remove 0,"01" : remove 0/1,"10" : remove 0/1/2,"11" : remove 0/1/2/3
	UINT8 coring_mode2;					///< coring mode,"00" : remove 0,"01" : remove 0/1,"10" : remove 0/1/2,"11" : remove 0/1/2/3
	UINT8 g_th0;						///< <edge/texture blending> th0
	UINT8 g_th1;						///< th1
	UINT8 var_h_th;						///< [m14b]flat region rejection threshold (H-Direction T-Map)if variance < var_th, reject the region
	UINT8 var_v_th;						///< [m14b]flat region rejection threshold (V-Direction T-Map)if variance < var_th, reject the region
	UINT8 tmap_sc_var_th;				///< [m14b]flat region rejection threshold (for SC T-Map)if variance < var_th, reject the region
	/*ti-h 7 */
	UINT8 enable;						///< ti-h enable
	UINT8 coring_step;					///< n : 2^n (transition width)
	UINT8 gain0_en;						///< gain0 enable
	UINT8 gain1_en;						///< gain1 enable
	UINT8 gain0_th0;					///< gain0_th0
	UINT8 gain0_th1;					///< [m14b]gain0_th1
	UINT8 gain1_div_mode;				///< gain1: div mode,"00" : div by 32,"01" : div by 16,"10" : div by 8,"11" : div by 64
	/* cti 8 */
	UINT8 cti_en;						///< cti enable
	UINT8 coring_th0;					///< th with coring th0 mmd
	UINT8 coring_th1;					///< coring_th1
	UINT8 coring_map_filter;			///< "000" : no ,"001" : 5-tap ,"010" : 7-tap ,"011" : 9-tap ,100: : 11-tap ,"101" : 13-tap 
	UINT8 coring_tap_size;				///< "000" 21 tap,"001" 19 tap,"010" 17 tap,"011" 15 tap,"100" 13 tap,"101" 11 tap,"110"  9 tap
	UINT8 ycm_en1;						///< y c miss matching.
	UINT8 ycm_band_sel;					///< ycm_band_sel
	UINT8 ycm_diff_th;					///< ycm_diff_th
}
LX_PE_SHP_RE3_MISC_T;

/**
 *	pe super resolution enhancement(SRE) control parameter type
 *	ver.0, for M14Bx
 *	- reg_out_mux_sel : 0x0:Bypass, 0x1:Weighting output,
 *					0x2,0x4,0x8,0x10,0x20,0x40:etc
 *	- reg_sc_ls_lv_sra : each value converts to 4bit(1 hex) in order arranged,
 *					Default: [8,8,12,15,15,15,14,14]
 *	- reg_sc_ls_lv_srb : each value converts to 4bit(1 hex) in order arranged,
 *					Default: [4,4,10,15,15,15,10,10]
 */
typedef struct
{
	LX_PE_WIN_ID win_id;		///< window id
	UINT32 reg_out_mux_sel;		///< SRE Ouput selection, 
	UINT32 reg_sc_gs_sra;		///< [0-127] Global scale of sra
	UINT32 reg_sc_gs_srb;		///< [0-127] Global scale of srb
	UINT32 reg_sc_ls_lv_sra;	///< [0-15]: local scale level of sra
	UINT32 reg_sc_ls_lv_srb;	///< [0-15]: local scale level of srb
	UINT32 reg_mode_wei;		///< [0-7]: wei_sra, 0:only srb,1:only sra
}
LX_PE_SHP_SR0_T;

/***************************************************************************/
/* ETC : misc. */
/***************************************************************************/
/**
 *	pe misc. table parameter type
 */
typedef struct
{
	UINT32 func_num;			///< func number(depending on chip version)
	UINT32 size;				///< the number of data
	UINT32 *data;				///< user data
}
LX_PE_ETC_TBL_T;

/***************************************************************************/
/* ETC : info. */
/***************************************************************************/
/**
 *	pe operation mode
 */
typedef struct
{
	UINT32 is_tp        : 1;	///< 0, test pattern mode, 1:yes,0:no
	UINT32 is_venc      : 1;	///< 1, video encoder mode, 1:yes,0:no
	UINT32 is_adpt_st   : 1;	///< 2, adaptive stream mode, 1:yes,0:no
	UINT32 is_ext_frc   : 1;	///< 3, external frc,3dfmt mode, 1:yes,0:no
	UINT32 is_wb_wog    : 1;	///< 4, wb without degamma,gamma, 1:yes,0:no(with)
	UINT32 is_reverse   : 1;	///< 5, reverse mode, 1:yes,0:no(default)
	UINT32 is_oled      : 1;	///< 6, oled, 1:yes,0:no(default)
}
LX_PE_INF_OP_MODE_T;

/**
 * pe window info.(offset, size)
 */
typedef struct
{
	UINT32 x_ofst;			///< x offset
	UINT32 y_ofst;			///< y offset
	UINT32 h_size;			///< horizontal size
	UINT32 v_size;			///< vertical size
}
LX_PE_INF_WIN_T;

/**
 *	pe display info.
 */
typedef struct
{
	LX_PE_WIN_ID win_id;			///< window id
	LX_PE_SRC_TYPE src_type;		///< pe source type
	LX_PE_FMT_TYPE fmt_type;		///< pe format type
	UINT32 in_h_size;				///< input horizontal active size
	UINT32 in_v_size;				///< input vertical active size
	LX_PE_FR_TYPE fr_type;			///< pe frame rate type
	UINT32 in_f_rate;				///< input frame rate(x10 hz)
	LX_PE_SCAN_TYPE scan_type;		///< pe scan type
	LX_PE_CSTD_TYPE cstd_type;		///< pe color standard type
	LX_PE_HDMI_TYPE hdmi_type;		///< pe hdmi type
	LX_PE_SCART_TYPE scart_type;	///< pe scart type
	LX_PE_DTV_TYPE dtv_type;		///< pe dtv play type
	LX_PE_HDD_SRC_TYPE hdd_type;	///< hdd src type
	LX_PE_OUT_TYPE out_type;		///< pe out type
	LX_PE_3D_IN_TYPE in_type;		///< 3d input type
	LX_PE_OUT_TYPE user_o_type;		///< user(customer) out type
	LX_PE_3D_IN_TYPE user_i_type;	///< user(customer) 3d input type
	LX_PE_INF_WIN_T in_win;			///< input window
	LX_PE_INF_WIN_T out_win;		///< output window
	LX_PE_INF_OP_MODE_T mode;		///< operation mode
}
LX_PE_INF_DISPLAY_T;

/**
 *	pe level info.
 */
typedef struct
{
	LX_PE_WIN_ID win_id;			///< window id
	LX_PE_LEVEL_TYPE sel;			///< pe level select. see LX_PE_LEVEL_TYPE
	UINT32 noise_level;				///< pe tnr noise level
	UINT32 peakpos_level;			///< pe tnr peak pos level
	UINT32 motion_level;			///< pe motion level
}
LX_PE_INF_LEVEL_T;

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PE_KAPI_H_ */

