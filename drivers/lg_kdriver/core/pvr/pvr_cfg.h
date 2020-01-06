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
 *  main configuration file for pvr device
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

#ifndef	_PVR_CFG_H_
#define	_PVR_CFG_H_

/*----------------------------------------------------------------------------------------
	Control Constants
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
    File Inclusions
----------------------------------------------------------------------------------------*/

#ifdef	__cplusplus
extern "C"
{
#endif /* __cplusplus */

/*----------------------------------------------------------------------------------------
	Constant Definitions
----------------------------------------------------------------------------------------*/
#define	PVR_MODULE			"pvr"
#define PVR_MAX_DEVICE		2

/*----------------------------------------------------------------------------------------
	Macro Definitions
----------------------------------------------------------------------------------------*/
//#define DVR_BUFFER_BASE				0x9A90000 // Jason gave new memory map on 28-1-10
#define DVR_COMMON_BUF_SIZE			( gui32DvrCommonBufSize )	/* Value multiple of 192, 188 & 4K boundary*/

#define DVR_DN0_BUF_SIZE			DVR_COMMON_BUF_SIZE	/* 30 rows */
#define DVR_DN1_BUF_SIZE			DVR_COMMON_BUF_SIZE	/* 30 rows */

#define DVR_UP0_BUF_SIZE			DVR_COMMON_BUF_SIZE	/* 30 rows */
#define DVR_UP1_BUF_SIZE			DVR_COMMON_BUF_SIZE	/* 30 rows */

/* PVR buffer bases */
#define DVR_DN0_BUFF_BASE           (DVR_BUFFER_BASE)					
#define DVR_UP0_BUFF_BASE           (DVR_DN0_BUFF_BASE+DVR_DN0_BUF_SIZE)
#define DVR_DN1_BUFF_BASE           (DVR_UP0_BUFF_BASE+DVR_UP0_BUF_SIZE)
#define DVR_UP1_BUFF_BASE           (DVR_DN1_BUFF_BASE+DVR_DN1_BUF_SIZE)


/*----------------------------------------------------------------------------------------
    Type Definitions
----------------------------------------------------------------------------------------*/

typedef struct tagBufferControlStruct
{
  UINT32		ui32BufferBase;
  UINT32		ui32BufferEnd;
  UINT32		ui32ReadPtr;
  UINT32		ui32WritePtr;
}DVR_BUFF_STRUCT;

typedef struct tagPvrMemStruct
{
	DVR_BUFF_STRUCT	stDvrDnBuff;
	DVR_BUFF_STRUCT	stDvrUpBuff;
	/* 22.06.2010 - Muru - Added for handling Pie data in user layer buffer */
	// DVR_BUFF_STRUCT	stPieUserVirtBuff;	//This buffer is user virtual memory 
	/* 2012. 12. 22 jinhwan.bae to operate with ioremapped Pie Buffer in kernel driver */
	DVR_BUFF_STRUCT stPiePhyBuff;
	DVR_BUFF_STRUCT stPieMappedBuff;
	SINT32 			si32PieVirtOffset;
} DVR_MEM_CFG;

/**
 * enum for mem cfg 
 */
typedef enum
{
	LX_PVR_MEM_DN					= 0,		/**< Download Buffer */
	LX_PVR_MEM_UP					= 1,		/**< Upload Buffer */
	LX_PVR_MEM_UPBUF				= 2,		/**< Buffer using copying upload packet */
	LX_PVR_MEM_PIEBUF				= 3,		/**< Buffer for Picture Index */
#if 0	
	LX_PVR_MEM_NUM					= 4,		/**< Number of memory structure */
#else                               // Chulho2.kim -- 2 Channel
    LX_PVR_MEM_DN1					= 4,		/**< Download Buffer1 */
	LX_PVR_MEM_UP1					= 5,		/**< Upload Buffer1 */
	LX_PVR_MEM_UPBUF1				= 6,		/**< Buffer1 using copying upload packet */
	LX_PVR_MEM_PIEBUF1				= 7,		/**< Buffer1 for Picture Index */
	LX_PVR_MEM_NUM
#endif
} LX_PVR_MEM_CFG_T;

/*----------------------------------------------------------------------------------------
	Extern Function Prototype Declaration
----------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------
	Extern Variables
----------------------------------------------------------------------------------------*/
extern UINT32 gui32DvrCommonBufSize;
extern UINT32 DVR_BUFFER_BASE;
extern DVR_MEM_CFG	astDvrMemMap[];
extern LX_MEMCFG_T	gMemCfgDvr[][LX_PVR_MEM_NUM];


#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* _PVR_CFG_H_ */

/** @} */

