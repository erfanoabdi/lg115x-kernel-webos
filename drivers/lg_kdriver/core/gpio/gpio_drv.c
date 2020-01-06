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
 *  main driver implementation for gpio device.
 *	gpio device will teach you how to make device driver with new platform.
 *
 *  author		ingyu.yang (ingyu.yang@lge.com)
 *  				jun.kong (jun.kong@lge.com)
 *  version		1.0
 *  date		2009.12.30
 *  note		Additional information.
 *
 *  @addtogroup lg1150_gpio
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/
#undef	SUPPORT_GPIO_DEVICE_READ_WRITE_FOPS
#undef	GPIO_DRV_PRINT_ENABLE
//#define GPIO_DRV_PRINT_ENABLE
//static void Debug_GPIO_Print(void);

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#ifdef KDRV_CONFIG_PM	// added by SC Jung for quick booting
#include <linux/platform_device.h>
#endif
#include <asm/uaccess.h>
#include <linux/poll.h>
#include "os_util.h"
#include "base_device.h"
#include "gpio_drv.h"
#include "gpio_reg.h"
#include "gpio_core.h"

#include <linux/irq.h>
#include <linux/interrupt.h>
/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
#define GPIO_COPY_FROM_USER(d,s,l) 							\
		do {												\
			if (copy_from_user((void*)d, (void *)s, l)) {	\
				GPIO_ERROR("ioctl: copy_from_user\n");		\
				return -EFAULT; 							\
			}												\
		} while(0)

#define GPIO_COPY_TO_USER(d,s,l) 							\
		do {												\
			if (copy_to_user((void*)d, (void *)s, l)) { 	\
				GPIO_ERROR("ioctl: copy_to_user\n");		\
				return -EFAULT; 							\
			}												\
		} while(0)



/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/
/**
 *	main control block for gpio device.
 *	each minor device has unique control block
 *
 */
typedef struct GPIO_DEVICE_t
{
// BEGIN of common device
	int						dev_open_count;		///< check if device is opened or not
	dev_t					devno;			///< device number
	struct cdev				cdev;			///< char device structure
// END of command device

// BEGIN of device specific data
	OS_SEM_T				mutex;
// END of device specific data
}
GPIO_DEVICE_T;


#ifdef KDRV_CONFIG_PM
typedef struct
{
	// add here extra parameter
	bool			is_suspended;
}GPIO_DRVDATA_T;
#endif




GPIO_INTR_CALLBACK_T _gpio_isr_func[GPIO_PIN_MAX]= {{NULL},};

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
extern	void	GPIO_PROC_Init(void);
extern	void	GPIO_PROC_Cleanup(void);

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions
----------------------------------------------------------------------------------------*/
int		GPIO_Init(void);
void	GPIO_Cleanup(void);

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
int		g_gpio_debug_fd;
int 	g_gpio_major = GPIO_MAJOR;
int 	g_gpio_minor = GPIO_MINOR;

int gpio_intr_pin = 0;
int gpio_intr_pin_value = 0;

spinlock_t gpioPoll_lock;
DECLARE_WAIT_QUEUE_HEAD(gGPIOPollWaitQueue);

UINT32 		gpio_intr_num[GPIO_IRQ_NUM_NR] ={M14_IRQ_GPIO0_2,M14_IRQ_GPIO3_5,M14_IRQ_GPIO6_8,M14_IRQ_GPIO9_11,M14_IRQ_GPIO12_14,M14_IRQ_GPIO15_17};
UINT8 *		gpio_intr_num_str[GPIO_IRQ_NUM_NR] ={

"gpioarray_00_02",
"gpioarray_03_05",
"gpioarray_06_08",
"gpioarray_09_11",
"gpioarray_12_14",
"gpioarray_15_17",
};

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static int      GPIO_Open(struct inode *, struct file *);
static int      GPIO_Close(struct inode *, struct file *);
static long		GPIO_Ioctl (struct file *file, unsigned int cmd, unsigned long arg);

#ifdef SUPPORT_GPIO_DEVICE_READ_WRITE_FOPS
static ssize_t  GPIO_Read(struct file *, char *, size_t, loff_t *);
static ssize_t  GPIO_Write(struct file *, const char *, size_t, loff_t *);
#endif

static unsigned int GPIO_Poll(struct file *filp, poll_table *wait);


/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/

/**
 * main control block for gpio device
*/
static GPIO_DEVICE_T*		g_gpio_device;

/**
 * file I/O description for gpio device
 *
*/
static struct file_operations g_gpio_fops =
{
	.open 	= GPIO_Open,
	.release= GPIO_Close,
	.unlocked_ioctl	= GPIO_Ioctl,
#ifdef SUPPORT_GPIO_DEVICE_READ_WRITE_FOPS
	.read 	= GPIO_Read,
	.write 	= GPIO_Write,
#else
	.read	= NULL,
	.write	= NULL,
#endif
	.poll	= GPIO_Poll,
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
 * @return	int 0 : OK, -1 : NOT OK
 *
 */
static int GPIO_suspend(struct platform_device *pdev, pm_message_t state)
{
	GPIO_DRVDATA_T	*drv_data;
	drv_data = platform_get_drvdata(pdev);

	GPIO_DevSuspend();

	drv_data->is_suspended = 1;
	GPIO_PRINT("[%s] done suspend\n", GPIO_MODULE);

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
static int GPIO_resume(struct platform_device *pdev)
{
	GPIO_DRVDATA_T	*drv_data;

	drv_data = platform_get_drvdata(pdev);
	if(drv_data->is_suspended == 0) return -1;

	GPIO_DevResume();

	drv_data->is_suspended = 0;
	GPIO_PRINT("[%s] done resume\n", GPIO_MODULE);

	return 0;
}
/**
 *
 * probing module.
 *
 * @param	struct platform_device *pdev
 * @return	int 0 : OK, -1 : NOT OK
 *
 */
 int  GPIO_probe(struct platform_device *pdev)
{

	GPIO_DRVDATA_T *drv_data;

	drv_data = (GPIO_DRVDATA_T *)kmalloc(sizeof(GPIO_DRVDATA_T) , GFP_KERNEL);

	// add here driver registering code & allocating resource code

	GPIO_PRINT("[%s] done probe\n", GPIO_MODULE);
	drv_data->is_suspended = 0;
	platform_set_drvdata(pdev, drv_data);

	return 0;
}


/**
 *
 * module remove function, this function will be called in rmmod gpio module
 *
 * @param	struct platform_device
 * @return	int 0 : OK , -1 : NOT OK
 *
 */
static int  GPIO_remove(struct platform_device *pdev)
{
	GPIO_DRVDATA_T *drv_data;

	// add here driver unregistering code & deallocating resource code

	drv_data = platform_get_drvdata(pdev);
	kfree(drv_data);

	GPIO_PRINT("released\n");

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
static void  GPIO_release(struct device *dev)
{
	GPIO_PRINT("device released\n");
}

/*
 *	module platform driver structure
 */
static struct platform_driver gpio_driver =
{
	.probe		= GPIO_probe,
	.suspend	= GPIO_suspend,
	.remove		= GPIO_remove,
	.resume		= GPIO_resume,
	.driver		=
	{
		.name	= GPIO_MODULE,
	},
};

static struct platform_device gpio_device = {
	.name = GPIO_MODULE,
	.id = 0,
	.id = -1,
	.dev = {
		.release = GPIO_release,
	},
};
#endif

/** Initialize the device environment before the real H/W initialization
 *
 *  @note main usage of this function is to initialize the HAL layer and memory size adjustment
 *  @note it's natural to keep this function blank :)
 */
void GPIO_PreInit(void)
{
    /* TODO: do something */
}

int GPIO_Init(void)
{
	int			i;
	int			err;
	dev_t		dev;

	/* Get the handle of debug output for gpio device.
	 *
	 * Most module should open debug handle before the real initialization of module.
	 * As you know, debug_util offers 4 independent debug outputs for your device driver.
	 * So if you want to use all the debug outputs, you should initialize each debug output
	 * using OS_DEBUG_EnableModuleByIndex() function.
	 */
	g_gpio_debug_fd = DBG_OPEN( GPIO_MODULE );
	if(g_gpio_debug_fd < 0) return -1;


	OS_DEBUG_EnableModule ( g_gpio_debug_fd );

#if 0
	OS_DEBUG_EnableModuleByIndex ( g_gpio_debug_fd, GPIO_MSG_TRACE, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_gpio_debug_fd, GPIO_MSG_TRACE, DBG_COLOR_NONE );
	OS_DEBUG_EnableModuleByIndex ( g_gpio_debug_fd, GPIO_MSG_DEBUG, DBG_COLOR_NONE );
#endif
	OS_DEBUG_EnableModuleByIndex ( g_gpio_debug_fd, GPIO_MSG_ERROR, DBG_COLOR_RED );

	/* allocate main device handler, register current device.
	 *
	 * If devie major is predefined then register device using that number.
	 * otherwise, major number of device is automatically assigned by Linux kernel.
	 *
	 */
#ifdef KDRV_CONFIG_PM
	// added by SC Jung for quick booting
	if(platform_driver_register(&gpio_driver) < 0)
	{
		GPIO_PRINT("[%s] platform driver register failed\n",GPIO_MODULE);
	}
	else
	{
		if(platform_device_register(&gpio_device))
		{
			platform_driver_unregister(&gpio_driver);
			GPIO_PRINT("[%s] platform device register failed\n",GPIO_MODULE);
		}
		else
		{
			GPIO_PRINT("[%s] platform register done\n", GPIO_MODULE);
		}
	}
#endif

	GPIO_DevInit();

	g_gpio_device = (GPIO_DEVICE_T*)OS_KMalloc( sizeof(GPIO_DEVICE_T)*GPIO_MAX_DEVICE );

	if ( NULL == g_gpio_device )
	{
		DBG_PRINT_ERROR("out of memory. can't allocate %d bytes\n", sizeof(GPIO_DEVICE_T)* GPIO_MAX_DEVICE );
		return -ENOMEM;
	}

	memset( g_gpio_device, 0x0, sizeof(GPIO_DEVICE_T)* GPIO_MAX_DEVICE );

	if (g_gpio_major)
	{
		dev = MKDEV( g_gpio_major, g_gpio_minor );
		err = register_chrdev_region(dev, GPIO_MAX_DEVICE, GPIO_MODULE );
	}
	else
	{
		err = alloc_chrdev_region(&dev, g_gpio_minor, GPIO_MAX_DEVICE, GPIO_MODULE );
		g_gpio_major = MAJOR(dev);
	}

	if ( err < 0 )
	{
		DBG_PRINT_ERROR("can't register gpio device\n" );
		return -EIO;
	}

	/* TODO : initialize your module not specific minor device */


	/* END */

	for ( i=0; i<GPIO_MAX_DEVICE; i++ )
	{
		/* initialize cdev structure with predefined variable */
		dev = MKDEV( g_gpio_major, g_gpio_minor+i );
		cdev_init( &(g_gpio_device[i].cdev), &g_gpio_fops );
		g_gpio_device[i].devno		= dev;
		g_gpio_device[i].cdev.owner = THIS_MODULE;
		g_gpio_device[i].cdev.ops   = &g_gpio_fops;

		/* TODO: initialize minor device */


		/* END */

		err = cdev_add (&(g_gpio_device[i].cdev), dev, 1 );

		if (err)
		{
			DBG_PRINT_ERROR("error (%d) while adding gpio device (%d.%d)\n", err, MAJOR(dev), MINOR(dev) );
			return -EIO;
		}
        OS_CreateDeviceClass ( g_gpio_device[i].devno, "%s%d", GPIO_MODULE, i );
	}




	for( i= 0; i < GPIO_IRQ_NUM_NR; i++)
	{
		//Initialize IRQ0 of ADEC DSP0
		err = request_irq(gpio_intr_num[i], (irq_handler_t)GPIO_interrupt, 0, gpio_intr_num_str[i], NULL);
		if (err)
		{
			DBG_PRINT_ERROR("request_irq IRQ_AUD0 in %s is failed %d\n", "GPIO0 ", err);
			return -1;
		}

	}

	/* initialize proc system */
	GPIO_PROC_Init ( );

	GPIO_PRINT("gpio device initialized\n");

	return 0;
}

void GPIO_Cleanup(void)
{
	int i;
	dev_t dev = MKDEV( g_gpio_major, g_gpio_minor );

#ifdef KDRV_CONFIG_PM
	// added by SC Jung for quick booting
	platform_driver_unregister(&gpio_driver);
	platform_device_unregister(&gpio_device);
#endif

	/* cleanup proc system */
	GPIO_PROC_Cleanup( );

	/* remove all minor devicies and unregister current device */
	for ( i=0; i<GPIO_MAX_DEVICE;i++)
	{
		/* TODO: cleanup each minor device */


		/* END */
		cdev_del( &(g_gpio_device[i].cdev) );
	}

	/* TODO : cleanup your module not specific minor device */

	unregister_chrdev_region(dev, GPIO_MAX_DEVICE );

	OS_Free( g_gpio_device );
}


///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * open handler for gpio device
 *
 */
static int GPIO_Open(struct inode *inode, struct file *filp)
{
    int					major,minor;
    struct cdev*    	cdev;
    GPIO_DEVICE_T*	my_dev;

    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, GPIO_DEVICE_T, cdev);

    /* TODO : add your device specific code */


	/* END */

	if(my_dev->dev_open_count == 0)
	{

	}

    my_dev->dev_open_count++;
    filp->private_data = my_dev;

	/* some debug */
    major = imajor(inode);
    minor = iminor(inode);
    GPIO_PRINT("device opened (%d:%d)\n", major, minor );

    return 0;
}

/**
 * release handler for gpio device
 *
 */
static int GPIO_Close(struct inode *inode, struct file *file)
{
    int					major,minor;
    GPIO_DEVICE_T*	my_dev;
    struct cdev*		cdev;

    cdev	= inode->i_cdev;
    my_dev	= container_of ( cdev, GPIO_DEVICE_T, cdev);

    if ( my_dev->dev_open_count > 0 )
    {
        --my_dev->dev_open_count;
    }

    /* TODO : add your device specific code */

	/* END */

	/* some debug */
    major = imajor(inode);
    minor = iminor(inode);
    GPIO_PRINT("device closed (%d:%d)\n", major, minor );
    return 0;
}



/**
 * ioctl handler for gpio device.
 *
 *
 * note: if you have some critial data, you should protect them using semaphore or spin lock.
 */
static long GPIO_Ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	GPIO_DEVICE_T*	gpio_dev;
	LX_GPIO_PARAM_T param;
	int err = 0, ret = 0;

	/*
	 * get current gpio device object
	 */
	gpio_dev = (GPIO_DEVICE_T*)file->private_data;

    /*
     * check if IOCTL command is valid or not.
     * - if magic value doesn't match, return error (-ENOTTY)
     * - if command is out of range, return error (-ENOTTY)
     *
     * note) -ENOTTY means "Inappropriate ioctl for device.
     */
    if (_IOC_TYPE(cmd) != GPIO_IOC_MAGIC)
    {
    	DBG_PRINT_WARNING("invalid magic. magic=0x%02X\n", _IOC_TYPE(cmd) );
    	return -ENOTTY;
    }
    if (_IOC_NR(cmd) > GPIO_IOC_MAXNR)
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

	GPIO_TRACE("cmd = %08X (cmd_idx=%d)\n", cmd, _IOC_NR(cmd) );

	switch(cmd)
	{
		case GPIO_IOW_COMMAND_SET:
		{
			GPIO_TRACE("GPIO_IOW_COMMAND_SET\n");
			GPIO_COPY_FROM_USER(&param, arg, sizeof(LX_GPIO_PARAM_T));
			switch(param.command)
			{
				case LX_GPIO_COMMAND_PIN_MUX:
					ret = GPIO_DevSetPinMux(param.pin_number, (BOOLEAN)param.data);
					break;
				case LX_GPIO_COMMAND_MODE:
					ret = GPIO_DevSetMode(param.pin_number, (LX_GPIO_MODE_T)param.data);
					break;
				case LX_GPIO_COMMAND_VALUE:
					ret = GPIO_DevSetValue(param.pin_number, (LX_GPIO_VALUE_T)param.data);
					break;
				case LX_GPIO_COMMAND_ISR:
					ret = GPIO_DevSetPinMux(param.pin_number, (BOOLEAN)param.data);
					ret = GPIO_DevSetISR(param.pin_number, param.cb.pfnGPIO_CB, param.intr_enable);
					break;

				default: GPIO_ERROR("GPIO_IOW_COMMAND_SET: unknown\n"); return -EFAULT;
			}
			break;
		}

		case GPIO_IORW_COMMAND_GET:
		{
			GPIO_TRACE("GPIO_IORW_COMMAND_GET\n");

			GPIO_COPY_FROM_USER(&param, arg, sizeof(LX_GPIO_PARAM_T));
			switch(param.command)
			{
				case LX_GPIO_COMMAND_PIN_MUX:
					ret = GPIO_DevGetPinMux(param.pin_number, (BOOLEAN*)&param.data);
					break;
				case LX_GPIO_COMMAND_MODE:
					ret = GPIO_DevGetMode(param.pin_number, (LX_GPIO_MODE_T*)&param.data);
					break;
				case LX_GPIO_COMMAND_VALUE:
					ret = GPIO_DevGetValue(param.pin_number, (LX_GPIO_VALUE_T*)&param.data);
					break;
				case LX_GPIO_COMMAND_INTR_VALUE:
					ret = GPIO_DevGetIntrValue((UINT32*)&param.pin_number, (LX_GPIO_VALUE_T*)&param.pin_value);
					break;


				default: GPIO_ERROR("GPIO_IORW_COMMAND_GET: unknown\n"); return -EFAULT;
			}
			GPIO_COPY_TO_USER(arg, &param, sizeof(LX_GPIO_PARAM_T));
			break;
		}

		case GPIO_IOW_EX_COMMAND_SET:
		{
			GPIO_TRACE("GPIO_IOW_EX_COMMAND_SET\n");
			GPIO_COPY_FROM_USER(&param, arg, sizeof(LX_GPIO_PARAM_T));
			switch(param.command)
			{
				case LX_GPIO_COMMAND_PIN_MUX:
					ret = GPIO_DevExSetPinMux(param.pin_number, (BOOLEAN)param.data);
					break;
				case LX_GPIO_COMMAND_MODE:
					ret = GPIO_DevExSetMode(param.pin_number, (LX_GPIO_MODE_T)param.data);
					break;
				case LX_GPIO_COMMAND_VALUE:
					ret = GPIO_DevExSetValue(param.pin_number, (LX_GPIO_VALUE_T)param.data);
					break;
				default: GPIO_ERROR("GPIO_IOW_EX_COMMAND_SET: unknown\n"); return -EFAULT;
			}
			break;
		}

		case GPIO_IORW_EX_COMMAND_GET:
		{
			GPIO_TRACE("GPIO_IORW_EX_COMMAND_GET\n");
			GPIO_COPY_FROM_USER(&param, arg, sizeof(LX_GPIO_PARAM_T));
			switch(param.command)
			{
				case LX_GPIO_COMMAND_PIN_MUX:
					ret = GPIO_DevExGetPinMux(param.pin_number, (BOOLEAN*)&param.data);
					break;
				case LX_GPIO_COMMAND_MODE:
					ret = GPIO_DevExGetMode(param.pin_number, (LX_GPIO_MODE_T*)&param.data);
					break;
				case LX_GPIO_COMMAND_VALUE:
					ret = GPIO_DevExGetValue(param.pin_number, (LX_GPIO_VALUE_T*)&param.data);
					break;
				default: GPIO_ERROR("GPIO_IORW_EX_COMMAND_GET: unknown\n"); return -EFAULT;
			}
			GPIO_COPY_TO_USER(arg, &param, sizeof(LX_GPIO_PARAM_T));
			break;
		}

	    default:
	    {
	    	GPIO_ERROR("ioctl: default\n");
			/* redundant check but it seems more readable */
    	    ret = -ENOTTY;
		}
    }

    return ret;
}

static unsigned int GPIO_Poll(struct file *filp, poll_table *wait)
{
	UINT8	i;

	//GPIO_ERROR("Audio Poll wait!!!\n");

	poll_wait(filp, &gGPIOPollWaitQueue, wait);

	//Set a audio GET event type for next event.
	if (gpio_intr_pin != 0)
	{
	 	return POLLIN;
	}
	else
	   	return 0;
}

static void _GPIO_ISR_GPIO0Hdr( unsigned int ui32IRQNum, unsigned int IntData )
{
	UINT32	gpio_array_n;

	UINT32	ui32Status = 0,ui32Status0 = 0,ui32Status1 = 0,ui32Status2 =0;
	UINT8	ucGpio = 0, realGpio = 0;
	ULONG flags = 0;

	//GPIO_ERROR("_GPIO_ISR_GPIO0Hdr [%d] \n", ui32IRQNum);


	LX_GPIO_VALUE_T value =LX_GPIO_VALUE_INVALID ;

	if (ui32IRQNum >= M14_IRQ_GPIO0_2 && ui32IRQNum <=M14_IRQ_GPIO9_11)
		gpio_array_n = ui32IRQNum-M14_IRQ_GPIO0_2;
	else if(ui32IRQNum >= M14_IRQ_GPIO12_14 && ui32IRQNum <=M14_IRQ_GPIO15_17)
		gpio_array_n = ui32IRQNum-M14_IRQ_GPIO12_14;
	else
	{
		GPIO_ERROR("this intr is not a gpio intr\n");
		return;
	}


	switch( ui32IRQNum )
	{
		case M14_IRQ_GPIO0_2 :
			gpio_array_n = 0;
			break;
		case M14_IRQ_GPIO3_5 :
			gpio_array_n = 3;
			break;
		case M14_IRQ_GPIO6_8 :
			gpio_array_n = 6;
			break;
		case M14_IRQ_GPIO9_11 :
			gpio_array_n = 9;
			break;
		case M14_IRQ_GPIO12_14 :
			gpio_array_n = 12;
			break;
		case M14_IRQ_GPIO15_17 :
			gpio_array_n = 15;
			break;

	}

	ui32Status0 = GPIONMIS(gpio_array_n);
	ui32Status1 = GPIONMIS(gpio_array_n + 1);
	ui32Status2 = GPIONMIS(gpio_array_n + 2);

	//GPIO_ERROR("ui32Status [%x][%x][%x][%x][%x][%x][%x][%x] \n", GPIONRIS(0),GPIONRIS(1),GPIONRIS(2),GPIONRIS(3),GPIONRIS(4),GPIONRIS(5),GPIONRIS(6),GPIONRIS(7));

	/* Clear the interrupt */


	if(ui32Status0 != 0)
	{
		ui32Status =ui32Status0;
		gpio_array_n = gpio_array_n;
		//GPIONIC(gpio_array_n) =ui32Status;

	}
	else if(ui32Status1 != 0)
	{
		ui32Status =ui32Status1;
		gpio_array_n = gpio_array_n +1 ;

	}
	else if(ui32Status2 != 0)
	{
		ui32Status =ui32Status2;
		gpio_array_n = gpio_array_n + 2;

	}
	//GPIO_ERROR("ui32Status ui32IRQNum[%d] [%x][%x][%x][%x][%x] [%d] [%x][%x][%x] \n",ui32IRQNum ,GPIONMIS(0),GPIONMIS(1),GPIONMIS(2),GPIONMIS(3),GPIONMIS(17),gpio_array_n,ui32Status0,ui32Status1,ui32Status2);

	GPIONIC(gpio_array_n) =ui32Status;

	for ( ucGpio = 0; ucGpio <= GPIO_NUM_IN_ARRAY; ++ucGpio )
	{
		if ( 0 == ui32Status ) break;

		/* Check for gpio interrupt */
		if ( ui32Status & ( 1 << ucGpio ) )
		{
			/* Check if the handler mask for the gpio is enabled */
			if ( GPIONIE(gpio_array_n) & ( 1 << ucGpio ) )
			{
				/* invoked the registered callback routine */
				//if ( _gpio_isr_func [ ucGpio ].pfnGPIO_CB )
				{
					if( (lx_chip_rev() < LX_CHIP_REV(M14, A0)) &&
						(lx_chip_rev() >= LX_CHIP_REV(H13, B0)) &&
						ui32IRQNum == M14_IRQ_GPIO15_17  &&
						(ui32Status2 == ui32Status)		//H13 GPIOBASE and gpiobase17 :136 ~143
						)
					{
						realGpio = (7	- ucGpio) + 8*(gpio_array_n);
						_GPIO_GetValue_H13Bx(realGpio, &value );
						GPIO_ERROR("_GPIO_ISR_GPIO0Hdr h13b0 pin[%d] value[%d]\n",realGpio, value);
					}
					else
					{
						realGpio = ucGpio + 8*(gpio_array_n);
						_GPIO_GetValue(realGpio, &value );
						GPIO_ERROR("_GPIO_ISR_GPIO0Hdr 		pin[%d] value[%d]\n",realGpio, value);
					}



					spin_lock_irqsave(&gpioPoll_lock, flags);
					gpio_intr_pin = realGpio + 1;
					gpio_intr_pin_value = value;
					spin_unlock_irqrestore(&gpioPoll_lock, flags);

					if(gpio_intr_pin)
					{
						wake_up_interruptible_all(&gGPIOPollWaitQueue);
					}

					//_gpio_isr_func [ ucGpio ].pfnGPIO_CB( 1 );
				}
			}

			/* Clear the serviced status */
			ui32Status ^= ( 1 << ucGpio );
			//GPIONIC(gpio_array_n) =ui32Status;
		}
	}
}

irqreturn_t GPIO_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int intr_data;
	_GPIO_ISR_GPIO0Hdr(irq,intr_data );
	return IRQ_HANDLED;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef KDRV_GLOBAL_LINK
#if defined(CONFIG_LG_BUILTIN_KDRIVER) && defined(CONFIG_LGSNAP)
user_initcall_grp("kdrv",GPIO_Init);
#else
module_init(GPIO_Init);
#endif
module_exit(GPIO_Cleanup);

MODULE_AUTHOR("LGE");
MODULE_DESCRIPTION("gpio driver");
MODULE_LICENSE("GPL");



#endif

/** @} */

