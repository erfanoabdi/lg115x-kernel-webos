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
 *  sdec driver
 *
 *  @author	Jihoon Lee ( gaius.lee@lge.com)
 *  @author	Jinhwan Bae ( jinhwan.bae@lge.com) - modifier
 *  @version	1.0
 *  @date		2010-03-30
 *  @note		Additional information.
 */


/*----------------------------------------------------------------------------------------
 *   Control Constants
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   File Inclusions
 *---------------------------------------------------------------------------------------*/
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#include "os_util.h"

#include "sdec_kapi.h"
#include "sdec_cfg.h"
#include "sdec_drv.h"
#include "sdec_io.h"

#include "hma_alloc.h"

/*----------------------------------------------------------------------------------------
 *   Constant Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Macro Definitions
 *---------------------------------------------------------------------------------------*/
#define USE_SDEC_HMA_MEMORY
#define SDEC_HMA_POOL_NAME		"sdec"

/*----------------------------------------------------------------------------------------
 *   Type Definitions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Function Prototype Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   External Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Functions
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   global Variables
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Function Prototypes Declarations
 *---------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
 *   Static Variables
 *---------------------------------------------------------------------------------------*/

#ifdef USE_SDEC_HMA_MEMORY
static int _gSdecMemInited = 0;
#else
/* 20131022 jinhwan.bae
    fix AATS Issue, mm final without mm init cause lock up in OS_CleanupRegion */
static OS_RGN_T g_SdecRgn = {0, 0, -1, };
#endif

static LX_SDEC_MEM_CFG_T _gstSdecMemMap;

/**
********************************************************************************
* @brief
*   initialize memory pool manager.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
int SDEC_MM_Init(LX_SDEC_GPB_INFO_T	*stpLXSdecGPBInfo)
{
	int 				ret		= 0;
	LX_SDEC_CFG_T		*pSdecConf = NULL;
	UINT8				memCfg = 0;

	LX_SDEC_CHECK_CODE( stpLXSdecGPBInfo == NULL, return INVALID_ARGS, "Invalid argument" );

	/* get config */
	pSdecConf 	= SDEC_CFG_GetConfig();
	memCfg 		= pSdecConf->memCfg;

	/* 20110412 gaius.lee  : modify scheme used with config file. if memory address will be changed someday, we can modified memCfg value */
	_gstSdecMemMap.memory_name		= gMemCfgSDECGPB[memCfg].memory_name;
	_gstSdecMemMap.gpb_memory_base	= gMemCfgSDECGPB[memCfg].gpb_memory_base;
	_gstSdecMemMap.gpb_memory_size	= gMemCfgSDECGPB[memCfg].gpb_memory_size;

#ifdef USE_SDEC_HMA_MEMORY
	ret = hma_pool_register(SDEC_HMA_POOL_NAME, _gstSdecMemMap.gpb_memory_base, _gstSdecMemMap.gpb_memory_size);
	if(ret == 0) _gSdecMemInited = 1;
#else
	ret = OS_InitRegion( &g_SdecRgn, (void *)_gstSdecMemMap.gpb_memory_base, _gstSdecMemMap.gpb_memory_size);
#endif			

	stpLXSdecGPBInfo->uiGpbBase = _gstSdecMemMap.gpb_memory_base;
	stpLXSdecGPBInfo->uiGpbSize = _gstSdecMemMap.gpb_memory_size;

	return ret;
}

/**
********************************************************************************
* @brief
*   Finalize memory pool manager.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   stpSdecParam :SDEC parameter
* @return
*  DTV_STATUS_T
********************************************************************************
*/
int SDEC_MM_Final(void)
{
	int 				ret		= 0;

#ifdef USE_SDEC_HMA_MEMORY
	LX_SDEC_CHECK_CODE( _gSdecMemInited == 0, return NOT_OK, "sdec mem is not ready" );

	hma_pool_unregister(SDEC_HMA_POOL_NAME);
	_gSdecMemInited = 0;
#else
	LX_SDEC_CHECK_CODE( g_SdecRgn.block_num == -1, return NOT_OK, "sdec region is not ready" );

	ret = OS_CleanupRegion(&g_SdecRgn);

	/* 20131022 jinhwan.bae
            fix AATS Issue, mm final without mm init cause lock up in OS_CleanupRegion 
            set init value to -1 after final */
	if(OK == ret)
	{
		g_SdecRgn.block_num = -1;
	}
#endif

	return ret;
}


/**
********************************************************************************
* @brief
*   allocate memory
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_MM_Alloc(LX_SDEC_MM_Alloc *stpLXSdecMMAlloc)
{
	DTV_STATUS_T eRet = NOT_OK;
	UINT32 size;
	void *ptr ;

	LX_SDEC_CHECK_CODE( stpLXSdecMMAlloc == NULL, return INVALID_ARGS, "Invalid argument" );
#ifdef USE_SDEC_HMA_MEMORY
	LX_SDEC_CHECK_CODE( _gSdecMemInited == 0, goto EXIT, "sdec mem is not ready" );
#else
	LX_SDEC_CHECK_CODE( g_SdecRgn.block_num == -1, goto EXIT, "sdec region is not ready" );
#endif

	/* get size */
	size = stpLXSdecMMAlloc->uiGpbSize;

#ifdef USE_SDEC_HMA_MEMORY
	ptr = (void*)hma_alloc(SDEC_HMA_POOL_NAME, (int)size + 0x1000, 1024);
#else
	ptr = OS_MallocRegion(&g_SdecRgn, size);
#endif

	if( ((UINT32)ptr >= _gstSdecMemMap.gpb_memory_base) && ((UINT32)ptr <= (_gstSdecMemMap.gpb_memory_base + _gstSdecMemMap.gpb_memory_size)))
	{
		stpLXSdecMMAlloc->uiGpbAddr = (UINT32)ptr;
		eRet = OK;
	}
	else
	{
		SDEC_DEBUG_Print("ERROR : dynamic allocation failed %08x\n" , (UINT32)ptr);
	}

EXIT:
	return eRet;
}


/**
********************************************************************************
* @brief
*   Free allocated memory
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_MM_Free(UINT32 ui32Arg)
{
	DTV_STATUS_T	eRet	= NOT_OK;
	int 			ret		= 0;
	UINT32			gpb_memory_base, gpb_memory__end;

	LX_SDEC_CHECK_CODE( ui32Arg == 0, goto EXIT, "memfree address is null" );
#ifdef USE_SDEC_HMA_MEMORY
	LX_SDEC_CHECK_CODE( _gSdecMemInited == 0, goto EXIT, "sdec mem is not ready" );
#else	
	LX_SDEC_CHECK_CODE( g_SdecRgn.block_num == -1, goto EXIT, "sdec region is not ready" );
#endif

	gpb_memory_base = _gstSdecMemMap.gpb_memory_base;
	gpb_memory__end = _gstSdecMemMap.gpb_memory_base + _gstSdecMemMap.gpb_memory_size;

	/* check if memory address is valid */
	LX_SDEC_CHECK_CODE( (gpb_memory_base > ui32Arg) || (ui32Arg > gpb_memory__end), goto EXIT, "ERROR : memory address is incorrect base[0x%08x]addr[0x%x]\n" , gpb_memory_base, ui32Arg);

#ifdef USE_SDEC_HMA_MEMORY
	hma_free(SDEC_HMA_POOL_NAME, ui32Arg);
#else
	ret = OS_FreeRegion(&g_SdecRgn, (void*)ui32Arg);
#endif

	if(ret == 0) eRet = OK;

EXIT:
	return eRet;
}

/**
********************************************************************************
* @brief
* 	get memory statistics for sdec memory
*	application can use this information to monitor memroy usage.
* @remarks
*  DETAIL INFORMATION
* @par requirements:
*
* @param
*   ui32Arg :arguments from userspace
* @return
*  DTV_STATUS_T
********************************************************************************
*/
DTV_STATUS_T SDEC_MM_GetMemoryStat (LX_SDEC_MM_GetStat *pMemStat)
{
#ifdef USE_SDEC_HMA_MEMORY
	// TODO: Can't get the mem state from hma
	return NOT_OK;
#else
	OS_RGN_INFO_T   mem_info;

	LX_SDEC_CHECK_CODE( g_SdecRgn.block_num == -1, return INVALID_ARGS, "sdec region is not ready" );
	LX_SDEC_CHECK_CODE( pMemStat == NULL, return INVALID_ARGS, "Invalid argument" );

	OS_GetRegionInfo(&g_SdecRgn, &mem_info);

	pMemStat->mem_base			= mem_info.phys_mem_addr;
	pMemStat->mem_length 		= mem_info.length;
	pMemStat->mem_alloc_size 	= mem_info.mem_alloc_size;
	pMemStat->mem_free_size 	= mem_info.mem_free_size;

	return OK;
#endif
	
}

