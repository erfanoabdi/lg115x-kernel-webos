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

/*-----------------------------------------------------------------------------
	0x0000 vid_out_sel ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_VID_OUT_SEL[] = {                      /* 0x0000 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 0, "vid0_sel"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 4, "vid1_sel"                        },
};

/*-----------------------------------------------------------------------------
	0x0004 dl_sel ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_DL_SEL[] = {                           /* 0x0004 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "dl0_sel"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "dl1_sel"                         },
};

/*-----------------------------------------------------------------------------
	0x0000~0x000c cdip0 ~ cdip3 ''
------------------------------------------------------------------------------*/
/*	no field */

/*-----------------------------------------------------------------------------
	0x0010~0x0014 cdiop0 ~ cdiop1 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CDIOP0[] = {                           /* 0x0010 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 0, "dtype"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 4, "sync_type"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 14, 8, "clk_div"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,16, "pconf"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,18, "val_sel"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 20,20, "req_en"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,21, "val_en"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 22,22, "err_en"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "req_act_lo"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,25, "val_act_lo"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 26,26, "clk_act_lo"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 27,27, "err_act_hi"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 30,30, "test_en"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "en"                              },
};

/*-----------------------------------------------------------------------------
	0x0018 cdin_parallel_sel ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CDIN_PARALLEL_SEL[] = {                /* 0x0018 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "p_sel"                           },
};

/*-----------------------------------------------------------------------------
	0x0020~0x0024 cdic0 ~ cdic1 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CDIC0[] = {                            /* 0x0020 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 0, "dtype"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "cdif_rpage"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 5, "cdif_wpage"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  6, 6, "cdif_ovflow"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  8, 8, "pd_wait"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 9, "cdif_full"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 10,10, "cdif_empty"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 12,12, "sb_dropped"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,13, "sync_lost"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 14,14, "no_wdata"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,16, "av_dec_id"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 23,20, "src"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,24, "max_sb_drp"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 27,26, "min_sb_det"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 28,28, "rst"                             },
};

/*-----------------------------------------------------------------------------
	0x0028~0x002c cdic_dsc0 ~ cdic_dsc1 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CDIC_DSC0[] = {                        /* 0x0028 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  2, 0, "cas_type"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 4, "blk_mode"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11, 8, "res_mode"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,12, "key_size"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,16, "even_mode"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,20, "odd_mode"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "psc_en"                          },
};

/*-----------------------------------------------------------------------------
	0x0030~0x0034 cdoc0 ~ cdoc1 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CDOC0[] = {                            /* 0x0030 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 27, 0, "buf_e_lev"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 0, "dtype"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0,  2, 0, "id_value"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 3, "id_msb"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 4, "q_len"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0,  6, 4, "id_mask"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13, 8, "gpb_idx"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0,  8, 8, "tso_src"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 14,14, "gpb_chan"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 15,15, "wptr_auto"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,16, "src"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "wptr_upd"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 28,28, "rst"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 28,28, "rst"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "en"                              },
};

/*-----------------------------------------------------------------------------
	0x0070 mrq_pri ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_MRQ_PRI[] = {                          /* 0x0070 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 0, "bdrc0"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 4, "bdrc1"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11, 8, "bdrc2"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0, 15,12, "bdrc3"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,16, "sdmwc"                           },
};

/*-----------------------------------------------------------------------------
	0x0080~0x009c stcc0 ~ stcc7 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_STCC0[] = {                            /* 0x0080 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 0, "pcr_9_0"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "pcr_41_10"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "stcc_41_10"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "latch_en"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 1, "copy_en"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "rate_ctrl"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0,  8, 8, "rd_mode"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,16, "stcc_9_0"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 28,16, "pcr_pid"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 29,29, "chan"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 30,30, "rst"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "en"                              },
};

/*-----------------------------------------------------------------------------
	0x00a0 sub_stcc_rate ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SUB_STCC_RATE[] = {                    /* 0x00a0 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 21, 0, "ctrl_step"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "high"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,25, "mismatch"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 28,28, "stcc_num"                        },
};

/*-----------------------------------------------------------------------------
	0x00a8~0x00ac stcc_err_ctrl0 ~ stcc_err_ctrl1 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_STCC_ERR_CTRL0[] = {                   /* 0x00a8 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 27, 0, "err_max"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "en"                              },
};

/*-----------------------------------------------------------------------------
	0x00b0 stcc_asg ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_STCC_ASG[] = {                         /* 0x00b0 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "main"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 16,16, "vid0"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,17, "vid1"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 18,18, "aud0"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,19, "aud1"                            },
};

/*-----------------------------------------------------------------------------
	0x00b4~0x00b8 byte_cnt0 ~ byte_cnt1 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_BYTE_CNT0[] = {                        /* 0x00b4 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "byte_cnt"                        },
};

/*-----------------------------------------------------------------------------
	0x00c0 stcc_g ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_STCC_G[] = {                           /* 0x00c0 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "stcc_41_10"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "rst"                             },
};

/*-----------------------------------------------------------------------------
	0x0100 intr_en ''
	0x0104 intr_stat ''
	0x0108 intr_rstat ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_INTR_EN[] = {                          /* 0x0100 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "pcr"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 1, "tb_dcont"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 4, "bdrc"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 12,12, "err_rpt"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,16, "gpb_data"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 23,20, "gpb_full"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,24, "tp_info"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 27,26, "sec_err"                         },
};

/*-----------------------------------------------------------------------------
	0x0110 err_intr_en ''
	0x0114 err_intr_stat ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_ERR_INTR_EN[] = {                      /* 0x0110 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "test_dcont"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 4, "sync_lost"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 6, "sb_dropped"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 8, "cdif_ovflow"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11,10, "cdif_wpage"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,12, "cdif_rpage"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 15,14, "stcc_dcont"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,16, "mpg_pd"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,18, "mpg_ts"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,20, "mpg_dup"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 23,22, "mpg_cc"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,24, "mpg_sd"                          },
};

/*-----------------------------------------------------------------------------
	0x0118 test ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_TEST[] = {                             /* 0x0118 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "vid0_rdy_en"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 1, "vid1_rdy_en"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "aud0_rdy_en"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 5, "aud1_rdy_en"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 8, "test_port"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 12,12, "auto_incr"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,16, "rst_pd"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 19,18, "rst_dsc"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,20, "rst_sys"                         },
};

/*-----------------------------------------------------------------------------
	0x011c version ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_VERSION[] = {                          /* 0x011c */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "version"                         },
};

/*-----------------------------------------------------------------------------
	0x0120 gpb_base_addr ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_BASE_ADDR[] = {                    /* 0x0120 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,28, "gpb_base"                        },
};

/*-----------------------------------------------------------------------------
	0x0000 conf ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CONF[] = {                             /* 0x0000 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "word_align"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "save_on_err"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 5, "chksum_type"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0,  6, 6, "pes_rdy_chk_a"                   },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 7, "pes_rdy_chk_b"                   },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11, 8, "dbg_cnt"                         },
};

/*-----------------------------------------------------------------------------
	0x0008 splice_cntdn ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SPLICE_CNTDN[] = {                     /* 0x0008 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 23,16, "splice_cntdn1"                   },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,24, "splice_cntdn0"                   },
};

/*-----------------------------------------------------------------------------
	0x0010 ext_conf ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_EXT_CONF[] = {                         /* 0x0010 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 19, 0, "gpb_f_lev"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 20,20, "gpb_ow"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "dpkt_vid"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,25, "dpkt_dcont"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 26,26, "seci_cce"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 27,27, "seci_dcont"                      },
};

/*-----------------------------------------------------------------------------
	0x0014 all_pass_conf ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_ALL_PASS_CONF[] = {                    /* 0x0014 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "dscrm_en"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 1, "null_pkt_pass"                   },
	{ REG_XS_N_RW,  0, 0,  0, 0,  2, 2, "all_pass_out"                    },
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 3, "all_pass_rec"                    },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 4, "rec_irq_cnt"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "all_pass_en"                     },
};

/*-----------------------------------------------------------------------------
	0x0018 chan_stat ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CHAN_STAT[] = {                        /* 0x0018 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 0, "pkt_cnt"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 10, 8, "sm_info"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 12,12, "vo0"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,13, "vo1"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 14,14, "ao0"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 15,15, "ao1"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 16,16, "cc_err"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,17, "seci"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 18,18, "dup_pkt"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 20,20, "tpi_q_ovf"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,21, "se_q_ovf"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 22,22, "mwf_ovf"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "zlen_pes"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0, 25,25, "rf_acc"                          },
	{ REG_XS_N_RW,  0, 0,  0, 0, 28,28, "pes_rdy"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "pload_pattern"                   },
};

/*-----------------------------------------------------------------------------
	0x0020 cc_chk_en_l ''
	0x0024 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_CC_CHK_EN_L[] = {                      /* 0x0020 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "cc_chk_en"                       },
};

/*-----------------------------------------------------------------------------
	0x0028 sec_crc_en_l ''
	0x002c h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SEC_CRC_EN_L[] = {                     /* 0x0028 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "crc_en"                          },
};

/*-----------------------------------------------------------------------------
	0x0030 sec_chksum_en_l ''
	0x0034 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SEC_CHKSUM_EN_L[] = {                  /* 0x0030 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "chksum_en"                       },
};

/*-----------------------------------------------------------------------------
	0x0038 pes_crc_en_l ''
	0x003c h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_PES_CRC_EN_L[] = {                     /* 0x0038 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "crc16_en"                        },
};

/*-----------------------------------------------------------------------------
	0x0040 tpi_iconf ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_TPI_ICONF[] = {                        /* 0x0040 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "afef"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 1, "tpdf"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  2, 2, "spf"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 3, "espi"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "rai"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 5, "di"                              },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 6, "tsc"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  8, 8, "tpri"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 9, "pusi"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11,11, "tei"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 17,16, "not_tsc"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 18,18, "auto_sc_chk"                     },
};

/*-----------------------------------------------------------------------------
	0x0044 tpi_istat ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_TPI_ISTAT[] = {                        /* 0x0044 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  0, 0, "afef"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  1, 1, "tpdf"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  2, 2, "spf"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 3, "espi"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 4, "rai"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 5, "di"                              },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 6, "tsc"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0,  8, 8, "tpri"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 9, "pusi"                            },
	{ REG_XS_N_RW,  0, 0,  0, 0, 10,10, "tei"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 12,12, "tsc_chg"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,13, "auto_sc_chk"                     },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,16, "pidf_idx"                        },
};

/*-----------------------------------------------------------------------------
	0x004c se_istat ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SE_ISTAT[] = {                         /* 0x004c */
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 0, "filter_idx"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0,  6, 6, "err_type"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  7, 7, "valid"                           },
};

/*-----------------------------------------------------------------------------
	0x0050 gpb_f_iconf_l ''
	0x0054 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_F_ICONF_L[] = {                    /* 0x0050 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "gpb_f_iconf"                     },
};

/*-----------------------------------------------------------------------------
	0x0058 gpb_f_istat_l ''
	0x005c h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_F_ISTAT_L[] = {                    /* 0x0058 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "gpb_f_istat"                     },
};

/*-----------------------------------------------------------------------------
	0x0060 gpb_d_istat_l ''
	0x0064 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_D_ISTAT_L[] = {                    /* 0x0060 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "gpb_d_istat"                     },
};

/*-----------------------------------------------------------------------------
	0x0070 tpdb_en_l ''
	0x0074 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_TPDB_EN_L[] = {                        /* 0x0070 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "tpdb_en"                         },
};

/*-----------------------------------------------------------------------------
	0x0078 tpdb_id ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_TPDB_ID[] = {                          /* 0x0078 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  6, 0, "gpb_id"                          },
};

/*-----------------------------------------------------------------------------
	0x007c v_seq_err_code ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_V_SEQ_ERR_CODE[] = {                   /* 0x007c */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "v_seq_err_code"                  },
};

/*-----------------------------------------------------------------------------
	0x0080 afedb_en_l ''
	0x0084 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_AFEDB_EN_L[] = {                       /* 0x0080 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "afedb_en"                        },
};

/*-----------------------------------------------------------------------------
	0x0088 afedb_id ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_AFEDB_ID[] = {                         /* 0x0088 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  6, 0, "gpb_id"                          },
};

/*-----------------------------------------------------------------------------
	0x008c pidf_map ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_PIDF_MAP[] = {                         /* 0x008c */
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 0, "pidf_u_bnd"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 21,16, "pidf_l_bnd"                      },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,31, "map_en"                          },
};

/*-----------------------------------------------------------------------------
	0x0090 pidf_addr ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_PIDF_ADDR[] = {                        /* 0x0090 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 0, "pidf_idx"                        },
};

/*-----------------------------------------------------------------------------
	0x0094 pidf_data ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_PIDF_DATA[] = {                        /* 0x0094 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "pidf_data"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0,  8, 6, "av_id"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 9, "ts_dn"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0, 10,10, "gpb_irq_conf"                    },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11,11, "sf_map_en"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 12,12, "pload_pes"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,13, "pes_ext_mode"                    },
	{ REG_XS_N_RW,  0, 0,  0, 0, 14,14, "no_dsc"                          },
};

/*-----------------------------------------------------------------------------
	0x0098 kmem_addr ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_KMEM_ADDR[] = {                        /* 0x0098 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  4, 2, "word_idx"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  5, 5, "odd_key"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 11, 6, "pid_idx"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 13,12, "key_type"                        },
};

/*-----------------------------------------------------------------------------
	0x009c kmem_data ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_KMEM_DATA[] = {                        /* 0x009c */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "kmem_data"                       },
};

/*-----------------------------------------------------------------------------
	0x00a0 secf_addr ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SECF_ADDR[] = {                        /* 0x00a0 */
	{ REG_XS_N_RW,  0, 0,  0, 0,  3, 0, "word_idx"                        },
	{ REG_XS_N_RW,  0, 0,  0, 0,  9, 4, "secf_idx"                        },
};

/*-----------------------------------------------------------------------------
	0x00a4 secf_data ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SECF_DATA[] = {                        /* 0x00a4 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "secf_data"                       },
	{ REG_XS_N_RW,  0, 0,  0, 0, 15, 8, "last_sec_num"                    },
	{ REG_XS_N_RW,  0, 0,  0, 0, 23,16, "sec_num"                         },
	{ REG_XS_N_RW,  0, 0,  0, 0, 24,24, "cni"                             },
	{ REG_XS_N_RW,  0, 0,  0, 0, 29,25, "ver_num"                         },
};

/*-----------------------------------------------------------------------------
	0x00b0 secf_en_l ''
	0x00b4 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SECF_EN_L[] = {                        /* 0x00b0 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "secf_en"                         },
};

/*-----------------------------------------------------------------------------
	0x00b8 secf_mtype_l ''
	0x00bc h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SECF_MTYPE_L[] = {                     /* 0x00b8 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "secf_mtype"                      },
};

/*-----------------------------------------------------------------------------
	0x00c0 secfb_valid_l ''
	0x00c4 h ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SECFB_VALID_L[] = {                    /* 0x00c0 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "secfb_valid"                     },
};

/*-----------------------------------------------------------------------------
	0x0100~0x01fc gpb_bnd0 ~ gpb_bnd63 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_BND0[] = {                         /* 0x0100 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 15, 0, "u_bnd"                           },
	{ REG_XS_N_RW,  0, 0,  0, 0, 31,16, "l_bnd"                           },
};

/*-----------------------------------------------------------------------------
	0x0200~0x02fc gpb_w_ptr0 ~ gpb_w_ptr63 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_W_PTR0[] = {                       /* 0x0200 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 27, 0, "gpb_w_ptr"                       },
};

/*-----------------------------------------------------------------------------
	0x0300~0x03fc gpb_r_ptr0 ~ gpb_r_ptr63 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_GPB_R_PTR0[] = {                       /* 0x0300 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 27, 0, "gpb_r_ptr"                       },
};

/*-----------------------------------------------------------------------------
	0x0400~0x05fc secf_map0 ~ secf_map127 ''
------------------------------------------------------------------------------*/
const DBG_REG_FIELD_T dbg_SECF_MAP0[] = {                        /* 0x0400 */
	{ REG_XS_N_RW,  0, 0,  0, 0, 31, 0, "secf_map"                        },
};

const DBG_REG_T gDbgRegInfo[] = {
{0x0000,N_FLD(dbg_VID_OUT_SEL),                     "VID_OUT_SEL"                     ,dbg_VID_OUT_SEL                     }, // 
{0x0004,N_FLD(dbg_DL_SEL),                          "DL_SEL"                          ,dbg_DL_SEL                          }, // 
{0x0000,0,                                  "CDIP0"                           ,NULL                                }, // 
{0x0010,N_FLD(dbg_CDIOP0),                          "CDIOP0"                          ,dbg_CDIOP0                          }, // 
{0x0018,N_FLD(dbg_CDIN_PARALLEL_SEL),               "CDIN_PARALLEL_SEL"               ,dbg_CDIN_PARALLEL_SEL               }, // 
{0x0020,N_FLD(dbg_CDIC0),                           "CDIC0"                           ,dbg_CDIC0                           }, // 
{0x0028,N_FLD(dbg_CDIC_DSC0),                       "CDIC_DSC0"                       ,dbg_CDIC_DSC0                       }, // 
{0x0030,N_FLD(dbg_CDOC0),                           "CDOC0"                           ,dbg_CDOC0                           }, // 
{0x0070,N_FLD(dbg_MRQ_PRI),                         "MRQ_PRI"                         ,dbg_MRQ_PRI                         }, // 
{0x0080,N_FLD(dbg_STCC0),                           "STCC0"                           ,dbg_STCC0                           }, // 
{0x00a0,N_FLD(dbg_SUB_STCC_RATE),                   "SUB_STCC_RATE"                   ,dbg_SUB_STCC_RATE                   }, // 
{0x00a8,N_FLD(dbg_STCC_ERR_CTRL0),                  "STCC_ERR_CTRL0"                  ,dbg_STCC_ERR_CTRL0                  }, // 
{0x00b0,N_FLD(dbg_STCC_ASG),                        "STCC_ASG"                        ,dbg_STCC_ASG                        }, // 
{0x00b4,N_FLD(dbg_BYTE_CNT0),                       "BYTE_CNT0"                       ,dbg_BYTE_CNT0                       }, // 
{0x00c0,N_FLD(dbg_STCC_G),                          "STCC_G"                          ,dbg_STCC_G                          }, // 
{0x0100,N_FLD(dbg_INTR_EN),                         "INTR_EN"                         ,dbg_INTR_EN                         }, // 
{0x0104,N_FLD(dbg_INTR_STAT),                       "INTR_STAT"                       ,dbg_INTR_STAT                       }, // 
{0x0108,N_FLD(dbg_INTR_RSTAT),                      "INTR_RSTAT"                      ,dbg_INTR_RSTAT                      }, // 
{0x0110,N_FLD(dbg_ERR_INTR_EN),                     "ERR_INTR_EN"                     ,dbg_ERR_INTR_EN                     }, // 
{0x0114,N_FLD(dbg_ERR_INTR_STAT),                   "ERR_INTR_STAT"                   ,dbg_ERR_INTR_STAT                   }, // 
{0x0118,N_FLD(dbg_TEST),                            "TEST"                            ,dbg_TEST                            }, // 
{0x011c,N_FLD(dbg_VERSION),                         "VERSION"                         ,dbg_VERSION                         }, // 
{0x0120,N_FLD(dbg_GPB_BASE_ADDR),                   "GPB_BASE_ADDR"                   ,dbg_GPB_BASE_ADDR                   }, // 
{0x0000,N_FLD(dbg_CONF),                            "CONF"                            ,dbg_CONF                            }, // 
{0x0008,N_FLD(dbg_SPLICE_CNTDN),                    "SPLICE_CNTDN"                    ,dbg_SPLICE_CNTDN                    }, // 
{0x0010,N_FLD(dbg_EXT_CONF),                        "EXT_CONF"                        ,dbg_EXT_CONF                        }, // 
{0x0014,N_FLD(dbg_ALL_PASS_CONF),                   "ALL_PASS_CONF"                   ,dbg_ALL_PASS_CONF                   }, // 
{0x0018,N_FLD(dbg_CHAN_STAT),                       "CHAN_STAT"                       ,dbg_CHAN_STAT                       }, // 
{0x0020,N_FLD(dbg_CC_CHK_EN_L),                     "CC_CHK_EN_L"                     ,dbg_CC_CHK_EN_L                     }, // 
{0x0024,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0028,N_FLD(dbg_SEC_CRC_EN_L),                    "SEC_CRC_EN_L"                    ,dbg_SEC_CRC_EN_L                    }, // 
{0x002c,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0030,N_FLD(dbg_SEC_CHKSUM_EN_L),                 "SEC_CHKSUM_EN_L"                 ,dbg_SEC_CHKSUM_EN_L                 }, // 
{0x0034,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0038,N_FLD(dbg_PES_CRC_EN_L),                    "PES_CRC_EN_L"                    ,dbg_PES_CRC_EN_L                    }, // 
{0x003c,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0040,N_FLD(dbg_TPI_ICONF),                       "TPI_ICONF"                       ,dbg_TPI_ICONF                       }, // 
{0x0044,N_FLD(dbg_TPI_ISTAT),                       "TPI_ISTAT"                       ,dbg_TPI_ISTAT                       }, // 
{0x004c,N_FLD(dbg_SE_ISTAT),                        "SE_ISTAT"                        ,dbg_SE_ISTAT                        }, // 
{0x0050,N_FLD(dbg_GPB_F_ICONF_L),                   "GPB_F_ICONF_L"                   ,dbg_GPB_F_ICONF_L                   }, // 
{0x0054,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0058,N_FLD(dbg_GPB_F_ISTAT_L),                   "GPB_F_ISTAT_L"                   ,dbg_GPB_F_ISTAT_L                   }, // 
{0x005c,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0060,N_FLD(dbg_GPB_D_ISTAT_L),                   "GPB_D_ISTAT_L"                   ,dbg_GPB_D_ISTAT_L                   }, // 
{0x0064,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0070,N_FLD(dbg_TPDB_EN_L),                       "TPDB_EN_L"                       ,dbg_TPDB_EN_L                       }, // 
{0x0074,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0078,N_FLD(dbg_TPDB_ID),                         "TPDB_ID"                         ,dbg_TPDB_ID                         }, // 
{0x007c,N_FLD(dbg_V_SEQ_ERR_CODE),                  "V_SEQ_ERR_CODE"                  ,dbg_V_SEQ_ERR_CODE                  }, // 
{0x0080,N_FLD(dbg_AFEDB_EN_L),                      "AFEDB_EN_L"                      ,dbg_AFEDB_EN_L                      }, // 
{0x0084,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0088,N_FLD(dbg_AFEDB_ID),                        "AFEDB_ID"                        ,dbg_AFEDB_ID                        }, // 
{0x008c,N_FLD(dbg_PIDF_MAP),                        "PIDF_MAP"                        ,dbg_PIDF_MAP                        }, // 
{0x0090,N_FLD(dbg_PIDF_ADDR),                       "PIDF_ADDR"                       ,dbg_PIDF_ADDR                       }, // 
{0x0094,N_FLD(dbg_PIDF_DATA),                       "PIDF_DATA"                       ,dbg_PIDF_DATA                       }, // 
{0x0098,N_FLD(dbg_KMEM_ADDR),                       "KMEM_ADDR"                       ,dbg_KMEM_ADDR                       }, // 
{0x009c,N_FLD(dbg_KMEM_DATA),                       "KMEM_DATA"                       ,dbg_KMEM_DATA                       }, // 
{0x00a0,N_FLD(dbg_SECF_ADDR),                       "SECF_ADDR"                       ,dbg_SECF_ADDR                       }, // 
{0x00a4,N_FLD(dbg_SECF_DATA),                       "SECF_DATA"                       ,dbg_SECF_DATA                       }, // 
{0x00b0,N_FLD(dbg_SECF_EN_L),                       "SECF_EN_L"                       ,dbg_SECF_EN_L                       }, // 
{0x00b4,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x00b8,N_FLD(dbg_SECF_MTYPE_L),                    "SECF_MTYPE_L"                    ,dbg_SECF_MTYPE_L                    }, // 
{0x00bc,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x00c0,N_FLD(dbg_SECFB_VALID_L),                   "SECFB_VALID_L"                   ,dbg_SECFB_VALID_L                   }, // 
{0x00c4,N_FLD(dbg_H),                               "H"                               ,dbg_H                               }, // 
{0x0100,N_FLD(dbg_GPB_BND0),                        "GPB_BND0"                        ,dbg_GPB_BND0                        }, // 
{0x0200,N_FLD(dbg_GPB_W_PTR0),                      "GPB_W_PTR0"                      ,dbg_GPB_W_PTR0                      }, // 
{0x0300,N_FLD(dbg_GPB_R_PTR0),                      "GPB_R_PTR0"                      ,dbg_GPB_R_PTR0                      }, // 
{0x0400,N_FLD(dbg_SECF_MAP0),                       "SECF_MAP0"                       ,dbg_SECF_MAP0                       }, // 
};

/* 401 regs, 69 types in Total*/

/* from 'LG1150-TE-MAN-01 v1.2 (SDEC register description-20100303).csv' 20100311 00:54:20   대한민국 표준시 by getregs v2.3 */
