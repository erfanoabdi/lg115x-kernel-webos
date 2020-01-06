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
 *	main driver implementation for de device.
 *	de device will teach you how to make device driver with new platform.
 *
 *	author		dongho7.park (dongho7.park@lge.com)
 *	version		1.0
 *	date		2009.12.30
 *	note		Additional information.
 *
 *	@addtogroup lg1150_de
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/
#undef	SUPPORT_DE_DEVICE_READ_WRITE_FOPS
#define SUPPORT_DE_RESOURCE_MANAGER

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include "de_qkb.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#ifdef KDRV_CONFIG_PM // added by SC Jung for quick booting
#include <linux/platform_device.h>
#endif

#include "os_util.h"
#include "base_device.h"
#include "de_drv.h"

#include "de_model.h"
#include "de_ver_def.h"

#include "de_def.h"
#include "mcu/de_ipc_def.h"
#include "mcu/de_ipc.h"
#include "mcu/de_int.h"
#include "de_io.h"
#include "de_hal.h"

#include "resource_mgr.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/**
 *	main control block for de device.
 *	each minor device has unique control block
 *
 */
typedef struct
{
// BEGIN of common device
	int						dev_open_count; ///< check if device is opened or not
	dev_t					devno;			///< device number
	struct cdev				cdev;			///< char device structure
// END of command device

// BEGIN of device specific data
	int						dev_de_initialized;   ///< check if device is initialized or not
	int						dev_de_fw_downloaded; ///< check if de firmware is initialized or not

// END of device specific data
}
DE_DEVICE_T;

#ifdef KDRV_CONFIG_PM
typedef struct
{
	// add here extra parameter
	bool is_suspended;
}DE_DRVDATA_T;

#endif
/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
extern	void	DE_PROC_Init(void);
extern	void	DE_PROC_Cleanup(void);
extern	void	DE_FW_Download(int fromFile);

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
int		DE_Init(void);
void	DE_Cleanup(void);

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
int		g_de_debug_fd;
int		g_de_major = DE_MAJOR;
int		g_de_minor = DE_MINOR;
int		g_de_suspended = FALSE;
int		g_de_device_opened = 0;

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static int		DE_Open(struct inode *, struct file *);
static int		DE_Close(struct inode *, struct file *);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int		DE_Ioctl (struct inode *, struct file *, unsigned int, unsigned long );
#else
static long     DE_Ioctl (struct file * filp, unsigned int cmd, unsigned long arg);
#endif
#ifdef SUPPORT_DE_DEVICE_READ_WRITE_FOPS
static ssize_t	DE_Read(struct file *, char *, size_t, loff_t *);
static ssize_t	DE_Write(struct file *, const char *, size_t, loff_t *);
#endif
static int      DE_Mmap(struct file *file, struct vm_area_struct *vma);
static int		DE_Thread    (void *data);

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

/**
 * main control block for de device
*/
static DE_DEVICE_T*		g_de_device;

/**
 * file I/O description for de device
 *
*/
static struct file_operations g_de_fops =
{
	.open	= DE_Open,
	.release= DE_Close,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	.ioctl	= DE_Ioctl,
#else
	.unlocked_ioctl = DE_Ioctl,
#endif
	.mmap	= DE_Mmap,
#ifdef SUPPORT_DE_DEVICE_READ_WRITE_FOPS
	.read	= DE_Read,
	.write	= DE_Write,
#else
	.read	= NULL,
	.write	= NULL,
#endif
};

static struct task_struct *stDE_Thread;

/*========================================================================================
	Implementation Group
========================================================================================*/
#ifdef	KDRV_CONFIG_PM	// added by SC Jung for quick booting
/**
 *
 * suspending module.
 *
 * @param	struct platform_device *pdev pm_message_t state
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static int DE_suspend(struct platform_device *pdev, pm_message_t state)
{
	DE_DRVDATA_T *drv_data;

	drv_data = platform_get_drvdata(pdev);

	// add here the suspend code
	DE_HAL_ResetDE(TRUE);
#ifdef SUPPORT_DE_RESOURCE_MANAGER	
	if(resource_mgr_suspend() != RET_OK)
	{
		DE_ERROR("fail to suspend.\n");
	}
#endif

	drv_data->is_suspended = 1;
	g_de_suspended = 1;
	DE_PRINT("[%s] done suspend\n", DE_MODULE);

	return 0;
}


/**
 *
 * resuming module.
 *
 * @param	struct platform_device *
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static int DE_resume(struct platform_device *pdev)
{
	DE_DRVDATA_T *drv_data;

	drv_data = platform_get_drvdata(pdev);

	if(drv_data->is_suspended == 0) return -1;

	// add here the resume code
#ifdef USE_DE_SUSPEND
#ifdef SUPPORT_DE_RESOURCE_MANAGER	
	if(g_de_device_opened)
	{
		DE_HAL_ResetDE(FALSE);
		DE_FW_Download(FALSE);

		if(resource_mgr_resume() != RET_OK)
		{
			DE_ERROR("fail to resume.\n");
		}
	
		DE_HAL_GPIO_Init();
		DE_HAL_LVDS_Init();
	}
#endif
#endif

	drv_data->is_suspended = 0;
	g_de_suspended = 0;
	DE_PRINT("[%s] done resume\n", DE_MODULE);

	return RET_OK;
}
/**
 *
 * probing module.
 *
 * @param	struct platform_device *pdev
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
int DE_probe(struct platform_device *pdev)
{
	DE_DRVDATA_T *drv_data;

	drv_data = (DE_DRVDATA_T *)kmalloc(sizeof(DE_DRVDATA_T) , GFP_KERNEL);


	// add here driver registering code & allocating resource code
#ifdef USE_DE_SUSPEND
#endif

	DE_PRINT("[%s] done probe\n", DE_MODULE);
	drv_data->is_suspended = 0;
	g_de_suspended = 0;
	platform_set_drvdata(pdev, drv_data);

	return RET_OK;
}


/**
 *
 * module remove function. this function will be called in rmmod de module.
 *
 * @param	struct platform_device
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static int	DE_remove(struct platform_device *pdev)
{
	DE_DRVDATA_T *drv_data;

	// add here driver unregistering code & deallocating resource code
#ifdef SUPPORT_DE_RESOURCE_MANAGER	
	resource_mgr_cleanup();
#endif

#ifdef USE_DE_SUSPEND
#endif

	drv_data = platform_get_drvdata(pdev);
	kfree(drv_data);

	DE_PRINT("removed\n");

	return 0;
}

/**
 *
 * module release function. this function will be called in rmmod de module.
 *
 * @param	struct device *dev
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static void DE_release(struct device *dev)
{
	DE_PRINT("device released\n");
}

/*
 *	module platform driver structure
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
static struct platform_driver de_driver =
#else
static struct platform_driver de_driver __refdata =
#endif
{
	.probe			= DE_probe,
	.suspend		= DE_suspend,
	.remove			= DE_remove,
	.resume			= DE_resume,
	.driver			=
	{
		.name	= DE_MODULE,
	},
};

static struct platform_device de_device = {
	.name = DE_MODULE,
	.id = 0,
	.dev =
	{
		.release = DE_release,
	},
};
#endif

/** Initialize the device environment before the real H/W initialization
 *
 *  @note main usage of this function is to initialize the HAL layer and memory size adjustment
 *  @note it's natural to keep this function blank :)
 */
void DE_PreInit(void)
{
    /* TODO: do something */
}

int DE_Init(void)
{
	int			i;
	int			err;
	dev_t		dev;

	/* Get the handle of debug output for de device.
	 *
	 * Most module should open debug handle before the real initialization of module.
	 * As you know, debug_util offers 4 independent debug outputs for your device driver.
	 * So if you want to use all the debug outputs, you should initialize each debug output
	 * using OS_DEBUG_EnableModuleByIndex() function.
	 */
	g_de_debug_fd = DBG_OPEN( DE_MODULE );
	if (g_de_debug_fd < 0) {
		DE_ERROR("can't open de module\n" );
		return -EIO;
	}
	OS_DEBUG_EnableModule ( g_de_debug_fd );

	OS_DEBUG_EnableModuleByIndex ( g_de_debug_fd, LX_LOGM_LEVEL_ERROR, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_de_debug_fd, LX_LOGM_LEVEL_WARNING, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_de_debug_fd, LX_LOGM_LEVEL_INFO, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_de_debug_fd, LX_LOGM_LEVEL_NOTI, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_de_debug_fd, LX_LOGM_LEVEL_TRACE, DBG_COLOR_NONE );
	OS_DEBUG_DisableModuleByIndex( g_de_debug_fd, LX_LOGM_LEVEL_TRACE);

	/* allocate main device handler, register current device.
	 *
	 * If devie major is predefined then register device using that number.
	 * otherwise, major number of device is automatically assigned by Linux kernel.
	 *
	 */
#ifdef	KDRV_CONFIG_PM
	// added by SC Jung for quick booting
	if(platform_driver_register(&de_driver) < 0)
	{
		DE_ERROR("[%s] platform driver register failed\n",DE_MODULE);

	}
	else
	{
		if(platform_device_register(&de_device))
		{
			platform_driver_unregister(&de_driver);
			DE_ERROR("[%s] platform device register failed\n",DE_MODULE);
		}
		else
		{
			DE_NOTI("[%s] platform register done\n", DE_MODULE);
		}


	}
#endif
	g_de_device = (DE_DEVICE_T*)OS_KMalloc( sizeof(DE_DEVICE_T)*DE_MAX_DEVICE );

	if ( NULL == g_de_device )
	{
		DE_ERROR("out of memory. can't allocate %d bytes\n", sizeof(DE_DEVICE_T)* DE_MAX_DEVICE );
		return -ENOMEM;
	}

	memset( g_de_device, 0x0, sizeof(DE_DEVICE_T)* DE_MAX_DEVICE );

	if (g_de_major)
	{
		dev = MKDEV( g_de_major, g_de_minor );
		err = register_chrdev_region(dev, DE_MAX_DEVICE, DE_MODULE );
	}
	else
	{
		err = alloc_chrdev_region(&dev, g_de_minor, DE_MAX_DEVICE, DE_MODULE );
		g_de_major = MAJOR(dev);
	}

	if ( err < 0 )
	{
		DE_ERROR("can't register de device\n" );
		return -EIO;
	}

	/* TODO : initialize your module not specific minor device */
#ifdef SUPPORT_DE_RESOURCE_MANAGER	
	resource_mgr_init();
#endif
	
	DE_IPC_Init();
	DE_HAL_InitPHY2VIRT();
	DE_HAL_GPIO_Init();
	DE_HAL_HDMI_Init();
	DE_HAL_LVDS_Init();
	DE_HAL_MISC_Init();
	DE_HAL_OSD_Init();
	DE_HAL_InitRegister();

	/* END */

	for ( i=0; i<DE_MAX_DEVICE; i++ )
	{
		/* initialize cdev structure with predefined variable */
		dev = MKDEV( g_de_major, g_de_minor+i );
		cdev_init( &(g_de_device[i].cdev), &g_de_fops );
		g_de_device[i].devno		= dev;
		g_de_device[i].cdev.owner = THIS_MODULE;
		g_de_device[i].cdev.ops   = &g_de_fops;

		/* TODO: initialize minor device */


		/* END */

		err = cdev_add (&(g_de_device[i].cdev), dev, 1 );

		if (err)
		{
			DE_ERROR("error (%d) while adding de device (%d.%d)\n", err, MAJOR(dev), MINOR(dev) );
			return -EIO;
		}
#ifndef USE_VIDEO_FOR_FPGA
		OS_CreateDeviceClass ( g_de_device[i].devno, "%s%d", DE_MODULE, i );
#endif
	}

	/* initialize proc system */
	DE_PROC_Init ( );

	stDE_Thread = kthread_create(DE_Thread, (void*)NULL, "de_thread");
	if(stDE_Thread)
	{
		wake_up_process(stDE_Thread);
	}
	else
	{
		DE_PRINT("DE Thread alreadu created\n");
	}

	DE_NOTI("de device initialized\n");

	return 0;
}

void DE_Cleanup(void)
{
	int i;
	dev_t dev = MKDEV( g_de_major, g_de_minor );

#ifdef	KDRV_CONFIG_PM
	// added by SC Jung for quick booting
	platform_driver_unregister(&de_driver);
	platform_device_unregister(&de_device);
#endif
	/* cleanup proc system */
	DE_PROC_Cleanup( );

	/* remove all minor devicies and unregister current device */
	for ( i=0; i<DE_MAX_DEVICE;i++)
	{
		/* TODO: cleanup each minor device */


		/* END */
		cdev_del( &(g_de_device[i].cdev) );
	}

	/* TODO : cleanup your module not specific minor device */
	DE_IPC_Free();
	DE_HAL_FreePHY2VIRT();

	if(stDE_Thread != NULL)
		kthread_stop(stDE_Thread);

	unregister_chrdev_region(dev, DE_MAX_DEVICE );

	OS_Free( g_de_device );

	DE_PRINT("de device cleanup\n");
}

/**
 * DE Thread 
 *
 * @param   
 * @return  
 * @see
 * @author
 */
static int DE_Thread(void *data)
{
	static int ret = RET_OK;

	while(1)
	{
		resource_mgr_thread();
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * open handler for de device
 *
 */
static int
DE_Open(struct inode *inode, struct file *filp)
{
	int					major,minor;
	struct cdev*		cdev;
	DE_DEVICE_T*	my_dev;

	cdev	= inode->i_cdev;
	my_dev	= container_of ( cdev, DE_DEVICE_T, cdev);

	major = imajor(inode);
	minor = iminor(inode);

	if(minor >= DE_MAX_DEVICE)
	{
		DE_ERROR( "Invalid device num[%d] for Open!!\n", minor);
		return -1;
	}

	/*	TODO : add your device specific code */
	/*	support multi-process system */
	if (g_de_device_opened == 0 )
	{
		my_dev->dev_de_initialized = 0;
		my_dev->dev_de_fw_downloaded = 0;
	}

	/* END */

	g_de_device_opened++;
	filp->private_data = my_dev;

	/* some debug */
	DE_PRINT("device opened (%d:%d)\n", major, minor );

	return 0;
}

/**
 * release handler for de device
 *
 */
static int
DE_Close(struct inode *inode, struct file *file)
{
	int					major,minor;
	DE_DEVICE_T*	my_dev;
	struct cdev*		cdev;


	cdev	= inode->i_cdev;
	my_dev	= container_of ( cdev, DE_DEVICE_T, cdev);

	if ( my_dev->dev_open_count > 0 )
	{
		--my_dev->dev_open_count;
	}

	/* TODO : add your device specific code */
	/*	support multi-process system */
	if ( my_dev->dev_open_count < 0 )
	{
		DE_INT_Cleanup();
	}

	/* END */

	/* some debug */
	major = imajor(inode);
	minor = iminor(inode);
	DE_PRINT("device closed (%d:%d)\n", major, minor );
	return 0;
}

/**
 * memory mapping to virtual region
 *
 */
static int DE_Mmap(struct file *file, struct vm_area_struct *vma)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	int	minor;
	ULONG phy_start,phy_end;
	ULONG offset= vma->vm_pgoff << PAGE_SHIFT;
	ULONG size	= vma->vm_end - vma->vm_start;
	ULONG end   = PAGE_ALIGN(offset + size);
	UINT32 cap_mem_0, cap_mem_1, cap_size;

	minor = iminor(inode);
	if(minor != 1) {
		DE_ERROR("only dev_minor_num_1 (req. num %d)\n", minor);
		return -EINVAL;
	}
	
	if (size & (PAGE_SIZE-1)) return -EINVAL;

	DE_NOTI("vma : %08lx %08lx %08lx", vma->vm_start, \
			vma->vm_pgoff<<PAGE_SHIFT, \
			vma->vm_end);

	if(de_cfg_get_fcmem(&cap_mem_0, &cap_mem_1, &cap_size)) return -EINVAL;
	if(cap_mem_0 == 0 || cap_mem_1 == 0 || cap_size == 0) return -EINVAL;

	// check offset is allowed range or not.
	phy_start = cap_mem_0 & PAGE_MASK;
	phy_end   = PAGE_ALIGN( phy_start + cap_size);
	if ( phy_start <= offset && end <= phy_end ) goto allowed;

	phy_start = cap_mem_1 & PAGE_MASK;
	phy_end   = PAGE_ALIGN( phy_start + cap_size);
	if ( phy_start <= offset && end <= phy_end ) goto allowed;

	return -EINVAL;

allowed:
	vma->vm_flags |= VM_IO;
// rm at kernel 3.10	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, \
				vma->vm_pgoff, size, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

/**
 * ioctl handler for de device.
 *
 *
 * note: if you have some critial data, you should protect them using semaphore or spin lock.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int
DE_Ioctl ( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
#else
static long
DE_Ioctl (struct file * filp, unsigned int cmd, unsigned long arg)
#endif
{
	int err = 0, ret = 0;

	DE_DEVICE_T*	my_dev;
	struct cdev*		cdev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
#else
	struct inode *inode = filp->f_path.dentry->d_inode;
#endif

	/*
	 * get current de device object
	 */
	cdev	= inode->i_cdev;
	my_dev	= container_of ( cdev, DE_DEVICE_T, cdev);

	/*
	 * check if IOCTL command is valid or not.
	 * - if magic value doesn't match, return error (-ENOTTY)
	 * - if command is out of range, return error (-ENOTTY)
	 *
	 * note) -ENOTTY means "Inappropriate ioctl for device.
	 */
	if (_IOC_TYPE(cmd) != DE_IOC_MAGIC)
	{
		DE_WARN("invalid magic. magic=0x%02X\n", _IOC_TYPE(cmd) );
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > DE_IOC_MAXNR)
	{
		DE_WARN("out of ioctl command. cmd_idx=%d\n", _IOC_NR(cmd) );
		return -ENOTTY;
	}

	/* TODO : add some check routine for your device */

	/*
	 * check if user memory is valid or not.
	 * if memory can't be accessed from kernel, return error (-EFAULT)
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
	{
		DE_WARN("memory access error. cmd_idx=%d, rw=%c%c, memptr=%p\n",
									_IOC_NR(cmd),
									(_IOC_DIR(cmd) & _IOC_READ)? 'r':'-',
									(_IOC_DIR(cmd) & _IOC_WRITE)? 'w':'-',
									(void*)arg );
		return -EFAULT;
	}
	
#ifdef SUPPORT_DE_RESOURCE_MANAGER	
	/*	support multi-process system */
	{
		if (my_dev->dev_de_initialized == 0)
		{
			DE_INT_Init();
			my_dev->dev_de_initialized++;
		}
	}
	ret = resource_mgr_ioctl(cmd, arg);
#else

#ifndef USE_QEMU_SYSTEM
	//DE_TRACE("cmd = %08X (cmd_idx=%d)\n", cmd, _IOC_NR(cmd) );
#else
	switch(cmd)
	{
		case DE_IOW_REG_WR:
		case DE_IOR_REG_RD:
			break;
		default :
			DE_TRACE("cmd = %08X (cmd_idx=%d)\n", cmd, _IOC_NR(cmd) );
	}
#endif

	switch(cmd)
	{

		case DE_IOW_DE_INIT:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_PANEL_TYPE_T param;
			DE_PRINT("DE_IOW_DE_INIT IOCTL\n");

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_PANEL_TYPE_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] panel_type = %s \n", param?"LX_PANEL_TYPE_1336":"LX_PANEL_TYPE_1920"  );

			ret = 0;
#else
			/*	support multi-process system */
			if (my_dev->dev_de_initialized == 0)
			{
				DE_INT_Init();
				ret = DE_IO_Init(arg); // TODO

				my_dev->dev_de_initialized++;
			}
			else
			{
				ret = 0;
			}
#endif
		}
		break;

		case DE_IOR_GET_FIRMWARE_INFO:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_FIRMWARE_INFO_T param;

			param.version = 0x100;
			param.date.year = 10;	/* 2010/1/27 */
			param.date.month = 1;
			param.date.day = 27;

			DE_PRINT("DE_IOR_GET_FIRMWARE_INFO IOCTL\n");
			DE_PRINT("[de_drv] firmware info (%0X, %d:%d:%d)\n", param.version,
											param.date.year, param.date.month, param.date.day );

			if ( copy_to_user((void __user *)arg, &param, sizeof(LX_DE_FIRMWARE_INFO_T)) )
				return -EFAULT;

			ret = 0;
#else
			ret = DE_IO_GetFirmwareInfo(arg);
#endif
		}
		break;

		case DE_IOW_SET_BG_COLOR:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_COLOR_T param;

			DE_PRINT("DE_IOW_SET_BG_COLOR IOCTL\n");

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_COLOR_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] bg_color = (%d,%d,%d)\n", param.r, param.g, param.b);

			ret = 0;
#else
			ret = DE_IO_SetBgColor(arg);
#endif
		}
		break;

		case DE_IOR_GET_OUT_FR_RATE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_FR_RATE_T param;

			param.isForceFreeRun = TRUE;
			param.fr_rate = 50;

			DE_PRINT("DE_IOR_GET_OUT_FR_RATE IOCTL\n");
			DE_PRINT("[de_drv] fr rate info is free run %d	freq %d Hz\n", param.isForceFreeRun,param.fr_rate);

			if ( copy_to_user((void __user *)arg, &param, sizeof(LX_DE_FR_RATE_T)) )
				return -EFAULT;

			ret = 0;
#else
			ret = DE_IO_GetOutFrRate(arg);
#endif
		}
		break;

		case DE_IOW_SET_WIN_BLANK:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_SET_WIN_BLANK_T param;

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_SET_WIN_BLANK_T)) )
				return -EFAULT;

			DE_PRINT("DE_IOW_SET_WIN_BLANK IOCTL\n");
			DE_PRINT("[de_drv] blank window id = %d, isEnable = %s.\n", param.win_id,param.bEnable ? "ON":"OFF" );
			DE_PRINT("[de_drv] blank window color = (%d,%d,%d)\n",
										param.win_color.r, param.win_color.g, param.win_color.b);

			ret = 0;
#else
			ret = DE_IO_SetWinBlank(arg);
#endif
		}
		break;

		case DE_IOR_GET_INPUT_WIN:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_WIN_DIMENSION_T wi_info;

			if ( copy_from_user(&wi_info ,(void __user *)arg, sizeof(LX_DE_WIN_DIMENSION_T)) )
				return -EFAULT;

			if( wi_info.win_id == LX_DE_WIN_MAIN )
			{
				wi_info.rect.w = 1920;
				wi_info.rect.h = 1080;
				wi_info.rect.x = 0;
				wi_info.rect.y = 0;
			}
			else
			{
				wi_info.rect.w = 720;
				wi_info.rect.h = 480;
				wi_info.rect.x = 0;
				wi_info.rect.y = 0;
			}

			if ( copy_to_user((void __user *)arg, &wi_info, sizeof(LX_DE_WIN_DIMENSION_T)) )
				return -EFAULT;

			DE_PRINT("DE_IOR_GET_INPUT_WIN IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_GetInputWin(arg);
#endif
		}
		break;

		case DE_IOR_GET_OUT_WIN:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_WIN_DIMENSION_T wo_info;

			if ( copy_from_user(&wo_info ,(void __user *)arg, sizeof(LX_DE_WIN_DIMENSION_T)) )
				return -EFAULT;

			if( wo_info.win_id == LX_DE_WIN_MAIN )
			{
				wo_info.rect.w = 1336;
				wo_info.rect.h = 768;
				wo_info.rect.x = 0;
				wo_info.rect.y = 0;
			}
			else
			{
				wo_info.rect.w = 720;
				wo_info.rect.h = 576;
				wo_info.rect.x = 0;
				wo_info.rect.y = 0;
			}

			if ( copy_to_user((void __user *)arg, &wo_info, sizeof(LX_DE_WIN_DIMENSION_T)) )
				return -EFAULT;

			DE_PRINT("DE_IOR_GET_OUT_WIN IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_GetOutWin(arg);
#endif
		}
		break;

		case DE_IOW_SET_INPUT_WIN:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_WIN_DIMENSION_T wi_info;

			if ( copy_from_user(&wi_info ,(void __user *)arg, sizeof(LX_DE_WIN_DIMENSION_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] set input window wid = %d whxy(%d,%d,%d,%d)\n", wi_info.win_id,
													wi_info.rect.w, wi_info.rect.h, wi_info.rect.x, wi_info.rect.y);

			DE_PRINT("DE_IOW_SET_INPUT_WIN IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_SetInputWin(arg);
#endif
		}
		break;

		case DE_IOW_SET_OUT_WIN:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_WIN_DIMENSION_T wo_info;

			if ( copy_from_user(&wo_info ,(void __user *)arg, sizeof(LX_DE_WIN_DIMENSION_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] set output window wid = %d whxy(%d,%d,%d,%d)\n", wo_info.win_id,
													wo_info.rect.w, wo_info.rect.h, wo_info.rect.x, wo_info.rect.y);
			DE_PRINT("DE_IOW_SET_OUT_WIN IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_SetOutWin(arg);
#endif
		}
		break;

		case DE_IOW_SET_DIS_OUT:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			BOOLEAN isOn;

			if ( copy_from_user(&isOn ,(void __user *)arg, sizeof(UINT32)) )
			return -EFAULT;

			DE_PRINT("[de_drv] set dis out	isOn = %s.\n", isOn?"ON":"OFF");
			DE_PRINT("[de_drv] set dis out	isOn = %d.\n", isOn);
			DE_PRINT("DE_IOW_SET_DIS_OUT IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_SetDisOut(arg);
#endif
		}
		break;

		case DE_IOW_SET_DIS_FMT:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_DIS_FMT_T param;

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_DIS_FMT_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] SET_DIS_FMT panel_type = %d	fr_rate = %d.\n", param.panel_type, param.fr_rate );
			DE_PRINT("DE_IOW_SET_DIS_FMT IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_SetDisFmt(arg);
#endif
		}
		break;

		case DE_IOW_SET_FR_RATE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_FR_RATE_T param;

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_FR_RATE_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] SET_FR_RATE isForceFreeRun = %d	fr_rate = %d.\n", param.isForceFreeRun, param.fr_rate);
			DE_PRINT("DE_IOW_SET_FR_RATE IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_SetFrRate(arg);
#endif
		}
		break;

		case DE_IOW_MULTI_WIN_ENABLE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_MULTI_WIN_ENABLE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_MultiWinEnable(arg);
#endif
		}
		break;

		case DE_IOW_SET_CVI_SRC_TYPE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_CVI_SRC_TYPE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCviSrcType(arg);
#endif
		}
		break;
		
		case DE_IOR_GET_MVI_COLORIMETRY:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_MVI_COLORIMETRY_T param;

			DE_PRINT("DE_IOR_GET_MVI_COLORIMETRY IOCTL\n");

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_MVI_COLORIMETRY_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv]GET_MVI_COLORIMETRY win %d.\n", param.win_id);

			param.colorimetry = LX_DE_COLOR_601;

			if ( copy_to_user((void __user *)arg, &param, sizeof(LX_DE_MVI_COLORIMETRY_T)) )
				return -EFAULT;

			ret = 0;
#else
			ret = DE_IO_GetMviColorimetry(arg);
#endif
		}
		break;

		case DE_IOW_SET_ZLIST:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_ZLIST IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetZList(arg);
#endif
		}
		break;

		case DE_IOW_SET_DEINTERLACE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_DEINTERLACE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetDeinterlace(arg);
#endif
		}
		break;

		case DE_IOW_SET_UART_FOR_MCU:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_UART_FOR_MCU IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetUart4MCU(arg);
#endif
		}
		break;
		
		case DE_IOW_SET_UD_MODE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_UD_MODE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetUDMode(arg);
#endif
		}
		break;
		
		case DE_IOW_SET_CVE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_CVE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCve(arg);
#endif
		}
		break;

		case DE_IOW_SET_VCS:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_VCS IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetVcs(arg);
#endif
		}
		break;

		case DE_IOR_REG_RD:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			UINT32 io_data;

			if ( copy_from_user(&io_data ,(void __user *)arg, sizeof(UINT32)) )
				return -EFAULT;

			DE_PRINT("[de_drv] address 0x%x.\n", io_data);
			io_data = 0xFAFAFAFA;	// read form L8 register
			DE_PRINT("[de_drv] rd data 0x%x.\n", io_data);

			if ( copy_to_user((void __user *)arg, &io_data, sizeof(UINT32)) )
				return -EFAULT;

			DE_PRINT("DE_IOR_REG_RD IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_RegRd(arg);
#endif
		}
		break;

		case DE_IOW_REG_WR:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_REG_WR_T param;

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_REG_WR_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv] address 0x%x.\n", param.wr_addr);
			DE_PRINT("[de_drv] wr data 0x%x.\n", param.wr_data);

			// write to  L8 register

			DE_PRINT("DE_IOW_REG_WR IOCTL\n");

			ret = 0;
#else
			ret = DE_IO_RegWr(arg);
#endif
		}
		break;
		
		case DE_IOW_FW_DOWNLOAD:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_FW_DWLD_T param;

			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_FW_DWLD_T)) )
				return -EFAULT;

			ret = 0;
#else
			/*	support multi-process system */
			if ( my_dev->dev_de_fw_downloaded == 0)
			{
				ret = DE_IO_FW_Download(arg);
				my_dev->dev_de_fw_downloaded++;
			}
			else
			{
				ret = 0;
			}
#endif
		}
		break;

		case DE_IOW_SET_CVI_FIR:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			LX_DE_CVI_FIR_T param;
			int i;
			DE_PRINT("DE_IOW_SET_CVI_FIR IOCTL\n");
			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_CVI_FIR_T)) )
				return -EFAULT;

			DE_PRINT("[de_drv]CVI_FIR cvi ch = %d  is_enable = %d\n", param.cvi_channel,param.isEnable);

			for(i=0; i< 8; i++)
					DE_PRINT("[de_drv]CVI_FIR coef %d = %x\n", i,param.fir_coef[i]);

			ret = 0;
#else
			ret = DE_IO_SetCviFir(arg);
#endif
		}
		break;

		case DE_IOW_SET_CVI_TPG:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_CVI_TPG IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCviTpg(arg);
#endif
		}
		break;

		case DE_IOW_SET_CVI_CSAMPLE_MODE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_CVI_CSAMPLE_MODE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCviCsampleMode(arg);
#endif
		}
		break;
		
		case DE_IOW_SET_DVR_FMT_CTRL:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_DVR_FMT_CTRL IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetDvrFmtCtrl(arg);
#endif
		}
		break;

		case DE_IOW_SET_SUB_SC_FREEZE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_SUB_SC_FREEZE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetSubScFreeze(arg);
#endif
		}
		break;

		case DE_IOW_SET_CVI_FREEZE:
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_CVI_FREEZE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCviFreeze(arg);
#endif
		}
		break;

		case DE_IOWR_SET_CVI_VIDEO_FRAME_BUFFER :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOWR_SET_CVI_VIDEO_FRAME_BUFFER IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCviVideoFrameBuffer(arg);
#endif

		}
		break;

		case  DE_IOW_SET_SCART_RGB_BYPASS_MODE :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_SCART_RGB_BYPASS_MODE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetScartRGBBypassMode(arg);
#endif
		}
		break;


		case DE_IOR_GET_DBG :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOR_GET_DBG IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_GetDebug(arg);
#endif
		}
		break;

		case DE_IOW_SET_DBG :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_DBG IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetDebug(arg);
#endif
		}
		break;
		
		case DE_IOW_SET_LOW_DELAY :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_LOW_DELAY IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetLowDelay(arg);
#endif
		}
		break;

		case DE_IOR_GET_LOW_DELAY :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOR_GET_LOW_DELAY IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_GetLowDelay(arg);
#endif
		}
		break;

		case DE_IOW_SET_3D_INOUT_CTRL :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_3D_INOUT_CTRL IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_Set3DInOutCtrl(arg);
#endif
		}
		break;

		case DE_IOW_SET_IF_CONFIG :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_IF_CONFIG IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetInterfaceConfig(arg);
#endif
		}
		break;

		case DE_IOR_GET_SYS_STATUS :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOR_GET_SYS_STATUS IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_GetSystemStatus(arg);
#endif
		}
		break;

		case DE_IOR_GET_SRC_STATUS :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOR_GET_SRC_STATUS IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_GetSourceStatus(arg);
#endif
		}
		break;

		case DE_IOW_SET_OPERATION :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_OPERATION IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetOperation(arg);
#endif
		}
		break;

		case DE_IOW_SET_SUB_OPERATION :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_SUB_OPERATION IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetSubOperation(arg);
#endif
		}
		break;

		case DE_IOW_SET_CAPTURE :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_CAPTURE IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetCaptureControl(arg);
#endif
		}
		break;

		case DE_IOW_SET_MULTI_VISION :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_MULTI_VISION IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetMultiVision(arg);
#endif
		}
		break;

		case DE_IOW_SET_VTM :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_VTM IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_SetVTM(arg);
#endif
		}
		break;

		case DE_IOR_GET_VTM :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOR_GET_VTM IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_GetVTM(arg);
#endif
		}
		break;

		case DE_IOR_GET_SCALER_INFO :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOR_GET_SCALER_INFO IOCTL\n");
			ret = 0;
#else
			ret = DE_IO_GetScalerInfo(arg);
#endif
		}
		break;

		case DE_IOW_SET_OUTPUT_MODE :
		{
			ret = 0;
		}
		break;

		case DE_IOW_SET_WIN_PORT_SRC:
		{
			ret = DE_IO_SetWinPortSrc(arg);
		}
		break;

		case DE_IOW_SET_WIN_FREEZE:
		{
			ret = DE_IO_SetWinFreeze(arg);
		}
		break;

		case DE_IOW_SET_EVAL_DBG :
		{
			#include "mcu/de_dbi.h"
			LX_DE_SET_EVAL_DBG_T param;
			if ( copy_from_user(&param ,(void __user *)arg, sizeof(LX_DE_SET_EVAL_DBG_T)) )
			{
				return -EFAULT;
			}
			switch (param.cmd)
			{
				case LX_DE_EVAL_CMD_DBI :
				{
					ret = DBI_VIDEO_Execute(param.arg.argc, param.arg.argv);
				}
				break;

				default :
					break;
			}
		}
		break;

		case DE_IOW_SET_SRC_MIRROR :
		{
#ifndef USE_VIDEO_IOCTL_CALLING
			DE_PRINT("DE_IOW_SET_SRC_MIRROR\n");
			ret = 0;
#else
			ret = 0;
#endif
		}
		break;
		
		default:
		{
			/* redundant check but it seems more readable */
			ret = -ENOTTY;
		}
	}
#endif

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef KDRV_GLOBAL_LINK
#if defined(CONFIG_LG_BUILTIN_KDRIVER) && defined(CONFIG_LGSNAP)
user_initcall_grp("kdrv", DE_Init);
#else
module_init(DE_Init);
#endif
module_exit(DE_Cleanup);

MODULE_AUTHOR("LGE");
MODULE_DESCRIPTION("base driver");
MODULE_LICENSE("GPL");
#endif

/** @} */

