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


#ifndef _SDEC_REG_h
#define _SDEC_REG_h


#ifdef __cplusplus
extern "C" {
#endif

/* Add to define BASE Address
    Define here instead of using linux_platform.h
    M14B0 address changed from A0, M14 linux_platform.h is one file,
    it's not easy to add the definition, no rule.
    So, at first, define here and consider using local definition from now */
// #define M14_PL301_BASE				0xc0000000
/* linux_platform.h define M14_B0_TE_BASE, remove following definition. 20130923 jinhwan.bae */
//#define M14_B0_TE_BASE					(0xc0000000+0x008000)

// for Real H/W
#ifndef USE_QEMU_SYSTEM
// for LG1150
#define L8_SDEC_TOP_REG_BASE		(L8_TE_BASE)
#define L8_SDEC_IO_REG_BASE			(L8_TE_BASE + 0x200)
#define L8_SDEC_MPG_REG_BASE0		(L8_TE_BASE + 0x400)
#define L8_SDEC_MPG_REG_BASE1		(L8_TE_BASE + 0xC00)
// for LG1152
#define L9_SDEC_TOP_REG_BASE		(L9_TE_BASE)
#define L9_SDEC_IO_REG_BASE			(L9_TE_BASE + 0x200)
#define L9_SDEC_MPG_REG_BASE0		(L9_TE_BASE + 0x400)
#define L9_SDEC_MPG_REG_BASE1		(L9_TE_BASE + 0xC00)
// for LG1154
#define H13_SDEC_TOP_REG_BASE		(H13_TE_BASE)
#define H13_SDEC_IO_REG_BASE		(H13_TE_BASE + 0x200)
#define H13_SDEC_MPG_REG_BASE0		(H13_TE_BASE + 0x400)
#define H13_SDEC_MPG_REG_BASE1		(H13_TE_BASE + 0xC00)
// for LG1311 M14 A0
#define M14_A0_SDEC_TOP_REG_BASE		(M14_TE_BASE)
#define M14_A0_SDEC_IO_REG_BASE			(M14_TE_BASE + 0x200)
#define M14_A0_SDEC_MPG_REG_BASE0		(M14_TE_BASE + 0x400)
#define M14_A0_SDEC_MPG_REG_BASE1		(M14_TE_BASE + 0xC00)
// for LG1311 M14 B0
#define M14_B0_SDEC_TOP_REG_BASE			(M14_B0_TE_BASE)
//#define M14_B0_SDEC_IO_REG_BASE			(M14_B0_TE_BASE + 0x200)
#define M14_B0_SDEC_CORE0_IO_REG_BASE		(M14_B0_TE_BASE + 0x200)
#define M14_B0_SDEC_CORE1_IO_REG_BASE		(M14_B0_TE_BASE + 0x1A00)
#define M14_B0_SDEC_CORE0_MPG_REG_BASE0		(M14_B0_TE_BASE + 0x400)
#define M14_B0_SDEC_CORE0_MPG_REG_BASE1		(M14_B0_TE_BASE + 0xC00)
#define M14_B0_SDEC_CORE1_MPG_REG_BASE0		(M14_B0_TE_BASE + 0x1C00)
#define M14_B0_SDEC_CORE1_MPG_REG_BASE1		(M14_B0_TE_BASE + 0x2400)
// for LG1156 H14
#define H14_SDEC_TOP_REG_BASE				(H14_TE_BASE)	/* 0xC0006000 */
// #define H14_SDEC_IO_REG_BASE				(H14_TE_BASE + 0x200)
#define H14_SDEC_CORE0_IO_REG_BASE			(H14_TE_BASE + 0x200)
#define H14_SDEC_CORE1_IO_REG_BASE			(H14_TE_BASE + 0x1A00)
#define H14_SDEC_CORE0_MPG_REG_BASE0		(H14_TE_BASE + 0x400)
#define H14_SDEC_CORE0_MPG_REG_BASE1		(H14_TE_BASE + 0xC00)
#define H14_SDEC_CORE1_MPG_REG_BASE0		(H14_TE_BASE + 0x1C00)
#define H14_SDEC_CORE1_MPG_REG_BASE1		(H14_TE_BASE + 0x2400)

#else
#ifndef IRQ_SDEC
#define IRQ_SDEC        75
#endif
#define LX_SDEC_TOP_REG_BASE 	(UINT32)&pSDEC_TOP_Reg
#define LX_SDEC_IO_REG_BASE		(UINT32)&pSDEC_IO_Reg
#define LX_SDEC_MPG_REG_BASE0	(UINT32)&pSDEC_MPG_Reg0
#define LX_SDEC_MPG_REG_BASE1	(UINT32)&pSDEC_MPG_Reg1
#endif	/* USE_QEMU_SYSTEM */


/*-----------------------------------------------------------------------------
	0x0000 vid_out_sel ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	vid0_sel                        : 2,	//  0: 1
	                                : 2,	//  2: 3 reserved
	vid1_sel                        : 2,	//  4: 5
	                                : 2,	//  6: 7 reserved
	vid2_sel                        : 2;	//  8: 9
} VID_OUT_SEL;

/*-----------------------------------------------------------------------------
	0x0004 dl_sel ''
------------------------------------------------------------------------------*/
#if 0 // _SDEC_14_CHG_
typedef struct {
	UINT32
	dl0_sel                         : 1,	//     0
	                                : 3,	//  1: 3 reserved
	dl1_sel                         : 1;	//     4
} DL_SEL;
#else
typedef struct {
	UINT32
	dl0_sel                         : 2,	//    0: 1
	                                : 2,	//  	2: 3 reserved
	dl1_sel                         : 2;	//    4: 5
} DL_SEL;
#endif

#if 1 // _SDEC_14_CHG_
/*-----------------------------------------------------------------------------
	0x0008 aud_out_sel ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	aud0_sel                        : 1,	//  0
	                                : 3,	//  1: 3 reserved	
	aud1_sel                        : 1;	//  4
} AUD_OUT_SEL;

/*-----------------------------------------------------------------------------
	0x000C up_sel ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	up0_sel                         : 1,	//     0
	                                : 3,	//  1: 3 reserved
	up1_sel                         : 1;	//     4
} UP_SEL;

/*-----------------------------------------------------------------------------
	0x0010 stpo_sel ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	stpo_sel                        : 1;	//     0
} STPO_SEL;

/*-----------------------------------------------------------------------------
	0x001C axi_flush_conf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	sdec0_flush_en                  : 1,	//     0
	dvr_flush_en  	                : 1,	//     1
	sdec1_flush_en 	                : 1;	//     2
} AXI_FLUSH_CONF;

/*-----------------------------------------------------------------------------
	0x0020 axi_user_conf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	dvr_aruser		                : 2,	//    0: 1
									: 2,	// 	2: 3
	dvr_awuser						: 2,	//	4: 5
									: 2,	// 	6: 7
	sdec0_aruser	                : 2,	//    8: 9
									: 2,	// 	10: 11
	sdec0_awuser	                : 2,	//    12: 13
									: 2,	// 	14: 15
	sdec1_aruser	                : 2,	//    16: 17
									: 2,	// 	18: 19
	sdec1_awuser	                : 2;	//    20: 21
} AXI_USER_CONF;

#endif

/*-----------------------------------------------------------------------------
	0x0000~0x000c cdip0 ~ cdip3 ''
------------------------------------------------------------------------------*/
typedef struct{
	UINT32
	dtype           :  4,
	sync_type       :  2,
	                       : 2,
	clk_div         :  7,
	                    :  1,
	pconf           :  2,
	                :  2,
	req_en          :  1,
	val_en          :  1,
	                :  1,
	err_en          :  1,
	req_act_lo      :  1,
	val_act_lo      :  1,
	clk_act_lo      :  1,
	err_act_hi      :  1,
	                :  2,
	test_en         :  1,
	en              :  1;
}CDIP;
/*-----------------------------------------------------------------------------
	0x0010~0x0014 cdiop0 ~ cdiop1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	dtype                           : 4,	//  0: 3
	sync_type                       : 2,	//  4: 5
	                                : 2,	//  6: 7 reserved
	clk_div                         : 7,	//  8:14
	                                : 1,	//    15 reserved
	pconf                           : 1,	// 16
	cdinout							: 1,	// 17
	val_sel                         : 2,	// 18:19
	req_en                          : 1,	//    20
	val_en                          : 1,	//    21
	err_en                          : 1,	//    22
	                                : 1,	//    23 reserved
	req_act_lo                      : 1,	//    24
	val_act_lo                      : 1,	//    25
	clk_act_lo                      : 1,	//    26
	err_act_hi                      : 1,	//    27
	                                : 2,	// 28:29 reserved
	test_en                         : 1,	//    30
	en                              : 1;	//    31
} CDIOP;

/*-----------------------------------------------------------------------------
	0x0018 cdin_parallel_sel ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	p_sel                           : 1;	//     0
} CDIN_PARALLEL_SEL;

/*-----------------------------------------------------------------------------
	0x001C cdic3_dl_conf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	dl0_sel                         : 1,	//    0
	dl1_sel 		 			 	: 1,	//	1
									: 2,	// 	2: 3 reserved
	dl0_ext_sel						: 1,	//	4
	dl1_ext_sel						: 1,	//	5
									: 2,	//	6: 7 reserved
	pidf0_en						: 1,	//	8
	pidf1_en						: 1,	// 	9
	pidf2_en						: 1,	//	10
	pidf3_en						: 1,	//	11
	pidf_addr						: 2,	//	12: 13
									: 2, 	// 	14: 15 reserved
	pidf_val						: 13,	//	16: 28
									: 2,	//	29: 30 reserved
	pid_wr_en						: 1;	//	31
} CDIC3_DL_CONF;

/*-----------------------------------------------------------------------------
	0x0020~0x0024 cdic0 ~ cdic1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	dtype                           : 4,	//  0: 3
	cdif_rpage                      : 1,	//     4
	cdif_wpage                      : 1,	//     5
	cdif_ovflow                     : 1,	//     6
	                                : 1,	//     7 reserved
	pd_wait                         : 1,	//     8
	cdif_full                       : 1,	//     9
	cdif_empty                      : 1,	//    10
	                                : 1,	//    11 reserved
	sb_dropped                      : 1,	//    12
	sync_lost                       : 1,	//    13
	no_wdata                        : 1,	//    14
	                                : 1,	//    15 reserved
	av_dec_id                       : 4,	// 16:19
	src                             : 4,	// 20:23
	max_sb_drp                      : 2,	// 24:25
	min_sb_det                      : 2,	// 26:27
	rst                             : 1;	//    28
} CDIC;

/*-----------------------------------------------------------------------------
	0x0028~0x002c cdic_dsc0 ~ cdic_dsc1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	cas_type                        : 3,	//  0: 2
	                                : 1,	//     3 reserved
	blk_mode                        : 4,	//  4: 7
	res_mode                        : 4,	//  8:11
	key_size                        : 2,	// 12:13
	                                : 2,	// 14:15 reserved
	even_mode                       : 2,	// 16:17
	                                : 2,	// 18:19 reserved
	odd_mode                        : 2,	// 20:21
	                                : 2,	// 22:23 reserved
	psc_en                          : 1;	//    24
} CDIC_DSC;

/*-----------------------------------------------------------------------------
	0x0030~0x0034 cdoc0 ~ cdoc1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
//	buf_e_lev                       :28,	//  0:27
//	dtype                           : 4,	//  0: 3
	id_value                        : 3,	//  0: 2
	id_msb                          : 1,	//     3
//	q_len                           : 4,	//  4: 7
	id_mask                         : 3,	//  4: 6
	                                : 1,	//     7 reserved
//	gpb_idx                         : 6,	//  8:13
	tso_src                         : 1,	//     8
	                                : 7,	//  9:13 reserved
//	gpb_chan                        : 1,	//    14
//	wptr_auto                       : 1,	//    15
	src                             : 2,	// 16:17
	                                : 10,	// 18:23 reserved
//	wptr_upd                        : 1,	//    24
//	                                : 3;	// 25:27 reserved
//	rst                             : 1,	//    28
	rst                             : 1,	//    28
	                                : 3;	// 29:30 reserved
//	en                              : 1;	//    31
} CDOC;

/* added at L8 B0
 * Start */
typedef struct {
   	UINT32
	VAL_POL        			:  1,
	TPIF_EN          		:  1,
	EN         				:  1,
	SW_RESET                :  1,
	SYNC_DROP         		:  2,
	                		:  2,
	SYNC_FOUND              :  2,
	                		:  2,
	SYNC_TYPE             	:  2,
	                		:  6,
	BUF_IDX                 :  1,
	                		:  3,
	SYNC_STATE             	:  2,
	                		:  2,
	NO_SYNC                 :  1,
	                		:  3;
} CDIP3_2ND;

typedef struct {
	UINT32
	VAL_POL        			:  1,
	TPIF_EN          		:  1,
	EN         				:  1,
	SW_RESET                :  1,
	SYNC_DROP         		:  2,
	                		:  2,
	SYNC_FOUND              :  2,
	                		:  2,
	SYNC_TYPE             	:  2,
	                		:  6,
	BUF_IDX                 :  1,
	                		:  3,
	SYNC_STATE             	:  2,
	                		:  2,
	NO_SYNC                	:  1,
	                		:  3;
} CDIP4_2ND;

typedef struct {
	UINT32
	dtype        			:  4,
	q_len 	         		:  4,
	gpb_idx         		:  6,
	gpb_chan                :  1,
	wptr_auto         		:  1,
	                		:  8,
	wptr_upd              	:  1,
	                		:  3,
	rst             		:  1,
	                		:  2,
	en                 		:  1;
} BDRC0_3;

typedef struct {
	UINT32
	buf_e_lev        		:  28,
			         		:  4;
} BDRC_LEV0_3;

/* added at L8 B0
 * End */

/*-----------------------------------------------------------------------------
	0x0070 mrq_pri ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	bdrc0                           : 4,	//  0: 3
	bdrc1                           : 4,	//  4: 7
	bdrc2                           : 4,	//  8:11
	bdrc3                           : 4,	// 12:15
	sdmwc                           : 4;	// 16:19
} MRQ_PRI;

/*-----------------------------------------------------------------------------
	0x0080~0x009c stcc0 ~ stcc7 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	latch_en						: 1,	//	   0
	copy_en 						: 1,	//	   1
									: 2,	//	2: 3 reserved
	rate_ctrl						: 1,	//	   4
									: 3,	//	5: 7 reserved
	rd_mode 						: 1,	//	   8
									: 7,	//  9:15 reserved
	pcr_pid                         :13,	// 16:28
	chan                            : 1,	//    29
	rst                             : 1,	//    30
	en                              : 1;	//    31

	UINT32
	stcc_41_10						;		// 31: 0

	UINT32
	pcr_41_10                      ;   	// 31: 0

	UINT32
	pcr_9_0                         :10,	//  0: 9
	                                : 6,	//  9:15 reserved
	stcc_9_0                        :10,	// 16:25
									: 6;	// 26:31 reserved
} STCC;

/*-----------------------------------------------------------------------------
	0x00a0 sub_stcc_rate ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	ctrl_step                       :22,	//  0:21
	                                : 2,	// 22:23 reserved
	high                            : 1,	//    24
	mismatch                        : 1,	//    25
	                                : 2,	// 26:27 reserved
	stcc_num                        : 1;	//    28
} SUB_STCC_RATE;

/*-----------------------------------------------------------------------------
	0x00a8~0x00ac stcc_err_ctrl0 ~ stcc_err_ctrl1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	err_max                         :28,	//  0:27
	                                : 3,	// 28:30 reserved
	en                              : 1;	//    31
} STCC_ERR_CTRL;

/*-----------------------------------------------------------------------------
	0x00b0 stcc_asg ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	main                            : 1,	//     0
	                                :15,	//  1:15 reserved
	vid0                            : 1,	//    16
	vid1                            : 1,	//    17
	aud0                            : 1,	//    18
	aud1                            : 1;	//    19
} STCC_ASG;

/*-----------------------------------------------------------------------------
	0x00b4~0x00b8 byte_cnt0 ~ byte_cnt1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	byte_cnt                        ;   	// 31: 0
} BYTE_CNT;

/*-----------------------------------------------------------------------------
	0x00c0 stcc_g ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	stcc_41_10                      ,   	// 31: 0
	rst                             : 1,	//    31
									: 21,	//	30: 10 reserved
	stcc_9_0                        : 10;   // 9: 0
} STCC_G;

/*-----------------------------------------------------------------------------
	0x00c0 gstcc ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	ext_unit						:12,	// 0:11
									:4, 	// 12:15
	ext_inc 						:2, 	// 16:17
									:12,	// 18:29
	rst 							:1, 	// 30
	en								:1, 	// 31

	stcc_bs_32_1					,		// 0:31

	stcc_ext_11_0					:12,	// 0:11
	stcc_bs_0						:1, 	// 12
									:19;	// 13:31
} GSTCC;

/*-----------------------------------------------------------------------------
	0x00D0~0x00D4 stcc_g_timer0~1 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	stcc_g_timer_44_13				,		// 0:31

	stcc_g_timer_12_0				:13,	// 0:12
									:18,	// 13:30
	en								:1; 	// 31
} STCC_G_TIMER;

/*-----------------------------------------------------------------------------
	0x0100 intr_en ''
	0x0104 intr_stat ''
	0x0108 intr_rstat ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	pcr                             : 1,	//     0
	tb_dcont                        : 1,	//     1
	                                : 2,	//  2: 3 reserved
	bdrc                            : 4,	//  4: 7
	                                : 4,	//  8:11 reserved
	err_rpt                         : 1,	//    12
	                                : 3,	// 13:15 reserved
	gpb_data                        : 4,	// 16:19
	gpb_full                        : 4,	// 20:23
	tp_info                         : 2,	// 24:25
	sec_err                         : 2;	// 26:27
} INTR_EN;

typedef struct {
	UINT32
	pcr                             : 1,	//     0
	tb_dcont                        : 1,	//     1
	                                : 2,	//  2: 3 reserved
	bdrc                            : 4,	//  4: 7
	                                : 4,	//  8:11 reserved
	err_rpt                         : 1,	//    12
	                                : 3,	// 13:15 reserved
	gpb_data                        : 4,	// 16:19
	gpb_full                        : 4,	// 20:23
	tp_info                         : 2,	// 24:25
	sec_err                         : 2;	// 26:27
} INTR_STAT;

typedef struct {
	UINT32
	pcr                             : 1,	//     0
	tb_dcont                        : 1,	//     1
	                                : 2,	//  2: 3 reserved
	bdrc                            : 4,	//  4: 7
	                                : 4,	//  8:11 reserved
	err_rpt                         : 1,	//    12
	                                : 3,	// 13:15 reserved
	gpb_data                        : 4,	// 16:19
	gpb_full                        : 4,	// 20:23
	tp_info                         : 2,	// 24:25
	sec_err                         : 2;	// 26:27
} INTR_RSTAT;



/*-----------------------------------------------------------------------------
	0x0110 err_intr_en ''
	0x0114 err_intr_stat ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	test_dcont                      : 1,	//     0
	                                : 3,	//  1: 3 reserved
	sync_lost                       : 2,	//  4: 5
	sb_dropped                      : 2,	//  6: 7
	cdif_ovflow                     : 2,	//  8: 9
	cdif_wpage                      : 2,	// 10:11
	cdif_rpage                      : 2,	// 12:13
	stcc_dcont                      : 2,	// 14:15
	mpg_pd                          : 2,	// 16:17
	mpg_ts                          : 2,	// 18:19
	mpg_dup                         : 2,	// 20:21
	mpg_cc                          : 2,	// 22:23
	mpg_sd                          : 2;	// 24:25
} ERR_INTR_EN;

typedef struct {
	UINT32
	test_dcont                      : 1,	//     0
	                                : 3,	//  1: 3 reserved
	sync_lost                       : 2,	//  4: 5
	sb_dropped                      : 2,	//  6: 7
	cdif_ovflow                     : 2,	//  8: 9
	cdif_wpage                      : 2,	// 10:11
	cdif_rpage                      : 2,	// 12:13
	stcc_dcont                      : 2,	// 14:15
	mpg_pd                          : 2,	// 16:17
	mpg_ts                          : 2,	// 18:19
	mpg_dup                         : 2,	// 20:21
	mpg_cc                          : 2,	// 22:23
	mpg_sd                          : 2;	// 24:25
} ERR_INTR_STAT;

/*-----------------------------------------------------------------------------
	0x0118 test ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	vid0_rdy_en                     : 1,	//     0
	vid1_rdy_en                     : 1,	//     1
	                                : 2,	//  2: 3 reserved
	aud0_rdy_en                     : 1,	//     4
	aud1_rdy_en                     : 1,	//     5
	                                : 2,	//  6: 7 reserved
	test_port                       : 2,	//  8: 9
	                                : 2,	// 10:11 reserved
	auto_incr                       : 1,	//    12
	                                : 3,	// 13:15 reserved
	rst_pd                          : 2,	// 16:17
	rst_dsc                         : 2,	// 18:19
	rst_sys                         : 2,	// 20:21
	                                : 2,	// 22:23 reserved
	last_bval                       : 1;	//    24
} TEST;

/*-----------------------------------------------------------------------------
	0x011c version ''
------------------------------------------------------------------------------*/
/* gaius.lee - VERSION -> VERSION_ collision with ctop_regs.h */
typedef struct {
	UINT32
	version                         ;   	// 31: 0
} VERSION_;

/*-----------------------------------------------------------------------------
	0x0120 gpb_base_addr ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	                                :28,	//  0:27 reserved
	gpb_base                        : 4;	// 28:31
} GPB_BASE_ADDR;

/* added at L8 B0
 * Start */
typedef struct{
	UINT32
	VAL_POL				:1,
	TPIF_EN				:1,
	EN					:1,
	SW_RESET			:1,
	SYNC_DROP			:2,
						:2,
	SYNC_FOUND			:2,
						:2,
	SYNC_TYPE			:2,
						:6,
	BUF_IDX				:1,
						:3,
	SYNC_STATE			:2,
						:2,
	NO_SYNC				:1,
						:3;
}CDIP0_2ND;

typedef struct{
	UINT32
	DTYPE				:4,
	CDIF_RPAGE			:1,
	CDIF_WPAGE			:1,
	CDIF_OVFLOW			:1,
						:1,
	PD_WAIT				:1,
	CDIF_FULL			:1,
	CDIF_EMPTY			:1,
						:1,
	SB_DROPPED			:1,
	SYNC_LOST			:1,
	NO_WDATA			:1,
						:1,
	AV_DEC_ID			:4,
	SRC					:4,
	MAX_SB_DRP			:2,
	MIN_SB_DET			:2,
	RST					:1,
						:3;
}CDIC2;

typedef struct{
	UINT32
	PID					:13,
						:3,
	ES_DATA				:8,
						:4,
	ES_VALID			:1,
						:3;
}CDIC2_TS2PES;

/* added at L8 B0
 * End */


/*-----------------------------------------------------------------------------
	0x0000 conf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	word_align                      : 1,	//     0
	                                : 3,	//  1: 3 reserved
	save_on_err                     : 1,	//     4
	chksum_type                     : 1,	//     5
	pes_rdy_chk_a                   : 1,	//     6
	pes_rdy_chk_b                   : 1,	//     7
	dbg_cnt                         : 4;	//  8:11
} CONF;

/*-----------------------------------------------------------------------------
	0x0008 splice_cntdn ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	                                :16,	//  0:15 reserved
	splice_cntdn1                   : 8,	// 16:23
	splice_cntdn0                   : 8;	// 24:31
} SPLICE_CNTDN;

/*-----------------------------------------------------------------------------
	0x0010 ext_conf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_f_lev                       :20,	//  0:19
	gpb_ow                          : 1,	//    20
	                                : 3,	// 21:23 reserved
	dpkt_vid                        : 1,	//    24
	dpkt_dcont                      : 1,	//    25
	seci_cce                        : 1,	//    26
	seci_dcont                      : 1;	//    27
} EXT_CONF;

/*-----------------------------------------------------------------------------
	0x0014 all_pass_conf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	dscrm_en                        : 1,	//     0
	null_pkt_pass                   : 1,	//     1
	all_pass_out                    : 1,	//     2
	all_pass_rec                    : 1,	//     3
	rec_irq_cnt                     : 4,	//  4: 7
	                                :23,	//  8:30 reserved
	all_pass_en                     : 1;	//    31
} ALL_PASS_CONF;

/*-----------------------------------------------------------------------------
	0x0018 chan_stat ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	pkt_cnt                         : 8,	//  0: 7
	sm_info                         : 3,	//  8:10
	                                : 1,	//    11 reserved
	vo0                             : 1,	//    12
	vo1                             : 1,	//    13
	ao0                             : 1,	//    14
	ao1                             : 1,	//    15
	cc_err                          : 1,	//    16
	seci                            : 1,	//    17
	dup_pkt                         : 1,	//    18
	                                : 1,	//    19 reserved
	tpi_q_ovf                       : 1,	//    20
	se_q_ovf                        : 1,	//    21
	mwf_ovf                         : 1,	//    22
	                                : 1,	//    23 reserved
	zlen_pes                        : 1,	//    24
	rf_acc                          : 1,	//    25
	                                : 2,	// 26:27 reserved
	pes_rdy                         : 1,	//    28
	                                : 2,	// 29:30 reserved
	pload_pattern                   : 1;	//    31
} CHAN_STAT;

/*-----------------------------------------------------------------------------
	0x0020 cc_chk_en_l ''
	0x0024 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	cc_chk_en                       ;   	// 31: 0
} CC_CHK_EN;

/*-----------------------------------------------------------------------------
	0x0028 sec_crc_en_l ''
	0x002c h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	crc_en                          ;   	// 31: 0
} SEC_CRC_EN;

/*-----------------------------------------------------------------------------
	0x0030 sec_chksum_en_l ''
	0x0034 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	chksum_en                       ;   	// 31: 0
} SEC_CHKSUM_EN;

/*-----------------------------------------------------------------------------
	0x0038 pes_crc_en_l ''
	0x003c h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	crc16_en                        ;   	// 31: 0
} PES_CRC_EN;

/*-----------------------------------------------------------------------------
	0x0040 tpi_iconf ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	afef                            : 1,	//     0
	tpdf                            : 1,	//     1
	spf                             : 1,	//     2
	espi                            : 1,	//     3
	rai                             : 1,	//     4
	di                              : 1,	//     5
	tsc                             : 2,	//  6: 7
	tpri                            : 1,	//     8
	pusi                            : 1,	//     9
	                                : 1,	//    10 reserved
	tei                             : 1,	//    11
	                                : 4,	// 12:15 reserved
	not_tsc                         : 2,	// 16:17
	auto_sc_chk                     : 1;	//    18
} TPI_ICONF;

/*-----------------------------------------------------------------------------
	0x0044 tpi_istat ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	afef                            : 1,	//     0
	tpdf                            : 1,	//     1
	spf                             : 1,	//     2
	espi                            : 1,	//     3
	rai                             : 1,	//     4
	di                              : 1,	//     5
	tsc                             : 2,	//  6: 7
	tpri                            : 1,	//     8
	pusi                            : 1,	//     9
	tei                             : 1,	//    10
	                                : 1,	//    11 reserved
	tsc_chg                         : 1,	//    12
	auto_sc_chk                     : 1,	//    13
	                                : 2,	// 14:15 reserved
	pidf_idx                        : 6;	// 16:21
} TPI_ISTAT;

/*-----------------------------------------------------------------------------
	0x004c se_istat ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	filter_idx                      : 6,	//  0: 5
	err_type                        : 1,	//     6
	valid                           : 1;	//     7
} SE_ISTAT;

/*-----------------------------------------------------------------------------
	0x0050 gpb_f_iconf_l ''
	0x0054 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_f_iconf                     ;   	// 31: 0
} GPB_F_ICONF;

/*-----------------------------------------------------------------------------
	0x0058 gpb_f_istat_l ''
	0x005c h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_f_istat                     ;   	// 31: 0
} GPB_F_ISTAT;

/*-----------------------------------------------------------------------------
	0x0060 gpb_d_istat_l ''
	0x0064 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_d_istat                     ;   	// 31: 0
} GPB_D_ISTAT;

/*-----------------------------------------------------------------------------
	0x0070 tpdb_en_l ''
	0x0074 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	tpdb_en                         ;   	// 31: 0
} TPDB_EN;

/*-----------------------------------------------------------------------------
	0x0078 tpdb_id ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_id                          : 7;	//  0: 6
} TPDB_ID;

/*-----------------------------------------------------------------------------
	0x007c v_seq_err_code ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	v_seq_err_code                  ;   	// 31: 0
} V_SEQ_ERR_CODE;

/*-----------------------------------------------------------------------------
	0x0080 afedb_en_l ''
	0x0084 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	afedb_en                        ;   	// 31: 0
} AFEDB_EN;

/*-----------------------------------------------------------------------------
	0x0088 afedb_id ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_id                          : 7;	//  0: 6
} AFEDB_ID;

/*-----------------------------------------------------------------------------
	0x008c pidf_map ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	pidf_u_bnd                      : 6,	//  0: 5
	                                :10,	//  6:15 reserved
	pidf_l_bnd                      : 6,	// 16:21
	                                : 9,	// 22:30 reserved
	map_en                          : 1;	//    31
} PIDF_MAP;

/*-----------------------------------------------------------------------------
	0x0090 pidf_addr ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	pidf_idx                        : 6;	//  0: 5
} PIDF_ADDR;

/*-----------------------------------------------------------------------------
	0x0094 pidf_data ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	pidf_data                     ;   	// 31: 0
} PIDF_DATA;

/*-----------------------------------------------------------------------------
	0x0098 kmem_addr ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	                                : 2,	//  0: 1 reserved
	word_idx                        : 3,	//  2: 4
	odd_key                         : 1,	//     5
	pid_idx                         : 6,	//  6:11
	key_type                        : 2;	// 12:13
} KMEM_ADDR;

/*-----------------------------------------------------------------------------
	0x009c kmem_data ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	kmem_data                       ;   	// 31: 0
} KMEM_DATA;

/*-----------------------------------------------------------------------------
	0x00a0 secf_addr ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	word_idx                        : 4,	//  0: 3
	secf_idx                        : 6;	//  4: 9
} SECF_ADDR;

/*-----------------------------------------------------------------------------
	0x00a4 secf_data ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	secf_data                       ;   	// 31: 0
} SECF_DATA;

/*-----------------------------------------------------------------------------
	0x00b0 secf_en_l ''
	0x00b4 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	secf_en                         ;   	// 31: 0
} SECF_EN;

/*-----------------------------------------------------------------------------
	0x00b8 secf_mtype_l ''
	0x00bc h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	secf_mtype                      ;   	// 31: 0
} SECF_MTYPE;

/*-----------------------------------------------------------------------------
	0x00c0 secfb_valid_l ''
	0x00c4 h ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	secfb_valid                     ;   	// 31: 0
} SECFB_VALID;

/*-----------------------------------------------------------------------------
	0x0100~0x01fc gpb_bnd0 ~ gpb_bnd63 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	u_bnd                           :16,	//  0:15
	l_bnd                           :16;	// 16:31
} GPB_BND;

/*-----------------------------------------------------------------------------
	0x0200~0x02fc gpb_w_ptr0 ~ gpb_w_ptr63 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_w_ptr                       :28;	//  0:27
} GPB_W_PTR;

/*-----------------------------------------------------------------------------
	0x0300~0x03fc gpb_r_ptr0 ~ gpb_r_ptr63 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	gpb_r_ptr                       :28;	//  0:27
} GPB_R_PTR;

/*-----------------------------------------------------------------------------
	0x0400~0x05fc secf_map0 ~ secf_map127 ''
------------------------------------------------------------------------------*/
typedef struct {
	UINT32
	secf_map                        ;   	// 31: 0
} SECF_MAP;

/* 362 regs, 46 types */

/* 401 regs, 69 types in Total*/

/*
 * _m : module name - SDTOP, SDIO, MPG...
 * _r : register name
 */
#define SD_RdFL_H13A0(_m, _r)		((gpReg##_m##_H13A0->_r)=(gpRealReg##_m##_H13A0->_r))
#define SD_RdFL_M14A0(_m, _r)		((gpReg##_m##_M14A0->_r)=(gpRealReg##_m##_M14A0->_r))
#define SD_RdFL_M14B0(_m, _r)		((gpReg##_m##_M14B0->_r)=(gpRealReg##_m##_M14B0->_r))
#define SD_RdFL_H14A0(_m, _r)		((gpReg##_m##_H14A0->_r)=(gpRealReg##_m##_H14A0->_r))

#define SD_WrFL_H13A0(_m, _r)			((gpRealReg##_m##_H13A0->_r)=(gpReg##_m##_H13A0->_r))
#define SD_WrFL_M14A0(_m, _r)			((gpRealReg##_m##_M14A0->_r)=(gpReg##_m##_M14A0->_r))
#define SD_WrFL_M14B0(_m, _r)			((gpRealReg##_m##_M14B0->_r)=(gpReg##_m##_M14B0->_r))
#define SD_WrFL_H14A0(_m, _r)			((gpRealReg##_m##_H14A0->_r)=(gpReg##_m##_H14A0->_r))

#define SD_Rd_H13A0(_m, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_H13A0->_r))))
#define SD_Rd_M14A0(_m, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_M14A0->_r))))
#define SD_Rd_M14B0(_m, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_M14B0->_r))))
#define SD_Rd_H14A0(_m, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_H14A0->_r))))

#define SD_Wr_H13A0(_m, _r, _v)				(*((UINT32*)(&(gpReg##_m##_H13A0->_r))) =	_v)
#define SD_Wr_M14A0(_m, _r, _v)				(*((UINT32*)(&(gpReg##_m##_M14A0->_r))) =	_v)
#define SD_Wr_M14B0(_m, _r, _v)				(*((UINT32*)(&(gpReg##_m##_M14B0->_r))) =	_v)
#define SD_Wr_H14A0(_m, _r, _v)				(*((UINT32*)(&(gpReg##_m##_H14A0->_r))) =	_v)

#define SD_Rd01_H13A0(_m, _r, _f01, _v01)		(_v01 = (gpReg##_m##_H13A0->_r._f01))
#define SD_Rd01_M14A0(_m, _r, _f01, _v01)		(_v01 = (gpReg##_m##_M14A0->_r._f01))
#define SD_Rd01_M14B0(_m, _r, _f01, _v01)		(_v01 = (gpReg##_m##_M14B0->_r._f01))
#define SD_Rd01_H14A0(_m, _r, _f01, _v01)		(_v01 = (gpReg##_m##_H14A0->_r._f01))

#define SD_Wr01_H13A0(_m, _r, _f01, _v01)	((gpReg##_m##_H13A0->_r._f01) = _v01)
#define SD_Wr01_M14A0(_m, _r, _f01, _v01)	((gpReg##_m##_M14A0->_r._f01) = _v01)
#define SD_Wr01_M14B0(_m, _r, _f01, _v01)	((gpReg##_m##_M14B0->_r._f01) = _v01)
#define SD_Wr01_H14A0(_m, _r, _f01, _v01)	((gpReg##_m##_H14A0->_r._f01) = _v01)

#define SD_RdFL(_m, _r)																	\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_RdFL_H14A0(_m, _r);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_RdFL_M14B0(_m, _r);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_RdFL_M14A0(_m, _r);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_RdFL_H13A0(_m, _r);								\
								} while(0)

#define SD_WrFL(_m, _r)																	\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_WrFL_H14A0(_m, _r);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_WrFL_M14B0(_m, _r);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_WrFL_M14A0(_m, _r);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_WrFL_H13A0(_m, _r);								\
								} while(0)

#define SD_Rd(_m, _r, _v)																	\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Rd_H14A0(_m, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Rd_M14B0(_m, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Rd_M14A0(_m, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Rd_H13A0(_m, _r, _v);						\
								} while(0)

#define SD_Wr(_m, _r, _v)																	\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Wr_H14A0(_m, _r, _v);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Wr_M14B0(_m, _r, _v);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Wr_M14A0(_m, _r, _v);								\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Wr_H13A0(_m, _r, _v);								\
								} while(0)

#define SD_Rd01(_m, _r, _f01, _v01)														\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Rd01_H14A0(_m, _r, _f01, _v01);					\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Rd01_M14B0(_m, _r, _f01, _v01);					\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Rd01_M14A0(_m, _r, _f01, _v01);					\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Rd01_H13A0(_m, _r, _f01, _v01);					\
								} while(0)

#define SD_Wr01(_m, _r, _f01, _v01)														\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Wr01_H14A0(_m, _r, _f01, _v01);					\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Wr01_M14B0(_m, _r, _f01, _v01);					\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Wr01_M14A0(_m, _r, _f01, _v01);					\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Wr01_H13A0(_m, _r, _f01, _v01);					\
								} while(0)

// Indexed Register Access
#define SD_RdFL_IDX_H13A0(_m, _i, _r)		((gpReg##_m##_H13A0[_i]->_r)=(gpRealReg##_m##_H13A0[_i]->_r))
#define SD_WrFL_IDX_H13A0(_m, _i, _r)		((gpRealReg##_m##_H13A0[_i]->_r)=(gpReg##_m##_H13A0[_i]->_r))
#define SD_Rd_IDX_H13A0(_m, _i, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_H13A0[_i]->_r))))
#define SD_Wr_IDX_H13A0(_m, _i, _r, _v)				(*((UINT32*)(&(gpReg##_m##_H13A0[_i]->_r))) =	_v)
#define SD_Rd01_IDX_H13A0(_m, _i, _r, _f01, _v01)	(_v01 = (gpReg##_m##_H13A0[_i]->_r._f01))
#define SD_Wr01_IDX_H13A0(_m, _i, _r, _f01, _v01)	((gpReg##_m##_H13A0[_i]->_r._f01) = _v01)

#define SD_RdFL_IDX_M14A0(_m, _i, _r)		((gpReg##_m##_M14A0[_i]->_r)=(gpRealReg##_m##_M14A0[_i]->_r))
#define SD_WrFL_IDX_M14A0(_m, _i, _r)		((gpRealReg##_m##_M14A0[_i]->_r)=(gpReg##_m##_M14A0[_i]->_r))
#define SD_Rd_IDX_M14A0(_m, _i, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_M14A0[_i]->_r))))
#define SD_Wr_IDX_M14A0(_m, _i, _r, _v)				(*((UINT32*)(&(gpReg##_m##_M14A0[_i]->_r))) =	_v)
#define SD_Rd01_IDX_M14A0(_m, _i, _r, _f01, _v01)	(_v01 = (gpReg##_m##_M14A0[_i]->_r._f01))
#define SD_Wr01_IDX_M14A0(_m, _i, _r, _f01, _v01)	((gpReg##_m##_M14A0[_i]->_r._f01) = _v01)

#define SD_RdFL_IDX_M14B0(_m, _i, _r)		((gpReg##_m##_M14B0[_i]->_r)=(gpRealReg##_m##_M14B0[_i]->_r))
#define SD_WrFL_IDX_M14B0(_m, _i, _r)		((gpRealReg##_m##_M14B0[_i]->_r)=(gpReg##_m##_M14B0[_i]->_r))
#define SD_Rd_IDX_M14B0(_m, _i, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_M14B0[_i]->_r))))
#define SD_Wr_IDX_M14B0(_m, _i, _r, _v)				(*((UINT32*)(&(gpReg##_m##_M14B0[_i]->_r))) =	_v)
#define SD_Rd01_IDX_M14B0(_m, _i, _r, _f01, _v01)	(_v01 = (gpReg##_m##_M14B0[_i]->_r._f01))
#define SD_Wr01_IDX_M14B0(_m, _i, _r, _f01, _v01)	((gpReg##_m##_M14B0[_i]->_r._f01) = _v01)

#define SD_RdFL_IDX_H14A0(_m, _i, _r)		((gpReg##_m##_H14A0[_i]->_r)=(gpRealReg##_m##_H14A0[_i]->_r))
#define SD_WrFL_IDX_H14A0(_m, _i, _r)		((gpRealReg##_m##_H14A0[_i]->_r)=(gpReg##_m##_H14A0[_i]->_r))
#define SD_Rd_IDX_H14A0(_m, _i, _r, _v)				(_v = *((UINT32*)(&(gpReg##_m##_H14A0[_i]->_r))))
#define SD_Wr_IDX_H14A0(_m, _i, _r, _v)				(*((UINT32*)(&(gpReg##_m##_H14A0[_i]->_r))) =	_v)
#define SD_Rd01_IDX_H14A0(_m, _i, _r, _f01, _v01)	(_v01 = (gpReg##_m##_H14A0[_i]->_r._f01))
#define SD_Wr01_IDX_H14A0(_m, _i, _r, _f01, _v01)	((gpReg##_m##_H14A0[_i]->_r._f01) = _v01)

#define SD_RdFL_IDX(_m, _i, _r)															\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_RdFL_IDX_H14A0(_m, _i, _r); 						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_RdFL_IDX_M14B0(_m, _i, _r); 						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_RdFL_IDX_M14A0(_m, _i, _r); 						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_RdFL_IDX_H13A0(_m, _i, _r); 						\
								} while(0)

#define SD_WrFL_IDX(_m, _i, _r)															\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_WrFL_IDX_H14A0(_m, _i, _r); 						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_WrFL_IDX_M14B0(_m, _i, _r); 						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_WrFL_IDX_M14A0(_m, _i, _r); 						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_WrFL_IDX_H13A0(_m, _i, _r); 						\
								} while(0)

#define SD_Rd_IDX(_m, _i, _r, _v)															\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Rd_IDX_H14A0(_m, _i, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Rd_IDX_M14B0(_m, _i, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Rd_IDX_M14A0(_m, _i, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Rd_IDX_H13A0(_m, _i, _r, _v);						\
								} while(0)

#define SD_Wr_IDX(_m, _i, _r, _v)															\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Wr_IDX_H14A0(_m, _i, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Wr_IDX_M14B0(_m, _i, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Wr_IDX_M14A0(_m, _i, _r, _v);						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Wr_IDX_H13A0(_m, _i, _r, _v);						\
								} while(0)

#define SD_Rd01_IDX(_m, _i, _r, _f01, _v01)												\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Rd01_IDX_H14A0(_m, _i, _r, _f01, _v01); 		\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Rd01_IDX_M14B0(_m, _i, _r, _f01, _v01); 		\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Rd01_IDX_M14A0(_m, _i, _r, _f01, _v01); 		\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Rd01_IDX_H13A0(_m, _i, _r, _f01, _v01); 		\
								} while(0)

#define SD_Wr01_IDX(_m, _i, _r, _f01, _v01)												\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))			\
										SD_Wr01_IDX_H14A0(_m, _i, _r, _f01, _v01); 		\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										SD_Wr01_IDX_M14B0(_m, _i, _r, _f01, _v01); 		\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										SD_Wr01_IDX_M14A0(_m, _i, _r, _f01, _v01); 		\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										SD_Wr01_IDX_H13A0(_m, _i, _r, _f01, _v01); 		\
								} while(0)

/*
 * @{
 * Naming for register pointer.
 * gpRealRegSDTOP : real register of SDTOP.
 * gpRegSDTOP     : shadow register.
 *
 * @def SDTOP_RdFL: Read  FLushing : Shadow <- Real.
 * @def SDTOP_WrFL: Write FLushing : Shadow -> Real.
 * @def SDTOP_Rd  : Read  whole register(UINT32) from Shadow register.
 * @def SDTOP_Wr  : Write whole register(UINT32) from Shadow register.
 * @def SDTOP_Rd01 ~ SDTOP_Rdnn: Read  given '01~nn' fields from Shadow register.
 * @def SDTOP_Wr01 ~ SDTOP_Wrnn: Write given '01~nn' fields to   Shadow register.
 * */


#define SDTOP_RdFL(_r)				SD_RdFL(SDTOP, _r)
#define SDTOP_WrFL(_r)				SD_WrFL(SDTOP, _r)

#define SDTOP_Rd(_r, _v)			SD_Rd(SDTOP, _r, _v)
#define SDTOP_Wr(_r, _v)			SD_Wr(SDTOP, _r, _v)

#define SDTOP_Rd01(_r,_f01,_v01)	SD_Rd01(SDTOP, _r, _f01, _v01)
#define SDTOP_Wr01(_r,_f01,_v01)	SD_Wr01(SDTOP, _r, _f01, _v01)

/*
 * @{
 * Naming for register pointer.
 * gpRealRegSDIO : real register of SDIO.
 * gpRegSDIO     : shadow register.
 *
 * @def SDIO_RdFL: Read  FLushing : Shadow <- Real.
 * @def SDIO_WrFL: Write FLushing : Shadow -> Real.
 * @def SDIO_Rd  : Read  whole register(UINT32) from Shadow register.
 * @def SDIO_Wr  : Write whole register(UINT32) from Shadow register.
 * @def SDIO_Rd01 ~ SDIO_Rdnn: Read  given '01~nn' fields from Shadow register.
 * @def SDIO_Wr01 ~ SDIO_Wrnn: Write given '01~nn' fields to   Shadow register.
 * */
#if 0 // jinhwan.bae for 2 Core ready from M14B0~
#define SDIO_RdFL(_r)			SD_RdFL(SDIO, _r)
#define SDIO_WrFL(_r)			SD_WrFL(SDIO, _r)

#define SDIO_Rd(_r,_v)			SD_Rd(SDIO, _r, _v)
#define SDIO_Wr(_r,_v)			SD_Wr(SDIO, _r, _v)

#define SDIO_Rd01(_r,_f01,_v01)	SD_Rd01(SDIO, _r, _f01, _v01)
#define SDIO_Wr01(_r,_f01,_v01)	SD_Wr01(SDIO, _r, _f01, _v01)
#endif
#define SDIO_RdFL(_i, _r)			SD_RdFL_IDX(SDIO,_i, _r)
#define SDIO_WrFL(_i,_r)			SD_WrFL_IDX(SDIO,_i, _r)

#define SDIO_Rd(_i, _r, _v)			SD_Rd_IDX(SDIO,_i, _r, _v)
#define SDIO_Wr(_i, _r,_v)			SD_Wr_IDX(SDIO,_i, _r, _v)

#define SDIO_Rd01(_i, _r,_f01,_v01)	SD_Rd01_IDX(SDIO,_i, _r,_f01,_v01)
#define SDIO_Wr01(_i, _r,_f01,_v01)	SD_Wr01_IDX(SDIO,_i, _r,_f01,_v01)


/*
 * @{
 * Naming for register pointer.
 * gpRealRegMPG : real register of MPG.
 * gpRegMPG     : shadow register.
 *
 * @def MPG_RdFL: Read  FLushing : Shadow <- Real.
 * @def MPG_WrFL: Write FLushing : Shadow -> Real.
 * @def MPG_Rd  : Read  whole register(UINT32) from Shadow register.
 * @def MPG_Wr  : Write whole register(UINT32) from Shadow register.
 * @def MPG_Rd01 ~ MPG_Rdnn: Read  given '01~nn' fields from Shadow register.
 * @def MPG_Wr01 ~ MPG_Wrnn: Write given '01~nn' fields to   Shadow register.
 * */
#define MPG_RdFL(_i, _r)			\
									do{						\
										if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){	\
											if(_i > 3) _i = _i - 2;					\
										}											\
										SD_RdFL_IDX(SDMPG,_i, _r);					\
									}while(0)

#define MPG_WrFL(_i,_r)				\
									do{						\
										if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){	\
											if(_i > 3) _i = _i - 2;					\
										}											\
										SD_WrFL_IDX(SDMPG,_i, _r);					\
									}while(0)

#define MPG_Rd(_i, _r, _v)			\
									do{						\
										if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){	\
											if(_i > 3) _i = _i - 2;					\
										}											\
										SD_Rd_IDX(SDMPG,_i, _r, _v);				\
									}while(0)
									
#define MPG_Wr(_i, _r,_v)			\
									do{						\
										if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){	\
											if(_i > 3) _i = _i - 2;					\
										}											\
										SD_Wr_IDX(SDMPG,_i, _r, _v);				\
									}while(0)

#define MPG_Rd01(_i, _r,_f01,_v01)	\
									do{						\
										if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){	\
											if(_i > 3) _i = _i - 2;					\
										}											\
										SD_Rd01_IDX(SDMPG,_i, _r,_f01,_v01);		\
									}while(0)
									
#define MPG_Wr01(_i, _r,_f01,_v01)	\
									do{						\
										if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){	\
											if(_i > 3) _i = _i - 2;					\
										}											\
										SD_Wr01_IDX(SDMPG,_i, _r,_f01,_v01);		\
									}while(0)

#if 0
#define MPG_RdFL(_i, _r)			SD_RdFL_IDX(SDMPG,_i, _r)
#define MPG_WrFL(_i,_r)				SD_WrFL_IDX(SDMPG,_i, _r)

#define MPG_Rd(_i, _r, _v)			SD_Rd_IDX(SDMPG,_i, _r, _v)
#define MPG_Wr(_i, _r,_v)			SD_Wr_IDX(SDMPG,_i, _r, _v)

#define MPG_Rd01(_i, _r,_f01,_v01)	SD_Rd01_IDX(SDMPG,_i, _r,_f01,_v01)
#define MPG_Wr01(_i, _r,_f01,_v01)	SD_Wr01_IDX(SDMPG,_i, _r,_f01,_v01)
#endif

#define gpRegSDTOP_H13A0		stSDEC_TOP_RegH13A0
#define gpRealRegSDTOP_H13A0	stSDEC_TOP_RegShadowH13A0
#define gpRegSDIO_H13A0			stpSDEC_IO_RegShadowH13A0
#define gpRealRegSDIO_H13A0		stSDEC_IO_RegH13A0
#define gpRegSDMPG_H13A0		stpSDEC_MPG_RegShadowH13A0
#define gpRealRegSDMPG_H13A0	stSDEC_MPG_RegH13A0

#define gpRegSDTOP_M14A0		stSDEC_TOP_RegM14A0
#define gpRealRegSDTOP_M14A0	stSDEC_TOP_RegShadowM14A0
#define gpRegSDIO_M14A0			stpSDEC_IO_RegShadowM14A0
#define gpRealRegSDIO_M14A0		stSDEC_IO_RegM14A0
#define gpRegSDMPG_M14A0		stpSDEC_MPG_RegShadowM14A0
#define gpRealRegSDMPG_M14A0	stSDEC_MPG_RegM14A0

#define gpRegSDTOP_M14B0		stSDEC_TOP_RegM14B0
#define gpRealRegSDTOP_M14B0	stSDEC_TOP_RegShadowM14B0
#define gpRegSDIO_M14B0			stpSDEC_IO_RegShadowM14B0
#define gpRealRegSDIO_M14B0		stSDEC_IO_RegM14B0
#define gpRegSDMPG_M14B0		stpSDEC_MPG_RegShadowM14B0
#define gpRealRegSDMPG_M14B0	stSDEC_MPG_RegM14B0

#define gpRegSDTOP_H14A0		stSDEC_TOP_RegH14A0
#define gpRealRegSDTOP_H14A0	stSDEC_TOP_RegShadowH14A0
#define gpRegSDIO_H14A0			stpSDEC_IO_RegShadowH14A0
#define gpRealRegSDIO_H14A0		stSDEC_IO_RegH14A0
#define gpRegSDMPG_H14A0		stpSDEC_MPG_RegShadowH14A0
#define gpRealRegSDMPG_H14A0	stSDEC_MPG_RegH14A0

#if 1 // PIDF_DATA Register Shadow
#define gRegShaShadowPidfData			stSDEC_MPG_RegPidfData
// read data from shashadow to shadow's pidf_data
#define MPG_RdFL_SShadow(_i, _r)	\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){				\
											if(_i > 3) _i = _i - 2;							\
									}														\
																							\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))				\
										((gpRegSDMPG_H14A0[_i]->pidf_data.pidf_data)=(gRegShaShadowPidfData[_i][_r]));						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										((gpRegSDMPG_M14B0[_i]->pidf_data.pidf_data)=(gRegShaShadowPidfData[_i][_r]));						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										((gpRegSDMPG_M14A0[_i]->pidf_data.pidf_data)=(gRegShaShadowPidfData[_i][_r]));						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										((gpRegSDMPG_H13A0[_i]->pidf_data.pidf_data)=(gRegShaShadowPidfData[_i][_r]));						\
								} while(0)

// write data from shadow's pidf_data to shashadow
#define MPG_WrFL_SShadow(_i, _r)		\
								do {														\
									if(lx_chip_rev() >= LX_CHIP_REV(M14, B0)){				\
											if(_i > 3) _i = _i - 2;							\
									}														\
																							\
									if(lx_chip_rev() >= LX_CHIP_REV(H14, A0))				\
										((gRegShaShadowPidfData[_i][_r])=(gpRegSDMPG_H14A0[_i]->pidf_data.pidf_data));						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, B0))			\
										((gRegShaShadowPidfData[_i][_r])=(gpRegSDMPG_M14B0[_i]->pidf_data.pidf_data));						\
									else if(lx_chip_rev() >= LX_CHIP_REV(M14, A0))			\
										((gRegShaShadowPidfData[_i][_r])=(gpRegSDMPG_M14A0[_i]->pidf_data.pidf_data));						\
									else if(lx_chip_rev() >= LX_CHIP_REV(H13, A0))			\
										((gRegShaShadowPidfData[_i][_r])=(gpRegSDMPG_H13A0[_i]->pidf_data.pidf_data));						\
								} while(0)

#endif

#ifdef __cplusplus
}
#endif

#endif

/* from 'LG1150-TE-MAN-01 v1.2 (SDEC register description-20100303).csv' 20100311 00:54:20   대한민국 표준시 by getregs v2.3 */
