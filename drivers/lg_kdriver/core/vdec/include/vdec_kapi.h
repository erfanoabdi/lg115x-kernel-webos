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
 *  application interface header for vdec device
 *
 *  author		seokjoo.lee (seokjoo.lee@lge.com)
 *  version		0.75
 *  date		2010.07.26
 *
 *  @addtogroup lg1150_vdec
 * @{
 */

#ifndef	_VDEC_KAPI_H_
#define	_VDEC_KAPI_H_


#define	LX_VDEC_IOC_MAGIC		'v'


/**
 * Video Codec Type.
 * decoding formats supported by the Video decoder.
 */
typedef enum
{
	LX_VDEC_CODEC_INVALID = -1,

	LX_VDEC_CODEC_H264_HP,
	LX_VDEC_CODEC_H264_MVC,
	LX_VDEC_CODEC_H263_SORENSON,
	LX_VDEC_CODEC_VC1_RCV_V1,
	LX_VDEC_CODEC_VC1_RCV_V2,
	LX_VDEC_CODEC_VC1_ES,
	LX_VDEC_CODEC_MPEG2_HP,
	LX_VDEC_CODEC_MPEG4_ASP,
	LX_VDEC_CODEC_XVID,
	LX_VDEC_CODEC_DIVX3,
	LX_VDEC_CODEC_DIVX4,
	LX_VDEC_CODEC_DIVX5,
	LX_VDEC_CODEC_RVX,
	LX_VDEC_CODEC_AVS,
	LX_VDEC_CODEC_VP8,
	LX_VDEC_CODEC_THEORA,
	LX_VDEC_CODEC_HEVC,
} LX_VDEC_CODEC_T;

typedef enum
{
	/** Video decoder input can be selected from system decoder 0. */
	LX_VDEC_SRC_SDEC0,

	/** Video decoder input can be selected from system decoder 1. */
	LX_VDEC_SRC_SDEC1,

	/** Video decoder input can be selected from system decoder 1. */
	LX_VDEC_SRC_SDEC2,

	/** Video decoder input is manually controlled by caller. */
	LX_VDEC_SRC_BUFF,

	/** Video decoder input is manually controlled by caller. */
	LX_VDEC_SRC_BUFFTVP,
} LX_VDEC_SRC_T;


typedef enum
{
	/**
	 * Video decoder output shall be directed to Main Window of DE(Display
	 * Engine) */
	LX_VDEC_DST_DE0,

	/**
	 * Video decoder output shall be directed to Sub Window of DE(Display
	 * Engine) */
	LX_VDEC_DST_DE1,

	/**
	 * Video decoder output shall be remains to DPB (Decoded Picture
	 * Buffer) not automatically passed to DE.  use this for thumbnail
	 * output. */
	LX_VDEC_DST_BUFF,
} LX_VDEC_DST_T;




typedef enum
{
	/**
	 * Video decoder will be operated on the broadcast/DVR/file play
	 * configuration
	 * */
	LX_VDEC_OPMOD_NORMAL = 0,

	/**
	 * Video decoder will be operated on the channel browser , thumbnail
	 * and drip decoding configuration */
	LX_VDEC_OPMOD_ONE_FRAME,
	xxLX_VDEC_OPMOD_DUALxx,
	LX_VDEC_OPMOD_LOW_LATENCY,
} LX_VDEC_OPMODE_T;

#define LX_VDEC_IO_FLUSH			_IO (LX_VDEC_IOC_MAGIC,  5)

#define LX_VDEC_IO_GET_BUFFER_STATUS		_IOR(LX_VDEC_IOC_MAGIC,  7, LX_VDEC_IO_BUFFER_STATUS_T)
#define LX_VDEC_IO_UPDATE_BUFFER		_IOW(LX_VDEC_IOC_MAGIC,  8, LX_VDEC_IO_UPDATE_BUFFER_T)


#define LX_VDEC_UNREF_DECODEBUFFER		_IOW (LX_VDEC_IOC_MAGIC, 17, unsigned long)
#define LX_VDEC_SET_DECODING_QUEUE_SIZE		_IOW (LX_VDEC_IOC_MAGIC, 18, int)
#define LX_VDEC_GET_DECODED_QUEUE_SIZE		_IOR (LX_VDEC_IOC_MAGIC, 19, int)
#define LX_VDEC_SET_OUTPUT_MEMORY_FORMAT	_IOW (LX_VDEC_IOC_MAGIC, 20, LX_VDEC_MEMORY_FORMAT_T)
#define LX_VDEC_SET_SCAN_PICTURE		_IO  (LX_VDEC_IOC_MAGIC, 21)
#define LX_VDEC_SET_CODEC			_IO  (LX_VDEC_IOC_MAGIC, 23)
#define LX_VDEC_SET_DISPLAY_OFFSET		_IO  (LX_VDEC_IOC_MAGIC, 24)
#define LX_VDEC_SET_INPUT_DEVICE		_IO  (LX_VDEC_IOC_MAGIC, 25)
#define LX_VDEC_SET_OUTPUT_DEVICE		_IO  (LX_VDEC_IOC_MAGIC, 26)
#define LX_VDEC_SET_BASETIME			_IOW (LX_VDEC_IOC_MAGIC, 27, LX_VDEC_BASETIME_T)
#define LX_VDEC_GET_GLOBAL_STC			_IOR (LX_VDEC_IOC_MAGIC, 28, LX_VDEC_STC_T)
#define LX_VDEC_SET_ID				_IO  (LX_VDEC_IOC_MAGIC, 29)
#define LX_VDEC_STEAL_USERDATA			_IO  (LX_VDEC_IOC_MAGIC, 30)
#define LX_VDEC_SET_LOW_LATENCY			_IO  (LX_VDEC_IOC_MAGIC, 31)
#define LX_VDEC_SET_3D_TYPE			_IO  (LX_VDEC_IOC_MAGIC, 32)
#define LX_VDEC_GET_CPB_INFO			_IOR (LX_VDEC_IOC_MAGIC, 33, LX_VDEC_CPB_INFO_T)
#define LX_VDEC_SET_CPB				_IOW (LX_VDEC_IOC_MAGIC, 34, LX_VDEC_BUFFER_T)
#define LX_VDEC_SET_REQUEST_PICTURES		_IO  (LX_VDEC_IOC_MAGIC, 35)
#define LX_VDEC_RESET				_IO  (LX_VDEC_IOC_MAGIC, 36)
#define LX_VDEC_USE_GSTC			_IO  (LX_VDEC_IOC_MAGIC, 37)
#define LX_VDEC_SET_FLAGS			_IO  (LX_VDEC_IOC_MAGIC, 38)
#define LX_VDEC_DISPLAY_FREEZE			_IO  (LX_VDEC_IOC_MAGIC, 39)
#define LX_VDEC_DISPLAY_SYNC			_IO  (LX_VDEC_IOC_MAGIC, 40)
#define LX_VDEC_SET_SPEED			_IO  (LX_VDEC_IOC_MAGIC, 41)
#define LX_VDEC_SET_FRAMERATE			_IO  (LX_VDEC_IOC_MAGIC, 42)
#define LX_VDEC_SET_FRAMEBUFFER			_IOW (LX_VDEC_IOC_MAGIC, 43, LX_VDEC_FB_T)
#define LX_VDEC_SET_STEP			_IO  (LX_VDEC_IOC_MAGIC, 44)
#define LX_VDEC_INIT				_IO  (LX_VDEC_IOC_MAGIC, 45)
#define LX_VDEC_CONVERT_FRAME			_IOW (LX_VDEC_IOC_MAGIC, 46, LX_VDEC_CONVERT_FRAME_T)
#define LX_VDEC_EOS				_IO  (LX_VDEC_IOC_MAGIC, 47)
#define LX_VDEC_SET_ADEC_CHANNEL		_IO  (LX_VDEC_IOC_MAGIC, 48)
#define LX_VDEC_ADD_FLAGS			_IO  (LX_VDEC_IOC_MAGIC, 49)
#define LX_VDEC_DEL_FLAGS			_IO  (LX_VDEC_IOC_MAGIC, 50)
#define LX_VDEC_DESTROY				_IO  (LX_VDEC_IOC_MAGIC, 51)
#define LX_VDEC_SET_CHANNEL_NUMBER		_IO  (LX_VDEC_IOC_MAGIC, 52)

/** @} */



typedef enum
{
	LX_VDEC_MEMORY_FORMAT_RASTER,
	LX_VDEC_MEMORY_FORMAT_TILED,
} LX_VDEC_MEMORY_FORMAT_T;

typedef struct
{
	unsigned int base_stc;
	unsigned int base_pts;
} LX_VDEC_BASETIME_T;

typedef struct
{
	unsigned int stc;
	unsigned int mask;
} LX_VDEC_STC_T;

typedef struct
{
	unsigned long addr;
	int size;
	int read_offset;
	int write_offset;
} LX_VDEC_CPB_INFO_T;

typedef struct
{
	unsigned long addr;
	int size;
} LX_VDEC_BUFFER_T;


/**
 * Buffer status for VDEC input buffer ( CPB )
 * used LX_VDEC_IO_GET_BUFFER_STATUS
 */
typedef struct
{
	unsigned int	cpb_depth;
	unsigned int	cpb_size;
	unsigned int	auib_depth;
	unsigned int	auib_size;
} LX_VDEC_IO_BUFFER_STATUS_T;

/**
 * Video Decoder Access Unit Type.
 * For update buffer write pointer for which type of this chunk of memory contains.
 * for LX_VDEC_IO_UPDATE_BUFFER_T::au_type
 */
typedef enum
{
	LX_VAU_SEQH = 1,	///< Sequence Header
	LX_VAU_SEQE,		///< Sequence End.
	LX_VAU_DATA,		///< Picture Data.
} LX_VAU_T;

/**
 * For update buffer write pointer for which type of this chunk of memory contains.
 * [NOTE] au_size should be 512 bytes unit.
 */
typedef struct
{
	unsigned int UId;		///< Unique ID
	LX_VAU_T au_type;		///< access unit type of this chunk memory.
	const unsigned char *au_ptr;	///< access unit pointer. in physical address.
	unsigned int au_size;		///< writing size should be multiple of 512 bytes.
#define	VDEC_UNKNOWN_TIMESTAMP		0xFFFFFFFFFFFFFFFFLL
	unsigned long long timestamp;	///< time stamp 1 ns.
} LX_VDEC_IO_UPDATE_BUFFER_T;


typedef enum
{
	lx_vdec_flags_pollerr	= (1<<0),

	/* vdc should copy DPB where user or I supplies. lx_vdec_flags_user_dpb
	 * also needed */
	lx_vdec_flags_copy_dpb	= (1<<1),

	/* user(middleware) will supply the DPB memory. no unref the displayed
	 * frame will be called */
	lx_vdec_flags_user_dpb	= (1<<2),

	/* UHD seemless mode. UHD DPB should be allocaded for small size video.
	 */
	lx_vdec_flags_uhd_seemless = (1<<3),

	/* adec master mode on local clock mode. */
	lx_vdec_flags_adec_master = (1<<4),
} lx_vdec_flags_t;

typedef struct
{
	/* memory from ion */
	int ion_y;
	unsigned long priv;

	/* memory from specific physical address */
	unsigned long y, cb, cr;

	/* color format */
	enum
	{
		fb_color_format_420,
	} color_format;
	enum
	{
		/* planner Y and following interleaved Cb and Cr */
		fb_buffer_format_y_ic,
	} buffer_format;

	/* buffer size */
	int width, height, stride;
} LX_VDEC_FB_T;

typedef struct
{
	/* buffer y address */
	unsigned long y;

	/* user buffer that the buffer will stored */
	unsigned char *result;
} LX_VDEC_CONVERT_FRAME_T;





/**
 * scan mode of video decoder.
 * only specified picture type of input data, others shall be skipped.\n
 */
typedef enum
{
	LX_VDEC_PIC_SCAN_ALL,			///< decode IPB frame.
	LX_VDEC_PIC_SCAN_I,			///< I picture only (PB skip)
	LX_VDEC_PIC_SCAN_IP,			///< IP picture only (B skip only)

	/* dish CP use top only frame on trick mode */
	LX_VDEC_PIC_SCAN_I_BRAINFART_DISH_TRICK,
} LX_VDEC_PIC_SCAN_T;

typedef enum
{
	LX_VDEC_DISPLAY_FREEZE_UNSET,		///< freeze unset.
	LX_VDEC_DISPLAY_FREEZE_SET,		///< freeze set.
} LX_VDEC_DISPLAY_FREEZE_T;

typedef enum
{
	LX_VDEC_DISPLAY_SYNC_MATCH,
	LX_VDEC_DISPLAY_SYNC_FREERUN,
} LX_VDEC_DISPLAY_SYNC_T;








#define LX_VDEC_READ_MAGIC(s)		(s[0]<<0|s[1]<<8|s[2]<<16|s[3]<<24)
#define LX_VDEC_READ_MAGIC2(s0,s1,s2,s3)	(s0<<0|s1<<8|s2<<16|s3<<24)

typedef struct decoded_buffer decoded_buffer_t;
struct decoded_buffer
{
	unsigned long addr_y;
	unsigned long addr_cb, addr_cr;
	//unsigned long addr_y_bot;	// bottom
	//unsigned long addr_cb_bot, addr_cr_bot;

	int buffer_index;
	unsigned long user_dpb_priv;

	unsigned long addr_tile_base;

	/* 0 for linear
	 * other for tiled. */
	int vdo_map_type;

	int error_blocks;

	/**
	 * @DECODED_REPORT_LOW_DELAY: there is low_delay option on sequence
	 * header.
	 *
	 * @DECODED_REPORT_HW_RESET: require hardware reset. not used.
	 *
	 * @DECODED_REPORT_EOS: EOS action has been completed after
	 * ioctl(LX_VDEC_EOS)
	 *
	 * @DECODED_REPORT_REQUEST_DPB: require DPB memory after sequence init.
	 * only available with lx_vdec_flags_user_dpb flag set.
	 *
	 * @DECODED_REPORT_SEQUENCE_INIT_FAILED: sequence init failed. broken
	 * sequence data or no sequence data.  cannot proceed further decoding.
	 *
	 * @DECODED_REPORT_DECODE_FAILED: frame decoding failed.  but, decoding
	 * can be proceeding.
	 *
	 * @DECODED_REPORT_NOT_SUPPORTED_STREAM: this hardware cannot decode
	 * this video ES. out of hardware spec.
	 *
	 * @DECODED_REPORT_RESOURCE_BUSY: hardware resource is insufficient to
	 * decode the video ES. after closing some other decoder instance,
	 * decoder may decode the ES.
	 *
	 * @DISPLAY_REPORT_DROPPED: this frame has been dropped and did not
	 * displayed.
	 */
	enum
	{
		DECODED_REPORT_LOW_DELAY		= (1<<0),
		DECODED_REPORT_HW_RESET			= (1<<1),
		DECODED_REPORT_EOS			= (1<<2),
		DECODED_REPORT_REQUEST_DPB		= (1<<3),

		/* error case, bit8 ~ bit15 */
#define DECODED_REPORT_ERROR_MASK	0xff00
		DECODED_REPORT_SEQUENCE_INIT_FAILED	= (1<<8),
		DECODED_REPORT_DECODE_FAILED		= (1<<9),
		DECODED_REPORT_NOT_SUPPORTED_STREAM	= (1<<10),
		DECODED_REPORT_RESOURCE_BUSY		= (1<<11),

		DISPLAY_REPORT_DROPPED			= (1<<16),
	} report;

	int framerate_num;
	int framerate_den;

	int crop_left, crop_right;
	int crop_top, crop_bottom;

	int stride;
	int width;
	int height;

	int display_width;
	int display_height;

	enum
	{
		decoded_buffer_interlace_top_first,
		decoded_buffer_interlace_bottom_first,
		decoded_buffer_interlace_none,		// progressive

		/* DISH CP only */
		decoded_buffer_interlace_top_only,
		decoded_buffer_interlace_bottom_only,
	} interlace;

	enum
	{
		decoded_buffer_picture_type_i,
		decoded_buffer_picture_type_p,
		decoded_buffer_picture_type_b,
		decoded_buffer_picture_type_bi,
		decoded_buffer_picture_type_d,
		decoded_buffer_picture_type_s,
		decoded_buffer_picture_type_pskip,
	} picture_type;
	int display_period;

	/*
	 * AFD in MPEG video. See
	 * - http://en.wikipedia.org/wiki/Active_Format_Description
	 * - http://webapp.etsi.org/workprogram/Report_WorkItem.asp?WKI_ID=21480
	 */
	unsigned char active_format;

	int ui8Mpeg2Dar;
	int par_w, par_h;

	/*
	 * FPA(Frame Packing Arrangement) in MPEG4 video. See
	 * - http://en.wikipedia.org/wiki/H.264/MPEG-4_AVC
	 */
	int frame_packing_arrangement;

	unsigned int stc_discontinuity;
	unsigned int dts;
	unsigned int pts;
	long long timestamp;

	enum
	{
		decoded_buffer_multi_picture_left,
		decoded_buffer_multi_picture_right,
		decoded_buffer_multi_picture_none,
	} multi_picture;

	unsigned long uid;

	void *user_data_addr;
	int user_data_size;
	int top_field_first;
	int repeat_first_field;

	int num_of_buffer_required;
};




/** @} */


#endif /* _VDEC_DRV_H_ */

/** @} */
/* vim:set ts=8: */
