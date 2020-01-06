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
 *  Linux proc interface for pvr device.
 *	pvr device will teach you how to make device driver with new platform.
 *
 *  author		kyungbin.pak
 *  version		1.0 
 *  date		2010.02.05
 *  note		Additional information. 
 *
 *  @addtogroup lg1150_pvr 
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
#include "pvr_drv.h"
#include "proc_util.h"
#include "debug_util.h"

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

/*----------------------------------------------------------------------------------------
	Static Variables
----------------------------------------------------------------------------------------*/
static OS_PROC_DESC_TABLE_T	_g_pvr_device_proc_table[] =
{
	{ "author",		PROC_ID_AUTHOR  , OS_PROC_FLAG_READ },
	{ "command",	PROC_ID_COMMAND , OS_PROC_FLAG_WRITE },
	{ NULL, 		PROC_ID_MAX		, 0 }
};

/*========================================================================================
	Implementation Group
========================================================================================*/

/*
 * read_proc implementation of pvr device 
 *
*/
static int	_PVR_ReadProcFunction(	UINT32 procId, char* buffer )
{
	int		ret;

	/* TODO: add your proc_write implementation */
	switch( procId )
	{
		case PROC_ID_AUTHOR:
		{
			ret = sprintf( buffer, "%s\n", "kyungbin.pak" );
		}
		break;	
		
		default:
		{
			ret = sprintf( buffer, "%s(%d)\n", "unimplemented read proc", procId );
		}
	}
		
	return ret;
}

/*
 * write_proc implementation of pvr device 
 *
*/
static int _PVR_WriteProcFunction( UINT32 procId, char* command )
{
	/* TODO: add your proc_write implementation */	
	switch( procId )
	{
		case PROC_ID_COMMAND:
		{
			PVR_KDRV_LOG( PVR_TRACE ,"command string : %s\n", command );
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
 * initialize proc utility for pvr device
 *
 * @see PVR_Init
*/
void	PVR_PROC_Init (void)
{
	OS_PROC_CreateEntryEx ( PVR_MODULE, _g_pvr_device_proc_table, 
											_PVR_ReadProcFunction,
											_PVR_WriteProcFunction );
}

/**
 * cleanup proc utility for pvr device
 *
 * @see PVR_Cleanup
*/
void	PVR_PROC_Cleanup (void)
{
	OS_PROC_RemoveEntry( PVR_MODULE );	
}

/** @} */


