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
 *  Linux proc interface for sample device.
 *	sample device will teach you how to make device driver with new platform.
 *
 *  author		root
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

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "sample_drv.h"
#include "proc_util.h"
#include "debug_util.h"
#include "os_util.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/
enum {
	PROC_ID_AUTHOR	= 0,
	PROC_ID_COMMAND,
	PROC_ID_TEST_MUTEX_LOCK,
	PROC_ID_TEST_MUTEX_UNLOCK,
	PROC_ID_TEST_MEMTEST,
	PROC_ID_MAX,
};

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static	OS_SEM_T	_g_test_sem;

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
static OS_PROC_DESC_TABLE_T	_g_sample_device_proc_table[] =
{
	{ "author",			PROC_ID_AUTHOR  , OS_PROC_FLAG_READ },
	{ "command",		PROC_ID_COMMAND , OS_PROC_FLAG_WRITE },
	{ "lock_mutex",		PROC_ID_TEST_MUTEX_LOCK , OS_PROC_FLAG_WRITE },
	{ "unlock_mutex",	PROC_ID_TEST_MUTEX_UNLOCK , OS_PROC_FLAG_WRITE },
	{ "memtest",		PROC_ID_TEST_MEMTEST, OS_PROC_FLAG_WRITE },
	{ NULL, 			PROC_ID_MAX		, 0 }
};

/*========================================================================================
	Implementation Group
========================================================================================*/

/*
 * read_proc implementation of sample device
 *
*/
static int	_SAMPLE_ReadProcFunction( UINT32 procId, char* buffer )
{
	int		ret;

	/* TODO: add your proc_write implementation */
	switch( procId )
	{
		case PROC_ID_AUTHOR:
		{
			ret = sprintf( buffer, "%s\n", "root" );
		}
		break;

		default:
		{
			ret = sprintf( buffer, "%s(%d)\n", "unimplemented read proc",procId );
		}
	}

	return ret;
}

/*
 * write_proc implementation of sample device
 *
*/
static int _SAMPLE_WriteProcFunction( UINT32 procId, char* command )
{
	printk("<!> input string : %s\n", command );

	/* TODO: add your proc_write implementation */
	switch( procId )
	{
		case PROC_ID_COMMAND:
		{
			printk("command string : %s\n", command );
		}
		break;

		case PROC_ID_TEST_MUTEX_LOCK:
		{
			int		ret;
			int		timeout;

			sscanf(command, " %d", &timeout );

			printk("Locking mutex with timeout %d\n", timeout );

			ret = OS_LockMutexEx( &_g_test_sem, timeout );

			printk("Locking mutex - %s\n", (ret==RET_TIMEOUT)? "Timeout Error": (ret)? "Error": "OK" );
		}
		break;

		case PROC_ID_TEST_MUTEX_UNLOCK:
		{
			OS_UnlockMutex( &_g_test_sem );
		}
		break;

		case PROC_ID_TEST_MEMTEST:
		{
			OS_RGN_T		memMgr;
			OS_RGN_INFO_T	memInfo;
			UINT32*			ptr_list;

			UINT32		phys_addr;
			int			block_size;
			int			block_num;
			int			max_num;
			int			req_size;
			int			i;
			int			alloc_cnt = 0;

			sscanf( command, " %x %d %d %d %d", &phys_addr, &block_size, &block_num, &max_num, &req_size );

			printk("phys_addr %p, block size %d, block_num %d, max_num %d, req_size %d\n", (void *)phys_addr, block_size, block_num, max_num, req_size );

			ptr_list = (UINT32*)OS_KMalloc( sizeof(char*) * max_num );
			memset( ptr_list, 0x0, sizeof(void*) * max_num );
//			OS_InitRegionEx( &memMgr, (void*)phys_addr, block_size, block_num );
			OS_InitRegion( &memMgr, (void*)phys_addr, 0x400000 );

			printk("++++ manager crated (%d byte)\n", memMgr.mem_pool_size );

			for ( i=0; i< max_num ;i++ )
			{
				ptr_list[i] = (UINT32)OS_MallocRegion ( &memMgr, req_size );

				if ( ptr_list[i] )
				{
					printk("[%4d] phys_addr : 0x%p : alloc\n", i, (void *)ptr_list[i] );
				}
				else
				{
					printk("can't allocate memory...\n");
					break;
				}
			}

			OS_GetRegionInfo ( &memMgr, &memInfo );

			printk("++++ Memory Status +++++\n");
			printk(" phys_addr = %p\n", memInfo.phys_mem_addr );
			printk(" length    = %10d KByte\n", memInfo.length >> 10 );
			printk(" block_size= %10d KByte\n", memInfo.block_size >> 10 );
			printk(" alloc_cnt = %10d \n", memInfo.mem_alloc_cnt );
			printk(" alloc_mem = %10d KByte\n", memInfo.mem_alloc_size >> 10);
			printk(" free_mem  = %10d KByte\n", memInfo.mem_free_size >> 10 );
			printk("\n");

			OS_MsecSleep( 3*1000 );

			alloc_cnt = memMgr.mem_alloc_cnt;

			for ( i=0; i < alloc_cnt; i++ )
			{
				printk("[%4d] phys_addr : 0x%p : free\n", i, (void *)ptr_list[i] );
				OS_FreeRegion ( &memMgr, (void*)ptr_list[i] );
			}

			OS_KFree( ptr_list );

			OS_CleanupRegion ( &memMgr );
		}
		break;

		default:
		{
			/* do nothing */
		}
		break;
	}

	return strlen(command);
}

/**
 * initialize proc utility for sample device
 *
 * @see SAMPLE_Init
*/
void	SAMPLE_PROC_Init (void)
{
	/* initialize sample mutex */
    OS_InitMutex(&_g_test_sem, OS_SEM_ATTR_DEFAULT);

	OS_PROC_CreateEntryEx ( SAMPLE_MODULE, 	_g_sample_device_proc_table,
											_SAMPLE_ReadProcFunction,
											_SAMPLE_WriteProcFunction );
}

/**
 * cleanup proc utility for sample device
 *
 * @see SAMPLE_Cleanup
*/
void	SAMPLE_PROC_Cleanup (void)
{
	OS_PROC_RemoveEntry( SAMPLE_MODULE );
}

/** @} */

