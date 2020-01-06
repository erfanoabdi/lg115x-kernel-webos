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
 *	de sync.
 *
 *	author		
 *	version		1.0
 *	date		
 *	note		
 *
 *	@addtogroup 
 *	@{
 */

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	File Inclusions
----------------------------------------------------------------------------------------*/
#include "os_util.h"
#include "base_device.h"

#include "de_sync.h"
#include "de_def.h"
#include "mcu/de_ipc_def.h"
#include "mcu/de_ipc.h"
#include "mcu/de_int.h"

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Type Definitions
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	External Function Prototype Declarations
----------------------------------------------------------------------------------------*/
extern void DE_VSYNC_Register(int (*func)(void));

/*----------------------------------------------------------------------------------------
	External Variables
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Functions  
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	global Variables
----------------------------------------------------------------------------------------*/
DE_SYNC_CB_T gcb[MAX_CB_NUM];

/*----------------------------------------------------------------------------------------
	Static Function Prototypes Declarations
----------------------------------------------------------------------------------------*/
static int _sync_call(void);

/*========================================================================================
 *   Implementation Group
 *=======================================================================================*/

/**
 * @callgraph
 * @callergraph
 *
 * @brief init sync handler for processing with de-vsync 
 */
int de_sync_init(void)
{
	int ret = RET_OK;

	do {
		memset(gcb, 0, sizeof(DE_SYNC_CB_T) * MAX_CB_NUM);
		DE_VSYNC_Register(_sync_call);
		DE_PRINT("de_sync_init :  done  ret[%d] %s\n", \
						ret, (ret==RET_OK)?"OK":"FAIL");
	} while(0);
	return ret;
}

/**
 * @callgraph
 * @callergraph
 *
 * @brief free sync handler 
 */
int de_sync_free(void)
{
	return DE_VSYNC_Free();
}

/**
 * @callgraph
 * @callergraph
 *
 * @brief  wait until next vsync 
 */
int de_sync_wait(DE_SYNC_WAIT_TYPE_T type)
{
	int ret = RET_OK;

	switch(type)
	{
		case SYNC_WAIT_JPG :
			VIDEO_JPG_WaitVsync();
			break;
		case SYNC_WAIT_USB :
			VIDEO_USB_WaitVsync();
			break;
		case SYNC_WAIT_WEL :
			VIDEO_WEL_WaitVsync();
			break;
		case SYNC_WAIT_WER :
			VIDEO_WER_WaitVsync();
			break;
		default :
			break;
	}

	return ret;
}

/**
 * @callgraph
 * @callergraph
 *
 * @brief  register callback function 
 */
int de_sync_register(BOOLEAN flag, int (*func)(void), char *str)
{
	int i = 0, j = 0;

	for(i=0;i<MAX_CB_NUM;i++)
	{
		if(flag) // register
		{
			if(!gcb[i].f)
			{
				gcb[i].f = func;
				for(j=0;j < MAX_CB_STR;j++)
				{
					gcb[i].s[j] = str[j];
					if(str[j] == 0) break;
				}
				return i;
			}
		}
		else     // unregister
		{
			if(gcb[i].f == func)
			{
				gcb[i].f = NULL;
				memset(gcb[i].s, 0, MAX_CB_STR);
				return i;
			}
		}
	}
	return -1;
}
EXPORT_SYMBOL(de_sync_register);

/**
 * @callgraph
 * @callergraph
 *
 * @brief display list of all registered callback function 
 */
int de_sync_display(void)
{
	int i = 0;
	DE_PRINT("List of de vsync callback function\n");
	for(i=0;i<MAX_CB_NUM;i++)
	{
		if(gcb[i].f) DE_PRINT("\t[%d]  %s\n", i, gcb[i].s);
	}
	return RET_OK;
}

/**
 * @callgraph
 * @callergraph
 *
 * @brief display list of all registered callback function 
 */
int de_sync_vsync_enable(BOOLEAN enable)
{
	int ret = RET_OK;
	static int initialized = 0;
	if(!initialized)
	{
		ret = DE_VSYNC_Init();
		initialized = 1;
	}
	return ret;
}

static int _sync_call(void)
{
	int i = 0;
	for(i=0;i<MAX_CB_NUM;i++)
	{
		if(gcb[i].f) gcb[i].f();
//		if(gcb[i].f) DE_PRINT("%s\n",gcb[i].s);
	}
	return RET_OK;
}

