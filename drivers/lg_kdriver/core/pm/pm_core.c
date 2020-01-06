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

/** @file
 *
 *  main driver implementation for pm device.
 *
 *  author		ingyu.yang (ingyu.yang@lge.com)
 *  version		1.0
 *  date		2009.12.30
 *  note		Additional information.
 *
 *  @addtogroup lg115x_pm
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "os_util.h"
#include "pm_drv.h"
//#include "pm_reg.h"
#include "sys_regs.h"
#include "sys_io.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define PM_LOCK_INIT(dev)		OS_InitMutex(&dev->lock, OS_SEM_ATTR_DEFAULT)
#define PM_LOCK(dev)			OS_LockMutex(&dev->lock)
#define PM_UNLOCK(dev)		OS_UnlockMutex(&dev->lock)

#define PM_EX_LOCK_INIT(dev)	OS_InitMutex(&dev->ex_lock, OS_SEM_ATTR_DEFAULT)
#define PM_EX_LOCK(dev)		OS_LockMutex(&dev->ex_lock)
#define PM_EX_UNLOCK(dev)		OS_UnlockMutex(&dev->ex_lock)


#if 0
#define PM_CORE_DEBUG(format, args...)	PM_ERROR(format, ##args)
#else
#define PM_CORE_DEBUG(format, args...)	do{}while(0)
#endif

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/
typedef struct
{
	UINT8	direction;
	UINT8	data;
} PM_PM_DATA_T;


typedef struct PM_DEV
{
	OS_SEM_T	lock;


#ifdef KDRV_CONFIG_PM
	int			(*Resume)	(void);
	int			(*Suspend)	(void);
	PM_PM_DATA_T*	pmdata;
#endif
} PM_DEV_T;


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

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

static PM_DEV_T *_pPmDev;
 UINT32*	_pmBaseAddr;
 UINT32*	_pmBaseAddr2;
 UINT32*	_pmBaseAddr3;
/*************************************************************************
* Set Pinmux
*************************************************************************/




extern unsigned char cm3_bin_start[];
//extern void cm3_bin_start(void);
  int load_cm3_firmware(void)
 {

#if 0
        // unsigned char temp[65536];
         int ret = 0;
		unsigned char * pcm3_bin_start = (unsigned char *)cm3_bin_start;
 //      return 0;

 //      ret = storage_read(0x20000, 0x10000, (void *)0x2000000);
 //      if(ret < 0) goto fail;

		//printk("cm3_bin_start[%x][%x][%x][%x]\n",cm3_bin_start[0],cm3_bin_start[1],cm3_bin_start[2],cm3_bin_start[3]);
		printk("cm3_bin_start[%x][%x][%x][%x]\n",pcm3_bin_start[0],pcm3_bin_start[1],pcm3_bin_start[2],pcm3_bin_start[3]);
         //printk("pms_firmware size = %u\n",sizeof(cm3_bin_start));
         //memcpy((void *)0xf7010000, cm3_bin_start, sizeof(cm3_bin_start));
         printk("pms_firmware size = %u\n",50744);


		_pmBaseAddr = (UINT32)ioremap(0xf7010000, 50744);
		_pmBaseAddr2 = (UINT32)ioremap(0xF709A19C, 4);
		_pmBaseAddr3 = (UINT32)ioremap(0xF709A190, 4);

         //memcpy((void *)_pmBaseAddr[0], cm3_bin_start, 50744);
         memcpy((void *)_pmBaseAddr, pcm3_bin_start, 50744);
         printk("Load CM3 DVFS&HOT PLUG CONTROL Firmware\n");

 //       a2m_boot_CA15_nM3
 //      ;D.S ASD:0xF709A19C %LE %LONG 1
         *(unsigned int*)_pmBaseAddr2 = 1;

 //       a2m_DDR_initialized
 //      ;D.S ASD:0xF709A190 %LONG 0X1
         *(unsigned int*)_pmBaseAddr3 = 1;
#else
        // unsigned char temp[65536];
         int ret = 0;
		unsigned char * pcm3_bin_start = (unsigned char *)cm3_bin_start;
       return 0;

 //      ret = storage_read(0x20000, 0x10000, (void *)0x2000000);
 //      if(ret < 0) goto fail;

#if 0 // fix statistic analysis
		printk("cm3_bin_start[%x][%x][%x][%x]\n",cm3_bin_start[0],cm3_bin_start[1],cm3_bin_start[2],cm3_bin_start[3]);
		//printk("cm3_bin_start[%x][%x][%x][%x]\n",pcm3_bin_start[0],pcm3_bin_start[1],pcm3_bin_start[2],pcm3_bin_start[3]);
         //printk("pms_firmware size = %u\n",sizeof(cm3_bin_start));
         //memcpy((void *)0xf7010000, cm3_bin_start, sizeof(cm3_bin_start));
         printk("ppms_firmware size = %u\n",50744);


		_pmBaseAddr = (UINT32)ioremap(0xf7010000, 50744);
		_pmBaseAddr2 = (UINT32)ioremap(0xF709A19C, 4);
		_pmBaseAddr3 = (UINT32)ioremap(0xF709A190, 4);

         memcpy((void *)_pmBaseAddr, cm3_bin_start, 50744);
         //memcpy((void *)_pmBaseAddr, pcm3_bin_start, 50744);
         printk("Load CM3 DVFS&HOT PLUG CONTROL Firmware\n");

 //       a2m_boot_CA15_nM3
 //      ;D.S ASD:0xF709A19C %LE %LONG 1
         *(unsigned int*)_pmBaseAddr2 = 1;

 //       a2m_DDR_initialized
 //      ;D.S ASD:0xF709A190 %LONG 0X1
         *(unsigned int*)_pmBaseAddr3 = 1;
#endif
#endif
         return 0;

// fail:
//         printf("Load CM3 DVFS&HOT PLUG CONTROL Firmware = FAIL\n");

//         return -1;

 }

int PM_DevInit(void)
{
	#if 0
	UINT32 i, num_blocks;
	UINT32 phys_base, addr_gap;

	_pPmDev = (PM_DEV_T*)OS_Malloc(sizeof(PM_DEV_T));
	memset(_pPmDev, 0, sizeof(PM_DEV_T));


	PM_LOCK_INIT(_pPmDev);


	_pmBaseAddr = (UINT32*)OS_Malloc(num_blocks * sizeof(UINT32));
	for(i=0; i<num_blocks; i++)
	{
		_pmBaseAddr[i] = (UINT32)ioremap(phys_base + i*addr_gap , 0x10000);
		PM_PRINT("_pmBaseAddr[%d]=[%8x] \n",i,_pmBaseAddr[i]);
	}
	#endif

	load_cm3_firmware();


	return 0;
}

int PM_DevResume(void)
{
	//load cm3 firmware again
	return load_cm3_firmware();

}
int PM_DevSuspend(void)
{
	return 0;
}



/** @} */
