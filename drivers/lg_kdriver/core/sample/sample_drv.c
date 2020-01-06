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
 *  main driver implementation for sample device.
 *	sample device will teach you how to make device driver with new platform.
 *
 *  author		root
 *  version		1.0
 *  date		2009.12.07
 *  note		Additional information.
 *
 *  @addtogroup lg1150_sample
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/
#undef	SUPPORT_SAMPLE_DEVICE_READ_WRITE_FOPS

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
#include "os_util.h"
#include "base_device.h"
#include "sample_drv.h"

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
 *	main control block for sample device.
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

	/* timer test */
	OS_TIMER_T				timer1;
	OS_TIMER_T				timer2;

	/* event test */
	OS_EVENT_T				test_event;

	OS_SEM_T				sem;			///< test semaphore ( each minor device has unique semaphore )
// END of device specific data
}
SAMPLE_DEVICE_T;

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
extern	void	SAMPLE_PROC_Init(void);
extern	void	SAMPLE_PROC_Cleanup(void);

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
int		SAMPLE_Init(void);
void	SAMPLE_Cleanup(void);

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
int		g_sample_debug_fd;
int 	g_sample_major = SAMPLE_MAJOR;
int 	g_sample_minor = SAMPLE_MINOR;

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static int      SAMPLE_Open(struct inode *, struct file *);
static int      SAMPLE_Close(struct inode *, struct file *);
static int 		SAMPLE_Ioctl (struct inode *, struct file *, unsigned int, unsigned long );
#ifdef SUPPORT_SAMPLE_DEVICE_READ_WRITE_FOPS
static ssize_t  SAMPLE_Read(struct file *, char *, size_t, loff_t *);
static ssize_t  SAMPLE_Write(struct file *, const char *, size_t, loff_t *);
#endif

static void		_SAMPLE_TestTimeOut ( ULONG data );
static void		_SAMPLE_TestTimeTick( ULONG data );

//static void		_SAMPLE_TestEventTimer ( ULONG data );

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

/**
 * main control block for sample device
*/
static SAMPLE_DEVICE_T*		g_sample_device;

/**
 * file I/O description for sample device
 *
*/
static struct file_operations g_sample_fops =
{
	.open 	= SAMPLE_Open,
	.release= SAMPLE_Close,
	.ioctl	= SAMPLE_Ioctl,
#ifdef SUPPORT_SAMPLE_DEVICE_READ_WRITE_FOPS
	.read 	= SAMPLE_Read,
	.write 	= SAMPLE_Write,
#else
	.read	= NULL,
	.write	= NULL,
#endif
};

/*========================================================================================
	Implementation Group
========================================================================================*/
int SAMPLE_Init(void)
{
	int			i;
	int			err;
	dev_t		dev;

	/* Get the handle of debug output for sample device.
	 *
	 * Most module should open debug handle before the real initialization of module.
	 * As you know, debug_util offers 4 independent debug outputs for your device driver.
	 * So if you want to use all the debug outputs, you should initialize each debug output
	 * using OS_DEBUG_EnableModuleByIndex() function.
	 */
	g_sample_debug_fd = DBG_OPEN( SAMPLE_MODULE );
	OS_DEBUG_EnableModule ( g_sample_debug_fd );

	OS_DEBUG_EnableModuleByIndex ( g_sample_debug_fd, 0, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_sample_debug_fd, 1, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_sample_debug_fd, 2, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_sample_debug_fd, 3, DBG_COLOR_NONE );

	/* allocate main device handler, register current device.
	 *
	 * If devie major is predefined then register device using that number.
	 * otherwise, major number of device is automatically assigned by Linux kernel.
	 *
	 */
	g_sample_device = (SAMPLE_DEVICE_T*)OS_KMalloc( sizeof(SAMPLE_DEVICE_T)*SAMPLE_MAX_DEVICE );

	if ( NULL == g_sample_device )
	{
		DBG_PRINT_ERROR("out of memory. can't allocate %d bytes\n", sizeof(SAMPLE_DEVICE_T)* SAMPLE_MAX_DEVICE );
		return -ENOMEM;
	}

	memset( g_sample_device, 0x0, sizeof(SAMPLE_DEVICE_T)* SAMPLE_MAX_DEVICE );

	if (g_sample_major)
	{
		dev = MKDEV( g_sample_major, g_sample_minor );
		err = register_chrdev_region(dev, SAMPLE_MAX_DEVICE, SAMPLE_MODULE );
	}
	else
	{
		err = alloc_chrdev_region(&dev, g_sample_minor, SAMPLE_MAX_DEVICE, SAMPLE_MODULE );
		g_sample_major = MAJOR(dev);
	}

	if ( err < 0 )
	{
		DBG_PRINT_ERROR("can't register sample device\n" );
		return -EIO;
	}

	/* TODO : initialize your module not specific minor device */


	/* END */

	for ( i=0; i<SAMPLE_MAX_DEVICE; i++ )
	{
		/* initialize cdev structure with predefined variable */
		dev = MKDEV( g_sample_major, g_sample_minor+i );
		cdev_init( &(g_sample_device[i].cdev), &g_sample_fops );
		g_sample_device[i].devno		= dev;
		g_sample_device[i].cdev.owner = THIS_MODULE;
		g_sample_device[i].cdev.ops   = &g_sample_fops;

		/* TODO: initialize minor device */


		/* END */

		err = cdev_add (&(g_sample_device[i].cdev), dev, 1 );

		if (err)
		{
			DBG_PRINT_ERROR("error (%d) while adding sample device (%d.%d)\n", err, MAJOR(dev), MINOR(dev) );
			return -EIO;
		}
        OS_CreateDeviceClass ( g_sample_device[i].devno, "%s%d", SAMPLE_MODULE, i );
	}

	/* initialize proc system */
	SAMPLE_PROC_Init ( );

	SAMPLE_PRINT("sample device initialized\n");

	return 0;
}

void SAMPLE_Cleanup(void)
{
	int i;
	dev_t dev = MKDEV( g_sample_major, g_sample_minor );

	/* cleanup proc system */
	SAMPLE_PROC_Cleanup( );

	/* remove all minor devicies and unregister current device */
	for ( i=0; i<SAMPLE_MAX_DEVICE;i++)
	{
		/* TODO: cleanup each minor device */


		/* END */
		cdev_del( &(g_sample_device[i].cdev) );
	}

	/* TODO : cleanup your module not specific minor device */

	unregister_chrdev_region(dev, SAMPLE_MAX_DEVICE );

	OS_Free( g_sample_device );
}


///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * open handler for sample device
 *
 */
static int
SAMPLE_Open(struct inode *inode, struct file *filp)
{
    int					major,minor;
    struct cdev*    	cdev;
    SAMPLE_DEVICE_T*	my_dev;

    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, SAMPLE_DEVICE_T, cdev);

    /* TODO : add your device specific code */

	OS_InitTimer( &my_dev->timer1 );
	OS_InitTimer( &my_dev->timer2 );

	OS_InitEvent( &my_dev->test_event );

	/* END */

    my_dev->dev_open_count++;
    filp->private_data = my_dev;

	/* some debug */
    major = imajor(inode);
    minor = iminor(inode);
    SAMPLE_PRINT("device opened (%d:%d)\n", major, minor );

    return 0;
}

/**
 * release handler for sample device
 *
 */
static int
SAMPLE_Close(struct inode *inode, struct file *file)
{
    int					major,minor;
    SAMPLE_DEVICE_T*	my_dev;
    struct cdev*		cdev;

    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, SAMPLE_DEVICE_T, cdev);

    if ( my_dev->dev_open_count > 0 )
    {
        --my_dev->dev_open_count;
    }

    /* TODO : add your device specific code */

	OS_StopTimer( &my_dev->timer1 );
	OS_StopTimer( &my_dev->timer2 );

	/* END */

	/* some debug */
    major = imajor(inode);
    minor = iminor(inode);
    SAMPLE_PRINT("device closed (%d:%d)\n", major, minor );
    return 0;
}

/**
 * ioctl handler for sample device.
 *
 *
 * note: if you have some critial data, you should protect them using semaphore or spin lock.
 */
static int
SAMPLE_Ioctl ( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
{
    int err = 0, ret = 0;
	//int tmp;

    SAMPLE_DEVICE_T*	my_dev;
    struct cdev*		cdev;

	/*
	 * get current sample device object
	 */
    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, SAMPLE_DEVICE_T, cdev);

    /*
     * check if IOCTL command is valid or not.
     * - if magic value doesn't match, return error (-ENOTTY)
     * - if command is out of range, return error (-ENOTTY)
     *
     * note) -ENOTTY means "Inappropriate ioctl for device.
     */
    if (_IOC_TYPE(cmd) != SAMPLE_IOC_MAGIC)
    {
    	DBG_PRINT_WARNING("invalid magic. magic=0x%02X\n", _IOC_TYPE(cmd) );
    	return -ENOTTY;
    }
    if (_IOC_NR(cmd) > SAMPLE_IOC_MAXNR)
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

	SAMPLE_TRACE("cmd = %08X (cmd_idx=%d)\n", cmd, _IOC_NR(cmd) );

	switch(cmd)
	{
		/*-----------------------------------------------------------------------------------------
		 * SIMPLE DATA I/O TEST
		 *
		 *
		 *
		 *-----------------------------------------------------------------------------------------*/
		case SAMPLE_IO_RESET:
		{
			SAMPLE_PRINT("reset ok\n");
			ret = 0;
		}
		break;

		case SAMPLE_IOR_CHIP_REV_INFO:
		{
			CHIP_REV_INFO_T	rev_info;

			rev_info.version = 0x100;
			rev_info.date[0] =  9;	/* 2009/11/24 */
			rev_info.date[0] = 11;
			rev_info.date[0] = 24;

			SAMPLE_PRINT("rev_info (%0X, %d:%d:%d)\n", rev_info.version,
														rev_info.date[0], rev_info.date[1], rev_info.date[2] );

	        if ( copy_to_user((void __user *)arg, &rev_info, sizeof(CHIP_REV_INFO_T)) )
    	        return -EFAULT;

			ret = 0;
		}
		break;

		case SAMPLE_IOW_WRITE_UINT32:
		{
			UINT32	data;

			ret = __get_user( data, (UINT32 __user *)arg );

			SAMPLE_PRINT("data = 0x%X\n", data );

			ret = 0;
		}
		break;

		/*-----------------------------------------------------------------------------------------
		 * TIMER TEST
		 *
		 *
		 *
		 *-----------------------------------------------------------------------------------------*/
		case SAMPLE_IOW_TEST_TIMEOUT:
		{
			UINT32	msec;

			ret = __get_user( msec, (UINT32 __user *)arg );

			SAMPLE_PRINT("testing %d msec timeout\n", msec );

			OS_StartTimer( &my_dev->timer1, _SAMPLE_TestTimeOut, OS_TIMER_TIMEOUT, msec, (UINT32)my_dev );
			ret = 0;
		}
		break;

		case SAMPLE_IOW_TEST_TIMETICK:
		{
			UINT32	msec;

			ret = __get_user( msec, (UINT32 __user *)arg );

			SAMPLE_PRINT("testing %d msec timetick\n", msec );

			OS_StartTimer( &my_dev->timer2, _SAMPLE_TestTimeTick, OS_TIMER_TIMETICK, msec, (UINT32)my_dev );
			ret = 0;
		}
		break;

		/*-----------------------------------------------------------------------------------------
		 * EVENT TEST
		 *
		 *
		 *
		 *-----------------------------------------------------------------------------------------*/
		case SAMPLE_IO_TEST_CLR_EVENT:
		{
			OS_ClearEvent( &my_dev->test_event );

			SAMPLE_TRACE("SAMPLE_IO_TEST_CLR_EVENT OK\n");
		}
		break;

		case SAMPLE_IOW_TEST_SEND_EVENT:
		{
			//int		ret;
			LX_SAMPLE_TEST_EVENT_WRITER_T	param;

	        if ( __copy_from_user( &param, (void __user *)arg, sizeof(LX_SAMPLE_TEST_EVENT_WRITER_T)) )
    	        return -EFAULT;

			OS_SendEvent( &my_dev->test_event, param.ev );

			SAMPLE_TRACE("SAMPLE_IOW_TEST_SEND_EVENT OK\n");
		}
		break;

		case SAMPLE_IORW_TEST_RECV_EVENT:
		{
			int		ret;
			UINT32	option;
			LX_SAMPLE_TEST_EVENT_READER_T	param;

	        if ( __copy_from_user( &param, (void __user *)arg, sizeof(LX_SAMPLE_TEST_EVENT_READER_T)) )
    	        return -EFAULT;

			option = (param.option==LX_SAMPLE_TEST_EVENT_READ_ANY)? OS_EVENT_RECEIVE_ANY : OS_EVENT_RECEIVE_ALL;

			SAMPLE_TRACE("SAMPLE_IORW_TEST_RECV_EVENT BEGIN - ev:%08X, opt:%d, tm:%d\n", param.ev, option, param.timeout );

			ret = OS_RecvEvent( &my_dev->test_event, param.ev, &param.rev, option, param.timeout );

			if ( ret != RET_OK )
			{
				SAMPLE_TRACE("event not recived (timeout)\n");
				return -EFAULT;
			}

	        if ( __copy_to_user( (void __user *)arg, &param, sizeof(LX_SAMPLE_TEST_EVENT_READER_T)) )
    	        return -EFAULT;

			SAMPLE_TRACE("SAMPLE_IORW_TEST_RECV_EVENT OK\n");
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

/*========================================================================================
	TEST FUNCTION IMPLEMTATION
========================================================================================*/
static void		_SAMPLE_TestTimeOut ( ULONG data )
{
	//SAMPLE_DEVICE_T*	my_dev = (SAMPLE_DEVICE_T*)data;

	SAMPLE_PRINT("[TIMEOUT] !!!!!\n");
}

static void		_SAMPLE_TestTimeTick( ULONG data )
{
	static int count = 0;

    SAMPLE_DEVICE_T*	my_dev = (SAMPLE_DEVICE_T*)data;

	SAMPLE_PRINT("[TIMETICK] %3d !!!!!\n", count++ );

	if ( (count%10) == 0 )
	{
		SAMPLE_PRINT("[TIMETICK] STOP..\n");
		OS_StopTimer( &(my_dev->timer2) );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef KDRV_GLOBAL_LINK
#if defined(CONFIG_LG_BUILTIN_KDRIVER) && defined(CONFIG_LGSNAP)
user_initcall_grp("kdrv",SAMPLE_Init);
#else
module_init(SAMPLE_Init);
#endif
module_exit(SAMPLE_Cleanup);

MODULE_AUTHOR("LGE");
MODULE_DESCRIPTION("base driver");
MODULE_LICENSE("GPL");
#endif

/** @} */

