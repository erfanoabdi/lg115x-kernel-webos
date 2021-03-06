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

#ifndef __ADEC_MODULE_SE_PARAM_MAP_H__
#define __ADEC_MODULE_SE_PARAM_MAP_H__


// SE param memory map 에 따라서 변동 되어야 함.
#define NUM_OF_BASIC_PARAM						3

#define NUM_OF_LGSE_MAIN_SIZEOF_INIT 			372
#define NUM_OF_LGSE_MAIN_SIZEOF_VAR 			0

#define NUM_OF_LGSEFN000_CV_SIZEOF_INIT 		295
#define NUM_OF_LGSEFN000_CV_SIZEOF_VAR 			1

#define NUM_OF_LGSEFN001_AV_SIZEOF_INIT 		21
#define NUM_OF_LGSEFN001_AV_SIZEOF_VAR 			130   //131 -> 130

#define NUM_OF_LGSEFN002_DB_SIZEOF_INIT 		123
#define NUM_OF_LGSEFN002_DB_SIZEOF_VAR	 		0

#define NUM_OF_LGSEFN003_DEQ_SIZEOF_INIT 		330
#define NUM_OF_LGSEFN003_DEQ_SIZEOF_VAR 		0

#define NUM_OF_LGSEFN004_PEQ_MODE1_SIZEOF_INIT 	0
#define NUM_OF_LGSEFN004_PEQ_MODE1_SIZEOF_VAR 	27
#define NUM_OF_LGSEFN004_PEQ_MODE2_SIZEOF_INIT 	0
#define NUM_OF_LGSEFN004_PEQ_MODE2_SIZEOF_VAR 	15
#define NUM_OF_LGSEFN004_PEQ_MODE3_SIZEOF_INIT 	0
#define NUM_OF_LGSEFN004_PEQ_MODE3_SIZEOF_VAR 	25

#define NUM_OF_LGSEFN005_HARM_SIZEOF_INIT 		13
#define NUM_OF_LGSEFN005_HARM_SIZEOF_VAR 		0

#define NUM_OF_LGSEFN008_HC_SIZEOF_INIT 		108
#define NUM_OF_LGSEFN008_HC_SIZEOF_VAR 			11     //8 -> 11

#define NUM_OF_LGSEFN009_OSD_SIZEOF_INIT 		31
#define NUM_OF_LGSEFN009_OSD_SIZEOF_VAR 		2

#define NUM_OF_LGSEFN010_IC_SIZEOF_INIT 		10
#define NUM_OF_LGSEFN010_IC_SIZEOF_VAR 			1

#define NUM_OF_LGSEFN011_IVSE_SIZEOF_INIT 		64
#define NUM_OF_LGSEFN011_IVSE_SIZEOF_VAR 		6

#define NUM_OF_LGSEFN013_NE_SIZEOF_INIT 		5
#define NUM_OF_LGSEFN013_NE_SIZEOF_VAR 			1

#define NUM_OF_LGSEFN014_ASC_SIZEOF_INIT 		136
#define NUM_OF_LGSEFN014_ASC_SIZEOF_VAR 		1

#define NUM_OF_LGSEFN015_SIZEOF_INIT 			13		//?
#define NUM_OF_LGSEFN015_SIZEOF_VAR 			0		//?

#define NUM_OF_LGSEFN016_ELC_SIZEOF_INIT 		361
#define NUM_OF_LGSEFN016_ELC_SIZEOF_VAR 		0

//return value
#define NUM_OF_LGSEFN010_IC_SIZEOF_OUT 			2
#define NUM_OF_LGSEFN014_ASC_SIZEOF_OUT 		7


#define NUM_OF_LGSEF_SIZEOF_GET_PARAM			400


#define ADDR_OF_LGSE_MAIN			(NUM_OF_BASIC_PARAM)
#define ADDR_OF_LGSEFN000_CV		(ADDR_OF_LGSE_MAIN			+ NUM_OF_LGSE_MAIN_SIZEOF_INIT				+ NUM_OF_LGSE_MAIN_SIZEOF_VAR)
#define ADDR_OF_LGSEFN001_AV		(ADDR_OF_LGSEFN000_CV		+ NUM_OF_LGSEFN000_CV_SIZEOF_INIT			+ NUM_OF_LGSEFN000_CV_SIZEOF_VAR)
#define ADDR_OF_LGSEFN002_DB		(ADDR_OF_LGSEFN001_AV		+ NUM_OF_LGSEFN001_AV_SIZEOF_INIT			+ NUM_OF_LGSEFN001_AV_SIZEOF_VAR)
#define ADDR_OF_LGSEFN003_DEQ		(ADDR_OF_LGSEFN002_DB 		+ NUM_OF_LGSEFN002_DB_SIZEOF_INIT			+ NUM_OF_LGSEFN002_DB_SIZEOF_VAR)
#define ADDR_OF_LGSEFN004_PEQMODE1	(ADDR_OF_LGSEFN003_DEQ		+ NUM_OF_LGSEFN003_DEQ_SIZEOF_INIT			+ NUM_OF_LGSEFN003_DEQ_SIZEOF_VAR)
#define ADDR_OF_LGSEFN004_PEQMODE2	(ADDR_OF_LGSEFN004_PEQMODE1 + NUM_OF_LGSEFN004_PEQ_MODE1_SIZEOF_INIT	+ NUM_OF_LGSEFN004_PEQ_MODE1_SIZEOF_VAR)
#define ADDR_OF_LGSEFN004_PEQMODE3	(ADDR_OF_LGSEFN004_PEQMODE2 + NUM_OF_LGSEFN004_PEQ_MODE2_SIZEOF_INIT	+ NUM_OF_LGSEFN004_PEQ_MODE2_SIZEOF_VAR)
#define ADDR_OF_LGSEFN005_HARM		(ADDR_OF_LGSEFN004_PEQMODE3 + NUM_OF_LGSEFN004_PEQ_MODE3_SIZEOF_INIT	+ NUM_OF_LGSEFN004_PEQ_MODE3_SIZEOF_VAR)
#define ADDR_OF_LGSEFN008_HC		(ADDR_OF_LGSEFN005_HARM		+ NUM_OF_LGSEFN005_HARM_SIZEOF_INIT			+ NUM_OF_LGSEFN005_HARM_SIZEOF_VAR)
#define ADDR_OF_LGSEFN009_OSD		(ADDR_OF_LGSEFN008_HC		+ NUM_OF_LGSEFN008_HC_SIZEOF_INIT			+ NUM_OF_LGSEFN008_HC_SIZEOF_VAR)
#define ADDR_OF_LGSEFN010_IC		(ADDR_OF_LGSEFN009_OSD		+ NUM_OF_LGSEFN009_OSD_SIZEOF_INIT			+ NUM_OF_LGSEFN009_OSD_SIZEOF_VAR)
#define ADDR_OF_LGSEFN011_IVSE		(ADDR_OF_LGSEFN010_IC		+ NUM_OF_LGSEFN010_IC_SIZEOF_INIT			+ NUM_OF_LGSEFN010_IC_SIZEOF_VAR)
#define ADDR_OF_LGSEFN013_NE		(ADDR_OF_LGSEFN011_IVSE		+ NUM_OF_LGSEFN011_IVSE_SIZEOF_INIT			+ NUM_OF_LGSEFN011_IVSE_SIZEOF_VAR)
#define ADDR_OF_LGSEFN014_ASC		(ADDR_OF_LGSEFN013_NE		+ NUM_OF_LGSEFN013_NE_SIZEOF_INIT			+ NUM_OF_LGSEFN013_NE_SIZEOF_VAR)
#define ADDR_OF_LGSEFN015			(ADDR_OF_LGSEFN014_ASC		+ NUM_OF_LGSEFN014_ASC_SIZEOF_INIT			+ NUM_OF_LGSEFN014_ASC_SIZEOF_VAR)
#define ADDR_OF_LGSEFN016_ELC		(ADDR_OF_LGSEFN015			+ NUM_OF_LGSEFN015_SIZEOF_INIT				+ NUM_OF_LGSEFN015_SIZEOF_VAR)

#define	ADDR_OF_LGSEF010_IC_OUT		(ADDR_OF_LGSEFN016_ELC		+ NUM_OF_LGSEFN016_ELC_SIZEOF_INIT			+ NUM_OF_LGSEFN016_ELC_SIZEOF_VAR)
#define	ADDR_OF_LGSEF014_ASC_OUT	(ADDR_OF_LGSEF010_IC_OUT	+ NUM_OF_LGSEFN010_IC_SIZEOF_OUT)

#define ADDR_OF_LGSEF_GET_PARAM		(ADDR_OF_LGSEF014_ASC_OUT	+ NUM_OF_LGSEFN014_ASC_SIZEOF_OUT)

#define	ADDR_OF_END					(ADDR_OF_LGSEF_GET_PARAM	+ NUM_OF_LGSEF_SIZEOF_GET_PARAM)




typedef enum
{
	AUD_SE_FN000_CV,				// 0
	AUD_SE_FN001_AV,				// 1
	AUD_SE_FN002_DB,				// 2
	AUD_SE_FN003_DEQ,				// 3
	AUD_SE_FN004_PEQMODE1,			// 4
	AUD_SE_FN004_PEQMODE2,			// 5
	AUD_SE_FN004_PEQMODE3,			// 6
	AUD_SE_FN005_HARM,				// 7
	AUD_SE_FN006_,					// 8
	AUD_SE_FN007_,					// 9
	AUD_SE_FN008_HC,				// 10
	AUD_SE_FN009_OSD,				// 11
	AUD_SE_FN010_IC,				// 12
	AUD_SE_FN011_IVSE,				// 13
	AUD_SE_FN012_,					// 14
	AUD_SE_FN013_NE,				// 15
	AUD_SE_FN014_ASC,				// 16
	AUD_SE_FN015_,					// 17
	AUD_SE_FN016_ELC,				// 18
	AUD_SE_MAIN,					// 19
	AUD_SE_FN_MAX,
	AUD_SE_FN_UPDATE_PARAM	= 30	, // 30
}AUD_SE_FN_MODE_T;






#endif
