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
 *  ISR handler implementation for ci device.
 *
 *  author		Srinivasan Shanmugam (srinivasan.shanmugam@lge.com)
 *  author		Hwajeong Lee (hwajeong.lee@lge.com)
 *  author		Jinhwan Bae (jinhwan.bae@lge.com) - modifier
 *  version		1.0
 *  date		2010.02.19
 *  note		Additional information.
 *
 *  @addtogroup lg1150_ci
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/memory.h>


#include "os_util.h"
#include "ci_drv.h"
#include "ci_regdefs.h"
#include "ci_io.h"


/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define CD_CHANGED				0x0001
#define HW_HOT_SWAP_DONE		0x0004
#define READY_IREQ				0x0008
#define CD_POL_VAL				0x80
#define POWER_OFF_SEQ			0x0000
#define POWER_ON_SEQ			0x0024
#define CD_CHANGED_DISABLE_INT	0xFFFF
#define CD_CHANGED_ENABLE_INT	0xFFFE

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/
extern volatile S_CI_REG_T* gpstCIReg;
extern volatile UINT32 guwDetectCard;

extern BOOLEAN gbIsCISChecked;


/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
irqreturn_t CI_ISR_Handler( int irq, void *dev_id, struct pt_regs *regs );

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
/**
 *	CI ISR handler
*/
irqreturn_t CI_ISR_Handler( int irq, void *dev_id, struct pt_regs *regs )
{
	UINT16 			byCurrentIt;
	UINT16 			unRegVal = 0;



	if( !gpstCIReg )
	{
		CI_DTV_SOC_Message(CI_INFO,"CI ISR - Null ptr!\n");

		return IRQ_HANDLED;
	}

	gpstCIReg->uniIntrMask.unIntrMask = CD_CHANGED_DISABLE_INT;

	byCurrentIt = gpstCIReg->uniIntrFlag.unIntrFlag;

	unRegVal = gpstCIReg->uniIntrCntrl.unIntrCntrl;


	/* Check CI detect interrupt and update the flag */
	if( byCurrentIt & CD_CHANGED )
	{
		if( 0x00 == ( gpstCIReg->uniCntrlStat2.unCntrlStat2 & 0x03 ) ){
			unRegVal |= CD_POL_VAL;
			guwDetectCard = 1;
			
			CI_DTV_SOC_Message(CI_INFO,"CAM Inserted ... \n");
		}
		else{
			unRegVal &= ~CD_POL_VAL;

			guwDetectCard = 0;

			gbIsCISChecked = FALSE;

			CI_DTV_SOC_Message(CI_INFO,"CAM removed ... \n");
		}


		gpstCIReg->uniIntrCntrl.unIntrCntrl = unRegVal;

		gpstCIReg->uniIntrFlag.unIntrFlag = byCurrentIt & 0xFE;

	}

	gpstCIReg->uniIntrMask.unIntrMask = CD_CHANGED_ENABLE_INT;

	/*------------------------------------------------------------------------
	  * Jinhwan Bae , 2012/09/17
	  * To Block POWER OFF and PC CARD Disable at the time of CAM removing
	  * Because, if we set like this, PCMCIA Bus latch Up happened 
	  * at the time of access bus by CPU without CAM,
	  * it causes the problem that System Latch Up when we insert/remove CAM repeatedly
	  * The problem opened at 2nd SU of L9 Platform, but same problem exist in H13.
	  * Found temp solution and adapted it until new H/W solution in H15.
	  *-----------------------------------------------------------------------*/ 
#if 0
	if( !guwDetectCard )
		gpstCIReg->uniCntrlStat1.unCntrlStat1 = 0x0;		// power off
#endif

	return IRQ_HANDLED;
}

/** @} */


