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


/*****************************************************************************
**
**  Name: ademod_common_SmartTune.c
**
**  Description:    ABB API example. SmartTune table descriptor.
**
**  Dependencies:   ademod_common_demod.h for basic types.
**
**  Revision History:
**
**     Date        Author          Description
**  -------------------------------------------------------------------------
**   31-07-2013   Jeongpil Yun    Initial draft.
**
*****************************************************************************/
#define _CRT_SECURE_NO_DEPRECATE (1)
#if defined( __cplusplus )
extern "C"                     /* Use "C" external linkage */
{
#endif
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>			/**< printk() */
#include <linux/slab.h>			 	/**< kmalloc() */
#include <linux/fs.h> 				/**< everything\ldots{} */
#include <linux/types.h>		 	/**< size_t */
#include <linux/fcntl.h>			/**< O_ACCMODE */
#include <asm/uaccess.h>
#include <linux/ioport.h>			/**< For request_region, check_region etc */
#include <asm/io.h>					/**< For ioremap_nocache */
#include <linux/workqueue.h>		/**< For working queue */
#include <linux/interrupt.h>
#include <linux/irq.h>


#include "ademod_common.h"
#include "ademod_common_SmartTune.h"

const LX_ADEMOD_SmartTuneEntry	*_gpSmartTuneTable 		= NULL;
UINT32					_gSmartTuneTableSize 	= 0;


const LX_ADEMOD_SmartTuneEntry SmartTuneTable_KR_US[] =
{
/*		Start		End 		Sample Rate	IF Freq  	0 */
	{ 75000000, 	80000000, 	240000000, 	45750000, 	0 }, //110110
	{ 82000000, 	85000000,  	240000000, 	45750000, 	0 },
	{ 85500000, 	85600000, 	240000000, 	45750000, 	0 }, //110118
	{ 209000000, 	213000000, 	240000000, 	45750000, 	0 }, //101228
	{ 228000000, 	231000000, 	240000000, 	45750000, 	0 },
	{ 246000000, 	248000000, 	240000000, 	45750000, 	0 },
	{ 266000000, 	267000000, 	240000000, 	45750000, 	0 },
	{ 312000000, 	315000000, 	240000000, 	45750000, 	0 },
	{ 420000000, 	421000000, 	240000000, 	45750000, 	0 },
	{ 422000000, 	423000000, 	240000000, 	45750000, 	0 },
	{ 450000000, 	453000000, 	240000000, 	45750000, 	0 },
	{ 470000000, 	473000000, 	240000000, 	45750000, 	0 },
	{ 476000000, 	479000000, 	240000000, 	45750000, 	0 },
	{ 480000000, 	483000000, 	240000000, 	45750000, 	0 },
	{ 506000000, 	509000000, 	240000000, 	45750000, 	0 },
	{ 534000000, 	537000000, 	240000000, 	45750000, 	0 },
	{ 564000000, 	567000000, 	240000000, 	45750000, 	0 },
	{ 716000000, 	719000000, 	240000000, 	45750000, 	0 },
	{ 722000000, 	725000000, 	240000000, 	45750000, 	0 },
	{ 728000000, 	731000000, 	240000000, 	45750000, 	0 },
	{ 740000000, 	743000000, 	240000000, 	45750000, 	0 },
	{ 824000000, 	827000000, 	240000000, 	45750000, 	0 },

};

const LX_ADEMOD_SmartTuneEntry SmartTuneTable4TDTD[] =			/* Channel Browser */
{
/*      Start       End       Sample Rate IF Freq  0 */
    { 54000000,   60000000, 240000000, 46750000, 0 }, // 54 	to 60   	+3
    { 60500000,   63500000, 240000000, 46750000, 0 }, // 60.5 	to 63.5   	+1
    { 66500000,   69500000, 240000000, 46750000, 0 }, // 66.5 	to 69.5   	-2
    { 72500000,   75500000, 240000000, 46750000, 0 }, // 72.5 	to 75.5   	-4
    { 82000000,   88000000, 240000000, 46750000, 0 }, // 82 	to 88   	-4
    { 114000000, 120000000, 240000000, 46750000, 0 }, // 114 	to 120   	-1
    { 132500000, 135500000, 240000000, 46750000, 0 }, // 132.5 	to 135.5 	+2
    { 150500000, 153500000, 240000000, 46750000, 0 }, // 150.5 	to 153.5 	-2
    { 156000000, 162000000, 240000000, 46750000, 0 }, // 156 	to 162   	-4
    { 168000000, 174000000, 240000000, 46750000, 0 }, // 168 	to 174   	+1
    { 180000000, 185000000, 240000000, 46750000, 0 }, // 180 	to 185   	+1
    { 186500000, 189500000, 240000000, 46750000, 0 }, // 186.5 	to 189.5 	-2
    { 198000000, 204000000, 240000000, 46750000, 0 }, // 198 	to 204   	-2
    { 210000000, 215000000, 240000000, 46750000, 0 }, // 210 	to 215   	-4
    { 216500000, 219500000, 240000000, 46750000, 0 }, // 216.5 	to 219.5 	-1
    { 228000000, 234000000, 240000000, 46750000, 0 }, // 228	to 234   	-1
    { 240000000, 246000000, 240000000, 46750000, 0 }, // 240 	to 246   	-3
    { 258000000, 264000000, 240000000, 46750000, 0 }, // 258 	to 264   	-1
    { 270000000, 275500000, 240000000, 46750000, 0 }, // 270 	to 275.5 	-1
    { 282500000, 285500000, 240000000, 46750000, 0 }, // 282.5 	to 285.5 	-2
    { 294500000, 297500000, 240000000, 46750000, 0 }, // 294.5 	to 297.5 	+1
    { 300000000, 305000000, 240000000, 46750000, 0 }, // 300 	to 305   	-1
    { 306500000, 309500000, 240000000, 46750000, 0 }, // 306.5 	to 309.5 	-3
    { 312000000, 318000000, 240000000, 46750000, 0 }, // 312 	to 318   	-1
    { 324000000, 327500000, 240000000, 46750000, 0 }, // 324 	to 327.5 	-2
    { 330500000, 333000000, 240000000, 46750000, 0 }, // 330.5 	to 333   	-1
    { 342000000, 348000000, 240000000, 46750000, 0 }, // 342 	to 348   	-1
    { 354000000, 359000000, 240000000, 46750000, 0 }, // 354 	to 359   	-2
    { 360000000, 365000000, 240000000, 46750000, 0 }, // 360 	to 365   	+2
    { 366000000, 372000000, 240000000, 46750000, 0 }, // 366 	to 372   	-3
    { 372000000, 378000000, 240000000, 46750000, 0 }, // 372 	to 378   	-1
    { 384000000, 390000000, 240000000, 46750000, 0 }, // 384 	to 390   	-2
    { 390000000, 395000000, 240000000, 46750000, 0 }, // 390 	to 395   	-2
    { 396000000, 400000000, 240000000, 46750000, 0 }, // 396 	to 400   	-2
    { 402500000, 405500000, 240000000, 46750000, 0 }, // 402.5 	to 405.5	-1
    { 414000000, 419000000, 240000000, 46750000, 0 }, // 414 	to 419   	-3
    { 420500000, 423500000, 240000000, 46750000, 0 }, // 420.5 	to 423.5	-1
    { 444500000, 447500000, 240000000, 46750000, 0 }, // 444.5 	to 447.5	-3
    { 450500000, 453000000, 240000000, 46750000, 0 }, // 450.5 	to 453		-2
    { 456500000, 459500000, 240000000, 46750000, 0 }, // 456.5 	to 459.5	-1
    { 486500000, 491500000, 240000000, 46750000, 0 }, // 486.5 	to 491.5	-1
    { 498000000, 503500000, 240000000, 46750000, 0 }, // 498 	to 503.5 	+1
    { 504500000, 506500000, 240000000, 46750000, 0 }, // 504.5 	to 506.5	-1
    { 516500000, 518500000, 240000000, 46750000, 0 }, // 516.5 	to 518.5	+1
    { 518600000, 521000000, 240000000, 46750000, 0 }, // 518.6 	to 521   	-1
    { 524000000, 530000000, 240000000, 46750000, 0 }, // 524 	to 530   	-1
    { 534000000, 539500000, 240000000, 46750000, 0 }, // 534 	to 539.5 	-2
    { 545750000, 545500000, 240000000, 46750000, 0 }, // 542.5 	to 545.5 	-1
    { 548000000, 554000000, 240000000, 46750000, 0 }, // 548 	to 554   	-1
    { 554500000, 557500000, 240000000, 46750000, 0 }, // 554.5 	to 557.5 	-1
    { 572000000, 578000000, 240000000, 46750000, 0 }, // 572 	to 578   	-1
    { 618000000, 620500000, 240000000, 46750000, 0 }, // 618 	to 620.5 	-1
    { 620600000, 623500000, 240000000, 46750000, 0 }, // 620.6 	to 623.5 	-1
    { 632500000, 635500000, 240000000, 46750000, 0 }, // 632.5 	to 635.5 	-1
    { 644000000, 650000000, 240000000, 46750000, 0 }, // 644 	to 650   	-1
    { 662500000, 665500000, 240000000, 46750000, 0 }, // 662.5 	to 665.5 	-1
    { 680000000, 686000000, 240000000, 46750000, 0 }, // 680 	to 686   	-1
    { 716000000, 722000000, 240000000, 46750000, 0 }, // 716 	to 722   	-3
    { 728000000, 734000000, 240000000, 46750000, 0 }, // 728 	to 734   	-1
    { 740000000, 746000000, 240000000, 46750000, 0 }, // 740 	to 746   	-1
    { 776500000, 779500000, 240000000, 46750000, 0 }, // 776.5 	to 779.5 	+1
    { 786000000, 794000000, 240000000, 46750000, 0 }, // 786 	to 794   	-1
    { 806500000, 809500000, 240000000, 46750000, 0 }, // 806.5 	to 809.5 	-1
    { 812500000, 815500000, 240000000, 46750000, 0 }, // 812.5 	to 815.5 	+1
    { 822000000, 828000000, 240000000, 46750000, 0 }, // 822 	to 828   	-2
    { 836500000, 839500000, 240000000, 46750000, 0 }, // 836.5 	to 839.5 	-1
    { 860500000, 863500000, 240000000, 46750000, 0 }, // 860.5 	to 863.5 	+1
};


int ADEMOD_SetSmartTuneTable(LX_DEMOD_RF_MODE_T rfMode)
{

		if(LX_DEMOD_NTSC == rfMode )
		{
			DEMOD_PRINT("^p^[ABB M14] SmartTuneTable_KR_US !!!\n");
			_gpSmartTuneTable		= &SmartTuneTable_KR_US[0];
			_gSmartTuneTableSize	= sizeof(SmartTuneTable_KR_US) / sizeof(LX_ADEMOD_SmartTuneEntry);
		}
		else
		{
			DEMOD_PRINT("[ABB M14] SmartTuneTable4TDTD !!!\n");
			_gpSmartTuneTable		= &SmartTuneTable4TDTD[0];
			_gSmartTuneTableSize	= sizeof(SmartTuneTable4TDTD) / sizeof(LX_ADEMOD_SmartTuneEntry);

		}
	return retOK;
}



/****************************************************************************************************
*   Function:      SpurAvoid
*
*   Description:
*       Searches through the Smart Tune look-up table and updates parameters,
*       such as Sampling rate, IF etc, if necessary
*
*  Parameters:  hDemod			  - handle to demodulator's data
*				pSmartTune        - SmartTune descriptor.
*
*   Returns:
*       0 - success, 1 - error.
*
****************************************************************************************************/
int ADEMOD_SpurAvoid(LX_ADEMOD_SmartTuneDescriptor *pSmartTune)
{
	int rc = 0;
	//int Size = 0;
	//int i;
	UINT32 OldSmplRate = pSmartTune->SmplRate;
	//pSmartTuneEntry pSmpartTuneTable;

	pSmartTune->Update = 0;
	pSmartTune->SmplRate = LX_ADEMOD_SAMPLING_RATE_240MHz; // default

#if 0
	// Search for the range, which includes the frequency
	for (i = 0; i < _gSmartTuneTableSize; i++)
	{
		if (pSmartTune->Freq < _gpSmartTuneTable[i].LoFreq)
		{
			continue;
		}
		else if (pSmartTune->Freq <= _gpSmartTuneTable[i].HiFreq)
		{
			pSmartTune->SmplRate = _gpSmartTuneTable[i].SmplRate;
			break;
		}
		else
		{
			continue;
		}
	}
#endif

	pSmartTune->SmplRate = LX_ADEMOD_SAMPLING_RATE_240MHz;

	DEMOD_PRINT("pSmartTune->Freq = %d, pSmartTune->SmplRate = %d  \n",pSmartTune->Freq,pSmartTune->SmplRate);

	if (pSmartTune->SmplRate != OldSmplRate)
	{
		pSmartTune->Update = 1;
	}
	return rc;

}


#if defined( __cplusplus )
}
#endif

