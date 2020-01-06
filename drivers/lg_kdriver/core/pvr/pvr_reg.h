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
 *  register access interface for the LG1150 hardware pvr
 *
 *  author		Murugan Durairaj (murugan.d@lge.com)
 *  author		modified by ki beom kim (kibeom.kim@lge.com)
 *  version		1.1
 *  date		2010.03.20
 *
 *  @addtogroup lg1150_pvr
 *	@{
 */

#ifndef _DVR_REG_h
#define _DVR_REG_h

#define PVR_OLD_REG 0

// for LG1154
#define H13_DVR_REG_BASE	(H13_TE_BASE + 0x1400)
#define M14_A0_DVR_REG_BASE	(M14_TE_BASE + 0x1400)
#define M14_B0_DVR_REG_BASE	(M14_B0_TE_BASE + 0x1400)
#define H14_DVR_REG_BASE	(H14_TE_BASE + 0x1400)

typedef union  { // 0x2604
	UINT32 ui32Val;
	struct {
		UINT32 almost1         : 1;   // 0       0
		UINT32 rep1            : 1;   // 1       1
		UINT32 up_err1         : 1;   // 2       2
		UINT32                 : 1;   // 3       3
		UINT32 almost          : 1;   // 4       4
		UINT32 rep             : 1;   // 5       5
		UINT32 up_err          : 1;   // 6       6
		UINT32                 : 1;   // 7       7
		UINT32 unit_buf1       : 1;   // 8       8
		UINT32 dn_err1         : 1;   // 9       9
		UINT32 sync_err1       : 1;   // 10      10
		UINT32                 : 1;   // 11      11
		UINT32 unit_buf        : 1;   // 12      12
		UINT32 dn_err          : 1;   // 13      13
		UINT32 sync_err        : 1;   // 14      14
		UINT32                 : 1;   // 15      15
		UINT32 scd1            : 1;   // 16      16
		UINT32 pit_err1        : 1;   // 17      17
		UINT32                 : 2;   // 19:18   18
		UINT32 scd             : 1;   // 20      20
		UINT32 pit_err         : 1;   // 21      21
		UINT32                 : 10;  // 31:22   22
	}le;
} INTR_EN_T;

typedef union  { // 0x2608
	UINT32 ui32Val;
	struct {
		UINT32 almost1         : 1;   // 0       0
		UINT32 rep1            : 1;   // 1       1
		UINT32 up_err1         : 1;   // 2       2
		UINT32                 : 1;   // 3       3
		UINT32 almost          : 1;   // 4       4
		UINT32 rep             : 1;   // 5       5
		UINT32 up_err          : 1;   // 6       6
		UINT32                 : 1;   // 7       7
		UINT32 unit_buf1       : 1;   // 8       8
		UINT32 dn_err1         : 1;   // 9       9
		UINT32 sync_err1       : 1;   // 10      10
		UINT32                 : 1;   // 11      11
		UINT32 unit_buf        : 1;   // 12      12
		UINT32 dn_err          : 1;   // 13      13
		UINT32 sync_err        : 1;   // 14      14
		UINT32                 : 1;   // 15      15
		UINT32 scd1            : 1;   // 16      16
		UINT32 pit_err1        : 1;   // 17      17
		UINT32                 : 2;   // 19:18   18
		UINT32 scd             : 1;   // 20      20
		UINT32 pit_err         : 1;   // 21      21
		UINT32                 : 10;  // 31:22   22
	}le;
} INTR_STAT_T;

typedef union  { // 0x260c
	UINT32 ui32Val;
	struct {
		UINT32 almost1         : 1;   // 0       0
		UINT32 rep1            : 1;   // 1       1
		UINT32 up_err1         : 1;   // 2       2
		UINT32                 : 1;   // 3       3
		UINT32 almost          : 1;   // 4       4
		UINT32 rep             : 1;   // 5       5
		UINT32 up_err          : 1;   // 6       6
		UINT32                 : 1;   // 7       7
		UINT32 unit_buf1       : 1;   // 8       8
		UINT32 dn_err1         : 1;   // 9       9
		UINT32 sync_err1       : 1;   // 10      10
		UINT32                 : 1;   // 11      11
		UINT32 unit_buf        : 1;   // 12      12
		UINT32 dn_err          : 1;   // 13      13
		UINT32 sync_err        : 1;   // 14      14
		UINT32                 : 1;   // 15      15
		UINT32 scd1            : 1;   // 16      16
		UINT32 pit_err1        : 1;   // 17      17
		UINT32                 : 2;   // 19:18   18
		UINT32 scd             : 1;   // 20      20
		UINT32 pit_err         : 1;   // 21      21
		UINT32                 : 10;  // 31:22   22
	}le;
} INTR_RSTAT_T;

typedef union  { // 0x2700
	UINT32 ui32Val;
	struct {
		UINT32                 : 24;  // 23:1    1
		UINT32 iack            : 1;   // 24      24
		UINT32                 : 3;   // 27:25   25
		UINT32 vd_rst          : 1;   // 28      28
		UINT32                 : 3;   // 31:29   29
	}le;
} PIE_CONF_A_T;

typedef union  { // 0x2704
	UINT32 ui32Val;
	struct {
		UINT32 pie_mode        : 2;   // 1:0     0
		UINT32 astc_ctr        : 1;   // 2       2
		UINT32 gscd_en         : 1;   // 3       3
		UINT32 scd0_bsel       : 1;   // 7:3     4
		UINT32 scd1_bsel       : 1;   // 7:3     5
		UINT32 scd2_bsel       : 1;   // 7:3     6
		UINT32 scd3_bsel       : 1;   // 7:3     7
		UINT32 strm_id         : 4;   // 11:8    8
		UINT32 strm_sel        : 1;   // 12      12
		UINT32 plen_chk        : 1;   // 13      13
		UINT32                  : 2;  // 15:14   14
		UINT32 gscd_filter   	: 4;   //17:14   16
		UINT32                  : 12;  // 31:20   20
	}le;
} PIE_MODE_T;

typedef union  { // 0x2710
	UINT32 ui32Val;
	struct {
		UINT32 pind_cnt        : 6;   // 5:0     0
		UINT32                 : 26;  // 31:6    6
	}le;
} PIE_STAT_A_T;

typedef union  { // 0x2714
	UINT32 ui32Val;
	struct {
		UINT32                 : 1;   // 0       0
		UINT32 mpg1_sys        : 1;   // 1       1
		UINT32                 : 14;  // 15:2    2
		UINT32 pesdec_state    : 4;   // 19:16   16
		UINT32                 : 12;  // 31:20   20
	}le;
} PIE_PES_STAT_A_T;

typedef union  { // 0x2718
	UINT32 ui32Val;
	struct {
		UINT32 pit             : 1;   // 0       0
		UINT32                 : 31;  // 31:1    1
	}le;
} PIE_ERR_STAT_A_T;

typedef union  { // 0x2720
	UINT32 ui32Val;
	struct {
		UINT32 scd_value       : 32;  // 31:0    0
	}le;
} PIE_SCD_VALUE_A_T;

typedef union  { // 0x2730
	UINT32 ui32Val;
	struct {
		UINT32 scd_mask        : 32;  // 31:0    0
	}le;
} PIE_SCD_MASK_A_T;

typedef union  { // 0x2780
	UINT32 ui32Val;
	struct {
		UINT32 pack_cnt        : 14;  // 13:0    0
		UINT32 buf_num         : 5;   // 18:14   14
		UINT32 maxp            : 1;   // 19      19
		UINT32 bdet            : 1;   // 20      20
		UINT32 pdet            : 1;   // 21      21
		UINT32 idet            : 1;   // 22      22
		UINT32 sdet            : 1;   // 23      23
		UINT32 byte_info       : 8;   // 31:24   24
	}le;
} PIE_IND_A_T;

typedef union  { // 0x2800
	UINT32 ui32Val;
	struct {
		UINT32                 : 24;  // 23:1    1
		UINT32 iack            : 1;   // 24      24
		UINT32                 : 3;   // 27:25   25
		UINT32 vd_rst          : 1;   // 28      28
		UINT32                 : 3;   // 31:29   29
	}le;
} PIE_CONF_B_T;

typedef union  { // 0x2810
	UINT32 ui32Val;
	struct {
		UINT32 pind_cnt        : 6;   // 5:0     0
		UINT32                 : 26;  // 31:6    6
	}le;
} PIE_STAT_B_T;

typedef union  { // 0x2814
	UINT32 ui32Val;
	struct {
		UINT32                 : 1;   // 0       0
		UINT32 mpg1_sys        : 1;   // 1       1
		UINT32                 : 14;  // 15:2    2
		UINT32 pesdec_state    : 4;   // 19:16   16
		UINT32                 : 12;  // 31:20   20
	}le;
} PIE_PES_STAT_B_T;

typedef union  { // 0x2818
	UINT32 ui32Val;
	struct {
		UINT32 pit             : 1;   // 0       0
		UINT32                 : 31;  // 31:1    1
	}le;
} PIE_ERR_STAT_B_T;

typedef union  { // 0x2820
	UINT32 ui32Val;
	struct {
		UINT32 scd_value       : 32;  // 31:0    0
	}le;
} PIE_SCD_VALUE_B_T;

typedef union  { // 0x2830
	UINT32 ui32Val;
	struct {
		UINT32 scd_mask        : 32;  // 31:0    0
	}le;
} PIE_SCD_MASK_B_T;

typedef union  { // 0x2880
	UINT32 ui32Val;
	struct {
		UINT32 pack_cnt        : 14;  // 13:0    0
		UINT32 buf_num         : 5;   // 18:14   14
		UINT32 maxp            : 1;   // 19      19
		UINT32 bdet            : 1;   // 20      20
		UINT32 pdet            : 1;   // 21      21
		UINT32 idet            : 1;   // 22      22
		UINT32 sdet            : 1;   // 23      23
		UINT32 byte_info       : 8;   // 31:24   24
	}le;
} PIE_IND_B_T;

typedef union  { // 0x2900
	UINT32 ui32Val;
	struct {
		UINT32 bypass_vfe      : 1;   // 0       0
		UINT32 vid_type        : 1;   // 1       1
		UINT32 type            : 1;   // 2       2
		UINT32                 : 29;  // 31:3    3
	}le;
} DN_CONF_A_T;

typedef union  { // 0x2904
	UINT32 ui32Val;
	struct {
		UINT32 record          : 1;   // 0       0
		UINT32 paus_rec        : 1;   // 1       1
		UINT32 l_cnt_rst       : 1;   // 2       2
		UINT32                 : 21;  // 23:3    3
		UINT32 w_sel_dn        : 2;   // 25:24   24
		UINT32                 : 2;   // 27:26   26
		UINT32 reset_sw        : 1;   // 28      28
		UINT32                 : 3;   // 31:29   29
	}le;
} DN_CTRL_A_T;

typedef union  { // 0x2908
	UINT32 ui32Val;
	struct {
		UINT32 pid             : 13;  // 12:0    0
		UINT32                 : 19;  // 31:13   13
	}le;
} DN_VPID_A_T;

typedef union  { // 0x290c
	UINT32 ui32Val;
	struct {
		UINT32 pkt_cnt_lim     : 14;  // 13:0    0
		UINT32                 : 2;   // 15:14   14
		UINT32 buf_num_lim     : 5;   // 20:16   16
		UINT32                 : 11;  // 31:21   21
	}le;
} DN_BUF_LIM_A_T;

typedef union  { // 0x2910
	UINT32 ui32Val;
	struct {
		UINT32 pkt_cnt         : 14;  // 13:0    0
		UINT32                 : 2;   // 15:14   14
		UINT32 buf_num         : 5;   // 20:16   16
		UINT32                 : 11;  // 31:21   21
	}le;
} DN_BUF_INFO_A_T;

typedef union  { // 0x2914
	UINT32 ui32Val;
	struct {
		UINT32 stc_resolution  : 7;   // 6:0     0
		UINT32 stc_src         : 1;   // 7       7
		UINT32                 : 24;  // 31:8    8
	}le;
} DN_STC_CTRL_A_T;

typedef union  { // 0x2918
	UINT32 ui32Val;
	struct {
		UINT32 unused   : 12;  // 12:0    0
		UINT32 sptr     : 20;  // 28:16   16
	}le;
} DVR_BUF_SPTR_T;

typedef union  { // 0x2918
	UINT32 ui32Val;
	struct {
		UINT32 unused   : 12;  // 12:0    0
		UINT32 eptr     : 20;  // 28:16   16
	}le;
} DVR_BUF_EPTR_T;

typedef union  { // 0x2918
	UINT32 ui32Val;
	struct {
		UINT32 unused   : 12;  // 12:0    0
		UINT32 level     : 20;  // 28:16   16
	}le;
} DVR_BUF_LEVEL_T;


typedef union  { // 0x2918
	UINT32 ui32Val;
	struct {
		UINT32 dn_buf_eptr     : 16;  // 12:0    0
		UINT32 dn_buf_sptr     : 16;  // 28:16   16
	}le;
} DN_BUF_BOUND_A_T;

typedef union  { // 0x291c
	UINT32 ui32Val;
	struct {
		UINT32                 : 3;   // 2:0     0
#if PVR_OLD_REG
		UINT32 dn_buf_wptr     : 25;  // 24:3    3
		UINT32                 : 4;   // 31:25   25
#else
		UINT32 dn_buf_wptr	   : 29;  // 24:3	 3
#endif
	}le;
} DN_BUF_WPTR_A_T;

typedef union  { // 0x2940
	UINT32 ui32Val;
	struct {
		UINT32 mpg_key0        : 32;  // 31:0    0
	}le;
} DN_MPG_KEY_A_T;

typedef union  { // 0x2980
	UINT32 ui32Val;
	struct {
		UINT32 bypass_vfe      : 1;   // 0       0
		UINT32 vid_type        : 1;   // 1       1
		UINT32 type            : 1;   // 2       2
		UINT32                 : 29;  // 31:3    3
	}le;
} DN_CONF_B_T;

typedef union  { // 0x2984
	UINT32 ui32Val;
	struct {
		UINT32 record          : 1;   // 0       0
		UINT32 paus_rec        : 1;   // 1       1
		UINT32 l_cnt_rst       : 1;   // 2       2
		UINT32                 : 21;  // 23:3    3
		UINT32 w_sel_dn        : 2;   // 25:24   24
		UINT32                 : 2;   // 27:26   26
		UINT32 reset_sw        : 1;   // 28      28
		UINT32                 : 3;   // 31:29   29
	}le;
} DN_CTRL_B_T;

typedef union  { // 0x2988
	UINT32 ui32Val;
	struct {
		UINT32 pid             : 13;  // 12:0    0
		UINT32                 : 19;  // 31:13   13
	}le;
} DN_VPID_B_T;

typedef union  { // 0x298c
	UINT32 ui32Val;
	struct {
		UINT32 pkt_cnt_lim     : 14;  // 13:0    0
		UINT32                 : 2;   // 15:14   14
		UINT32 buf_num_lim     : 5;   // 20:16   16
		UINT32                 : 11;  // 31:21   21
	}le;
} DN_BUF_LIM_B_T;

typedef union  { // 0x2990
	UINT32 ui32Val;
	struct {
		UINT32 pkt_cnt         : 14;  // 13:0    0
		UINT32                 : 2;   // 15:14   14
		UINT32 buf_num         : 5;   // 20:16   16
		UINT32                 : 11;  // 31:21   21
	}le;
} DN_BUF_INFO_B_T;

typedef union  { // 0x2994
	UINT32 ui32Val;
	struct {
		UINT32 stc_resolution  : 7;   // 6:0     0
		UINT32 stc_src         : 1;   // 7       7
		UINT32                 : 24;  // 31:8    8
	}le;
} DN_STC_CTRL_B_T;

typedef union  { // 0x2998
	UINT32 ui32Val;
	struct {
		UINT32 dn_buf_eptr     : 16;  // 12:0    0
		UINT32 dn_buf_sptr     : 16;  // 28:16   16
	}le;
} DN_BUF_BOUND_B_T;

typedef union  { // 0x299c
	UINT32 ui32Val;
	struct {
		UINT32                 : 3;   // 2:0     0
#if PVR_OLD_REG
		UINT32 dn_buf_wptr	   : 25;  // 24:3	 3
		UINT32				   : 4;   // 31:25	 25
#else
		UINT32 dn_buf_wptr	   : 29;  // 24:3	 3
#endif
	}le;
} DN_BUF_WPTR_B_T;

typedef union  { // 0x29c0
	UINT32 ui32Val;
	struct {
		UINT32 mpg_key0        : 32;  // 31:0    0
	}le;
} DN_MPG_KEY_B_T;

typedef union  { // 0x2a00
	UINT32 ui32Val;
	struct {
		UINT32 upen            : 1;   // 0       0
		UINT32 upps            : 1;   // 1       1
		UINT32 rep             : 1;   // 2       2
		UINT32 tcp             : 1;   // 3       3
		UINT32                 : 10;  // 13:4    4
		UINT32 wsel_up         : 2;   // 15:14   14
		UINT32                 : 8;   // 23:16   16
		UINT32 iack            : 3;   // 27:24   24
		UINT32                 : 1;   // 27:24   24
		UINT32 rst             : 1;   // 28      28
		UINT32 nm_rst          : 1;   // 29      29
		UINT32                 : 2;   // 31:30   30
	}le;
} UP_CONF_A_T;

typedef union  { // 0x2a04
	UINT32 ui32Val;
	struct {
		UINT32 play_mod       	: 3;   // 2:0     0
		UINT32 its_mod        	: 1;   // 3       3
		UINT32 res_01         	: 1;   // 4       4
		UINT32 tsc          	: 1;   // 5       5
		UINT32 res_02           : 1;   // 6       6
		UINT32 atcp            	: 1;   // 7       7
		UINT32 up_type         	: 3;   // 10:8    8
		UINT32 flush            : 1;   // 15:11   11
		UINT32 num_serr        	: 3;   // 18:16   16
		UINT32                 	: 1;   // 15:11   11
		UINT32 wait_cycle     	: 16;   // 27:24   24
	}le;
} UP_MODE_A_T;

typedef union  { // 0x2a88
	UINT32 ui32Val;
	struct {
		UINT32 al_empty_level  : 16;  // 12:0    0
		UINT32 al_full_level   : 16;  // 28:16   16
	}le;
} UP_BUF_CONF_A_T;

typedef union  { // 0x2a8c
	UINT32 ui32Val;
	struct {
		UINT32 up_buf_eptr     : 16;  // 10:0    0
		UINT32 up_buf_sptr     : 16;  // 26:16   16
	}le;
} UP_BUF_BOUND_A_T;

typedef union  { // 0x2a90
	UINT32 ui32Val;
	struct {
#if PVR_OLD_REG
		UINT32 up_buf_wptr     : 28;  // 24:3    3
		UINT32                 : 4;   // 31:25   25
#else
		UINT32 up_buf_wptr	   : 32;  // 24:3	 3
#endif
	}le;
} UP_BUF_WPTR_A_T;

typedef union  { // 0x2a94
	UINT32 ui32Val;
	struct {
#if PVR_OLD_REG
	UINT32 up_buf_rptr	   : 28;  // 24:3	 3
	UINT32				   : 4;   // 31:25	 25
#else
	UINT32 up_buf_rptr	   : 32;  // 24:3	 3
#endif
	}le;
} UP_BUF_RPTR_A_T;

typedef union  { // 0x2a98
	UINT32 ui32Val;
	struct {
		UINT32 up_buf_pptr     : 28;  // 22:0    0
		UINT32                 : 4;   // 31:23   23
	}le;
} UP_BUF_PPTR_A_T;

typedef union  { // 0x2a1c
	UINT32 ui32Val;
	struct {
		UINT32 al_jitter       : 32;  // 31:0    0
	}le;
} UP_MAX_JITTER_A_T;

/*-----------------------------------------------------------------------------
	0x0020 UP_BUF_STAT ''
------------------------------------------------------------------------------*/
typedef union  {
	UINT32	ui32Val;
	struct {
		UINT32
		al_ist                          : 2,	//  1: 0
		_rsvd_01                        :14,	// 15: 2
		al_imsk                         : 2,	// 17:16
		_rsvd_00                        :14;	// 31:18
	}le;
} UP_BUF_STAT;


typedef union  { // 0x2a40
	UINT32 ui32Val;
	struct {
		UINT32 up_mpg_key0     : 32;  // 31:0    0
	}le;
} UP_MPG_KEY_A_T;

typedef union  { // 0x2a50
	UINT32 ui32Val;
	struct {
		UINT32 sync_err        : 1;   // 0       0
		UINT32 overlap         : 1;   // 1       1
		UINT32                 : 30;  // 31:2    2
	}le;
} UP_ERR_STAT_A_T;

typedef union  { // 0x2a54
	UINT32 ui32Val;
	struct {
		UINT32 tsc_jitter      : 32;  // 31:0    0
	}le;
} UP_TSC_JITTER_A_T;

typedef union  { // 0x2a58
	UINT32 ui32Val;
	struct {
		UINT32 up_state        : 5;   // 4:0     0
		UINT32                 : 3;   // 7:5     5
		UINT32 ecnt            : 2;   // 9:8     8
		UINT32 fcnt            : 2;   // 11:10   10
		UINT32 mrq             : 1;   // 12      12
		UINT32 invp            : 1;   // 13      13
		UINT32                 : 2;   // 15:14   14
		UINT32 dtype           : 2;   // 17:16   16
		UINT32                 : 14;  // 31:18   18
	}le;
} UP_STAT0_A_T;

typedef union  { // 0x2a5c
	UINT32 ui32Val;
	struct {
		UINT32 pack_byte_cnt   : 3;   // 2:0     0
		UINT32                 : 5;   // 7:3     3
		UINT32 pack_word_cnt   : 5;   // 12:8    8
		UINT32                 : 3;   // 15:13   13
		UINT32 trans_word_cnt  : 9;   // 24:16   16
		UINT32                 : 7;   // 31:25   25
	}le;
} UP_STAT1_A_T;

typedef union  { // 0x2a80
	UINT32 ui32Val;
	struct {
		UINT32 upen            : 1;   // 0       0
		UINT32 upps            : 1;   // 1       1
		UINT32 rep             : 1;   // 2       2
		UINT32 tcp             : 1;   // 3       3
		UINT32                 : 10;  // 13:4    4
		UINT32 wsel_up         : 2;   // 15:14   14
		UINT32                 : 8;   // 23:16   16
		UINT32 iack            : 4;   // 27:24   24
		UINT32 rst             : 1;   // 28      28
		UINT32 nm_rst          : 1;   // 29      29
		UINT32                 : 2;   // 31:30   30
	}le;
} UP_CONF_B_T;

typedef union  { // 0x2a84
	UINT32 ui32Val;
	struct {
		UINT32 play_mod       	: 3;   // 2:0     0
		UINT32 its_mod        	: 1;   // 3       3
		UINT32 res_01         	: 1;   // 4       4
		UINT32 tsc          	: 1;   // 5       5
		UINT32 res_02           : 1;   // 6       6
		UINT32 atcp            	: 1;   // 7       7
		UINT32 up_type         	: 3;   // 10:8    8
		UINT32 flush            : 1;   // 15:11   11
		UINT32 num_serr        	: 3;   // 18:16   16
		UINT32                 	: 1;   // 15:11   11
		UINT32 wait_cycle     	: 16;   // 27:24   24
	}le;
} UP_MODE_B_T;

typedef union  { // 0x2a88
	UINT32 ui32Val;
	struct {
		UINT32 al_empty_level  : 16;  // 12:0    0
		UINT32 al_full_level   : 16;  // 28:16   16
	}le;
} UP_BUF_CONF_B_T;

typedef union  { // 0x2a8c
	UINT32 ui32Val;
	struct {
		UINT32 up_buf_eptr     : 16;  // 10:0    0
		UINT32 up_buf_sptr     : 16;  // 26:16   16
	}le;
} UP_BUF_BOUND_B_T;

typedef union  { // 0x2a90
	UINT32 ui32Val;
	struct {
#if PVR_OLD_REG
	UINT32 up_buf_wptr	   : 28;  // 24:3	 3
	UINT32				   : 4;   // 31:25	 25
#else
	UINT32 up_buf_wptr	   : 32;  // 24:3	 3
#endif
	}le;
} UP_BUF_WPTR_B_T;

typedef union  { // 0x2a94
	UINT32 ui32Val;
	struct {
#if PVR_OLD_REG
		UINT32 up_buf_rptr     : 28;  // 24:3    3
		UINT32                 : 4;   // 31:25   25
#else
		UINT32 up_buf_rptr	   : 32;  // 24:3	 3
#endif
	}le;
} UP_BUF_RPTR_B_T;

typedef union  { // 0x2a98
	UINT32 ui32Val;
	struct {
		UINT32 up_buf_pptr     : 28;  // 22:0    0
		UINT32                 : 4;   // 31:23   23
	}le;
} UP_BUF_PPTR_B_T;

typedef union  { // 0x2a9c
	UINT32 ui32Val;
	struct {
		UINT32 al_jitter       : 32;  // 31:0    0
	}le;
} UP_MAX_JITTER_B_T;

typedef union  { // 0x2ac0
	UINT32 ui32Val;
	struct {
		UINT32 up_mpg_key0     : 32;  // 31:0    0
	}le;
} UP_MPG_KEY_B_T;

typedef union  { // 0x2ad0
	UINT32 ui32Val;
	struct {
		UINT32 sync_err        : 1;   // 0       0
		UINT32 overlap         : 1;   // 1       1
		UINT32                 : 30;  // 31:2    2
	}le;
} UP_ERR_STAT_B_T;

typedef union  { // 0x2ad4
	UINT32 ui32Val;
	struct {
		UINT32 tsc_jitter      : 32;  // 31:0    0
	}le;
} UP_TSC_JITTER_B_T;

typedef union  { // 0x2ad8
	UINT32 ui32Val;
	struct {
		UINT32 up_state        : 5;   // 4:0     0
		UINT32                 : 3;   // 7:5     5
		UINT32 ecnt            : 2;   // 9:8     8
		UINT32 fcnt            : 2;   // 11:10   10
		UINT32 mrq             : 1;   // 12      12
		UINT32 invp            : 1;   // 13      13
		UINT32                 : 2;   // 15:14   14
		UINT32 dtype           : 2;   // 17:16   16
		UINT32                 : 14;  // 31:18   18
	}le;
} UP_STAT0_B_T;

typedef union  { // 0x2a5c
	UINT32 ui32Val;
	struct {
		UINT32 pack_byte_cnt   : 3;   // 2:0     0
		UINT32                 : 5;   // 7:3     3
		UINT32 pack_word_cnt   : 5;   // 12:8    8
		UINT32                 : 3;   // 15:13   13
		UINT32 trans_word_cnt  : 9;   // 24:16   16
		UINT32                 : 7;   // 31:25   25
	}le;
} UP_STAT1_B_T;


typedef struct {
	UINT32              reserved1;           // 0x2600
	INTR_EN_T           intr_en;             // 0x2604
	INTR_STAT_T         intr_stat;           // 0x2608
	INTR_RSTAT_T        intr_rstat;          // 0x260c
	UINT32              reserved2[60];       // 0x2610~0x26fc
	PIE_CONF_A_T        pie_conf_a;          // 0x2700
	PIE_MODE_T        	pie_mode_a;          // 0x2704
	UINT32              reserved3[2];        // 0x2708~0x270c
	PIE_STAT_A_T        pie_stat_a;          // 0x2710
	PIE_PES_STAT_A_T    pie_pes_stat_a;      // 0x2714
	PIE_ERR_STAT_A_T    pie_err_stat_a;      // 0x2718
	UINT32              reserved4;           // 0x271c
	PIE_SCD_VALUE_A_T   pie_scd_value_a[4];  // 0x2720~0x272c
	PIE_SCD_MASK_A_T    pie_scd_mask_a[4];   // 0x2730~0x273c
	UINT32              reserved5[16];       // 0x2740~0x277c
	PIE_IND_A_T         pie_ind_a[32];       // 0x2780~0x27fc
	PIE_CONF_B_T        pie_conf_b;          // 0x2800
	PIE_MODE_T        	pie_mode_b;          // 0x2804
	UINT32              reserved6[2];        // 0x2808~0x280c
	PIE_STAT_B_T        pie_stat_b;          // 0x2810
	PIE_PES_STAT_B_T    pie_pes_stat_b;      // 0x2814
	PIE_ERR_STAT_B_T    pie_err_stat_b;      // 0x2818
	UINT32              reserved7;           // 0x281c
	PIE_SCD_VALUE_B_T   pie_scd_value_b[4];  // 0x2820~0x282c
	PIE_SCD_MASK_B_T    pie_scd_mask_b[4];   // 0x2830~0x283c
	UINT32              reserved8[16];       // 0x2840~0x287c
	PIE_IND_B_T         pie_ind_b[32];       // 0x2880~0x28fc
	DN_CONF_A_T         dn_conf_a;           // 0x2900
	DN_CTRL_A_T         dn_ctrl_a;           // 0x2904
	DN_VPID_A_T         dn_vpid_a;           // 0x2908
	DN_BUF_LIM_A_T      dn_buf_lim_a;        // 0x290c
	DN_BUF_INFO_A_T     dn_buf_info_a;       // 0x2910
	DN_STC_CTRL_A_T     dn_stc_ctrl_a;       // 0x2914
	DVR_BUF_SPTR_T      dn_buf_sptr_a;       // 0x2918
	DVR_BUF_EPTR_T      dn_buf_eptr_a;       // 0x2918
	DN_BUF_WPTR_A_T     dn_buf_wptr_a;       // 0x291c
	UINT32              reserved9[7];        // 0x2920~0x293c
	DN_MPG_KEY_A_T      dn_mpg_key_a[4];     // 0x2940~0x294c
	UINT32              reserved10[12];      // 0x2950~0x297c
	DN_CONF_B_T         dn_conf_b;           // 0x2980
	DN_CTRL_B_T         dn_ctrl_b;           // 0x2984
	DN_VPID_B_T         dn_vpid_b;           // 0x2988
	DN_BUF_LIM_B_T      dn_buf_lim_b;        // 0x298c
	DN_BUF_INFO_B_T     dn_buf_info_b;       // 0x2990
	DN_STC_CTRL_B_T     dn_stc_ctrl_b;       // 0x2994
	DVR_BUF_SPTR_T      dn_buf_sptr_b;       // 0x2918
	DVR_BUF_EPTR_T      dn_buf_eptr_b;       // 0x2918
	DN_BUF_WPTR_B_T     dn_buf_wptr_b;       // 0x299c
	UINT32              reserved11[7];       // 0x29a0~0x29bc
	DN_MPG_KEY_B_T      dn_mpg_key_b[4];     // 0x29c0~0x29cc
	UINT32              reserved12[12];      // 0x29d0~0x29fc
	UP_CONF_A_T         up_conf_a;           // 0x2a00
	UP_MODE_A_T         up_mode_a;           // 0x2a04
//	UP_BUF_CONF_A_T     up_buf_conf_a;       // 0x2a08
	DVR_BUF_LEVEL_T     up_al_ful_a;       	 // 0x2a08
	DVR_BUF_LEVEL_T     up_al_empty_a;       // 0x2a08
	DVR_BUF_SPTR_T      up_buf_sptr_a;       // 0x2918
	DVR_BUF_EPTR_T      up_buf_eptr_a;       // 0x2918
//	UP_BUF_BOUND_A_T    up_buf_bound_a;      // 0x2a0c
	UP_BUF_WPTR_A_T     up_buf_wptr_a;       // 0x2a10
	UP_BUF_RPTR_A_T     up_buf_rptr_a;       // 0x2a14
	UP_BUF_PPTR_A_T     up_buf_pptr_a;       // 0x2a18
	UP_MAX_JITTER_A_T   up_max_jitter_a;     // 0x2a1c
	UP_BUF_STAT			up_buf_stat_a;
	UINT32              reserved13[5];       // 0x2a20~0x2a3c
	UP_MPG_KEY_A_T      up_mpg_key_a[4];     // 0x2a40~0x2a4c
	UP_ERR_STAT_A_T     up_err_stat_a;       // 0x2a50
	UP_TSC_JITTER_A_T   up_tsc_jitter_a;     // 0x2a54
	UP_STAT0_A_T        up_stat0_a;          // 0x2a58
	UP_STAT1_A_T        up_stat1_a;          // 0x2a5c
	UINT32              reserved14[8];       // 0x2a60~0x2a7c
	UP_CONF_B_T         up_conf_b;           // 0x2a80
	UP_MODE_B_T         up_mode_b;           // 0x2a84
//	UP_BUF_CONF_B_T     up_buf_conf_b;       // 0x2a88
	DVR_BUF_LEVEL_T     up_al_ful_b;       	 // 0x2a08
	DVR_BUF_LEVEL_T     up_al_empty_b;       // 0x2a08
	DVR_BUF_SPTR_T      up_buf_sptr_b;       // 0x2918
	DVR_BUF_EPTR_T      up_buf_eptr_b;       // 0x2918
//	UP_BUF_BOUND_B_T    up_buf_bound_b;      // 0x2a8c
	UP_BUF_WPTR_B_T     up_buf_wptr_b;       // 0x2a90
	UP_BUF_RPTR_B_T     up_buf_rptr_b;       // 0x2a94
	UP_BUF_PPTR_B_T     up_buf_pptr_b;       // 0x2a98
	UP_MAX_JITTER_B_T   up_max_jitter_b;     // 0x2a9c
	UP_BUF_STAT			up_buf_stat_b;
	UINT32              reserved15[5];       // 0x2aa0~0x2abc
	UP_MPG_KEY_B_T      up_mpg_key_b[4];     // 0x2ac0~0x2acc
	UP_ERR_STAT_B_T     up_err_stat_b;       // 0x2ad0
	UP_TSC_JITTER_B_T   up_tsc_jitter_b;     // 0x2ad4
	UP_STAT0_B_T        up_stat0_b;          // 0x2ad8
	UP_STAT1_B_T        up_stat1_b;          // 0x2adc
} DVR_REG_T;

extern volatile DVR_REG_T *gpPvrReg;
extern DVR_REG_T *gpLocalPvrReg, stLocalPvrReg;

UINT32 DVR_RegInit(void);
UINT32 DVR_DN_EnableReg(LX_PVR_CH_T ch ,BOOLEAN bEnable);
UINT32 DVR_UP_EnableReg(LX_PVR_CH_T ch ,BOOLEAN bEnable, UINT8 ui8PacketLen);
UINT32 DVR_UP_TimeStampCopyReg(LX_PVR_CH_T ch ,BOOLEAN bEnable);
BOOLEAN DVR_UP_GetEnableState(LX_PVR_CH_T ch);
UINT32 DVR_UP_SetSpeedReg(LX_PVR_CH_T ch ,SINT32 speed);
UINT32 DVR_DN_GetBufBoundReg( LX_PVR_CH_T ch, UINT32 *pBase, UINT32 *pEnd );
UINT32 DVR_DN_GetWritePtrReg( LX_PVR_CH_T ch, UINT32 *pWrite);
UINT32 DVR_UP_GetBufBoundReg(LX_PVR_CH_T ch, UINT32 *pBase, UINT32 *pEnd);
UINT32 DVR_UP_GetPointersReg(LX_PVR_CH_T ch, UINT32 *pWrite, UINT32 *pRead);
UINT32  DVR_UP_GetLevel(LX_PVR_CH_T ch, UINT32 *pFullLevel );
UINT32 DVR_UP_SetWritePtrReg(LX_PVR_CH_T ch, UINT32 ptrWrite);
UINT32 DVR_DN_SetWritePtrReg(LX_PVR_CH_T ch, UINT32 ptrWrite);
UINT32 DVR_UP_SetReadPtrReg(LX_PVR_CH_T ch, UINT32 ptrRead);
UINT32 DVR_UP_ResetBlock(LX_PVR_CH_T ch);
UINT32 DVR_DN_ResetBlock(LX_PVR_CH_T ch);
UINT32 DVR_UP_EnableEmptyLevelReg(LX_PVR_CH_T ch, UINT32 bEnable);
UINT32 DVR_UP_PauseReg(LX_PVR_CH_T ch ,BOOLEAN bEnable);
UINT32 DVR_UP_RepPauseReg(LX_PVR_CH_T ch ,BOOLEAN bEnable, UINT32 ui32Pptr);
UINT32 DVR_UP_SetEmptyLevelReg(LX_PVR_CH_T ch, UINT32 level);
UINT32 DVR_DN_SetBufBoundReg(LX_PVR_CH_T ch, UINT32 ptrBase, UINT32 ptrEnd);
UINT32 DVR_UP_SetBufBoundReg(LX_PVR_CH_T ch, UINT32 ptrBase, UINT32 ptrEnd);
UINT32 DVR_DN_EnableINTReg(LX_PVR_CH_T ch ,BOOLEAN bEnable);
UINT32 DVR_PIE_EnableINTReg(LX_PVR_CH_T ch ,BOOLEAN bEnable);
UINT32 DVR_DN_ConfigureIntrLevel(LX_PVR_CH_T ch ,UINT32 ui32BufNumLim, UINT32 ui32PktNumLim );
UINT32 DVR_DN_GetPacketBufLimit(LX_PVR_CH_T ch ,UINT32 *pui32PktNumLim, UINT32 *pui32BufNumLim );
UINT32 DVR_PIE_SetModeReg(LX_PVR_CH_T ch, UINT32 mode);
UINT32 DVR_PIE_ConfigureSCD(LX_PVR_CH_T ch, UINT8 ui8ScdIndex, UINT32 ui32Mask, UINT32 ui32Value, UINT8 ui8Enable );
UINT32 DVR_PIE_GscdByteInfoConfig(LX_PVR_CH_T ch, UINT8 ui8ByteSel0, UINT8 ui8ByteSel1, UINT8 ui8ByteSel2, UINT8 ui8ByteSel3 );
UINT32 DVR_PIE_EnableSCDReg(LX_PVR_CH_T ch, BOOLEAN bEnable);
UINT32 DVR_PIE_ClearIACKReg(LX_PVR_CH_T ch, BOOLEAN bClear);
UINT32 DVR_UP_ClearIACKReg(LX_PVR_CH_T ch, UINT8 ui8AckValue);
UINT32 DVR_PIE_ResetBlock(LX_PVR_CH_T ch);
#if 1
void DVR_EnableWaitCycle (LX_PVR_CH_T ch, UINT16 ui16WaitCycle );
#else
void DVR_EnableWaitCycle ( UINT16 ui16WaitCycle );
#endif
UINT32 DVR_DN_PauseReg(LX_PVR_CH_T ch ,BOOLEAN bEnable);
UINT32 DVR_REG_ReadAllRegisters ( DVR_REG_T *pstDvrRegDest );
UINT32 DVR_CheckPatternMismatch ( LX_PVR_CH_T ch );
UINT8 DVR_UP_GetErrorStat(LX_PVR_CH_T ch);
UINT32 DVR_UP_TimeStampCheckDisableReg(LX_PVR_CH_T ch ,BOOLEAN bDisable);
UINT32 DVR_UP_ChangePlaymode(LX_PVR_CH_T ch , UINT8 ui8PlayMode );
UINT32 DVR_UP_GetTSCJitterReg(LX_PVR_CH_T ch, UINT32 *pJitter);

UINT32 DVR_DN_SetPIDReg(LX_PVR_CH_T ch, UINT32 PID);


extern UINT32 ui32DvrUpEmptyLevel;
extern UINT32 ui32DvrDnMaxPktCount;



#define COPY_VOLATILE_32( __dest__, __src__, __n_words__ )		\
{															\
	UINT32	__ui32Words2Copy;								\
	volatile UINT32	*__pui32Src, *__pui32Dest;				\
															\
	__pui32Src = (volatile UINT32 *) (__src__);					\
	__pui32Dest = (volatile UINT32 *) (__dest__);					\
	__ui32Words2Copy = (UINT32) (__n_words__);				\
															\
	while ( __ui32Words2Copy-- )							\
	{														\
		*__pui32Dest ++ = *__pui32Src ++;				\
	}														\
}


#define MOD_REG_WRITE( _REG_NAME_ )	\
{																	\
	gpPvrReg->##_REG_NAME_.ui32Val	= gpLocalPvrReg->##_REG_NAME_.ui32Val; \
}

#define MOD_REG_READ( _REG_NAME_ )	\
{													\
	gpLocalPvrReg->##_REG_NAME_.ui32Val = gpPvrReg->##_REG_NAME_.ui32Val;	\
}


#endif /* !_DVR_REG_h */
