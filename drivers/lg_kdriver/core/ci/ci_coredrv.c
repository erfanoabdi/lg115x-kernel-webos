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
 *  Core driver implementation for ci device.
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
#include <linux/delay.h>	// for usleep_range, jinhwan.bae 20131018

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/memory.h>

#include "os_util.h"
#include "ci_drv.h"
#include "ci_cis.h"
#include "ci_io.h"
#include "ci_regdefs.h"
#include "ci_defs.h"			// for delay definition, jinhwan.bae

#include "../sys/sys_regs.h"	//for CTOP CTRL Reg. map
#include "ci_coredrv.h"


/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define INIT_VAL			0x0000
#define CLK_128				0x0080
#define CLK_511				0X01FF
#define CLK_639 			0X027F
#define	CD1_CD2_HIGH		0x03
#define CD_POL				0x80

#define DA_STATUS			0x80

/* Control and Status1 */
#define VCC_SW0				0x0004
#define EN_PCCARD			0x0020
#define CARD_RESET			0x0080

/* Control and Status3 */
#define IO_INT_MODE			0x0004
#define POD_MODE			0x0010

/* PC Card Control1 */
#define CHIP_MODE_IO		0X001D
#define CHIP_MODE_ATTR		0X001B

/* for CI plus */
#define RESET_PHY_INT		0x10
#define IIR_STATUS			0x10


/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
extern irqreturn_t CI_ISR_Handler( int irq, void *dev_id, struct pt_regs *regs );

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
volatile S_CI_REG_T* gpstCIReg = NULL;

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
volatile UINT32 guwDetectCard;
volatile UINT32 guwIsPowerRestart;

unsigned long	CiModBaseAddr[MAX_MOD_NUM];

UINT32 gCI_KDRV_Delay[CI_D_ENUM_MAX];
UINT32 gDefault_CI_Delay[CI_D_ENUM_MAX] = { DEFAULT_CI_DELAY_CIS_CONFIG_FIRST_TUPLE_OK,				// 5
											DEFAULT_CI_DELAY_CIS_CONFIG_FIRST_TUPLE_NG,				// 10
											DEFAULT_CI_DELAY_CIS_END_WRITE_COR,						// 100
											DEFAULT_CI_DELAY_CIS_DURING_READ_TUPLE,					// 5
											DEFAULT_CI_DELAY_CIS_END_READ_TUPLE_INITIAL,			// SLEEP_VALUE_INIT, 20
											DEFAULT_CI_DELAY_CIS_PARSE_NON_CI_TUPLE,				// 3
											DEFAULT_CI_DELAY_INIT_POWER_CONTROL,					// H13 Blocked, L9 5
											DEFAULT_CI_DELAY_INIT_AFTER_INTERRUPT_ENABLE,			// 10
											DEFAULT_CI_DELAY_CAM_INIT_BTW_VCC_CARDRESET,			// 300
											DEFAULT_CI_DELAY_CAM_INIT_BTW_CARDRESET_NOTRESET,		// 5
											DEFAULT_CI_DELAY_IO_SOFT_RESET_CHECK_FR,				// 10
											DEFAULT_CI_DELAY_IO_END_SOFT_RESET,						// L9 Blocked, 0 Originally (10)
											DEFAULT_CI_DELAY_IO_NEGOBUF_BEFORE_SOFTRESET,			// L9 Blocked, 0 Originally (100)
											DEFAULT_CI_DELAY_IO_NEGOBUF_CHECK_DA,					// 10
											DEFAULT_CI_DELAY_IO_NEGOBUF_CHECK_FR,					// 10
											DEFAULT_CI_DELAY_IO_NEGOBUF_AFTER_WRITE_DATA,			// 5 , Previously (10)
											DEFAULT_CI_DELAY_IO_READ_CHECK_DA,						// 10
											DEFAULT_CI_DELAY_IO_WRITE_CHECK_DA,						// 10
											DEFAULT_CI_DELAY_IO_WRITE_CHECK_FR,						// 10
											DEFAULT_CI_DELAY_IO_WRITE_FIRST_BYTE_STAT_RD_FR_WE,		// 10
											DEFAULT_CI_DELAY_IO_WRITE_MIDDLE_BYTE_CHECK_WE,			// 10
											DEFAULT_CI_DELAY_IO_WRITE_LAST_BYTE_CHECK_WE		};	// 10

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

#if 1 /* Add code for supporting Hibernation Suspend/Resume 2013. 08. 14. jinhwan.bae */

UINT32 gSMCVirtAddr = 0;
void CI_InitSMC(void)
{
	gSMCVirtAddr	= (UINT32) ioremap(0xFF400038, 0x1C);
	*(UINT32*)(gSMCVirtAddr) = (UINT32)0x0;		
	*(UINT32*)(gSMCVirtAddr+0x14) = (UINT32)0x45;

	return;
}

void CI_UnmapSMC(void)
{
	if(gSMCVirtAddr)
	{
		iounmap((void *)gSMCVirtAddr);
		gSMCVirtAddr = 0;
	}

	return;
}

void CI_DefaultInit(void)
{
	UINT16 	unRetVal = 0;
	
	/* jinhwan.bae 20140303 make ci related ports internal pull-up for no-ci platform */
	if( lx_chip_rev() >= LX_CHIP_REV(H14, A0) )
	{
		/* TBD - should be checked CTOP Register  */	
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, B0) )
	{
		/* set TOP CTRL18 , offset 0x48,  to 0 */
		CTOP_CTRL_M14B0_WRITE(TOP, 0x48, 0);
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, A0) )
	{
		/* set CTOP CTRL53, offset 0xD4, to 0 */
		CTOP_CTRL_M14_WRITE(0xD4, 0);
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(H13, A0) )
	{
		/* set CTOP CTRL53, offset 0xD4, to 0 */
		CTOP_CTRL_H13_WRITE(0xD4, 0);
	}
	
	// Initialize global varaibles 
	guwDetectCard 			= 0;
	guwIsPowerRestart 		= 0;

	
	// Set Not-POD Mode 
	gpstCIReg->uniCntrlStat3.unCntrlStat3 = INIT_VAL;
	gpstCIReg->uniIntrCntrl.unIntrCntrl = INIT_VAL;

	// Interrupt Enable 
	gpstCIReg->uniIntrMask.unIntrMask = 0xFFFE;	

#ifdef _CI_KDRV_DELAY_USLEEP_RANGE_
	CI_DTV_SOC_Message(CI_NOTI,"CI Driver use usleep_range \n");
	usleep_range(gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE]*1000, gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE]*1000);
#else
	CI_DTV_SOC_Message(CI_NOTI,"CI Driver use msleep \n");
	OS_MsecSleep( gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE] );	 			// for interrupt signal is stabled
#endif	

	unRetVal = gpstCIReg->uniIntrCntrl.unIntrCntrl;

	// CI Module detect ... check if module is already inserted 
	if( 0x00 == ( gpstCIReg->uniCntrlStat2.unCntrlStat2 & CD1_CD2_HIGH ) )
	{
		// CAM Inserted 
		unRetVal |= CD_POL;
		guwDetectCard = 1;

		CI_DTV_SOC_Message(CI_DBG_INFO,"CAM Module is inserted ...\n");
	}
	else
	{
		// CAM Removed 
		unRetVal &= ~CD_POL;
		guwDetectCard = 0;

		CI_DTV_SOC_Message(CI_DBG_INFO,"CAM Module is not inserted ... \n");
	}
		
	gpstCIReg->uniPCCardCntrl2.unPCCardCntrl2 = 0x0f03;
	gpstCIReg->uniPCCardCntrl3.unPCCardCntrl3 = 0x0003;

	gpstCIReg->uniIntrCntrl.unIntrCntrl = unRetVal;

	gpstCIReg->uniIntrFlag.unIntrFlag = INIT_VAL;

	return;

}

#endif

/**
 *	Initialize CICAM and start Irq
*/
SINT32 CI_Initialize( void )
{
	UINT32	i = 0;

	/* CI Delay Constant Set */
	for(i=0; i<CI_D_ENUM_MAX; i++)
	{
		gCI_KDRV_Delay[i] = gDefault_CI_Delay[i];
	}

	if( lx_chip_rev() >= LX_CHIP_REV(H14, A0) )
	{
		// Set the register area
		gpstCIReg = ( S_CI_REG_T* )ioremap(H14_CI_REG_BASE, 0x3E);
		CiModBaseAddr[MOD_A]	= (unsigned long) ioremap(H14_CI_CAM_BASE, 0x8000);

		// Control for SMC H14_TBD jinhwan.bae please check the address and functionalities
#if 0
		virtAddr	= (UINT32) ioremap(0xFF400038, 0x1C);
		*(UINT32*)(virtAddr) = (UINT32)0x0;		
		*(UINT32*)(virtAddr+0x14) = (UINT32)0x45;
#else
		CI_InitSMC();
#endif
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, B0) )
	{
		// Set the register area
		gpstCIReg = ( S_CI_REG_T* )ioremap(M14_B0_CI_REG_BASE, 0x3E);
#ifdef _CI_ACCESS_BURST_MODE_IO_
		/* 	jinhwan.bae for IO Read/Write Register 
			DATA32_RD(mId)			DWORD(CiModBaseAddr[mId]  + DATA32_REG) */
		CiModBaseAddr[MOD_A]	= (unsigned long) ioremap(M14_B0_CI_CAM_BASE, 0x8010);
#else
		CiModBaseAddr[MOD_A]	= (unsigned long) ioremap(M14_B0_CI_CAM_BASE, 0x8000);
#endif

		// Control for SMC M14_TBD jinhwan.bae please check the address and functionalities
#if 0		
		virtAddr	= (UINT32) ioremap(0xFF400038, 0x1C);
		*(UINT32*)(virtAddr) = (UINT32)0x0;		
		*(UINT32*)(virtAddr+0x14) = (UINT32)0x45;
#else
		CI_InitSMC();
#endif
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, A0) )
	{
		// Set the register area
		gpstCIReg = ( S_CI_REG_T* )ioremap(M14_A0_CI_REG_BASE, 0x3E);
		CiModBaseAddr[MOD_A]	= (unsigned long) ioremap(M14_A0_CI_CAM_BASE, 0x8000);

		// Control for SMC
#if 0		
		virtAddr	= (UINT32) ioremap(0xFF400038, 0x1C);
		*(UINT32*)(virtAddr) = (UINT32)0x0;		
		*(UINT32*)(virtAddr+0x14) = (UINT32)0x45;
#else
		CI_InitSMC();
#endif		
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(H13, A0) )
	{
		// Set the register area
		gpstCIReg = ( S_CI_REG_T* )ioremap(H13_CI_REG_BASE, 0x3E);
		CiModBaseAddr[MOD_A]	= (unsigned long) ioremap(H13_CI_CAM_BASE, 0x8000);

		// Control for SMC
#if 0		
		virtAddr	= (UINT32) ioremap(0xFF400038, 0x1C);
		*(UINT32*)(virtAddr) = (UINT32)0x0;		
		*(UINT32*)(virtAddr+0x14) = (UINT32)0x45;
#else
		CI_InitSMC();
#endif		

		/* jinhwan.bae 2012. 05. 28 Difference exist from L9. VCC inverter design is differ from L9.
		  * If on, at the time of VCC_SW0 set in Control and Status Register 1, bit 2 VCCEN_N, 
		  * Card VCC (5V_CI) is down to 2V, Card is not active, CIS can't be read
		  * Remember H13 Bring Up 													*/

#if 0	
		/* release reset */
		gpstCIReg->uniCntrlStat1.unCntrlStat1 = 0x0;
#ifdef _CI_KDRV_DELAY_USLEEP_RANGE_
		usleep_range(gCI_KDRV_Delay[CI_D_INIT_POWER_CONTROL]*1000, gCI_KDRV_Delay[CI_D_INIT_POWER_CONTROL]*1000);
#else
		OS_MsecSleep( gCI_KDRV_Delay[CI_D_INIT_POWER_CONTROL] );
#endif
		// CTOP Power control (H13)
		CTOP_CTRL_H13A0_RdFL(ctr32);
		CTOP_CTRL_H13A0_Wr01(ctr32, cam_vcc_pol, 0x1); 
		CTOP_CTRL_H13A0_WrFL(ctr32);		
#endif
	}

#if 0
	// Initialize global varaibles 
	guwDetectCard 			= 0;
	guwIsPowerRestart 		= 0;

	
	// Set Not-POD Mode 
	gpstCIReg->uniCntrlStat3.unCntrlStat3 = INIT_VAL;
	gpstCIReg->uniIntrCntrl.unIntrCntrl = INIT_VAL;

	// Interrupt Enable 
	gpstCIReg->uniIntrMask.unIntrMask = 0xFFFE;	
#ifdef _CI_KDRV_DELAY_USLEEP_RANGE_
	usleep_range(gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE]*1000, gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE]*1000);
#else
	OS_MsecSleep( gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE] );	 			// for interrupt signal is stabled
#endif

	unRetVal = gpstCIReg->uniIntrCntrl.unIntrCntrl;

	// CI Module detect ... check if module is already inserted 
	if( 0x00 == ( gpstCIReg->uniCntrlStat2.unCntrlStat2 & CD1_CD2_HIGH ) )
	{
		// CAM Inserted 
		unRetVal |= CD_POL;
		guwDetectCard = 1;

		CI_DTV_SOC_Message(CI_DBG_INFO,"CAM Module is inserted ...\n");
	}
	else
	{
		// CAM Removed 
		unRetVal &= ~CD_POL;
		guwDetectCard = 0;

		CI_DTV_SOC_Message(CI_DBG_INFO,"CAM Module is not inserted ... \n");
	}
		
	gpstCIReg->uniPCCardCntrl2.unPCCardCntrl2 = 0x0f03;
	gpstCIReg->uniPCCardCntrl3.unPCCardCntrl3 = 0x0003;

	gpstCIReg->uniIntrCntrl.unIntrCntrl = unRetVal;

	gpstCIReg->uniIntrFlag.unIntrFlag = INIT_VAL;
#else
	CI_DefaultInit();
#endif

	if( lx_chip_rev() >= LX_CHIP_REV(H14, A0) )
	{
		if( request_irq ( H14_IRQ_DVBCI, ( irq_handler_t )CI_ISR_Handler , 0, "GPCI", NULL ) )
		{
			CI_DTV_SOC_Message(CI_ERR,"Request IRQ failed!\n");
			return NOT_OK;
		}
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, B0) )
	{
		if( request_irq ( M14_B0_IRQ_DVBCI, ( irq_handler_t )CI_ISR_Handler , 0, "GPCI", NULL ) )
		{
			CI_DTV_SOC_Message(CI_ERR,"Request IRQ failed!\n");
			return NOT_OK;
		}
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, A0) )
	{
		if( request_irq ( M14_A0_IRQ_DVBCI, ( irq_handler_t )CI_ISR_Handler , 0, "GPCI", NULL ) )
		{
			CI_DTV_SOC_Message(CI_ERR,"Request IRQ failed!\n");
			return NOT_OK;
		}
	}
	else if( lx_chip_rev() >= LX_CHIP_REV(H13, A0) )
	{
		if( request_irq ( H13_IRQ_DVBCI, ( irq_handler_t )CI_ISR_Handler , 0, "GPCI", NULL ) )
		{
			CI_DTV_SOC_Message(CI_ERR,"Request IRQ failed!\n");
			return NOT_OK;
		}
	}
	
	return OK;
}


/**
 *	free Irq
*/
SINT32 CI_UnInitialize( void )
{
	if( lx_chip_rev() >= LX_CHIP_REV(H14, A0) )
		free_irq( H14_IRQ_DVBCI, NULL );
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, B0) )
		free_irq( M14_B0_IRQ_DVBCI, NULL );
	else if( lx_chip_rev() >= LX_CHIP_REV(M14, A0) )
		free_irq( M14_A0_IRQ_DVBCI, NULL );
	else if( lx_chip_rev() >= LX_CHIP_REV(H13, A0) )
		free_irq( H13_IRQ_DVBCI, NULL );

	return OK;
}


/**
 *	Reset CAM module
*/
SINT32 CI_ResetCI( void )
{
	guwIsPowerRestart = 1;

	return OK;
}


/**
 *	CI CAM initialize (Manual HotSwap Sequence)
*/
SINT32 CI_CAMInit( void )  	
{
	UINT16 unRegVal = 0x0;

	/* Power Off */
	gpstCIReg->uniCntrlStat1.unCntrlStat1 = 0x0;		// power off

	/* jinhwan.bae 20131018
	    change constant 300 to gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET] , default 300 */
#ifdef _CI_KDRV_DELAY_USLEEP_RANGE_
	usleep_range(gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET]*1000, gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET]*1000);
#else
	OS_MsecSleep( gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET] ); //delay 300ms
#endif

	/* Power On */
	unRegVal = gpstCIReg->uniCntrlStat1.unCntrlStat1;

	unRegVal |= VCC_SW0;
	gpstCIReg->uniCntrlStat1.unCntrlStat1 = unRegVal;
	
	/* Enable Card */
#ifdef _CI_KDRV_DELAY_USLEEP_RANGE_
	usleep_range(gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET]*1000, gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET]*1000);
#else
	OS_MsecSleep( gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET] );
#endif

	unRegVal = gpstCIReg->uniCntrlStat1.unCntrlStat1;
	unRegVal |= (EN_PCCARD | CARD_RESET);
	gpstCIReg->uniCntrlStat1.unCntrlStat1 = unRegVal;

#ifdef _CI_KDRV_DELAY_USLEEP_RANGE_
	usleep_range(gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_CARDRESET_NOTRESET]*1000, gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_CARDRESET_NOTRESET]*1000);
#else
	OS_MsecSleep( gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_CARDRESET_NOTRESET] );
#endif	
	unRegVal &= ~CARD_RESET;
	gpstCIReg->uniCntrlStat1.unCntrlStat1 = unRegVal;

	return OK;
}


/**
 *	CAM power off
*/
SINT32 CI_CAMPowerOff( void )
{
	guwIsPowerRestart = 1;

	gpstCIReg->uniCntrlStat1.unCntrlStat1 = 0x0;

	return OK;
}


/**
 *	CAM power on
*/
SINT32 CI_CAMPowerOnCompleted( void )
{
	guwIsPowerRestart = 0;			

	return OK;
}


/**
 *	Check CIS(Card Information Structure)
*/
SINT32 CI_CheckCIS( void )
{
	if( guwDetectCard )
	{
		/* Set POD mode & PCMCIA attribute memory R/W */
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = POD_MODE;		
		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_ATTR;

		if( CIS_OK == ( CIS_Config( MOD_A ) ) )
		{
			CI_DTV_SOC_Message(CI_INFO,"Check CIS <<S>>\n");
			return OK;
		}
		else
		{
			CI_DTV_SOC_Message(CI_ERR,"Check CIS <<F>>\n");
			return NOT_OK;
		}
	}

	CI_DTV_SOC_Message(CI_ERR,"CAM not inserted <<F>>\n");

	return NOT_OK;
}



/**
 *	Write COR(Configuration Option Register)
*/
SINT32 CI_WriteCOR ( void )
{
	SINT32 wRetVal = NOT_OK;

	if( guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = INIT_VAL;
		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_ATTR;

	
		if ( CIS_OK == ( CIS_WriteCOR( MOD_A ) ) )
		{
			wRetVal = OK;
			CI_DTV_SOC_Message(CI_INFO,"Wirte COR <<S>>\n");
		}
		else
		{
			CI_DTV_SOC_Message(CI_ERR,"Wirte COR <<F>>\n");
		}
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"[ CI_WriteCOR: CAM not inserted <<F>> ]\n");
	}

	return wRetVal;
}


/**
 *	Check whether CI module is detect or remove
*/
SINT32 CI_CAMDetectStatus( UINT32 *o_puwIsCardDetected )
{
	SINT32 wRetVal = NOT_OK;

	if( o_puwIsCardDetected )
	{
		if( guwDetectCard )
		{
			CI_DTV_SOC_Message(CI_DBG_INFO,"CAM Module is inserted ... \n");

			*o_puwIsCardDetected = 1;
		}
		else
		{
//			CI_DTV_SOC_Message(CI_DBG_INFO,"CAM Module is removed ... \n");

			*o_puwIsCardDetected = 0;
		}

		wRetVal = OK;
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_CAMDetectStatus: Invalidate argument <<F>>\n");
	}

	return wRetVal;
}


/**
 *	Read data from CI module
*/
SINT32 CI_ReadData( UINT8 *o_pbyData, UINT16 *io_pwDataBufSize )
{
	SINT32 wRetVal = NOT_OK;

	if( o_pbyData && io_pwDataBufSize && guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );

		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;


		if( HW_IO_OK == ( HW_IO_Read( MOD_A, o_pbyData, io_pwDataBufSize ) ) )
		{
			wRetVal = OK;

			CI_DTV_SOC_Message(CI_INFO," Read IO Data <<S>>\n");
		}
		else
		{
			CI_DTV_SOC_Message(CI_ERR," Read IO Data <<F>>\n");
		}

	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_ReadData: CAM not inserted <<F>>\n");
	}

	return wRetVal;
}


/**
 * Negotiate the buffer size between host and CI Module.
*/
SINT32 CI_NegoBuff( UINT32 *o_puwBufSize )
{

	if( o_puwBufSize && guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );
		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;


		if( HW_IO_OK == ( HW_IO_NegoBuf( MOD_A, o_puwBufSize ) ) )
		{
			CI_DTV_SOC_Message(CI_INFO,"Calc Negotiation Buffer <<S>>\n");
			return OK;			
		}
		else
		{
			*o_puwBufSize = 0;

			CI_DTV_SOC_Message(CI_ERR,"Calc Negotiation Buffer <<F>>\n");
		}

	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_NegoBuff failed\n");
	}

	return NOT_OK;
}



/**
 * Read DA (Data Available register) status
 */
SINT32 CI_ReadDAStatus( UINT32 *o_puwIsDataAvailable )
{
	SINT32 wRetVal = NOT_OK;

	if( o_puwIsDataAvailable && guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );

		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;


		if ( DA_STATUS & CHECK_DA( MOD_A ) )
		{
			*o_puwIsDataAvailable = 1;
		}
		else
		{
			*o_puwIsDataAvailable = 0;
		}

		CI_DTV_SOC_Message(CI_DBG_INFO,"DA register status 0x%x\n", *o_puwIsDataAvailable);

		wRetVal = OK;
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_ReadDAStatus failed\n");
	}

	return wRetVal;
}


/**
 *	Write data to CI module
*/
SINT32 CI_WriteData( UINT8 *i_pbyData, UINT32 i_wDataBufSize )
{
	SINT32 wRetVal = NOT_OK;

	if( i_pbyData && i_wDataBufSize && guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );

		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;

		if ( HW_IO_OK == ( HW_IO_Write( MOD_A, i_pbyData, i_wDataBufSize ) ) )
		{
			wRetVal = OK;

			CI_DTV_SOC_Message(CI_INFO,"Write IO Data <<S>>\n");
		}
		else
		{
			CI_DTV_SOC_Message(CI_ERR,"Write IO Data <<F>>\n");
		}

	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_WriteData failed\n");
	}

	return wRetVal;
}


/**
 *	Physical Reset
*/
SINT32 CI_ResetPhysicalIntrf( void )
{
	SINT32 wRetVal = NOT_OK;

	if( guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );

		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;

		STAT_RD( MOD_A ) = RESET_PHY_INT;

		CI_DTV_SOC_Message(CI_INFO,"Reset Physical Intrf <<S>>\n");

		wRetVal = OK;
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_WriteData: CAM not inserted <<F>>\n");
	}

	return wRetVal;
}


/**
 *	Set RS Bit to soft reset CAM
*/
SINT32 CI_SetRS(void )
{
	SINT32 wRetVal = NOT_OK;

	if( guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );

		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;
		
		if( HW_IO_OK == HW_IO_SetRS( MOD_A ) )
		{
			wRetVal = OK;

			CI_DTV_SOC_Message(CI_INFO," Set RS <<S>>\n");
		}
		else
		{
			CI_DTV_SOC_Message(CI_ERR," Set RS <<F>>\n");
		}
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_SetRS: CAM not inserted <<F>>\n");
	}

	return wRetVal;
}


/**
 *	Read IIR(Initialze Interface Request) status form CI module
*/
SINT32 CI_ReadIIRStatus( UINT32 *o_puwIIRStatus )
{
	SINT32 wRetVal = NOT_OK;

	if( o_puwIIRStatus && guwDetectCard )
	{
		gpstCIReg->uniCntrlStat3.unCntrlStat3 = ( POD_MODE | IO_INT_MODE );

		gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = CHIP_MODE_IO;

		if ( IIR_STATUS & CHECK_IIR( MOD_A ) )
		{
			*o_puwIIRStatus = 1;
		}
		else
		{
			*o_puwIIRStatus = 0;
		}

		CI_DTV_SOC_Message(CI_INFO,"Read IIR Status <<S>>\n");

		wRetVal = OK;
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_ReadIIRStatus: CAM removed\n");
	}

	return wRetVal;
}


/**
 *	Check CAM type
*/
SINT32 CI_CheckCAMType( SINT8 *o_pRtnValue, UINT8 *o_puwCheckCAMType )
{
	SINT32 wRetVal = NOT_OK;

	if( o_puwCheckCAMType && guwDetectCard )
	{
		wRetVal = CIS_WhatCAM( (UINT8) MOD_A, o_puwCheckCAMType );
		*o_pRtnValue = wRetVal;		// if not process CIS function, return -1
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_CheckCAMType failed\n");	
	}


	return wRetVal;
}


/**
 *	Get CI+ Support Version
*/
SINT32 CI_GetCIPlusSupportVersion( SINT8 *o_pRtnValue, UINT32 *o_puwVersion )
{
	SINT32 wRetVal = NOT_OK;

	if( o_puwVersion && guwDetectCard )
	{
		wRetVal = CIS_GetCIPlusCAMSupportVersion( (UINT8) MOD_A, o_puwVersion );
		*o_pRtnValue = wRetVal;		// if not process CIS function, return -1
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_GetCIPlusSupportVersion failed\n");	
	}


	return wRetVal;
}


/**
 *	Get CI+ Operator Profile
*/
SINT32 CI_GetCIPlusOperatorProfile( SINT8 *o_pRtnValue, UINT32 *o_puwProfile)
{
	SINT32 wRetVal = NOT_OK;

	if( o_puwProfile && guwDetectCard )
	{
		wRetVal = CIS_GetCIPlusCAMOperatorProfile( (UINT8) MOD_A, o_puwProfile );
		*o_pRtnValue = wRetVal;		// if not process CIS function, return -1
	}
	else
	{
		CI_DTV_SOC_Message(CI_ERR,"CI_GetCIPlusOperatorProfile failed\n");
	}


	return wRetVal;
}




/**
 *	Print CIModule's register
*/
SINT32 CI_RegPrint( void )
{
	SINT32 wRetVal = NOT_OK;

	if( gpstCIReg )
	{
		CI_DTV_SOC_Message(CI_ERR,"\nControl and Status 1 [0x%x]", gpstCIReg->uniCntrlStat1.unCntrlStat1 );
		CI_DTV_SOC_Message(CI_ERR,"\nControl and Status 2 [0x%x]", gpstCIReg->uniCntrlStat2.unCntrlStat2 );
		CI_DTV_SOC_Message(CI_ERR,"\nControl and Status 3 [0x%x]", gpstCIReg->uniCntrlStat3.unCntrlStat3 );
		CI_DTV_SOC_Message(CI_ERR,"\nPC Card Control 1 [0x%x]", gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 );
		CI_DTV_SOC_Message(CI_ERR,"\nPC Card Control 2 [0x%x]", gpstCIReg->uniPCCardCntrl2.unPCCardCntrl2 );
		CI_DTV_SOC_Message(CI_ERR,"\nPC Card Control 3 [0x%x]", gpstCIReg->uniPCCardCntrl3.unPCCardCntrl3 );
		CI_DTV_SOC_Message(CI_ERR,"\nBurst Control [0x%x]", gpstCIReg->uniBurstCntrl.unBurstCntrl);
		CI_DTV_SOC_Message(CI_ERR,"\nInterrupt Flag [0x%x]", gpstCIReg->uniIntrFlag.unIntrFlag );
		CI_DTV_SOC_Message(CI_ERR,"\nInterrupt Mask [0x%x]", gpstCIReg->uniIntrMask.unIntrMask );
		CI_DTV_SOC_Message(CI_ERR,"\nInterrupt Control [0x%x]", gpstCIReg->uniIntrCntrl.unIntrCntrl );
		CI_DTV_SOC_Message(CI_ERR,"\nPCMCIA Address Index Register [0x%x]", gpstCIReg->unPCMCIAAddrInd );
		CI_DTV_SOC_Message(CI_ERR,"\nPower On Interval 1 [0x%x]", gpstCIReg->uniHWHotSwapCntrl1.unHWHotSwapCntrl1 );
		CI_DTV_SOC_Message(CI_ERR,"\nPower On Interval 2 [0x%x]", gpstCIReg->uniHWHotSwapCntrl2.unHWHotSwapCntrl2 );
		CI_DTV_SOC_Message(CI_ERR,"\nEnable Interval 1 [0x%x]", gpstCIReg->uniHWHotSwapCntrl3.unHWHotSwapCntrl3 );
		CI_DTV_SOC_Message(CI_ERR,"\nEnable Interval 2 [0x%x]", gpstCIReg->uniHWHotSwapCntrl4.unHWHotSwapCntrl4 );
		CI_DTV_SOC_Message(CI_ERR,"\nCard Insert Done Interval 1 [0x%x]", gpstCIReg->uniHWHotSwapCntrl5.unHWHotSwapCntrl5 );
		CI_DTV_SOC_Message(CI_ERR,"\nCard Insert Done Interval 2 [0x%x]", gpstCIReg->uniHWHotSwapCntrl6.unHWHotSwapCntrl6 );
		CI_DTV_SOC_Message(CI_ERR,"\nCard Removal Done Interval 1 [0x%x]", gpstCIReg->uniHWHotSwapCntrl7.unHWHotSwapCntrl7 );
		CI_DTV_SOC_Message(CI_ERR,"\nCard Removal Done Interval 2 [0x%x]\n", gpstCIReg->uniHWHotSwapCntrl8.unHWHotSwapCntrl8 );
		wRetVal = OK;
	}


	return wRetVal;
}


/**
 *	Print CIModule's register
*/
SINT32 CI_RegWrite( UINT32 ui32Offset, UINT32 ui32Value )
{
	SINT32 wRetVal = OK;

	switch(ui32Offset)
	{
		case 0x0 :	{ gpstCIReg->uniCntrlStat1.unCntrlStat1 = ui32Value;	break; }
		case 0x2 :  { gpstCIReg->uniCntrlStat2.unCntrlStat2 = ui32Value;	break; }
		case 0x4 :	{ gpstCIReg->uniCntrlStat3.unCntrlStat3 = ui32Value;	break; }
		case 0x6 :	{ gpstCIReg->uniPCCardCntrl1.unPCCardCntrl1 = ui32Value;	break; }
		case 0x8 :	{ gpstCIReg->uniPCCardCntrl2.unPCCardCntrl2  = ui32Value;	break; }
		case 0xa :	{ gpstCIReg->uniPCCardCntrl3.unPCCardCntrl3 = ui32Value;	break; }
		case 0x10 :	{ gpstCIReg->uniBurstCntrl.unBurstCntrl = ui32Value;	break; }
		case 0x26 :	{ gpstCIReg->uniIntrFlag.unIntrFlag = ui32Value;	break; }
		case 0x28 :	{ gpstCIReg->uniIntrMask.unIntrMask = ui32Value;	break; }
		case 0x2a :	{ gpstCIReg->uniIntrCntrl.unIntrCntrl = ui32Value;	break; }
		case 0x2c : { gpstCIReg->unPCMCIAAddrInd = ui32Value;	break; }
		case 0x2e :	{ gpstCIReg->uniHWHotSwapCntrl1.unHWHotSwapCntrl1 = ui32Value;	break; }
		case 0x30 :	{ gpstCIReg->uniHWHotSwapCntrl2.unHWHotSwapCntrl2 = ui32Value;	break; }
		case 0x32 :	{ gpstCIReg->uniHWHotSwapCntrl3.unHWHotSwapCntrl3 = ui32Value;	break; }
		case 0x34 :	{ gpstCIReg->uniHWHotSwapCntrl4.unHWHotSwapCntrl4 = ui32Value;	break; }
		case 0x36 :	{ gpstCIReg->uniHWHotSwapCntrl5.unHWHotSwapCntrl5 = ui32Value;	break; }
		case 0x38 :	{ gpstCIReg->uniHWHotSwapCntrl6.unHWHotSwapCntrl6 = ui32Value;	break; }
		case 0x3a :	{ gpstCIReg->uniHWHotSwapCntrl7.unHWHotSwapCntrl7 = ui32Value;	break; }
		case 0x3c :	{ gpstCIReg->uniHWHotSwapCntrl8.unHWHotSwapCntrl8 = ui32Value;	break; }
		default	:	{ CI_DTV_SOC_Message(CI_ERR,"NO VALID ADDRESS in CI REG (0x%x)\n",ui32Offset);		wRetVal = NOT_OK; 	break;	}
		
	}

	return wRetVal;
}



/**
 *	SET CI Module's Delay Value
*/
SINT32 CI_CAMSetDelay( CI_DELAY_TYPE_T eDelayType, UINT32 uiDelayValue )
{
	SINT32 wRetVal = NOT_OK;
	UINT32 i = 0;

	if( eDelayType == CI_D_ENUM_MAX )
	{
		/* All Delay Value * 10 ms */
		CI_DTV_SOC_Message(CI_INFO,"\nAll Delay Values Are Increased to ORG x uiDelayValue(%d) ms \n", uiDelayValue);

		for(i=0;i<CI_D_ENUM_MAX;i++)
		{
			gCI_KDRV_Delay[i] = gCI_KDRV_Delay[i] * uiDelayValue;
		}
	}
	else
	{
		CI_DTV_SOC_Message(CI_INFO,"\nSet the eDelayType[%d] from [%d]ms to [%d]ms\n",eDelayType,gCI_KDRV_Delay[eDelayType],uiDelayValue);
		gCI_KDRV_Delay[eDelayType] = uiDelayValue;
	}

	wRetVal = OK;	/* All Works Done */

	return wRetVal;
}



/**
 *	Print Out CI Module's Delay Value
*/
SINT32 CI_CAMPrintDelayValues( void )
{
	SINT32 wRetVal = NOT_OK;

	CI_DTV_SOC_Message(CI_ERR,"CI_D_CIS_CONFIG_FIRST_TUPLE_OK 			[%4d][%4d]\n", 	
		gDefault_CI_Delay[CI_D_CIS_CONFIG_FIRST_TUPLE_OK], 			gCI_KDRV_Delay[CI_D_CIS_CONFIG_FIRST_TUPLE_OK]);
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CIS_CONFIG_FIRST_TUPLE_NG			[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_CIS_CONFIG_FIRST_TUPLE_NG], 			gCI_KDRV_Delay[CI_D_CIS_CONFIG_FIRST_TUPLE_NG]);
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CIS_END_WRITE_COR					[%4d][%4d]\n",			
		gDefault_CI_Delay[CI_D_CIS_END_WRITE_COR], 					gCI_KDRV_Delay[CI_D_CIS_END_WRITE_COR]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CIS_DURING_READ_TUPLE				[%4d][%4d]\n",		
		gDefault_CI_Delay[CI_D_CIS_DURING_READ_TUPLE], 				gCI_KDRV_Delay[CI_D_CIS_DURING_READ_TUPLE]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CIS_END_READ_TUPLE_INITIAL			[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_CIS_END_READ_TUPLE_INITIAL], 		gCI_KDRV_Delay[CI_D_CIS_END_READ_TUPLE_INITIAL]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CIS_PARSE_NON_CI_TUPLE				[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_CIS_PARSE_NON_CI_TUPLE], 			gCI_KDRV_Delay[CI_D_CIS_PARSE_NON_CI_TUPLE]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_INIT_POWER_CONTROL					[%4d][%4d]\n",		
		gDefault_CI_Delay[CI_D_INIT_POWER_CONTROL], 				gCI_KDRV_Delay[CI_D_INIT_POWER_CONTROL]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_INIT_AFTER_INTERRUPT_ENABLE		[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE], 		gCI_KDRV_Delay[CI_D_INIT_AFTER_INTERRUPT_ENABLE]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CAM_INIT_BTW_VCC_CARDRESET			[%4d][%4d]\n",	
		gDefault_CI_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET], 		gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_VCC_CARDRESET]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_CAM_INIT_BTW_CARDRESET_NOTRESET	[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_CAM_INIT_BTW_CARDRESET_NOTRESET], 	gCI_KDRV_Delay[CI_D_CAM_INIT_BTW_CARDRESET_NOTRESET]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_SOFT_RESET_CHECK_FR				[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_SOFT_RESET_CHECK_FR], 			gCI_KDRV_Delay[CI_D_IO_SOFT_RESET_CHECK_FR]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_END_SOFT_RESET					[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_END_SOFT_RESET], 					gCI_KDRV_Delay[CI_D_IO_END_SOFT_RESET]);
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_NEGOBUF_BEFORE_SOFTRESET		[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_NEGOBUF_BEFORE_SOFTRESET], 		gCI_KDRV_Delay[CI_D_IO_NEGOBUF_BEFORE_SOFTRESET]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_NEGOBUF_CHECK_DA				[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_NEGOBUF_CHECK_DA], 				gCI_KDRV_Delay[CI_D_IO_NEGOBUF_CHECK_DA]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_NEGOBUF_CHECK_FR				[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_NEGOBUF_CHECK_FR], 				gCI_KDRV_Delay[CI_D_IO_NEGOBUF_CHECK_FR]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_NEGOBUF_AFTER_WRITE_DATA		[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_NEGOBUF_AFTER_WRITE_DATA], 		gCI_KDRV_Delay[CI_D_IO_NEGOBUF_AFTER_WRITE_DATA]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_READ_CHECK_DA					[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_READ_CHECK_DA], 					gCI_KDRV_Delay[CI_D_IO_READ_CHECK_DA]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_WRITE_CHECK_DA					[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_WRITE_CHECK_DA], 					gCI_KDRV_Delay[CI_D_IO_WRITE_CHECK_DA]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_WRITE_CHECK_FR					[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_WRITE_CHECK_FR], 					gCI_KDRV_Delay[CI_D_IO_WRITE_CHECK_FR]);	
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_WRITE_FIRST_BYTE_STAT_RD_FR_WE	[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_WRITE_FIRST_BYTE_STAT_RD_FR_WE], 	gCI_KDRV_Delay[CI_D_IO_WRITE_FIRST_BYTE_STAT_RD_FR_WE]);
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_WRITE_MIDDLE_BYTE_CHECK_WE		[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_WRITE_MIDDLE_BYTE_CHECK_WE], 		gCI_KDRV_Delay[CI_D_IO_WRITE_MIDDLE_BYTE_CHECK_WE]);
	CI_DTV_SOC_Message(CI_ERR,"CI_D_IO_WRITE_LAST_BYTE_CHECK_WE		[%4d][%4d]\n",
		gDefault_CI_Delay[CI_D_IO_WRITE_LAST_BYTE_CHECK_WE], 		gCI_KDRV_Delay[CI_D_IO_WRITE_LAST_BYTE_CHECK_WE]);	

	wRetVal = OK;	/* All Works Done */

	return wRetVal;
}



/**
 *	Set PCMCIA PC Card Bus Timing Fast or Slow
*/
SINT32 CI_SetPCCardBusTiming( CI_BUS_SPEED_T speed )
{
	SINT32 wRetVal = NOT_OK;

	CI_DTV_SOC_Message(CI_INFO,"SET PCMCIA PC Card Bus Timing for CI (%x) \n", (UINT32)speed);
	
	switch( speed )
	{
		case PCMCIA_BUS_SPEED_MIN :
		case PCMCIA_BUS_SPEED_LOW :
		{
			gpstCIReg->uniPCCardCntrl2.unPCCardCntrl2 = 0x0f03;
			gpstCIReg->uniPCCardCntrl3.unPCCardCntrl3 = 0x0003;

			break;
		}
		case PCMCIA_BUS_SPEED_HIGH :
		case PCMCIA_BUS_SPEED_MAX :
		{
			gpstCIReg->uniPCCardCntrl2.unPCCardCntrl2 = 0x0201;  //change the register from 0x101 to 0x201 (20130218, ohsung.roh)
			gpstCIReg->uniPCCardCntrl3.unPCCardCntrl3 = 0x0001;

			break;
		}
		default : break;
	}

	wRetVal = OK;	/* All Works Done */

	return wRetVal;
}

SINT32 CI_ChangeAccessMode ( CI_ACCESS_MODE_T mode )
{
	SINT32 wRetVal = NOT_OK;

	CI_DTV_SOC_Message(CI_INFO,"SET CI_ChangeAccessMode for CI (%x) \n", (UINT32)mode);
	
	switch( mode )
	{
		case ACCESS_1BYTE_MODE :
		{
			gpstCIReg->uniBurstCntrl.unBurstCntrl= 0x00;

			break;
		}
		case ACCESS_2BYTE_MODE :
		{
			gpstCIReg->uniBurstCntrl.unBurstCntrl= 0x01;

			break;
		}
		case ACCESS_4BYTE_MODE :
		{
			gpstCIReg->uniBurstCntrl.unBurstCntrl= 0x03;

			break;
		}
		default : 
			break;
	}

	wRetVal = OK;	/* All Works Done */

	return wRetVal;
}

