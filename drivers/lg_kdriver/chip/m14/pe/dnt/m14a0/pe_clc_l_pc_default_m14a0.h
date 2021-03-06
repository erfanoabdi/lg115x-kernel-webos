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

const PE_REG_PARAM_T clc_l_pc_default_m14a0[] = 
{
	/* clc, 0x0560~0x0578 */
	{0x0560, 0xF88F12F0},		//cl_filter_enable[1]:0x0,clc_detection_enable[2]:0x0
	{0x0568, 0x19000985},
	{0x0564, 0x32500225},
	{0x056C, 0xC030F030},
	{0x0570, 0x00000020},
	{0x066C, 0x000004C8},
	{0x0574, 0x1FFD8020},
	{0x0578, 0xC0003E00}
};
