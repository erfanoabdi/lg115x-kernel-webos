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
 *  main driver implementation for ci device.
 *	ci device will teach you how to make device driver with new platform.
 *
 *  author		Srinivasan Shanmugam	(srinivasan.shanmugam@lge.com)
 *  author		Hwajeong Lee (hwajeong.lee@lge.com)
 *  author		Jinhwan Bae (jinhwan.bae@lge.com) - modifier
 *  version		1.0
 *  date		2009.12.30
 *  note		Additional information.
 *
 *  @addtogroup lg1150_ci
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/
#undef	SUPPORT_CI_DEVICE_READ_WRITE_FOPS

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
#ifdef KDRV_CONFIG_PM	// added by SC Jung for quick booting
#include <linux/platform_device.h>
#endif
#include "os_util.h"
#include "base_device.h"
#include "ci_drv.h"
#include "ci_coredrv.h"
#include "ci_regdefs.h"
#include "ci_io.h"

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
 *	main control block for ci device.
 *	each minor device has unique control block
 *
 */
typedef struct
{
// BEGIN of common device
	int						dev_open_count;		///< check if device is opened or not
	dev_t					devno;			///< device number
	struct cdev				cdev;			///< char device structure
// END of command device

// BEGIN of device specific data


// END of device specific data
}
CI_DEVICE_T;

#ifdef KDRV_CONFIG_PM
typedef struct
{
	// add here extra parameter
	bool	is_suspended;
}CI_DRVDATA_T;

#endif
/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
extern	void	CI_PROC_Init(void);
extern	void	CI_PROC_Cleanup(void);

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
int		CI_Init(void);
void	CI_Cleanup(void);

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
int		g_ci_debug_fd;
int 	g_ci_major = CI_MAJOR;
int 	g_ci_minor = CI_MINOR;

#ifdef KDRV_CONFIG_PM

	S_CI_REG_T gCIReg_QB;

	extern volatile S_CI_REG_T* gpstCIReg;

#endif /* KDRV_CONFIG_PM */
/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static int      CI_Open(struct inode *, struct file *);
static int      CI_Close(struct inode *, struct file *);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)	
static int 		CI_Ioctl (struct inode *, struct file *, unsigned int, unsigned long );
#else
static long		CI_Ioctl (struct file *, unsigned int, unsigned long );
#endif

#ifdef SUPPORT_CI_DEVICE_READ_WRITE_FOPS
static ssize_t  CI_Read(struct file *, char *, size_t, loff_t *);
static ssize_t  CI_Write(struct file *, const char *, size_t, loff_t *);
#endif

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

/**
 * main control block for ci device
*/
static CI_DEVICE_T*		g_ci_device;

/**
 * file I/O description for ci device
 *
*/
static struct file_operations g_ci_fops =
{
	.open 	= CI_Open,
	.release= CI_Close,

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)	
	.ioctl	= CI_Ioctl,
#else
	.unlocked_ioctl = CI_Ioctl,
#endif

#ifdef SUPPORT_CI_DEVICE_READ_WRITE_FOPS
	.read 	= CI_Read,
	.write 	= CI_Write,
#else
	.read	= NULL,
	.write	= NULL,
#endif
};

/*========================================================================================
	Implementation Group
========================================================================================*/
#ifdef KDRV_CONFIG_PM	// added by SC Jung for quick booting
/**
 *
 * suspending module.
 *
 * @param	struct platform_device *pdev pm_message_t state
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static int CI_suspend(struct platform_device *pdev, pm_message_t state)
{
	CI_DRVDATA_T *drv_data;

	drv_data = platform_get_drvdata(pdev);
	
	if ( drv_data->is_suspended == 1 )
	{
		return -1;	//If already in suspend state, so ignore
	}

	/* unmap SMC only, resume do set again with ioremap and default init */
	CI_UnmapSMC();

	drv_data->is_suspended = 1;
	CI_DTV_SOC_Message(CI_DBG_INFO,"[%s] done suspend\n", CI_MODULE);
	
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
static int CI_resume(struct platform_device *pdev)
{
	CI_DRVDATA_T *drv_data;

	drv_data = platform_get_drvdata(pdev);

	if(drv_data->is_suspended == 0) return -1;

	/* resume , SMC set with ioremap and do Default Init */
	CI_InitSMC();
	CI_DefaultInit();

	CI_DTV_SOC_Message(CI_DBG_INFO,"[%s] done resume\n", CI_MODULE);

	drv_data->is_suspended = 0;

	return 0;
}
/**
 *
 * probing module.
 *
 * @param	struct platform_device *pdev
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
 int CI_probe(struct platform_device *pdev)
{

	CI_DRVDATA_T *drv_data;

	drv_data = (CI_DRVDATA_T *)kmalloc(sizeof(CI_DRVDATA_T) , GFP_KERNEL);


	// add here driver registering code & allocating resource code



	CI_DTV_SOC_Message(CI_DBG_INFO,"[%s] done probe\n", CI_MODULE);
	drv_data->is_suspended = 0;
	platform_set_drvdata(pdev, drv_data);

	return 0;
}


/**
 *
 * module remove function. this function will be called in rmmod fbdev module.
 *
 * @param	struct platform_device
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static int  CI_remove(struct platform_device *pdev)
{
	CI_DRVDATA_T *drv_data;

	// add here driver unregistering code & deallocating resource code



	drv_data = platform_get_drvdata(pdev);
	kfree(drv_data);

	CI_DTV_SOC_Message(CI_DBG_INFO,"released\n");

	return 0;
}

/**
 *
 * module release function. this function will be called in rmmod module.
 *
 * @param	struct device *dev
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static void  CI_release(struct device *dev)
{


	CI_DTV_SOC_Message(CI_DBG_INFO,"device released\n");
}

/*
 *	module platform driver structure
 */
static struct platform_driver ci_driver =
{
	.probe          = CI_probe,
	.suspend        = CI_suspend,
	.remove         = CI_remove,
	.resume         = CI_resume,
	.driver         =
	{
		.name   = CI_MODULE,
	},
};

static struct platform_device ci_device = {
	.name = CI_MODULE,
	.id = 0,
	.id = -1,
	.dev = {
		.release = CI_release,
	},
};
#endif

/** Initialize the device environment before the real H/W initialization
 *
 *  @note main usage of this function is to initialize the HAL layer and memory size adjustment
 *  @note it's natural to keep this function blank :)
 */
void CI_PreInit(void)
{
    /* TODO: do something */
}

/**
 *	Initialize CI device
*/
int CI_Init(void)
{
	int			i;
	int			err;
	dev_t		dev;

	/* Get the handle of debug output for ci device.
	 *
	 * Most module should open debug handle before the real initialization of module.
	 * As you know, debug_util offers 4 independent debug outputs for your device driver.
	 * So if you want to use all the debug outputs, you should initialize each debug output
	 * using OS_DEBUG_EnableModuleByIndex() function.
	 */
	g_ci_debug_fd = DBG_OPEN( CI_MODULE );
	if ( g_ci_debug_fd >= 0 )
	{
		OS_DEBUG_EnableModule ( g_ci_debug_fd );
		OS_DEBUG_EnableModuleByIndex ( g_ci_debug_fd, CI_ERR, DBG_COLOR_NONE );
		OS_DEBUG_EnableModuleByIndex ( g_ci_debug_fd, CI_NOTI, DBG_COLOR_NONE );	/* NOTI */
	}
	else
	{
		CI_DTV_SOC_Message( CI_ERR, "CI DBG_OPEN failed\n" );
		return -EIO;
	}

	/* allocate main device handler, register current device.
	 *
	 * If devie major is predefined then register device using that number.
	 * otherwise, major number of device is automatically assigned by Linux kernel.
	 *
	 */
#ifdef KDRV_CONFIG_PM
	// added by SC Jung for quick booting
	if(platform_driver_register(&ci_driver) < 0)
	{
		CI_DTV_SOC_Message( CI_ERR,"CI platform driver register failed\n");
	}
	else
	{
		if(platform_device_register(&ci_device))
		{
			platform_driver_unregister(&ci_driver);
			CI_DTV_SOC_Message( CI_ERR,"CI platform device register failed\n");
		}
		else
		{
			CI_DTV_SOC_Message(CI_DBG_INFO,"[%s] platform register done\n", CI_MODULE);
		}
	}
#endif
	g_ci_device = (CI_DEVICE_T*)OS_KMalloc( sizeof(CI_DEVICE_T)*CI_MAX_DEVICE );

	if ( NULL == g_ci_device )
	{
		CI_DTV_SOC_Message( CI_ERR,"out of memory. can't allocate %d bytes\n", sizeof(CI_DEVICE_T)* CI_MAX_DEVICE );
		return -ENOMEM;
	}

	memset( g_ci_device, 0x0, sizeof(CI_DEVICE_T)* CI_MAX_DEVICE );

	if (g_ci_major)
	{
		dev = MKDEV( g_ci_major, g_ci_minor );
		err = register_chrdev_region(dev, CI_MAX_DEVICE, CI_MODULE );
	}
	else
	{
		err = alloc_chrdev_region(&dev, g_ci_minor, CI_MAX_DEVICE, CI_MODULE );
		g_ci_major = MAJOR(dev);
	}

	if ( err < 0 )
	{
		CI_DTV_SOC_Message( CI_ERR,"can't register ci device\n" );
		return -EIO;
	}

	/* TODO : initialize your module not specific minor device */


	/* END */

	for ( i=0; i<CI_MAX_DEVICE; i++ )
	{
		/* initialize cdev structure with predefined variable */
		dev = MKDEV( g_ci_major, g_ci_minor+i );
		cdev_init( &(g_ci_device[i].cdev), &g_ci_fops );
		g_ci_device[i].devno		= dev;
		g_ci_device[i].cdev.owner = THIS_MODULE;
		g_ci_device[i].cdev.ops   = &g_ci_fops;

		/* TODO: initialize minor device */


		/* END */

		err = cdev_add (&(g_ci_device[i].cdev), dev, 1 );

		if (err)
		{
			CI_DTV_SOC_Message( CI_ERR,"error (%d) while adding ci device (%d.%d)\n", err, MAJOR(dev), MINOR(dev) );
			return -EIO;
		}
        OS_CreateDeviceClass ( g_ci_device[i].devno, "%s%d", CI_MODULE, i );
	}

	/* initialize proc system */
	CI_PROC_Init ( );


	/* initialze CI H/W */
	CI_Initialize();


	CI_DTV_SOC_Message( CI_TRACE,"ci device initialized\n");

	return 0;
}


/**
 *	Cleanup CI device
*/
void CI_Cleanup(void)
{
	int i;
	dev_t dev = MKDEV( g_ci_major, g_ci_minor );

#ifdef KDRV_CONFIG_PM
	// added by SC Jung for quick booting
	platform_driver_unregister(&ci_driver);
	platform_device_unregister(&ci_device);
#endif

	/* cleanup proc system */
	CI_PROC_Cleanup( );

	/* remove all minor devicies and unregister current device */
	for ( i=0; i<CI_MAX_DEVICE;i++)
	{
		/* TODO: cleanup each minor device */


		/* END */
		cdev_del( &(g_ci_device[i].cdev) );
	}

	/* TODO : cleanup your module not specific minor device */

	// shutdown H/W
	CI_UnInitialize();					
	

	unregister_chrdev_region(dev, CI_MAX_DEVICE );

	OS_Free( g_ci_device );
}


///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * open handler for ci device
 *
 */
static int CI_Open(struct inode *inode, struct file *filp)
{
    int					major,minor;
    struct cdev*    	cdev;
    CI_DEVICE_T*	my_dev;

    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, CI_DEVICE_T, cdev);


#if 0
	if(my_dev->dev_open_count == 0) 
	{	// check first open
		CI_Initialize();	// init H/W
	}
#endif


    my_dev->dev_open_count++;
    filp->private_data = my_dev;

	/* some debug */
    major = imajor(inode);
    minor = iminor(inode);

    return 0;
}

/**
 * release handler for ci device
 *
 */
static int CI_Close(struct inode *inode, struct file *file)
{
    int					major,minor;
    CI_DEVICE_T*		my_dev;
    struct cdev*		cdev;

    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, CI_DEVICE_T, cdev);

    if ( my_dev->dev_open_count > 0 )
    {
        --my_dev->dev_open_count;
    }

#if 0
	if( my_dev->dev_open_count <= 0 ) {		// check last close
		CI_UnInitialize();					// shutdown H/W
	}
#endif


	/* some debug */
    major = imajor(inode);
    minor = iminor(inode);

	CI_DTV_SOC_Message(CI_DBG_INFO,"[ CI_Close: CI device closed  <<S>> ]  \n");

    return 0;
}

/**
 * ioctl handler for ci device.
 *
 *
 * note: if you have some critial data, you should protect them using semaphore or spin lock.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)	 
static int CI_Ioctl ( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
#else
static long CI_Ioctl ( struct file *filp, unsigned int cmd, unsigned long arg )
#endif
{
    int err = 0, ret = 0;


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)	 
    CI_DEVICE_T*	my_dev;
    struct cdev*		cdev;

	// get current ci device object
    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, CI_DEVICE_T, cdev);	

	// if need iode 
	// struct inode *inode = filp->f_path.dentry->d_inode;
#endif	

    /*
     * check if IOCTL command is valid or not.
     * - if magic value doesn't match, return error (-ENOTTY)
     * - if command is out of range, return error (-ENOTTY)
     *
     * note) -ENOTTY means "Inappropriate ioctl for device.
     */
    if (_IOC_TYPE(cmd) != CI_IOC_MAGIC)
    {
    	DBG_PRINT_WARNING("invalid magic. magic=0x%02X\n", _IOC_TYPE(cmd) );
    	return -ENOTTY;
    }
    if (_IOC_NR(cmd) > CI_IOC_MAXNR)
    {
    	DBG_PRINT_WARNING("out of ioctl command. cmd_idx=%d\n", _IOC_NR(cmd) );
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
    	DBG_PRINT_WARNING("memory access error. cmd_idx=%d, rw=%c%c, memptr=%p\n",
    													_IOC_NR(cmd),
    													(_IOC_DIR(cmd) & _IOC_READ)? 'r':'-',
    													(_IOC_DIR(cmd) & _IOC_WRITE)? 'w':'-',
    													(void*)arg );
        return -EFAULT;
	}

	//CI_TRACE("cmd = %08X (cmd_idx=%d)\n", cmd, _IOC_NR(cmd) );

	switch(cmd)
	{
		case CI_IO_RESET:
		{
			ret = CI_ResetCI();
		}
		break;

		case CI_IO_CHECK_CIS:
		{
			ret = CI_CheckCIS();
		}
		break;

		case CI_IO_WRITE_COR:
		{
			ret = CI_WriteCOR();
		}
		break;

		case CI_IOR_DETECT_CARD:
		{
			UINT32 uwDetect = 0;

			ret = CI_CAMDetectStatus( &uwDetect );
			if( ret == 0 )
			{
				CI_DTV_SOC_Message(CI_DBG_INFO,"[ CI Card %s <<S>> ]  \n", ((uwDetect==1)? ("INSERTED") : ("REMOVED")));

	 	        if( copy_to_user( ( void __user * )arg, &uwDetect, sizeof( UINT32 ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_DETECT_CARD): copy_to_user <<F>> ]\n");

    	        	return -1;
	 	        }
			}
		}
		break;

		case CI_IOR_READ_DATA:
		{
			if( arg )
			{
				LX_CI_IOCTL_PARAM_T stReadData;

				if( copy_from_user( &stReadData, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_T ) ) )
				{
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_READ_DATA): copy_to_user <<F>> ]\n");

    	        	return -1;
	 	        }

				ret = CI_ReadData( stReadData.pBuf, &stReadData.sLength );

				if( copy_to_user( ( void __user * )arg, &stReadData, sizeof( LX_CI_IOCTL_PARAM_T ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR, " copy_to_user <<F>>\n" );

		        	return -1;
	 	        }
			}
		}
		break;

		case CI_IOR_NEGOTIATE_BUF_SIZE:
		{
			UINT32 uwBufSize = 0;

			//Negotiate Buffer size
			ret = CI_NegoBuff( &uwBufSize );
			if( ret == 0 )
			{
				// return the resulted size
				CI_DTV_SOC_Message(CI_DBG_INFO,"Negotiated BUF size %d <<S>> ]\n", uwBufSize);

				if( copy_to_user( ( void __user * )arg, &uwBufSize, sizeof( UINT32 ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_NEGOTIATE_BUF_SIZE): copy_to_user <<F>> ] \n");

    	        	return -1;
	 	        }
			}
			else
			{
				CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_NEGOTIATE_BUF_SIZE): "
									"Calc Negotiation Buffer <<F>> ]\n");
			}
		}
		break;

		case CI_IOR_READ_DA_STATUS:
		{
			UINT32 uwDataAvailable = 0;

			//read the status of being Data-Available
			ret = CI_ReadDAStatus( &uwDataAvailable );
			if( ret == 0 )
			{
				CI_DTV_SOC_Message(CI_DBG_INFO,"[ CI Read DA status is %s <<S>> ]\n",
						((uwDataAvailable)? ("Available") : ("Not Available")) );

				if( copy_to_user( ( void __user * )arg, &uwDataAvailable, sizeof( UINT32 ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_READ_DA_STATUS): copy_to_user <<F>> ]\n");
					
    	        	return -1;
	 	        }
			}
		}
		break;

		case CI_IOW_WRITE_DATA:
		{
			if( arg )
			{
				LX_CI_IOCTL_PARAM_T stReadData;

				if ( copy_from_user( &stReadData, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_T ) ) )
				{
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOW_WRITE_DATA): copy_from_user <<F>> ]\n");

    	        	return -1;
	 	        }

				ret = CI_WriteData( stReadData.pBuf, stReadData.sLength );
			}
		}
		break;

		case CI_IO_SET_PHY_RESET:
		{
			ret = CI_ResetPhysicalIntrf( );
		}
		break;

		case CI_IO_SET_RS:
		{
			ret = CI_SetRS( );
		}
		break;

		case CI_IOR_READ_IIR_STATUS:
		{
			// Read IIR status from status register and return it.
			UINT32 uwIIRStatus = 0;

			//read the status of being Data-Available
			ret = CI_ReadIIRStatus( &uwIIRStatus );
			if( ret == 0 )
			{
				CI_DTV_SOC_Message(CI_DBG_INFO,"[ CI_Ioctl(CI_IOR_READ_IIR_STATUS): Read IIR status is %s <<S>> ]\n", ((uwIIRStatus)? ("RESET") : ("NO")));

				if ( copy_to_user( ( void __user * )arg, &uwIIRStatus, sizeof( UINT32 ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_READ_IIR_STATUS): copy_to_user <<F>> ]\n");

    	        	return -1;
	 	        }
			}
		}
		break;

		case CI_IOR_CHECK_CAPABILITY:
		{
			LX_CI_IOCTL_PARAM_CAMTYPE stData;

			ret = CI_CheckCAMType( &stData.uwRtnValue, &stData.uwCheckCAMType );

			if( ret == 0 )
			{
				if ( copy_to_user( ( void __user * )arg, &stData, sizeof( LX_CI_IOCTL_PARAM_CAMTYPE ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_CHECK_CAPABILITY): copy_to_user <<F>> ]\n");

    	        	return -1;
	 	        }
			}
			else
			{
				CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_CHECK_CAPABILITY): Check CAM Type <<F>> ]\n");
			}
		}
		break;

		case CI_IOR_GET_CIPLUS_VERSION:
		{
			LX_CI_IOCTL_PARAM_VERSION stData;

			ret = CI_GetCIPlusSupportVersion( &stData.uwRtnValue, &stData.uwVersion );

			if( ret == 0 )
			{
				if ( copy_to_user( ( void __user * )arg, &stData, sizeof( LX_CI_IOCTL_PARAM_VERSION ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_GET_CIPLUS_VERSION): copy_to_user <<F>> ]\n");

    	        	return -1;
	 	        }
			}
			else
			{
				CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_GET_CIPLUS_VERSION): Check CAM Support Version <<F>> ]\n");
			}
		}
		break;

		case CI_IOR_GET_CIPLUS_OPROFILE:
		{
			LX_CI_IOCTL_PARAM_OPROFILE stData;

			ret = CI_GetCIPlusOperatorProfile( &stData.uwRtnValue, &stData.uwProfile );

			if( ret == 0 )
			{
				if ( copy_to_user( ( void __user * )arg, &stData, sizeof( LX_CI_IOCTL_PARAM_OPROFILE ) ) )
	 	        {
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_GET_CIPLUS_OPROFILE): copy_to_user <<F>> ]\n");

    	        	return -1;
	 	        }
			}
			else
			{
				CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOR_GET_CIPLUS_OPROFILE): Check CAM Operator Profile <<F>> ]\n");
			}
		}
		break;

		case CI_IOR_CAM_INIT:
		{
			ret = CI_CAMInit( );
		}
		break;

		case CI_IOR_PRINT_REG:
		{
			ret = CI_RegPrint();
		}
		break;

		case CI_IOR_CAM_POWEROFF:
		{
			ret = CI_CAMPowerOff( );
		}
		break;

		case CI_IOR_CAM_POWERONCOMPLETED:
		{
			ret = CI_CAMPowerOnCompleted( );
		}
		break;

		case CI_IOW_CAM_SET_DELAY:
		{
			if( arg )
			{
				LX_CI_IOCTL_PARAM_SETDELAY stSetDelay;

				if( copy_from_user( &stSetDelay, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_SETDELAY ) ) )
				{
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(CI_IOW_CAM_SET_DELAY): copy_from_user <<F>> ]\n");

		        	return -1;
	 	        }
				
				ret = CI_CAMSetDelay( stSetDelay.eDelayType , stSetDelay.uiDelayValue);
			}
		}
		break;

		case CI_IO_CAM_PRINT_DELAY_VALUES:
		{
			ret = CI_CAMPrintDelayValues( );
		}
		break;

		case CI_IO_READ_REGISTERS:
		{
			ret = CI_RegPrint( );
		}
		break;

		case CI_IOW_WRITE_REGISTER:
		{
			if( arg )
			{
				LX_CI_IOCTL_PARAM_REGISTER stRegWrite;

				if( copy_from_user( &stRegWrite, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_REGISTER ) ) )
				{
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(LX_CI_IOCTL_PARAM_REGISTER): copy_from_user <<F>> ]\n");

		        	return -1;
	 	        }
				
				ret = CI_RegWrite( stRegWrite.uiOffset, stRegWrite.uiValue );
			}
			
		}
		break;

		case CI_IOW_SET_PCMCIA_SPEED:
		{
			CI_DTV_SOC_Message(CI_NOTI, "\n\n CI_IOW_SET_PCMCIA_SPEED 1 \n\n");
			if( arg )
			{
				LX_CI_IOCTL_PARAM_SPEED stPCMCIASpeed;
				CI_BUS_SPEED_T speed;

				if( copy_from_user( &stPCMCIASpeed, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_SPEED ) ) )
				{
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(LX_CI_IOCTL_PARAM_SPEED): copy_from_user <<F>> ]\n");

		        	return -1;
	 	        }

				if(stPCMCIASpeed.ui8Speed == 0) speed = PCMCIA_BUS_SPEED_LOW;
				else						 speed = PCMCIA_BUS_SPEED_HIGH;
				ret = CI_SetPCCardBusTiming( speed );
			}
		}
		break;
		
		case CI_IOW_ENABLE_LOG:
		{
			if( arg )
			{
				LX_CI_IOCTL_PARAM_LOG_ENABLE stLogEnable;
				
				if( copy_from_user( &stLogEnable, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_LOG_ENABLE ) ) )
				{
					CI_DTV_SOC_Message(CI_ERR,"[ CI_Ioctl(LX_CI_IOCTL_PARAM_LOG_ENABLE): copy_from_user <<F>> ]\n");

		        	return -1;
				}
				
				ret = CI_IO_EnableLog(stLogEnable.mask);
				
				CI_DTV_SOC_Message( CI_DBG_INFO, "CI_IOW_ENABLE_LOG ok\n");
			}
		}
		break;
	
		case CI_IOW_CHANGE_ACCESSMODE:
		{
			if( arg )
			{
				LX_CI_IOCTL_PARAM_ACCESSMODE_CHANGE stAcessMode;
				CI_ACCESS_MODE_T mode;

				if( copy_from_user( &stAcessMode, (void __user *)arg, sizeof( LX_CI_IOCTL_PARAM_ACCESSMODE_CHANGE ) ) )
				{
					CI_DTV_SOC_Message(CI_DBG_INFO,"[ CI_Ioctl(LX_CI_IOCTL_PARAM_ACCESSMODE_CHANGE): copy_from_user <<F>> ]\n");

		        		return -1;
	 	        	}

				if	(stAcessMode.ui32Mode== 0) 			mode = ACCESS_1BYTE_MODE;
				else	if (stAcessMode.ui32Mode== 1 )		mode = ACCESS_2BYTE_MODE;
				else									mode = ACCESS_4BYTE_MODE;
				
				ret = CI_ChangeAccessMode( mode );
				if(LX_IS_ERR(ret))
				{
					CI_DEBUG_Print("CI_ChangeAccessMode failed:[%d]", ret);

					ret = -ENOTTY;
				}
				else
				{
					ret = 0;
				}
			}
		}
		break;
	
	    default:
	    {
			/* redundant check but it seems more readable */
    	    ret = -ENOTTY;
		}
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef KDRV_GLOBAL_LINK
#if defined(CONFIG_LG_BUILTIN_KDRIVER) && defined(CONFIG_LGSNAP)
user_initcall_grp("kdrv",CI_Init);
#else
module_init(CI_Init);
#endif
module_exit(CI_Cleanup);

MODULE_AUTHOR("LGE");
MODULE_DESCRIPTION("base driver");
MODULE_LICENSE("GPL");
#endif

/** @} */

