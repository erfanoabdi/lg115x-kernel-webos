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

/** @file pe_cmg_hw_m14.c
 *
 *  driver for picture enhance color management functions. ( used only within kdriver )
 *	
 *	@author		Seung-Jun,Youm(sj.youm@lge.com)
 *	@version	0.1
 *	@note		
 *	@date		2012.03.15
 *	@see		
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include "base_types.h"
#include "os_util.h"

#include "pe_hw_m14.h"
#include "pe_reg_m14.h"
#include "pe_fwi_m14.h"
#include "pe_cmg_hw_m14.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define PE_CMG_HW_M14_ERROR	printk

#define PE_CMG_HW_M14_DBG_PRINT(fmt,args...)	\
	if(_g_cmg_hw_m14_trace) printk("[%x,%x][%s,%d] "fmt,PE_CHIP_VER,g_pe_kdrv_ver_mask,__F__,__L__,##args)
#define PE_CMG_HW_M14_CHECK_CODE(_checker,_action,fmt,args...)	\
	{if(_checker){PE_CMG_HW_M14_ERROR(fmt,##args);_action;}}

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static void PE_CMG_HW_M14A_Init_CenRegister(void);
static void PE_CMG_HW_M14B_Init_CenRegister(void);

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
static UINT32 _g_cmg_hw_m14_trace=0x0;	//default should be off.
static PE_CMG_HW_M14_SETTINGS_T _g_pe_cmg_hw_m14_info;

/*========================================================================================
	Implementation Group
========================================================================================*/
/**
 * init color management
 *
 * @param   void
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_Init(void)
{
	int ret = RET_OK;
	do{
		if(PE_KDRV_VER_M14BX)
		{
			PE_CMG_HW_M14B_Init_CenRegister();
			PE_PE1_M14B0_QWr02(pe1_cen_ctrl_00,	reg_cen_bypass,	0x1, \
												demo_mode,		0x0);
			memset(&_g_pe_cmg_hw_m14_info,0x0,sizeof(PE_CMG_HW_M14_SETTINGS_T));
		}
		else if(PE_KDRV_VER_M14AX)
		{
			PE_CMG_HW_M14A_Init_CenRegister();
			PE_P1L_M14A0_QWr01(pe1_cen_ctrl_00,	reg_cen_bypass,	0x1);	//default
			memset(&_g_pe_cmg_hw_m14_info,0xffff,sizeof(PE_CMG_HW_M14_SETTINGS_T));
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	}while(0);
	return ret;
}
/**
 * set cen initial param
 *
 * @param   void
 * @return  void
 * @see
 * @author
 */
static void PE_CMG_HW_M14B_Init_CenRegister(void)
{
	UINT32 i = 0;

	/* hue */
	PE_PE1_M14B0_Wr(pe1_cen_ia_ctrl,	0x00001000);
	PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_TBLPOINT); i++)
	{
		PE_PE1_M14B0_Wr(pe1_cen_ia_data,0x00000000);
		PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
	}
	/* saturation */
	PE_PE1_M14B0_Wr(pe1_cen_ia_ctrl,	0x00001100);
	PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_TBLPOINT); i++)
	{
		PE_PE1_M14B0_Wr(pe1_cen_ia_data,0x00000000);
		PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
	}
	/* value */
	PE_PE1_M14B0_Wr(pe1_cen_ia_ctrl,	0x00001200);
	PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_TBLPOINT); i++)
	{
		PE_PE1_M14B0_Wr(pe1_cen_ia_data,0x00000000);
		PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
	}
	/* region debug color */
	PE_PE1_M14B0_Wr(pe1_cen_ia_ctrl,	0x00001300);
	PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM); i++)
	{
		PE_PE1_M14B0_Wr(pe1_cen_ia_data,0x00000000);
		PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
	}
	/* global delta gain */
	PE_PE1_M14B0_Wr(pe1_cen_ia_ctrl,	0x00001600);
	PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_DELTANUM); i++)
	{
		PE_PE1_M14B0_Wr(pe1_cen_ia_data,0x00000000);
		PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
	}
	/* normal mode */
	PE_PE1_M14B0_Wr(pe1_cen_ia_ctrl,	0x00008000);
	PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);

	/* region delta gain */
	PE_PE1_M14B0_Wr(pe1_cen_delta_ia_ctrl,	0x00001000);
	PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_DELTA_SETNUM); i++)
	{
		PE_PE1_M14B0_Wr(pe1_cen_delta_ia_data,0x00000000);
		PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_data);
	}
	/* normal mode */
	PE_PE1_M14B0_Wr(pe1_cen_delta_ia_ctrl,	0x00008000);
	PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_ctrl);

	/* master gain */
	PE_PE1_M14B0_Wr(pe1_cen_ctrl_04,	0x00000000);
	PE_PE1_M14B0_WrFL(pe1_cen_ctrl_04);
	PE_PE1_M14B0_Wr(pe1_cen_ctrl_05,	0x00000000);
	PE_PE1_M14B0_WrFL(pe1_cen_ctrl_05);
	PE_PE1_M14B0_Wr(pe1_cen_ctrl_06,	0x00000000);
	PE_PE1_M14B0_WrFL(pe1_cen_ctrl_06);
	PE_PE1_M14B0_Wr(pe1_cen_ctrl_07,	0x00000000);
	PE_PE1_M14B0_WrFL(pe1_cen_ctrl_07);

	return;
}
/**
 * set cen initial param
 *
 * @param   void
 * @return  void
 * @see
 * @author
 */
static void PE_CMG_HW_M14A_Init_CenRegister(void)
{
	UINT32 i;
	/* hue */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00001000);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_TBLPOINT); i++)
	{
		PE_P1L_M14A0_Wr(pe1_cen_ia_data,0x00000000);
		PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
	}
	/* saturation */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00001100);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_TBLPOINT); i++)
	{
		PE_P1L_M14A0_Wr(pe1_cen_ia_data,0x00000000);
		PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
	}
	/* value */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00001200);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_TBLPOINT); i++)
	{
		PE_P1L_M14A0_Wr(pe1_cen_ia_data,0x00000000);
		PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
	}
	/* debug color */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00001300);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM); i++)
	{
		PE_P1L_M14A0_Wr(pe1_cen_ia_data,0x00000000);
		PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
	}
	/* delta */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00001400);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM*LX_PE_CMG_DELTA_SETNUM); i++)
	{
		PE_P1L_M14A0_Wr(pe1_cen_ia_data,0x00000000);
		PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
	}
	/* master gain */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00001500);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	for(i=0; i<(LX_PE_CMG_REGION_NUM); i++)
	{
		PE_P1L_M14A0_Wr(pe1_cen_ia_data,0x00000000);
		PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
	}
	/* normal mode */
	PE_P1L_M14A0_Wr(pe1_cen_ia_ctrl,	0x00008000);
	PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
	return;
}
/**
 * debug setting
 *
 * @param   *pstParams [in] LX_PE_DBG_SETTINGS_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetDebugSettings(LX_PE_DBG_SETTINGS_T *pstParams)
{
	int ret = RET_OK;
	do{
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		PE_CMG_HW_M14_DBG_PRINT("wid:%d,type:0x%x,[0x%x]print:0x%x,bypass:0x%x\n",\
			pstParams->win_id,pstParams->type,(0x1<<PE_ITEM_PKTMODL(CMG)),\
			pstParams->print_lvl,pstParams->bypass);
		/* set print level */
		if(pstParams->type&LX_PE_DBG_LV)
		{
			_g_cmg_hw_m14_trace = (pstParams->print_lvl & (0x1<<PE_ITEM_PKTMODL(CMG)))? 0x1:0x0;
		}
		/* set bypass */
		if(pstParams->type&LX_PE_DBG_BY)
		{
			if(PE_KDRV_VER_M14BX)
			{
				if(pstParams->bypass & (0x1<<PE_ITEM_PKTMODL(CMG)))
				{
					PE_CMG_HW_M14_DBG_PRINT("cen bypass.\n");
					if(PE_CHECK_WIN0(pstParams->win_id))
					{
						PE_PE1_M14B0_QWr01(pe1_cen_ctrl_00,	reg_cen_bypass,	0x0);	//CEN block OFF
					}
				}
				else
				{
					PE_CMG_HW_M14_DBG_PRINT("cen on.\n");
					if(PE_CHECK_WIN0(pstParams->win_id))
					{
						PE_PE1_M14B0_QWr01(pe1_cen_ctrl_00,	reg_cen_bypass,	0x1);	//CEN block ON
					}
				}
			}
			else if(PE_KDRV_VER_M14AX)
			{
				if(pstParams->bypass & (0x1<<PE_ITEM_PKTMODL(CMG)))
				{
					PE_CMG_HW_M14_DBG_PRINT("cen bypass.\n");
					if(PE_CHECK_WIN0(pstParams->win_id))
					{
						PE_P1L_M14A0_QWr01(pe1_cen_ctrl_00,	reg_cen_bypass,	0x0);	//CEN block OFF
					}
				}
				else
				{
					PE_CMG_HW_M14_DBG_PRINT("cen on.\n");
					if(PE_CHECK_WIN0(pstParams->win_id))
					{
						PE_P1L_M14A0_QWr01(pe1_cen_ctrl_00,	reg_cen_bypass,	0x1);	//CEN block ON
					}
				}
			}
			else
			{
				PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
			}
		}
	}while(0);
	return ret;
}
/**
 * set cen enable
 *
 * @param   *pstParams [in] LX_PE_CMG_ENABLE_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetCenEnable(LX_PE_CMG_ENABLE_T *pstParams)
{
	int ret = RET_OK;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		PE_CMG_HW_M14_DBG_PRINT(" set[%d]: cen enable:%d\n",pstParams->win_id,pstParams->enable);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_PE1_M14B0_RdFL(pe1_cen_ctrl_00);
				PE_PE1_M14B0_Wr01(pe1_cen_ctrl_00,	reg_cen_bypass,	GET_BITS(pstParams->enable,0,1));
				PE_PE1_M14B0_WrFL(pe1_cen_ctrl_00);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_P1L_M14A0_RdFL(pe1_cen_ctrl_00);
				PE_P1L_M14A0_Wr01(pe1_cen_ctrl_00,	reg_cen_bypass,	GET_BITS(pstParams->enable,0,1));
				PE_P1L_M14A0_WrFL(pe1_cen_ctrl_00);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while(0);
	return ret;
}
/**
 * get cen enable
 *
 * @param   *pstParams [in/out] LX_PE_CMG_ENABLE_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetCenEnable(LX_PE_CMG_ENABLE_T *pstParams)
{
	int ret = RET_OK;
	LX_PE_WIN_ID win_id;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		win_id = PE_GET_CHECKED_WINID(pstParams->win_id);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_PE1_M14B0_RdFL(pe1_cen_ctrl_00);
				PE_PE1_M14B0_Rd01(pe1_cen_ctrl_00,	reg_cen_bypass,	pstParams->enable);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_P1L_M14A0_RdFL(pe1_cen_ctrl_00);
				PE_P1L_M14A0_Rd01(pe1_cen_ctrl_00,	reg_cen_bypass,	pstParams->enable);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
		PE_CMG_HW_M14_DBG_PRINT(" get[%d]: cen enable:%d\n",pstParams->win_id,pstParams->enable);
	} while(0);
	return ret;
}
/**
 * set cen region enable
 *
 * @param   *pstParams [in] LX_PE_CMG_REGION_ENABLE_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetCenRegionEnable(LX_PE_CMG_REGION_ENABLE_T *pstParams)
{
	int ret = RET_OK;
	UINT32 wdata=0;
	UINT32 count=0;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		for(count=0;count<LX_PE_CMG_REGION_NUM;count++)
		{
			if(pstParams->enable[count])
				wdata |= (0x1<<(count+LX_PE_CMG_REGION_NUM));
			else
				wdata &= ~(0x1<<(count+LX_PE_CMG_REGION_NUM));

			if(pstParams->show_region[count])
				wdata |= (0x1<<(count));
			else
				wdata &= ~(0x1<<(count));
		}
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_PE1_M14B0_RdFL(pe1_cen_ctrl_01);
				PE_PE1_M14B0_Wr(pe1_cen_ctrl_01,wdata);
				PE_PE1_M14B0_WrFL(pe1_cen_ctrl_01);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_P1L_M14A0_RdFL(pe1_cen_ctrl_01);
				PE_P1L_M14A0_Wr(pe1_cen_ctrl_01,wdata);
				PE_P1L_M14A0_WrFL(pe1_cen_ctrl_01);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while(0);
	return ret;
}
/**
 * get cen region enable
 *
 * @param   *pstParams [in/out] LX_PE_CMG_REGION_ENABLE_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetCenRegionEnable(LX_PE_CMG_REGION_ENABLE_T *pstParams)
{
	int ret = RET_OK;
	LX_PE_WIN_ID win_id;
	UINT32 rdata=0;
	UINT32 count=0;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		win_id = PE_GET_CHECKED_WINID(pstParams->win_id);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_PE1_M14B0_RdFL(pe1_cen_ctrl_01);
				rdata = PE_PE1_M14B0_Rd(pe1_cen_ctrl_01);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_P1L_M14A0_RdFL(pe1_cen_ctrl_01);
				rdata = PE_P1L_M14A0_Rd(pe1_cen_ctrl_01);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
		for(count=0;count<LX_PE_CMG_REGION_NUM;count++)
		{
			if(rdata & (0x1<<(count+LX_PE_CMG_REGION_NUM)))
				pstParams->enable[count] = 1;
			else
				pstParams->enable[count] = 0;

			if(rdata & (0x1<<(count)))
				pstParams->show_region[count] = 1;
			else
				pstParams->show_region[count] = 0;
		}
	} while(0);
	return ret;
}
/**
 * set cen region
 *
 * @param   *pstParams [in] LX_PE_CMG_REGION_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetCenRegion(LX_PE_CMG_REGION_T *pstParams)
{
	int ret = RET_OK;
	UINT32 count=0;
	UINT32 start_addr=0;
	UINT32 x_wdata,y_wdata;
	UINT32 set_flag=0;
	PE_CMG_HW_M14_SETTINGS_T *pInfo=&_g_pe_cmg_hw_m14_info;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		PE_CMG_HW_M14_CHECK_CODE(pstParams->region_num>LX_PE_CMG_REGION_MAX,ret=RET_ERROR;break,\
			"[%s,%d] region_num(%d) is out of range.\n",__F__,__L__,pstParams->region_num);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				set_flag = 0;
				/* check double setting */
				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					if(pInfo->rgn_set[pstParams->region_num].hue_x[count]!=pstParams->hue_x[count] || \
						pInfo->rgn_set[pstParams->region_num].hue_g[count]!=pstParams->hue_g[count] || \
						pInfo->rgn_set[pstParams->region_num].sat_x[count]!=pstParams->sat_x[count] || \
						pInfo->rgn_set[pstParams->region_num].sat_g[count]!=pstParams->sat_g[count] || \
						pInfo->rgn_set[pstParams->region_num].val_x[count]!=pstParams->val_x[count] || \
						pInfo->rgn_set[pstParams->region_num].val_g[count]!=pstParams->val_g[count])
					{
						set_flag=1;
						break;
					}
				}
				if(set_flag)
				{
					PE_CMG_HW_M14_DBG_PRINT(" set region : num:%d\n", pstParams->region_num);
					PE_PE1_M14B0_QWr03(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0, 	//[15] 0:host access, 1:normal mode
														hif_cen_ai, 		0x1,	//[12] ai 0:disable, 1:enable
														hif_cen_address,	0x0);	//[7:0] address
					start_addr = pstParams->region_num*LX_PE_CMG_TBLPOINT;
					/* H color region table : 000 */
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);		//[10:8]
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						/* hue_x,x_wdata : 0~1024(0~720 degree) */
						x_wdata = (UINT32)GET_BITS(pstParams->hue_x[count],0,10);
						y_wdata = GET_BITS(pstParams->hue_g[count],0,7);	// 0~127
						PE_PE1_M14B0_Wr01(pe1_cen_ia_data,	hif_cen_x_wdata, 	x_wdata);	//[25:16] x data
						PE_PE1_M14B0_Wr01(pe1_cen_ia_data,	hif_cen_y_wdata, 	y_wdata);	//[9:0] y data
						PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
					}
					/* S color region table : 001 */
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x1);		//[10:8]
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);

					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						x_wdata = GET_BITS(pstParams->sat_x[count],0,7);	// 0~100
						y_wdata = GET_BITS(pstParams->sat_g[count],0,7);	// 0~127

						PE_PE1_M14B0_Wr01(pe1_cen_ia_data,	hif_cen_x_wdata, 	x_wdata);	//[25:16] x data
						PE_PE1_M14B0_Wr01(pe1_cen_ia_data,	hif_cen_y_wdata, 	y_wdata);	//[9:0] y data
						PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
					}
					/* V color region table : 010 */
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x2);		//[10:8]
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);

					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						x_wdata = GET_BITS(pstParams->val_x[count],0,8);	// 0~255
						y_wdata = GET_BITS(pstParams->val_g[count],0,7);	// 0~127

						PE_PE1_M14B0_Wr01(pe1_cen_ia_data,	hif_cen_x_wdata, 	x_wdata);	//[25:16] x data
						PE_PE1_M14B0_Wr01(pe1_cen_ia_data,	hif_cen_y_wdata, 	y_wdata);	//[9:0] y data
						PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
					}
					/* normal operation */
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						pInfo->rgn_set[pstParams->region_num].hue_x[count]=pstParams->hue_x[count];
						pInfo->rgn_set[pstParams->region_num].hue_g[count]=pstParams->hue_g[count];
						pInfo->rgn_set[pstParams->region_num].sat_x[count]=pstParams->sat_x[count];
						pInfo->rgn_set[pstParams->region_num].sat_g[count]=pstParams->sat_g[count];
						pInfo->rgn_set[pstParams->region_num].val_x[count]=pstParams->val_x[count];
						pInfo->rgn_set[pstParams->region_num].val_g[count]=pstParams->val_g[count];
					}
				}
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				/* check double setting */
				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					if(pInfo->rgn_set[pstParams->region_num].hue_x[count]!=pstParams->hue_x[count] || \
						pInfo->rgn_set[pstParams->region_num].hue_g[count]!=pstParams->hue_g[count] || \
						pInfo->rgn_set[pstParams->region_num].sat_x[count]!=pstParams->sat_x[count] || \
						pInfo->rgn_set[pstParams->region_num].sat_g[count]!=pstParams->sat_g[count] || \
						pInfo->rgn_set[pstParams->region_num].val_x[count]!=pstParams->val_x[count] || \
						pInfo->rgn_set[pstParams->region_num].val_g[count]!=pstParams->val_g[count])
					{
						set_flag=1;
						break;
					}
				}
				if(set_flag)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					PE_P1L_M14A0_QWr03(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0, 	//[15] 0:host access, 1:normal mode
														hif_cen_ai, 		0x1,	//[12] ai 0:disable, 1:enable
														hif_cen_address,	0x0);	//[7:0] address
					start_addr = pstParams->region_num*LX_PE_CMG_TBLPOINT;
					/* H color region table : 000 */
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);		//[10:8]
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						/* hue_x,x_wdata : 0~1024(0~720 degree) */
						x_wdata = (UINT32)GET_BITS(pstParams->hue_x[count],0,10);
						y_wdata = GET_BITS(pstParams->hue_g[count],0,7);	// 0~127
						PE_P1L_M14A0_Wr01(pe1_cen_ia_data,	hif_cen_x_wdata, 	x_wdata);	//[25:16] x data
						PE_P1L_M14A0_Wr01(pe1_cen_ia_data,	hif_cen_y_wdata, 	y_wdata);	//[9:0] y data
						PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
					}
					/* S color region table : 001 */
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x1);		//[10:8]
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						x_wdata = GET_BITS(pstParams->sat_x[count],0,7);	// 0~100
						y_wdata = GET_BITS(pstParams->sat_g[count],0,7);	// 0~127

						PE_P1L_M14A0_Wr01(pe1_cen_ia_data,	hif_cen_x_wdata, 	x_wdata);	//[25:16] x data
						PE_P1L_M14A0_Wr01(pe1_cen_ia_data,	hif_cen_y_wdata, 	y_wdata);	//[9:0] y data
						PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
					}
					/* V color region table : 010 */
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x2);		//[10:8]
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						x_wdata = GET_BITS(pstParams->val_x[count],0,8);	// 0~255
						y_wdata = GET_BITS(pstParams->val_g[count],0,7);	// 0~127

						PE_P1L_M14A0_Wr01(pe1_cen_ia_data,	hif_cen_x_wdata, 	x_wdata);	//[25:16] x data
						PE_P1L_M14A0_Wr01(pe1_cen_ia_data,	hif_cen_y_wdata, 	y_wdata);	//[9:0] y data
						PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
					}
					/* show color region table : 011 */
					start_addr = pstParams->region_num;
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x3);		//[10:8]
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr);	//[7:0] address
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						PE_P1L_M14A0_Wr(pe1_cen_ia_data,	0x0);
						PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
					}
					/* normal operation */
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
					for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
					{
						pInfo->rgn_set[pstParams->region_num].hue_x[count]=pstParams->hue_x[count];
						pInfo->rgn_set[pstParams->region_num].hue_g[count]=pstParams->hue_g[count];
						pInfo->rgn_set[pstParams->region_num].sat_x[count]=pstParams->sat_x[count];
						pInfo->rgn_set[pstParams->region_num].sat_g[count]=pstParams->sat_g[count];
						pInfo->rgn_set[pstParams->region_num].val_x[count]=pstParams->val_x[count];
						pInfo->rgn_set[pstParams->region_num].val_g[count]=pstParams->val_g[count];
					}
				}
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while(0);
	return ret;
}
/**
 * get cen region
 *
 * @param   *pstParams [in/out] LX_PE_CMG_REGION_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetCenRegion(LX_PE_CMG_REGION_T *pstParams)
{
	int ret = RET_OK;
	LX_PE_WIN_ID win_id;
	UINT32 count=0;
	UINT32 start_addr=0;
	UINT32 x_wdata=0,y_wdata=0;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		PE_CMG_HW_M14_CHECK_CODE(pstParams->region_num>LX_PE_CMG_REGION_MAX,ret=RET_ERROR;break,\
			"[%s,%d] region_num(%d) is out of range.\n",__F__,__L__,pstParams->region_num);
		win_id = PE_GET_CHECKED_WINID(pstParams->win_id);
		start_addr = pstParams->region_num*LX_PE_CMG_TBLPOINT;
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_PE1_M14B0_RdFL(pe1_cen_ia_ctrl);
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0);	//[15] 0:host access, 1:normal mode
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x1);	//[12] ai 0:disable, 1:enable
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address

				/* H color region table : 000 */
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);		//[10:8]
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					PE_PE1_M14B0_RdFL(pe1_cen_ia_data);
					PE_PE1_M14B0_Rd01(pe1_cen_ia_data,	hif_cen_x_wdata,	x_wdata);	//[25:16] x data
					PE_PE1_M14B0_Rd01(pe1_cen_ia_data,	hif_cen_y_wdata,	y_wdata);	//[9:0] y data

					/* x_wdata,hue_x 0~1024 (0~720 degree) */
					pstParams->hue_x[count] = (UINT16)GET_BITS(x_wdata,0,10);
					pstParams->hue_g[count] = (UINT8)GET_BITS(y_wdata,0,7);	// 0~127
				}
				/* S color region table : 001 */
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x1);		//[10:8]
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					PE_PE1_M14B0_RdFL(pe1_cen_ia_data);
					PE_PE1_M14B0_Rd01(pe1_cen_ia_data,	hif_cen_x_wdata,	x_wdata);	//[25:16] x data
					PE_PE1_M14B0_Rd01(pe1_cen_ia_data,	hif_cen_y_wdata,	y_wdata);	//[9:0] y data

					pstParams->sat_x[count] = (UINT8)GET_BITS(x_wdata,0,7);	// 0~100
					pstParams->sat_g[count] = (UINT8)GET_BITS(y_wdata,0,7);	// 0~127
				}
				/* V color region table : 010 */
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x2);		//[10:8]
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					PE_PE1_M14B0_RdFL(pe1_cen_ia_data);
					PE_PE1_M14B0_Rd01(pe1_cen_ia_data,	hif_cen_x_wdata,	x_wdata);	//[25:16] x data
					PE_PE1_M14B0_Rd01(pe1_cen_ia_data,	hif_cen_y_wdata,	y_wdata);	//[9:0] y data

					pstParams->val_x[count] = (UINT8)GET_BITS(x_wdata,0,8);	// 0~255
					pstParams->val_g[count] = (UINT8)GET_BITS(y_wdata,0,7);	// 0~127
				}
				/* normal operation */
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_P1L_M14A0_RdFL(pe1_cen_ia_ctrl);
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0);	//[15] 0:host access, 1:normal mode
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x1);	//[12] ai 0:disable, 1:enable
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address

				/* H color region table : 000 */
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);		//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					PE_P1L_M14A0_Rd01(pe1_cen_ia_data,	hif_cen_x_wdata,	x_wdata);	//[25:16] x data
					PE_P1L_M14A0_Rd01(pe1_cen_ia_data,	hif_cen_y_wdata,	y_wdata);	//[9:0] y data

					/* x_wdata,hue_x 0~1024 (0~720 degree) */
					pstParams->hue_x[count] = (UINT16)GET_BITS(x_wdata,0,10);
					pstParams->hue_g[count] = (UINT8)GET_BITS(y_wdata,0,7);	// 0~127
				}
				/* S color region table : 001 */
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x1);		//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					PE_P1L_M14A0_Rd01(pe1_cen_ia_data,	hif_cen_x_wdata,	x_wdata);	//[25:16] x data
					PE_P1L_M14A0_Rd01(pe1_cen_ia_data,	hif_cen_y_wdata,	y_wdata);	//[9:0] y data

					pstParams->sat_x[count] = (UINT8)GET_BITS(x_wdata,0,7);	// 0~100
					pstParams->sat_g[count] = (UINT8)GET_BITS(y_wdata,0,7);	// 0~127
				}
				/* V color region table : 010 */
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x2);		//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_TBLPOINT;count++)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					PE_P1L_M14A0_Rd01(pe1_cen_ia_data,	hif_cen_x_wdata,	x_wdata);	//[25:16] x data
					PE_P1L_M14A0_Rd01(pe1_cen_ia_data,	hif_cen_y_wdata,	y_wdata);	//[9:0] y data

					pstParams->val_x[count] = (UINT8)GET_BITS(x_wdata,0,8);	// 0~255
					pstParams->val_g[count] = (UINT8)GET_BITS(y_wdata,0,7);	// 0~127
				}
				/* normal operation */
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while (0);
	return ret;
}
/**
 * set cen region ctrl
 *
 * @param   *pstParams [in] LX_PE_CMG_REGION_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetCenRegionCtrl(LX_PE_CMG_REGION_CTRL_T *pstParams)
{
	int ret = RET_OK;
	UINT32 count=0;
	UINT32 start_addr=0;
	UINT32 delta[3];		// -128 ~ 127, hsv or gbr
	UINT32 wdata=0;
	PE_CMG_HW_M14_SETTINGS_T *pInfo=&_g_pe_cmg_hw_m14_info;
	PE_FWI_M14_CEN_CTRL fw_param;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		PE_CMG_HW_M14_CHECK_CODE(pstParams->region_num>LX_PE_CMG_REGION_MAX,ret=RET_ERROR;break,\
			"[%s,%d] region_num(%d) is out of range.\n",__F__,__L__,pstParams->region_num);
		PE_CMG_HW_M14_DBG_PRINT(" set: path:%d num:%d, m_g:%d, delta(hsvgbr):%d,%d,%d,%d,%d,%d\n",\
				pstParams->win_id,pstParams->region_num,pstParams->master_gain,\
				pstParams->region_delta[0],pstParams->region_delta[1],pstParams->region_delta[2],\
				pstParams->region_delta[3],pstParams->region_delta[4],pstParams->region_delta[5]);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				/* region delta */
				if(pInfo->rgn_ctrl[pstParams->region_num].region_delta[0]!=pstParams->region_delta[0] || \
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[1]!=pstParams->region_delta[1] || \
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[2]!=pstParams->region_delta[2] || \
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[3]!=pstParams->region_delta[3] || \
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[4]!=pstParams->region_delta[4] || \
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[5]!=pstParams->region_delta[5])
				{
					PE_CMG_HW_M14_DBG_PRINT(" set delta\n");
					start_addr = pstParams->region_num*LX_PE_CMG_DELTA_SETNUM;
					PE_PE1_M14B0_RdFL(pe1_cen_delta_ia_ctrl);
					PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_enable, 0x0);	//[15] 0:host access, 1:normal mode
					PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_ai, 	0x1);	//[12] ai 0:disable, 1:enable
					PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_address,start_addr);	//[7:0] address	
					PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_ctrl);
					for(count=0;count<LX_PE_CMG_DELTA_SETNUM;count++)
					{
						wdata=0;
						delta[0] = GET_BITS(pstParams->region_delta[count*3],0,8);		// -128 ~ 127
						delta[1] = GET_BITS(pstParams->region_delta[count*3+1],0,8);	// -128 ~ 127
						delta[2] = GET_BITS(pstParams->region_delta[count*3+2],0,8);	// -128 ~ 127
						wdata = (delta[0]<<16)|(delta[1]<<8)|(delta[2]);
						PE_PE1_M14B0_Wr(pe1_cen_delta_ia_data,	wdata);
						PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_data);
					}
					PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_enable, 0x1);	//[15] 0:host access, 1:normal mode
					PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_ai, 	0x0);	//[12] ai 0:disable, 1:enable
					PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_address,0x0);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_ctrl);
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[0]=pstParams->region_delta[0];
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[1]=pstParams->region_delta[1];
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[2]=pstParams->region_delta[2];
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[3]=pstParams->region_delta[3];
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[4]=pstParams->region_delta[4];
					pInfo->rgn_ctrl[pstParams->region_num].region_delta[5]=pstParams->region_delta[5];
				}
				/* region gain */
				if(pInfo->rgn_ctrl[pstParams->region_num].master_gain!=pstParams->master_gain)
				{
					PE_CMG_HW_M14_DBG_PRINT(" set master_gain\n");
					wdata = GET_BITS(pstParams->master_gain,0,8);	// 0~128~255
					switch(pstParams->region_num)
					{
						case 0:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_04,	reg_master_gain_cr0,	wdata);
							break;
						case 1:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_04,	reg_master_gain_cr1,	wdata);
							break;
						case 2:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_04,	reg_master_gain_cr2,	wdata);
							break;
						case 3:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_04,	reg_master_gain_cr3,	wdata);
							break;
						case 4:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_05,	reg_master_gain_cr4,	wdata);
							break;
						case 5:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_05,	reg_master_gain_cr5,	wdata);
							break;
						case 6:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_05,	reg_master_gain_cr6,	wdata);
							break;
						case 7:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_05,	reg_master_gain_cr7,	wdata);
							break;
						case 8:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_06,	reg_master_gain_cr8,	wdata);
							break;
						case 9:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_06,	reg_master_gain_cr9,	wdata);
							break;
						case 10:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_06,	reg_master_gain_cr10,	wdata);
							break;
						case 11:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_06,	reg_master_gain_cr11,	wdata);
							break;
						case 12:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_07,	reg_master_gain_cr12,	wdata);
							break;
						case 13:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_07,	reg_master_gain_cr13,	wdata);
							break;
						case 14:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_07,	reg_master_gain_cr14,	wdata);
							break;
						case 15:
						default:
							PE_PE1_M14B0_QWr01(pe1_cen_ctrl_07,	reg_master_gain_cr15,	wdata);
							break;
					}
					pInfo->rgn_ctrl[pstParams->region_num].master_gain=pstParams->master_gain;
				}
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				fw_param.rgn_num = (UINT8)pstParams->region_num;
				fw_param.delta_h = (UINT8)pstParams->region_delta[0];
				fw_param.delta_s = (UINT8)pstParams->region_delta[1];
				fw_param.delta_v = (UINT8)pstParams->region_delta[2];
				fw_param.delta_g = (UINT8)pstParams->region_delta[3];
				fw_param.delta_b = (UINT8)pstParams->region_delta[4];
				fw_param.delta_r = (UINT8)pstParams->region_delta[5];
				fw_param.ma_gain = pstParams->master_gain;
				ret = PE_FWI_M14_SetCenCtrl(&fw_param);
				PE_CMG_HW_M14_CHECK_CODE(ret,break,"[%s,%d] PE_FWI_M14_SetCenCtrl() error.\n",__F__,__L__);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while (0);
	return ret;
}
/**
 * get cen region ctrl
 *
 * @param   *pstParams [in/out] LX_PE_CMG_REGION_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetCenRegionCtrl(LX_PE_CMG_REGION_CTRL_T *pstParams)
{
	int ret = RET_OK;
	LX_PE_WIN_ID win_id;
	UINT32 count=0;
	UINT32 start_addr=0;
	UINT32 re_mgain=0;
	UINT32 re_delta[LX_PE_CMG_DELTA_SETNUM];	// hsv or gbr
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		win_id = PE_GET_CHECKED_WINID(pstParams->win_id);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				/* region delta */
				start_addr = pstParams->region_num*LX_PE_CMG_DELTA_SETNUM;
				PE_PE1_M14B0_RdFL(pe1_cen_delta_ia_ctrl);
				PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_enable, 0x0);	//[15] 0:host access, 1:normal mode
				PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_ai, 	0x1);	//[12] ai 0:disable, 1:enable
				PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_address,start_addr);	//[7:0] address	
				PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_ctrl);
				for(count=0;count<LX_PE_CMG_DELTA_SETNUM;count++)
				{
					PE_PE1_M14B0_RdFL(pe1_cen_delta_ia_data);
					re_delta[count] = PE_PE1_M14B0_Rd(pe1_cen_delta_ia_data);
				}
				PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_enable, 0x1);	//[15] 0:host access, 1:normal mode
				PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_ai, 	0x0);	//[12] ai 0:disable, 1:enable
				PE_PE1_M14B0_Wr01(pe1_cen_delta_ia_ctrl,	hif_cen_delta_address,0x0);	//[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_delta_ia_ctrl);
				/* region gain */
				switch(pstParams->region_num)
				{
					case 0:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_04,	reg_master_gain_cr0,	re_mgain);
						break;
					case 1:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_04,	reg_master_gain_cr1,	re_mgain);
						break;
					case 2:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_04,	reg_master_gain_cr2,	re_mgain);
						break;
					case 3:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_04,	reg_master_gain_cr3,	re_mgain);
						break;
					case 4:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_05,	reg_master_gain_cr4,	re_mgain);
						break;
					case 5:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_05,	reg_master_gain_cr5,	re_mgain);
						break;
					case 6:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_05,	reg_master_gain_cr6,	re_mgain);
						break;
					case 7:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_05,	reg_master_gain_cr7,	re_mgain);
						break;
					case 8:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_06,	reg_master_gain_cr8,	re_mgain);
						break;
					case 9:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_06,	reg_master_gain_cr9,	re_mgain);
						break;
					case 10:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_06,	reg_master_gain_cr10,	re_mgain);
						break;
					case 11:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_06,	reg_master_gain_cr11,	re_mgain);
						break;
					case 12:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_07,	reg_master_gain_cr12,	re_mgain);
						break;
					case 13:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_07,	reg_master_gain_cr13,	re_mgain);
						break;
					case 14:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_07,	reg_master_gain_cr14,	re_mgain);
						break;
					case 15:
					default:
						PE_PE1_M14B0_QRd01(pe1_cen_ctrl_07,	reg_master_gain_cr15,	re_mgain);
						break;
				}
				pstParams->master_gain = (UINT8)GET_BITS(re_mgain,0,8);	// region master gain, 0~128~255
				pstParams->region_delta[0] = (SINT8)GET_BITS(re_delta[0],16,8);	// h, -128 ~ 127
				pstParams->region_delta[1] = (SINT8)GET_BITS(re_delta[0],8,8);	// s, -128 ~ 127
				pstParams->region_delta[2] = (SINT8)GET_BITS(re_delta[0],0,8);	// v, -128 ~ 127
				pstParams->region_delta[3] = (SINT8)GET_BITS(re_delta[1],16,8);	// g, -128 ~ 127
				pstParams->region_delta[4] = (SINT8)GET_BITS(re_delta[1],8,8);	// b, -128 ~ 127
				pstParams->region_delta[5] = (SINT8)GET_BITS(re_delta[1],0,8);	// r, -128 ~ 127
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_P1L_M14A0_QWr03(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0,	//[15] 0:host access, 1:normal mode
													hif_cen_ai, 		0x1,	//[12] ai 0:disable, 1:enable
													hif_cen_address,	0x0);	//[4:0] address
				/* region master gain : 101 */
				start_addr = pstParams->region_num;
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x5);	//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

				PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
				re_mgain = PE_P1L_M14A0_Rd(pe1_cen_ia_data);
				/* region delta gain : 100 */
				start_addr = pstParams->region_num*LX_PE_CMG_DELTA_SETNUM;

				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x4);	//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	start_addr); //[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_DELTA_SETNUM;count++)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					re_delta[count] = PE_P1L_M14A0_Rd(pe1_cen_ia_data);
				}
				pstParams->master_gain = (UINT8)GET_BITS(re_mgain,0,8);	// region master gain, 0~128~255
				pstParams->region_delta[0] = (SINT8)GET_BITS(re_delta[0],16,8);	// h, -128 ~ 127
				pstParams->region_delta[1] = (SINT8)GET_BITS(re_delta[0],8,8);	// s, -128 ~ 127
				pstParams->region_delta[2] = (SINT8)GET_BITS(re_delta[0],0,8);	// v, -128 ~ 127
				pstParams->region_delta[3] = (SINT8)GET_BITS(re_delta[1],16,8);	// g, -128 ~ 127
				pstParams->region_delta[4] = (SINT8)GET_BITS(re_delta[1],8,8);	// b, -128 ~ 127
				pstParams->region_delta[5] = (SINT8)GET_BITS(re_delta[1],0,8);	// r, -128 ~ 127
				/* normal operation */
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
		PE_CMG_HW_M14_DBG_PRINT(" Get: path:%d num:%d, m_g:%d, delta(hsvgbr):%d,%d,%d,%d,%d,%d\n",\
				pstParams->win_id,pstParams->region_num,pstParams->master_gain,\
				pstParams->region_delta[0],pstParams->region_delta[1],pstParams->region_delta[2],\
				pstParams->region_delta[3],pstParams->region_delta[4],pstParams->region_delta[5]);
	} while (0);
	return ret;
}
/**
 * set cen global ctrl
 *
 * @param   *pstParams [in] LX_PE_CMG_GLOBAL_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetCenGlobalCtrl(LX_PE_CMG_GLOBAL_CTRL_T *pstParams)
{
	int ret = RET_OK;
	UINT32 count=0;
	UINT32 wdata=0;	// h,s,v,g,b,r
	UINT32 set_flag=0;
	PE_CMG_HW_M14_SETTINGS_T *pInfo=&_g_pe_cmg_hw_m14_info;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				/* check double setting */
				if(pInfo->gbl_ctrl.global_delta[0]!=pstParams->global_delta[0] || \
					pInfo->gbl_ctrl.global_delta[1]!=pstParams->global_delta[1] || \
					pInfo->gbl_ctrl.global_delta[2]!=pstParams->global_delta[2] || \
					pInfo->gbl_ctrl.global_delta[3]!=pstParams->global_delta[3] || \
					pInfo->gbl_ctrl.global_delta[4]!=pstParams->global_delta[4] || \
					pInfo->gbl_ctrl.global_delta[5]!=pstParams->global_delta[5])
				{
					/* global master gain : 110 */
					PE_PE1_M14B0_RdFL(pe1_cen_ia_ctrl);
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0);	//[15] 0:host access, 1:normal mode
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x1);	//[12] ai 0:disable, 1:enable
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x6);	//[10:8]
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
					for(count=0;count<LX_PE_CMG_DELTANUM;count++)
					{
						wdata = GET_BITS(pstParams->global_delta[count],0,10);	// -512 ~ 511, [0]h [1]s [2]v [3]g [4]b [5]r
						PE_PE1_M14B0_Wr(pe1_cen_ia_data,	wdata);
						PE_PE1_M14B0_WrFL(pe1_cen_ia_data);
					}
					/* normal operation */
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
					PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
					PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
					pInfo->gbl_ctrl.global_delta[0]=pstParams->global_delta[0];
					pInfo->gbl_ctrl.global_delta[1]=pstParams->global_delta[1];
					pInfo->gbl_ctrl.global_delta[2]=pstParams->global_delta[2];
					pInfo->gbl_ctrl.global_delta[3]=pstParams->global_delta[3];
					pInfo->gbl_ctrl.global_delta[4]=pstParams->global_delta[4];
					pInfo->gbl_ctrl.global_delta[5]=pstParams->global_delta[5];
				}
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				/* check double setting */
				if(pInfo->gbl_ctrl.global_delta[0]!=pstParams->global_delta[0] || \
					pInfo->gbl_ctrl.global_delta[1]!=pstParams->global_delta[1] || \
					pInfo->gbl_ctrl.global_delta[2]!=pstParams->global_delta[2] || \
					pInfo->gbl_ctrl.global_delta[3]!=pstParams->global_delta[3] || \
					pInfo->gbl_ctrl.global_delta[4]!=pstParams->global_delta[4] || \
					pInfo->gbl_ctrl.global_delta[5]!=pstParams->global_delta[5])
				{
					set_flag=1;
				}
				if(set_flag)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					PE_P1L_M14A0_QWr03(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0,	//[15] 0:host access, 1:normal mode
														hif_cen_ai, 		0x1,	//[12] ai 0:disable, 1:enable
														hif_cen_address,	0x0);	//[7:0] address
					/* global master gain : 110 */
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x6);	//[10:8]
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
					for(count=0;count<LX_PE_CMG_DELTANUM;count++)
					{
						wdata = GET_BITS(pstParams->global_delta[count],0,10);		// -512 ~ 511, [0]h [1]s [2]v [3]g [4]b [5]r
						PE_P1L_M14A0_Wr(pe1_cen_ia_data,	wdata);
						PE_P1L_M14A0_WrFL(pe1_cen_ia_data);
					}
					/* normal operation */
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
					PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
					PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
					pInfo->gbl_ctrl.global_delta[0]=pstParams->global_delta[0];
					pInfo->gbl_ctrl.global_delta[1]=pstParams->global_delta[1];
					pInfo->gbl_ctrl.global_delta[2]=pstParams->global_delta[2];
					pInfo->gbl_ctrl.global_delta[3]=pstParams->global_delta[3];
					pInfo->gbl_ctrl.global_delta[4]=pstParams->global_delta[4];
					pInfo->gbl_ctrl.global_delta[5]=pstParams->global_delta[5];
				}
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while (0);
	return ret;
}
/**
 * get cen global ctrl
 *
 * @param   *pstParams [in/out] LX_PE_CMG_GLOBAL_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetCenGlobalCtrl(LX_PE_CMG_GLOBAL_CTRL_T *pstParams)
{
	int ret = RET_OK;
	LX_PE_WIN_ID win_id;
	UINT32 count=0;
	UINT32 rdata=0;	// h,s,v,g,b,r
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		win_id = PE_GET_CHECKED_WINID(pstParams->win_id);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				/* global master gain : 110 */
				PE_PE1_M14B0_RdFL(pe1_cen_ia_ctrl);
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0);	//[15] 0:host access, 1:normal mode
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x1);	//[12] ai 0:disable, 1:enable
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x6);	//[10:8]
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
				for(count=0;count<LX_PE_CMG_DELTANUM;count++)
				{
					PE_PE1_M14B0_RdFL(pe1_cen_ia_data);
					rdata = PE_PE1_M14B0_Rd(pe1_cen_ia_data);
					pstParams->global_delta[count] = (SINT16)PE_CONVHEX2DEC(rdata,9);// -512 ~ 511, [0]h [1]s [2]v [3]g [4]b [5]r
				}
				/* normal operation */
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
				PE_PE1_M14B0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
				PE_PE1_M14B0_WrFL(pe1_cen_ia_ctrl);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_P1L_M14A0_QWr03(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x0,	//[15] 0:host access, 1:normal mode
													hif_cen_ai, 		0x1,	//[12] ai 0:disable, 1:enable
													hif_cen_address,	0x0);	//[7:0] address
				/* global master gain : 110 for L9A0*/
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x6);	//[10:8]
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);

				for(count=0;count<LX_PE_CMG_DELTANUM;count++)
				{
					PE_P1L_M14A0_RdFL(pe1_cen_ia_data);
					rdata = PE_P1L_M14A0_Rd(pe1_cen_ia_data);
					pstParams->global_delta[count] = (SINT16)PE_CONVHEX2DEC(rdata,9);// -512 ~ 511, [0]h [1]s [2]v [3]g [4]b [5]r
				}
				/* normal operation */
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_enable, 	0x1);	//[15] 0:host access, 1:normal mode
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai, 		0x0);	//[12] ai 0:disable, 1:enable
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_ai_sel, 	0x0);	//[10:8]
				PE_P1L_M14A0_Wr01(pe1_cen_ia_ctrl,	hif_cen_address,	0x0);	//[7:0] address
				PE_P1L_M14A0_WrFL(pe1_cen_ia_ctrl);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while (0);
	return ret;
}
/**
 * set cen color ctrl
 *
 * @param   *pstParams [in] LX_PE_CMG_COLOR_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetCenColorCtrl(LX_PE_CMG_COLOR_CTRL_T *pstParams)
{
	int ret = RET_OK;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		PE_CMG_HW_M14_DBG_PRINT("set pstParams[%d] : sat:%d\n", \
			pstParams->win_id,pstParams->saturation);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_PE1_M14B0_QWr01(pe1_cen_ctrl_02, ihsv_sgain,	GET_BITS(pstParams->saturation,0,8));//[7:0] reg_ihsv_sgain
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_P1L_M14A0_QWr01(pe1_cen_ctrl_02, ihsv_sgain,	GET_BITS(pstParams->saturation,0,8));//[7:0] reg_ihsv_sgain
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	} while (0);
	return ret;
}
/**
 * get cen color ctrl
 *
 * @param   *pstParams [in/out] LX_PE_CMG_COLOR_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetCenColorCtrl(LX_PE_CMG_COLOR_CTRL_T *pstParams)
{
	int ret = RET_OK;
	LX_PE_WIN_ID win_id;
	UINT32 rdata=0;
	do {
		CHECK_KNULL(pstParams);
		PE_CHECK_WINID(pstParams->win_id);
		win_id = PE_GET_CHECKED_WINID(pstParams->win_id);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_PE1_M14B0_QRd01(pe1_cen_ctrl_02, ihsv_sgain,	rdata);	//[7:0] reg_ihsv_sgain
				pstParams->saturation=(UINT16)rdata;
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(win_id))
			{
				PE_P1L_M14A0_QRd01(pe1_cen_ctrl_02, ihsv_sgain,	rdata);	//[7:0] reg_ihsv_sgain
				pstParams->saturation=(UINT16)rdata;
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
		PE_CMG_HW_M14_DBG_PRINT("get pstParams[%d] : sat:%d\n",pstParams->win_id,pstParams->saturation);
	} while (0);
	return ret;
}
/**
 * set clear white
 *
 * @param   *pstParams [in] LX_PE_CMG_CW_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetClearWhiteCtrl(LX_PE_CMG_CW_CTRL_T *pstParams)
{
	int ret = RET_OK;
	do{
		CHECK_KNULL(pstParams);
		PE_CMG_HW_M14_DBG_PRINT("set[%d] en:%d, yc:%d, x:%d,%d,%d,%d,%d\n"\
			" y:%d,%d,%d,%d,%d, sel:0x%x, g:%d\n", \
			pstParams->win_id, pstParams->cw_en, \
			pstParams->gain_sel, pstParams->gain_x[0], \
			pstParams->gain_x[1], pstParams->gain_x[2], pstParams->gain_x[3], \
			pstParams->gain_x[4], pstParams->gain_y[0], pstParams->gain_y[1], \
			pstParams->gain_y[2], pstParams->gain_y[3], pstParams->gain_y[4], \
			pstParams->region_sel, pstParams->region_gain);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_PE1_M14B0_QWr04(pe1_cw_ctrl_00, 	cw_en,				GET_BITS(pstParams->cw_en,0,1),\
													cw_gain_selection,	GET_BITS(pstParams->gain_sel,0,1),\
													reg_cw_y4,			GET_BITS(pstParams->gain_y[4],0,8),\
													reg_cw_x4,			GET_BITS(pstParams->gain_x[4],0,8));
				PE_PE1_M14B0_QWr04(pe1_cw_ctrl_03,	reg_cw_y0,			GET_BITS(pstParams->gain_y[0],0,8),\
													reg_cw_x0,			GET_BITS(pstParams->gain_x[0],0,8),\
													reg_cw_y1,			GET_BITS(pstParams->gain_y[1],0,8),\
													reg_cw_x1,			GET_BITS(pstParams->gain_x[1],0,8));
				PE_PE1_M14B0_QWr04(pe1_cw_ctrl_02,	reg_cw_y2,			GET_BITS(pstParams->gain_y[2],0,8),\
													reg_cw_x2,			GET_BITS(pstParams->gain_x[2],0,8),\
													reg_cw_y3,			GET_BITS(pstParams->gain_y[3],0,8),\
													reg_cw_x3,			GET_BITS(pstParams->gain_x[3],0,8));
				PE_PE1_M14B0_RdFL(pe1_cw_ctrl_04);
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region0_sel,	GET_BITS(pstParams->region_sel,0,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region1_sel,	GET_BITS(pstParams->region_sel,1,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region2_sel,	GET_BITS(pstParams->region_sel,2,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region3_sel,	GET_BITS(pstParams->region_sel,3,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region4_sel,	GET_BITS(pstParams->region_sel,4,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region5_sel,	GET_BITS(pstParams->region_sel,5,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region6_sel,	GET_BITS(pstParams->region_sel,6,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region7_sel,	GET_BITS(pstParams->region_sel,7,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region8_sel,	GET_BITS(pstParams->region_sel,8,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region9_sel,	GET_BITS(pstParams->region_sel,9,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region10_sel,	GET_BITS(pstParams->region_sel,10,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region11_sel,	GET_BITS(pstParams->region_sel,11,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region12_sel,	GET_BITS(pstParams->region_sel,12,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region13_sel,	GET_BITS(pstParams->region_sel,13,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region14_sel,	GET_BITS(pstParams->region_sel,14,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region15_sel,	GET_BITS(pstParams->region_sel,15,1));
				PE_PE1_M14B0_Wr01(pe1_cw_ctrl_04,	color_region_gain,	GET_BITS(pstParams->region_gain,0,8));
				PE_PE1_M14B0_WrFL(pe1_cw_ctrl_04);
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_P1L_M14A0_QWr04(pe1_cw_ctrl_00, 	cw_en,				GET_BITS(pstParams->cw_en,0,1),\
													cw_gain_selection,	GET_BITS(pstParams->gain_sel,0,1),\
													reg_cw_y4,			GET_BITS(pstParams->gain_y[4],0,8),\
													reg_cw_x4,			GET_BITS(pstParams->gain_x[4],0,8));
				PE_P1L_M14A0_QWr04(pe1_cw_ctrl_03,	reg_cw_y0,			GET_BITS(pstParams->gain_y[0],0,8),\
													reg_cw_x0,			GET_BITS(pstParams->gain_x[0],0,8),\
													reg_cw_y1,			GET_BITS(pstParams->gain_y[1],0,8),\
													reg_cw_x1,			GET_BITS(pstParams->gain_x[1],0,8));
				PE_P1L_M14A0_QWr04(pe1_cw_ctrl_02,	reg_cw_y2,			GET_BITS(pstParams->gain_y[2],0,8),\
													reg_cw_x2,			GET_BITS(pstParams->gain_x[2],0,8),\
													reg_cw_y3,			GET_BITS(pstParams->gain_y[3],0,8),\
													reg_cw_x3,			GET_BITS(pstParams->gain_x[3],0,8));
				PE_P1L_M14A0_RdFL(pe1_cw_ctrl_04);
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region0_sel,	GET_BITS(pstParams->region_sel,0,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region1_sel,	GET_BITS(pstParams->region_sel,1,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region2_sel,	GET_BITS(pstParams->region_sel,2,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region3_sel,	GET_BITS(pstParams->region_sel,3,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region4_sel,	GET_BITS(pstParams->region_sel,4,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region5_sel,	GET_BITS(pstParams->region_sel,5,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region6_sel,	GET_BITS(pstParams->region_sel,6,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region7_sel,	GET_BITS(pstParams->region_sel,7,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region8_sel,	GET_BITS(pstParams->region_sel,8,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region9_sel,	GET_BITS(pstParams->region_sel,9,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region10_sel,	GET_BITS(pstParams->region_sel,10,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region11_sel,	GET_BITS(pstParams->region_sel,11,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region12_sel,	GET_BITS(pstParams->region_sel,12,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region13_sel,	GET_BITS(pstParams->region_sel,13,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region14_sel,	GET_BITS(pstParams->region_sel,14,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region15_sel,	GET_BITS(pstParams->region_sel,15,1));
				PE_P1L_M14A0_Wr01(pe1_cw_ctrl_04,	color_region_gain,	GET_BITS(pstParams->region_gain,0,8));
				PE_P1L_M14A0_WrFL(pe1_cw_ctrl_04);
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	}while(0);
	return ret;
}
/**
 * get clear white
 *
 * @param   *pstParams [in/out] LX_PE_CMG_CW_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetClearWhiteCtrl(LX_PE_CMG_CW_CTRL_T *pstParams)
{
	int ret = RET_OK;
	UINT32 sel_data;
	do{
		CHECK_KNULL(pstParams);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				if(pstParams->region_gain==0)	pstParams->region_gain=1;
				PE_PE1_M14B0_QRd04(pe1_cw_ctrl_00, 	cw_en,				(pstParams->cw_en),\
													cw_gain_selection,	(pstParams->gain_sel),\
													reg_cw_y4,			(pstParams->gain_y[4]),\
													reg_cw_x4,			(pstParams->gain_x[4]));
				PE_PE1_M14B0_QRd04(pe1_cw_ctrl_03,	reg_cw_y0,			(pstParams->gain_y[0]),\
													reg_cw_x0,			(pstParams->gain_x[0]),\
													reg_cw_y1,			(pstParams->gain_y[1]),\
													reg_cw_x1,			(pstParams->gain_x[1]));
				PE_PE1_M14B0_QRd04(pe1_cw_ctrl_02,	reg_cw_y2,			(pstParams->gain_y[2]),\
													reg_cw_x2,			(pstParams->gain_x[2]),\
													reg_cw_y3,			(pstParams->gain_y[3]),\
													reg_cw_x3,			(pstParams->gain_x[3]));
				PE_PE1_M14B0_RdFL(pe1_cw_ctrl_04);
				sel_data = PE_PE1_M14B0_Rd(pe1_cw_ctrl_04);
				pstParams->region_sel = GET_BITS(sel_data,0,16);	//[15:0]
				PE_PE1_M14B0_Rd01(pe1_cw_ctrl_04,	color_region_gain,	(pstParams->region_gain));
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				if(pstParams->region_gain==0)	pstParams->region_gain=1;
				PE_P1L_M14A0_QRd04(pe1_cw_ctrl_00, 	cw_en,				(pstParams->cw_en),\
													cw_gain_selection,	(pstParams->gain_sel),\
													reg_cw_y4,			(pstParams->gain_y[4]),\
													reg_cw_x4,			(pstParams->gain_x[4]));
				PE_P1L_M14A0_QRd04(pe1_cw_ctrl_03,	reg_cw_y0,			(pstParams->gain_y[0]),\
													reg_cw_x0,			(pstParams->gain_x[0]),\
													reg_cw_y1,			(pstParams->gain_y[1]),\
													reg_cw_x1,			(pstParams->gain_x[1]));
				PE_P1L_M14A0_QRd04(pe1_cw_ctrl_02,	reg_cw_y2,			(pstParams->gain_y[2]),\
													reg_cw_x2,			(pstParams->gain_x[2]),\
													reg_cw_y3,			(pstParams->gain_y[3]),\
													reg_cw_x3,			(pstParams->gain_x[3]));
				PE_P1L_M14A0_RdFL(pe1_cw_ctrl_04);
				sel_data = PE_P1L_M14A0_Rd(pe1_cw_ctrl_04);
				pstParams->region_sel = GET_BITS(sel_data,0,16);	//[15:0]
				PE_P1L_M14A0_Rd01(pe1_cw_ctrl_04,	color_region_gain,	(pstParams->region_gain));
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
		PE_CMG_HW_M14_DBG_PRINT("get[%d] en:%d, yc:%d, x:%d,%d,%d,%d,%d\n"\
			" y:%d,%d,%d,%d,%d, sel:0x%x, g:%d\n", \
			pstParams->win_id, pstParams->cw_en, \
			pstParams->gain_sel, pstParams->gain_x[0], \
			pstParams->gain_x[1], pstParams->gain_x[2], pstParams->gain_x[3], \
			pstParams->gain_x[4], pstParams->gain_y[0], pstParams->gain_y[1], \
			pstParams->gain_y[2], pstParams->gain_y[3], pstParams->gain_y[4], \
			pstParams->region_sel, pstParams->region_gain);
	}while(0);
	return ret;
}
/**
 * set clear white gain
 *
 * @param   *pstParams [in] LX_PE_CMG_CW_GAIN_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_SetClearWhiteGainCtrl(LX_PE_CMG_CW_GAIN_CTRL_T *pstParams)
{
	int ret = RET_OK;
	do{
		CHECK_KNULL(pstParams);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_PE1_M14B0_QWr03(pe1_cw_ctrl_01,	user_ctrl_g_gain,	GET_BITS(pstParams->g_gain,0,8),\
													user_ctrl_b_gain,	GET_BITS(pstParams->b_gain,0,8),\
													user_ctrl_r_gain,	GET_BITS(pstParams->r_gain,0,8));
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_P1L_M14A0_QWr03(pe1_cw_ctrl_01,	user_ctrl_g_gain,	GET_BITS(pstParams->g_gain,0,8),\
													user_ctrl_b_gain,	GET_BITS(pstParams->b_gain,0,8),\
													user_ctrl_r_gain,	GET_BITS(pstParams->r_gain,0,8));
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	}while(0);
	return ret;
}
/**
 * get clear white gain
 *
 * @param   *pstParams [in/out] LX_PE_CMG_CW_GAIN_CTRL_T
 * @return  OK if success, ERROR otherwise.
 * @see
 * @author
 */
int PE_CMG_HW_M14_GetClearWhiteGainCtrl(LX_PE_CMG_CW_GAIN_CTRL_T *pstParams)
{
	int ret = RET_OK;
	do{
		CHECK_KNULL(pstParams);
		if(PE_KDRV_VER_M14BX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_PE1_M14B0_QRd03(pe1_cw_ctrl_01,	user_ctrl_g_gain,	(pstParams->g_gain),\
													user_ctrl_b_gain,	(pstParams->b_gain),\
													user_ctrl_r_gain,	(pstParams->r_gain));
			}
		}
		else if(PE_KDRV_VER_M14AX)
		{
			if(PE_CHECK_WIN0(pstParams->win_id))
			{
				PE_P1L_M14A0_QRd03(pe1_cw_ctrl_01,	user_ctrl_g_gain,	(pstParams->g_gain),\
													user_ctrl_b_gain,	(pstParams->b_gain),\
													user_ctrl_r_gain,	(pstParams->r_gain));
			}
		}
		else
		{
			PE_CMG_HW_M14_DBG_PRINT("nothing to do.\n");	ret = RET_OK;
		}
	}while(0);
	return ret;
}

