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
 *  register api implementation for venc device of H13.
 *	venc device will teach you how to make device driver with new platform.
 *
 *  author		jaeseop.so (jaeseop.so@lge.com)
 *  version		1.0
 *  date		2012.06.12
 *  note		Additional information.
 *
 *  @addtogroup lg1154_venc
 *	@{
 */

/*-----------------------------------------------------------------------------
	Control Constants
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	File Inclusions
-----------------------------------------------------------------------------*/
#include <linux/kthread.h>
#include <asm/io.h>			// for ioremap()/iounmap()
#include <linux/slab.h>
#include <linux/delay.h>	// for msleep()

#ifdef H1ENCODE_USE_TIMER_VSYNC
#include <linux/timer.h>	
#endif

#include "base_types.h"
#include "os_util.h"
#include "venc_kapi.h"
#include "../venc_cfg.h"
#include "../venc_drv.h"

#include "venc_h13b0_regprep.h"
#include "h1encode.h"

#ifdef VENC_HXENC_BUILTIN
#include "hxenc_api.h"
#else
#include "h1encode_wrapper.h"
#endif

#ifdef H1ENCODE_USE_HMA
#include "hma_alloc.h"
#endif

/*-----------------------------------------------------------------------------
	Constant Definitions
-----------------------------------------------------------------------------*/
#ifdef H1ENCODE_USE_HMA
#define VENC_HMA_NAME		"VENC_HMA"
#endif

/*-----------------------------------------------------------------------------
	Macro Definitions
-----------------------------------------------------------------------------*/
#ifdef H1ENCODE_USE_LOCK
#define H1ENCODE_LOCK()				spin_lock(&h1enc_lock)
#define H1ENCODE_UNLOCK()			spin_unlock(&h1enc_lock)
#else
#define H1ENCODE_LOCK()
#define H1ENCODE_UNLOCK()
#endif

#ifdef H1ENCODE_DEBUG_AUI_SEQ
#define SEQ_LOCK()					spin_lock(&h1enc_lock_seq)
#define SEQ_UNLOCK()				spin_unlock(&h1enc_lock_seq)
#else
#define SEQ_LOCK()					/* NOP */
#define SEQ_UNLOCK()				/* NOP */
#endif

/*-----------------------------------------------------------------------------
	Type Definitions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	External Function Prototype Declarations
-----------------------------------------------------------------------------*/
// Clock Gate
extern void ClockGateOn( void );
extern void ClockGateOff( void );

/*-----------------------------------------------------------------------------
	External Variables
-----------------------------------------------------------------------------*/
H1ENCODE_T 			gH1Encode;

/*-----------------------------------------------------------------------------
	global Functions
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Static Function Prototypes Declarations
-----------------------------------------------------------------------------*/
#ifdef H1ENCODE_USE_HMA
static int H1EncodeMemoryInitHMA( void );
static int H1EncodeMemoryCleanupHMA( void );
#else
static int H1EncodeMemoryInit( void );
static int H1EncodeMemoryCleanup( void );
#endif
static int H1EncodeRegisterInit( u32 regPREPBase, u32 regPREPSize );
static int H1EncodeRegisterCleanup( void );
static int H1EncodeTask( void* pParam );
static void H1EncodeReset( void );

#if 0
static void H1EncodeCheckTime( H1ENCODE_T *pstEncode, u32 time );
static void H1EncodeCheckBitrate( H1ENCODE_T *pstEncode, u32 streamSize );
#endif

static int H1EncodeWaitDoneTimeout( int timeout );
static void H1EncodeReadPREP(H1ENCODE_PREP_STATUS_T *pstPrepStatus);
static int H1EncodeStop( void );

static void _InterruptEnable( void );
static void _InterruptDisable( void );
static void _InterruptEnableVsync( BOOLEAN enable );
static void _VsyncWait( void );
static int _EventWait( H1ENCODE_EVENT_T event, int timeout );
static void _EventPost( H1ENCODE_EVENT_T event );
static void _EventClear( void );

#ifdef H1ENCODE_ENABLE_DELAYED_ENCODING
static void _EncodeStartPost( void );
#endif

static void H1EncodeVsyncHandler( unsigned long temp );

#ifdef H1ENCODE_UNUSE_ENCODE_THREAD
static void H1EncodeEncodeHandler( unsigned long temp );
DECLARE_TASKLET( VENC_TASKLET_VSYNC, H1EncodeEncodeHandler, 0 );
#endif

/*-----------------------------------------------------------------------------
	Static Variables
-----------------------------------------------------------------------------*/
static volatile VENC_H13_REG_PREP_T *gpRegVENCPREP;
static volatile VENC_H13_REG_PREP_T *gpRealRegVENCPREP;
static volatile void *gpIPCExternal;	// for DE IPC Register.

static struct task_struct *g_pstH1EncodeTask = NULL;
static H1ENCODE_OUTPUT_STATUS_T	gstOutputStatus;

#ifdef H1ENCODE_USE_LOCK
DEFINE_SPINLOCK(h1enc_lock);
#endif

#ifdef H1ENCODE_DEBUG_AUI_SEQ
DEFINE_SPINLOCK(h1enc_lock_seq);
static int gNextAUISeq = 0;
#endif

#ifdef H1ENCODE_USE_TIMER_VSYNC
struct timer_list encode_timer;
#endif

static OS_EVENT_T	gstEventH1;

#ifdef H1ENCODE_ENABLE_DELAYED_ENCODING
UINT32 maxDelayedTime;
#endif

#ifdef H1ENCODE_USE_LOGFILE
struct timeval _irqTime;
#endif

#ifdef H1ENCODE_DEBUG_MEM_LEAK
static atomic_t encparams_count = ATOMIC_INIT( 0 );
#endif

/*========================================================================================
	Implementation Group
========================================================================================*/

////////////////////////////////////////////////////////////////////////////////
//
//	Stream Dump
//
////////////////////////////////////////////////////////////////////////////////

#if defined(H1ENCODE_DEBUG_DUMP)

static OS_FILE_T dump_file;
static BOOLEAN	isDumpOpened = FALSE;

static void H1EncodeSaveStream(u32* buf, u32 size)
{
	if ( !isDumpOpened )
	{
		char filepath[100];

		if (gH1Encode.outputType == LX_VENC_ENCODE_TYPE_VP8)
		{
			sprintf(filepath, "h1enc_dump.vp8");
		}
		else
		{
			sprintf(filepath, "h1enc_dump.264");
		}

		if ( RET_OK != OS_OpenFile( &dump_file, filepath, O_CREAT|O_TRUNC|O_RDWR|O_LARGEFILE, 0666 ) )
		{
			return;
		}

		isDumpOpened = TRUE;
	}

#if 0
	if(endian == 1)
	 {
		 u32 i = 0, words = (size + 3) / 4;

		 while(words)
		 {
			 u32 val = strmbuf[i];
			 u32 tmp = 0;

			 tmp |= (val & 0xFF) << 24;
			 tmp |= (val & 0xFF00) << 8;
			 tmp |= (val & 0xFF0000) >> 8;
			 tmp |= (val & 0xFF000000) >> 24;
			 strmbuf[i] = tmp;
			 words--;
			 i++;
		 }

	 }
#endif

	OS_WriteFile( &dump_file, (char *)buf, size );

	VENC_DEBUG("[SAVESTRM] Addr: 0x%08x, Size: 0x%08x\n", (u32)buf, size);

}

static void H1EncodeDumpStop( void )
{
	if ( isDumpOpened )
	{
		OS_CloseFile( &dump_file );
		isDumpOpened = FALSE;
	}
}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//	Common
//
////////////////////////////////////////////////////////////////////////////////

void H1EncodeInitialize( )
{
	TRACE_ENTER();

#ifdef H1ENCODE_ENABLE_SCD
	SET_VENC_CFG_CTRL_INDEX( VENC_CFG_CTRL_SCD );
#endif
	
#ifdef H1ENCODE_ENABLE_SAR
	SET_VENC_CFG_CTRL_INDEX( VENC_CFG_CTRL_SAR );
#endif

#ifdef H1ENCODE_ENABLE_FIELD_REPEAT
	SET_VENC_CFG_CTRL_INDEX( VENC_CFG_CTRL_FIELD_REPEAT);
#endif

	H1EncodeReset();

#ifdef H1ENCODE_USE_HMA
	H1EncodeMemoryInitHMA();
#else
	H1EncodeMemoryInit();
#endif

	_EncParamInit();

	H1EncodeRegisterInit( gpstVencConfig->venc_reg_base + 0x800, 0x100 );
	_InterruptDisable();
	_InterruptEnableVsync(FALSE);
	H1EncodeRegisterCleanup();

	// Init global variables
	gH1Encode.stInput.gopLength = H1ENCODE_GOPLENGTH_DEFAULT;
	gH1Encode.stInput.qpValue = -1;
	
	// Initialize for sync
	init_waitqueue_head( &gH1Encode.wqEncodeVsync );

	// Initialize waitq for encoding done
	init_waitqueue_head( &gH1Encode.wqEncodeDone );

#ifdef H1ENCODE_ENABLE_DELAYED_ENCODING
	init_waitqueue_head( &gH1Encode.wqEncodeStart );
#endif

	OS_InitEvent( &gstEventH1 );

#ifdef H1ENCODE_USE_TIMER_VSYNC
	init_timer(&encode_timer);
#endif

#ifndef H1ENCODE_UNUSE_ENCODE_THREAD
	// Create kthread for encode task
	g_pstH1EncodeTask = kthread_run( &H1EncodeTask, NULL, "H1EncodeTask" );

#ifdef H1ENCODE_ENABLE_TASK_PRIORITY_HIGH
	{
		// set priority of kthread
		struct sched_param param = { 0 };
		param.sched_priority = H1ENCODE_SCHED_PRIORITY;
		if ( sched_setscheduler( g_pstH1EncodeTask, SCHED_RR, &param ) != 0 )
		{
			VENC_ERROR("[%s:%u] Fail set priority of kthread\n", __F__, __L__);
		}
	}
#endif
#endif

	_WorkQInit();

	ClockGateOff();

	TRACE_EXIT();
}

void H1EncodeFinalize( void )
{
	TRACE_ENTER();

#ifdef H1ENCODE_USE_TIMER_VSYNC
	del_timer_sync(&encode_timer);
#endif

#ifndef H1ENCODE_UNUSE_ENCODE_THREAD
	if ( g_pstH1EncodeTask != NULL )
	{
		kthread_stop( g_pstH1EncodeTask );
	}
#endif

#ifdef H1ENCODE_USE_HMA
	H1EncodeMemoryCleanupHMA();
#else
	H1EncodeMemoryCleanup();
#endif
	H1EncodeRegisterCleanup();

	_EncParamFinal();
	_WorkQCleanup();

	TRACE_EXIT();
}

int H1EncodeOpen( int channel )
{
	ClockGateOn();

	H1EncodeHxencInit();
	H1EncodeRegisterInit( gpstVencConfig->venc_reg_base + 0x800, 0x100 );

	_EncParamInit();

	_InterruptEnable();
	_InterruptEnableVsync( FALSE );

	return RET_OK;
}

int H1EncodeClose( int channel )
{
	if ( gH1Encode.eStatus == H1ENCODE_STATUS_TYPE_START )
	{
		_InterruptEnableVsync( FALSE );
		
		H1EncodeStop();

		H1ENCODE_LOCK();
		gH1Encode.eStatus = H1ENCODE_STATUS_TYPE_STOP;
		H1ENCODE_UNLOCK();		
	}
	
	_InterruptDisable();

	_EncParamFinal();
	H1EncodeRegisterCleanup();
	HXENC_Cleanup();

	ClockGateOff();

	return RET_OK;
}

void H1EncodeHxencInit( void )
{
	HXENC_Init( gpstVencConfig->venc_reg_base, 0x600 );
	HXENC_MemallocInit( gpstVencMemConfig->uiH1EncBufBase, gpstVencMemConfig->uiH1EncBufBase + gpstVencMemConfig->uiH1EncBufSize);
}
EXPORT_SYMBOL( H1EncodeHxencInit );

static int H1EncodeRegisterInit( u32 regPREPBase, u32 regPREPSize )
{
	// Do ioremap for real & shadow registers
	gpRealRegVENCPREP = (VENC_H13_REG_PREP_T*)ioremap( regPREPBase, regPREPSize);
	gpRegVENCPREP = (VENC_H13_REG_PREP_T*)kzalloc( regPREPSize, GFP_KERNEL);

// INCLUDE_M14_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV( M14, B0 ) && lx_chip_rev() < LX_CHIP_REV( H14, A0 ) )
	{
		gpIPCExternal = ioremap( 0xC0025300, 0x100 );

		VENC_DEBUG("gpIPCExternal = 0x%08X\n", (UINT32)gpIPCExternal );
	}

	if ( gpRealRegVENCPREP == NULL || gpRegVENCPREP == NULL )
	{
		VENC_ERROR("Can't ioremap for registers.\n");
		return RET_ERROR;
	}

	return RET_OK;
}


static int H1EncodeRegisterCleanup( void )
{

	if ( gpRealRegVENCPREP )
	{
		iounmap( (void *)gpRealRegVENCPREP );
		gpRealRegVENCPREP = NULL;
	}

	if ( gpRegVENCPREP )
	{
		kfree( (void *)gpRegVENCPREP );
		gpRegVENCPREP = NULL;
	}

// INCLUDE_M14_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV( M14, B0 ) && lx_chip_rev() < LX_CHIP_REV( H14, A0 ) )
	{
		if ( gpIPCExternal != NULL )
		{
			iounmap( (void *)gpIPCExternal );
		}
	}

	return RET_OK;
}

#ifdef H1ENCODE_USE_HMA
static int H1EncodeMemoryInitHMA( void )
{
	void *virt = NULL;
	
	UINT32 venc_base = gpstVencMemConfig->uiDPBBase;
	UINT32 venc_size = gpstVencMemConfig->uiInBufBase - gpstVencMemConfig->uiDPBBase;
	if ( hma_pool_register( VENC_HMA_NAME, venc_base, venc_size ) != RET_OK )
	{
		VENC_ERROR("Can't register hma pool: %s\n", VENC_HMA_NAME);
		goto RETURN_ERROR;
	}
		
	// Set DPB address & ioremap
	gH1Encode.memOSB.u32Size = gpstVencMemConfig->uiDPBSize;
	gH1Encode.memOSB.u32Phys = hma_alloc( VENC_HMA_NAME, gH1Encode.memOSB.u32Size, 0x1000 );
	gH1Encode.memOSB.u32Virt = (void *)ioremap( gH1Encode.memOSB.u32Phys, gH1Encode.memOSB.u32Size );
	if ( (void*)gH1Encode.memOSB.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: Output frame buffer.\n");
		goto RETURN_ERROR;
	}

	gH1Encode.memESB.u32Size = gpstVencMemConfig->uiESBufSize;
	gH1Encode.memESB.u32Phys = hma_alloc( VENC_HMA_NAME, gH1Encode.memESB.u32Size, 0x1000 );	
	gH1Encode.memESB.u32Virt = (void *)ioremap( gH1Encode.memESB.u32Phys, gH1Encode.memESB.u32Size );
	if ( (void*)gH1Encode.memESB.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: Coded frame buffer.\n");
		goto RETURN_ERROR;
	}
	
	gH1Encode.memAUI.u32Size = gpstVencMemConfig->uiAUIBufSize;
	gH1Encode.memAUI.u32Phys = hma_alloc( VENC_HMA_NAME, gH1Encode.memAUI.u32Size, 0x1000 );
	gH1Encode.memAUI.u32Virt = (void *)ioremap( gH1Encode.memAUI.u32Phys, gH1Encode.memAUI.u32Size );
	if ( (void*)gH1Encode.memAUI.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: AUI buffer.\n");
		goto RETURN_ERROR;
	}
	
	gH1Encode.memTFB.u32Size = gpstVencMemConfig->uiScalerSize;
	gH1Encode.memTFB.u32Phys = hma_alloc ( VENC_HMA_NAME, gH1Encode.memTFB.u32Size, 0x1000 );
	gH1Encode.memTFB.u32Virt = (void *)ioremap( gH1Encode.memTFB.u32Phys, gH1Encode.memTFB.u32Size );
	if ( (void*)gH1Encode.memTFB.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: Input frame buffer.\n");
		goto RETURN_ERROR;
	}

	return RET_OK;

RETURN_ERROR:
	H1EncodeMemoryCleanupHMA();

	return RET_ERROR;

}

static int H1EncodeMemoryCleanupHMA( void )
{
	if ( gH1Encode.memESB.u32Virt )
	{
		iounmap( (void *)gH1Encode.memESB.u32Virt );
		hma_free( VENC_HMA_NAME, gH1Encode.memESB.u32Phys );
		gH1Encode.memESB.u32Virt = 0;
		gH1Encode.memESB.u32Phys = 0;
	}

	if ( gH1Encode.memAUI.u32Virt )
	{
		iounmap( (void *)gH1Encode.memAUI.u32Virt );
		hma_free( VENC_HMA_NAME, gH1Encode.memAUI.u32Phys );
		gH1Encode.memAUI.u32Virt = 0;
		gH1Encode.memAUI.u32Phys = 0;
	}

	if ( gH1Encode.memOSB.u32Virt )
	{
		iounmap( (void *)gH1Encode.memOSB.u32Virt );
		hma_free( VENC_HMA_NAME, gH1Encode.memOSB.u32Phys );
		gH1Encode.memOSB.u32Virt = 0;
		gH1Encode.memOSB.u32Phys = 0;
	}

	if ( gH1Encode.memTFB.u32Virt )
	{
		iounmap( (void *)gH1Encode.memTFB.u32Virt );
		hma_free( VENC_HMA_NAME, gH1Encode.memTFB.u32Phys );
		gH1Encode.memTFB.u32Virt = 0;
		gH1Encode.memTFB.u32Phys = 0;
	}

	hma_pool_unregister( VENC_HMA_NAME );

	return RET_OK;
}
#else
static int H1EncodeMemoryInit( void )
{
	// Set DPB address & ioremap
	// !!! FIXME(IPC_SetVideoESBaseAddr, IPC_SetVideoESEndAddr)
	gH1Encode.memOSB.u32Phys = gpstVencMemConfig->uiDPBBase;
	gH1Encode.memOSB.u32Size = gpstVencMemConfig->uiDPBSize;
	gH1Encode.memOSB.u32Virt = (u32)ioremap( gH1Encode.memOSB.u32Phys, gH1Encode.memOSB.u32Size );

	if ( (void*)gH1Encode.memOSB.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: Output frame buffer.\n");
		goto RETURN_ERROR;
	}

	// Set ES Buffer address & ioremap
	gH1Encode.memESB.u32Phys = gpstVencMemConfig->uiESBufBase;
	gH1Encode.memESB.u32Size = gpstVencMemConfig->uiESBufSize;
	gH1Encode.memESB.u32Virt =(u32)ioremap( gH1Encode.memESB.u32Phys, gH1Encode.memESB.u32Size );

	// Clear ES Buffer
	memset( (void *)gH1Encode.memESB.u32Virt, 0x0, gH1Encode.memESB.u32Size );

	if ( (void*)gH1Encode.memESB.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: ES buffer.\n");
		goto RETURN_ERROR;
	}

	// Set AUI Buffer address & ioremap
	gH1Encode.memAUI.u32Phys = gpstVencMemConfig->uiAUIBufBase;
	gH1Encode.memAUI.u32Size = gpstVencMemConfig->uiAUIBufSize;
	gH1Encode.memAUI.u32Virt =(u32)ioremap( gH1Encode.memAUI.u32Phys, gH1Encode.memAUI.u32Size );

	// Clear AUI Buffer
	memset( (void *)gH1Encode.memAUI.u32Virt, 0x0, gH1Encode.memAUI.u32Size );

	if ( (void*)gH1Encode.memAUI.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: AUI buffer.\n");
		goto RETURN_ERROR;
	}

	// Set Input frame image buffer for thumbnail
	gH1Encode.memTFB.u32Phys = gpstVencMemConfig->uiScalerBase;
	gH1Encode.memTFB.u32Size = gpstVencMemConfig->uiScalerSize;
	gH1Encode.memTFB.u32Virt = (u32)ioremap( gH1Encode.memTFB.u32Phys, gH1Encode.memTFB.u32Size );

	if ( (void*)gH1Encode.memTFB.u32Virt == NULL )
	{
		VENC_ERROR("Error ioremap: Input frame buffer.\n");
		goto RETURN_ERROR;
	}

	return RET_OK;

RETURN_ERROR:
	H1EncodeMemoryCleanup();

	return RET_ERROR;

}

static int H1EncodeMemoryCleanup( void )
{

	if ( gH1Encode.memESB.u32Virt )
	{
		iounmap( (void *)gH1Encode.memESB.u32Virt );
		gH1Encode.memESB.u32Virt = 0;
	}

	if ( gH1Encode.memAUI.u32Virt )
	{
		iounmap( (void *)gH1Encode.memAUI.u32Virt );
		gH1Encode.memAUI.u32Virt = 0;
	}

	if ( gH1Encode.memOSB.u32Virt )
	{
		iounmap( (void *)gH1Encode.memOSB.u32Virt );
		gH1Encode.memOSB.u32Virt = 0;
	}

	if ( gH1Encode.memTFB.u32Virt )
	{
		iounmap( (void *)gH1Encode.memTFB.u32Virt );
		gH1Encode.memTFB.u32Virt = 0;
	}

	return RET_OK;
}
#endif

// Reset internal variables
static void H1EncodeReset( void )
{
//	gH1Encode.u32OutBufferRd = 0;
//	gH1Encode.u32OutBufferWr = 0;

	gH1Encode.memESB.u32ReadOffset = 0;
	gH1Encode.memESB.u32WriteOffset = 0;

	gH1Encode.memAUI.u32ReadOffset = 0;
	gH1Encode.memAUI.u32WriteOffset = 0;
	gH1Encode.u32AUIIndex = 0;

	gH1Encode.u32AUIStreamSize = 0;

	gH1Encode.u32EncodedFrames = 0;

	memset( gH1Encode.arEncodeTimes, 0x0, sizeof(gH1Encode.arEncodeTimes) );
	gH1Encode.u32EncodedTime = 0;
	gH1Encode.timeIndex = 0;

	memset( gH1Encode.arStreamSizes, 0x0, sizeof(gH1Encode.arStreamSizes) );
	gH1Encode.u32EncodedBitrate = 0;
	gH1Encode.sizeIndex = 0;

	gH1Encode.ui32CountFrameSkip = 0;

#ifdef H1ENCODE_CHECK_INTERLACED_INPUT
	#ifdef H1ENCODE_ENABLE_INTERLACED_BF
	gH1Encode.prevFrameType = LX_VENC_FRAME_TYPE_TOP;
	#else
	gH1Encode.prevFrameType = LX_VENC_FRAME_TYPE_BOTTOM;
	#endif

	if ( gH1Encode.pstEncoder != NULL )
	{
		gH1Encode.pstEncoder->bIsFirstFrame = TRUE;
	}
#endif
}


void H1EncodeSetEncodeType( LX_VENC_ENCODE_TYPE_T eEncodeType )
{
	if ( eEncodeType == LX_VENC_ENCODE_TYPE_H264 || eEncodeType == LX_VENC_ENCODE_TYPE_VP8 )
	{
		gH1Encode.outputType = eEncodeType;

		VENC_TRACE("Encode Type: %s\n", eEncodeType == LX_VENC_ENCODE_TYPE_H264 ? "H264" : "VP8");
	}
	else
	{
		VENC_ERROR("Wrong encode type.\n");
	}
}

int H1EncodeGetOutputBuffer( LX_VENC_RECORD_OUTPUT_T *pstEncOutput )
{
	// Wait 100ms
	if ( H1EncodeWaitDoneTimeout( 100 ) != RET_OK )
	{
		return RET_ERROR;
	}

//	LOCK();

	pstEncOutput->ui32OffsetStart = gstOutputStatus.ui32ESRptr - gH1Encode.memESB.u32Phys;
	pstEncOutput->ui32OffsetEnd	 = gstOutputStatus.ui32ESWptr - gH1Encode.memESB.u32Phys;
	gstOutputStatus.ui32ESRptr = gstOutputStatus.ui32ESWptr;

	pstEncOutput->ui32AUIOffsetStart = gstOutputStatus.ui32AUIRptr - gH1Encode.memAUI.u32Phys;
	pstEncOutput->ui32AUIOffsetEnd	 = gstOutputStatus.ui32AUIWptr - gH1Encode.memAUI.u32Phys;
	gstOutputStatus.ui32AUIRptr = gstOutputStatus.ui32AUIWptr;

//	UNLOCK();

	return RET_OK;
}

u32 H1EncodeGetEncodedFrameCount( void )
{
	return gH1Encode.u32EncodedFrames;
}

u32 H1EncodeGetEncodeMsec( void )
{
	if ( gH1Encode.u32EncodedTime > 0 )
	{
		return gH1Encode.u32EncodedTime / 1000;
	}
	else
	{
		return 0;
	}
}

s32	H1EncodeSetResolution( u16 width, u16 height )
{
	if ( gH1Encode.pstEncoder != NULL )
	{
		VENC_WARN("Should be called this function before starting.\n");
		return RET_ERROR;
	}
	
	if ( width == 0 || height == 0 )
	{
		return RET_ERROR;
	}

	if ( width > 1920 || height > 1088 )
	{
		// Not suppored resolution size.
		return RET_ERROR;
	}

	gH1Encode.stInput.width = width;
	gH1Encode.stInput.height = height;

	return RET_OK;
}

s32 H1EncodeGetResolution( u16 *pWidth, u16 *pHeight )
{
	if ( pWidth == NULL || pHeight == NULL )
	{
		return RET_ERROR;
	}

	if( gH1Encode.pstEncoder == NULL )
	{
		return RET_ERROR;
	}
	
	*pWidth = gH1Encode.pstEncoder->width;
	*pHeight = gH1Encode.pstEncoder->height;
	
	return RET_OK;
}

s32 H1EncodeSetFramerate( u32 framerate )
{
	u32		frameRateCode;
	u32 	frameRateDenom;
	u32		frameRateNum;

	switch( framerate )
	{
		case 24:
			frameRateCode = LX_VENC_FRAME_RATE_24HZ;
			frameRateDenom = 1000;
			frameRateNum = 24000;
			break;
		
		case 25:
			frameRateCode = LX_VENC_FRAME_RATE_25HZ;
			frameRateDenom = 1000;
			frameRateNum = 25000;
			break;
			
		case 30:
			frameRateCode = LX_VENC_FRAME_RATE_30HZ;
			frameRateDenom = 1000;
			frameRateNum = 30000;
			break;

		case 50:
			frameRateCode = LX_VENC_FRAME_RATE_50HZ;
			frameRateDenom = 1000;
			frameRateNum = 50000;
			break;
			
		case 60:
			frameRateCode = LX_VENC_FRAME_RATE_60HZ;
			frameRateDenom = 1000;
			frameRateNum = 60000;
			break;
			
		default:
			frameRateCode = LX_VENC_FRAME_RATE_NONE;
			frameRateDenom = 0;
			frameRateNum = 0;
			break;
	}
	
	gH1Encode.stInput.frameRateCode 	= frameRateCode;
	gH1Encode.stInput.frameRateDenom 	= frameRateDenom;
	gH1Encode.stInput.frameRateNum		= frameRateNum;
	
	return RET_OK;
}

s32 H1EncodeGetFramerate(	LX_VENC_RECORD_FRAME_RATE_T *peFramerate,
							UINT32	*pFramerate,
							LX_VENC_SOURCE_TYPE_T	*peSourceType )
{
	if ( gH1Encode.pstEncoder == NULL)
	{
		return RET_ERROR;
	}

	if ( peFramerate == NULL ||
		pFramerate == NULL ||
		peSourceType == NULL )
	{
		return RET_ERROR;
	}
	
	if ( gH1Encode.pstEncoder->GetFrameRate != NULL )
	{
		LX_VENC_RECORD_FRAME_RATE_T eFramerate = LX_VENC_FRAME_RATE_NONE;
		LX_VENC_SOURCE_TYPE_T eSourceType = LX_VENC_SOURCE_TYPE_FIELD;
		UINT32	nFramerate = 0;
		
		gH1Encode.pstEncoder->GetFrameRate( gH1Encode.pstEncoder, &eFramerate );

		switch( eFramerate )
		{
			case LX_VENC_FRAME_RATE_24HZ:	nFramerate = 24; break;
			case LX_VENC_FRAME_RATE_25HZ:	nFramerate = 25; break;
			case LX_VENC_FRAME_RATE_29HZ:	nFramerate = 29; break;
			case LX_VENC_FRAME_RATE_30HZ:	nFramerate = 30; break;
			case LX_VENC_FRAME_RATE_50HZ:	nFramerate = 50; break;
			case LX_VENC_FRAME_RATE_59HZ:	nFramerate = 59; break;
			case LX_VENC_FRAME_RATE_60HZ:	nFramerate = 60; break;
			case LX_VENC_FRAME_RATE_15HZ:	nFramerate = 15; break;
			default :						nFramerate =  0; break;
		}

		if ( gH1Encode.pstEncoder->eFrameType == LX_VENC_FRAME_TYPE_PROGRESSIVE )
		{
			eSourceType = LX_VENC_SOURCE_TYPE_FRAME;
		}
		else
		{
			eSourceType = LX_VENC_SOURCE_TYPE_FIELD;
		}

		*peFramerate = eFramerate;
		*pFramerate = nFramerate;
		*peSourceType = eSourceType;

		//printk("nFramerate=%d, eSourceType=%d\n", nFramerate, eSourceType);
		return RET_OK;
	}
	else
	{
		return RET_ERROR;
	}
}

s32 H1EncodeSetBitrate( u32 targetBitrate, BOOLEAN bEnableCBR )
{
	//printk("%s :: targetBitrate=%d, bEnableCBR=%d\n", __F__, targetBitrate, bEnableCBR);
	
	if ( gH1Encode.pstEncoder == NULL || gH1Encode.pstEncoder->SetBitrate == NULL )
	{
		gH1Encode.stInput.targetBitrate = targetBitrate;
		gH1Encode.stInput.bEnableCBR = bEnableCBR;
	}
	else if ( gH1Encode.pstEncoder->SetBitrate != NULL )
	{
		gH1Encode.pstEncoder->SetBitrate( gH1Encode.pstEncoder, targetBitrate, bEnableCBR );
	}
	else
	{
		return RET_NOT_SUPPORTED;
	}

	return RET_OK;
}

u32 H1EncodeGetBitrate( void )
{
	if ( gH1Encode.u32EncodedBitrate > 0 )
	{
		return gH1Encode.u32EncodedBitrate;
	}
	else
	{
		return 0;
	}
}

s32 H1EncodeSetGOP( u32 gopLength )
{
	int ret = RET_ERROR;

	if ( gopLength < 1 || gopLength > 300 )
	{
		// Invalid parameter
		return RET_ERROR;
	}
	
	if ( gH1Encode.pstEncoder == NULL || gH1Encode.pstEncoder->SetGOPLength == NULL)
	{
		gH1Encode.stInput.gopLength = gopLength;

		return RET_OK;
	}
	else if ( gH1Encode.pstEncoder->SetGOPLength != NULL )
	{		
		gH1Encode.stInput.gopLength = gopLength;
		ret = gH1Encode.pstEncoder->SetGOPLength( gH1Encode.pstEncoder, gopLength );
	}
	else
	{
		return RET_NOT_SUPPORTED;
	}

	return ret;
}

u32 H1EncodeGetGOP( void )
{
	u32 gopLength = H1ENCODE_GOPLENGTH_DEFAULT;
	
	if ( gH1Encode.pstEncoder != NULL &&
		gH1Encode.pstEncoder->GetGOPLength != NULL )
	{
		gH1Encode.pstEncoder->GetGOPLength( gH1Encode.pstEncoder, &gopLength );
	}

	return gopLength;
}

s32 H1EncodeSetQPValue( u32 qp )
{
	int ret = RET_ERROR;
	
	if ( gH1Encode.pstEncoder == NULL )
	{
		gH1Encode.stInput.qpValue = qp;
		return RET_OK;
	}

	if ( gH1Encode.pstEncoder->SetQPValue != NULL )
	{
		ret = gH1Encode.pstEncoder->SetQPValue( gH1Encode.pstEncoder, qp );
	}
	else
	{
		return RET_NOT_SUPPORTED;
	}

	return ret;
}

u32 H1EncodeGetQPValue( void )
{
	u32 qp = 0;

	if ( gH1Encode.pstEncoder == NULL )
	{
		return RET_ERROR;
	}

	if ( gH1Encode.pstEncoder->GetQPValue != NULL )
	{
		gH1Encode.pstEncoder->GetQPValue( gH1Encode.pstEncoder, &qp );
	}
	else
	{
		return RET_NOT_SUPPORTED;
	}

	return qp;	
}

s32 H1EncodeSetSAR( u32 sarWidth, u32 sarHeight )
{
	if ( gH1Encode.pstEncoder == NULL )
	{
		gH1Encode.stInput.sarWidth = sarWidth;
		gH1Encode.stInput.sarHeight = sarHeight;

		return RET_OK;
	}
	else
	{
		return RET_NOT_SUPPORTED;
	}
}

s32 H1EncodeGetSAR( u32 *pSarWidth, u32 *pSarHeight )
{
	if ( pSarWidth == NULL || pSarHeight == NULL )
	{
		return RET_INVALID_PARAMS;
	}
	
	if ( gH1Encode.pstEncoder == NULL )
	{
		return RET_ERROR;
	}
	else
	{
		*pSarWidth = gH1Encode.stInput.sarWidth;
		*pSarHeight= gH1Encode.stInput.sarHeight;

		return RET_OK;
	}
}

s32 H1EncodeSetInputSource( LX_VENC_INPUT_SRC_T eInputSource )
{
	gH1Encode.eInputSource = eInputSource;

	return RET_OK;
}

void H1EncodeSetTime( u32 clock )
{
	// !!! FIXME (2012/05/31)
	// 시간 측정을 위해서 임시로 만든 레지스터이며, 기존 레지스터 명칭을 바꾸지 않음,
	VENC_PREP_RdFL(prep_wbase1);
	VENC_PREP_Wr(prep_wbase1, clock);
	VENC_PREP_WrFL(prep_wbase1);
}

u32 H1EncodeGetTime( void )
{
	u32 time;
	// !!! FIXME (2012/05/31)
	// 시간 측정을 위해서 임시로 만든 레지스터이며, 기존 레지스터 명칭을 바꾸지 않음,
	VENC_PREP_RdFL(prep_wbase1);

	time = VENC_PREP_Rd(prep_wbase1);

#if 0	
	time = time / ( H1ENCODE_HW_CLOCK / 1000000 );	// microsec (㎲)
#else
	time = time / gpstVencConfig->hw_clock_mhz;	// microsec (㎲)
#endif
	//VENC_DEBUG("encoding time: %d\n", time);

	return time;	// return microsec
}

#if 0
static void H1EncodeCheckTime( H1ENCODE_T *pstEncode, u32 time )
{
	int i;
	int checkFrameCount = 10;
	u32 validValue = 0;
	u32 totalTime = 0;
	u32 avgTime = 0;
	u32 minTime = ~0;
	u32 maxTime = 0;

#if 0
	if ( checkFrameCount > H1ENCODE_CHECK_FRAME_MAX )
	{
		checkFrameCount = H1ENCODE_CHECK_FRAME_MAX;
	}
#endif

	pstEncode->arEncodeTimes[pstEncode->timeIndex] = time;
	pstEncode->timeIndex++;

	if ( pstEncode->timeIndex >= checkFrameCount )
	{
		u32 milliSec, microSec;

		for ( i = 0; i < checkFrameCount; i++ )
		{
			if (pstEncode->arEncodeTimes[i] > 0)
			{
				validValue++;
				totalTime += pstEncode->arEncodeTimes[i];

				minTime = MIN(minTime, pstEncode->arEncodeTimes[i]);
				maxTime = MAX(maxTime, pstEncode->arEncodeTimes[i]);
			}
		}

		if ( validValue > 0 )
		{
			avgTime = totalTime / validValue;

			milliSec = avgTime / 1000;
			microSec = avgTime % 1000;

			VENC_DEBUG("Encoding Time: Last %d Frames AVG[%u.%03u ms] MIN[%u ms] MAX[%u ms]\n",
				checkFrameCount, milliSec, microSec, (minTime / 1000), (maxTime / 1000));
		}
		else
		{
			VENC_DEBUG("Encoding Time: No valid values\n");
		}

		pstEncode->timeIndex = 0;
	}
}

static void H1EncodeCheckBitrate( H1ENCODE_T *pstEncode, u32 streamSize )
{
	int i;
	int checkFrameCount = H1ENCODE_CHECK_FRAME_MAX;
	u32 validValue = 0;
	u32 totalStreamSize = 0;
	u32 bitrate;

	if ( pstEncode->pstEncoder == NULL )
	{
		return;
	}

	pstEncode->arStreamSizes[pstEncode->sizeIndex] = streamSize;

	// Calculate the sum of stream sizes
	for ( i = 0; i < checkFrameCount; i++ )
	{
		if (pstEncode->arStreamSizes[i] > 0)
		{
			validValue++;
			totalStreamSize += pstEncode->arStreamSizes[i];
		}
	}

	// Calculate the bitrate at latest n frames
	if ( validValue > 0 && pstEncode->pstEncoder->frameRateDenom > 0 )
	{
		u32 tmp = totalStreamSize / validValue;
		tmp *= (u32) pstEncode->pstEncoder->frameRateNum;
		bitrate = (u32) (8 * (tmp / (u32) pstEncode->pstEncoder->frameRateDenom));

		pstEncode->u32EncodedBitrate = bitrate;

		VENC_DEBUG("Bitrate: %d\n", bitrate);
	}

	// Set the next index
	pstEncode->sizeIndex++;

	if ( pstEncode->sizeIndex >= checkFrameCount )
	{
		pstEncode->sizeIndex = 0;
	}

}
#endif

void H1EncodeUpdateStatus( u32 frameCount, u32 streamSize, u32 encodingTime )
{
	gH1Encode.u32EncodedFrames += frameCount;
	gH1Encode.u32EncodedTime = encodingTime;

	//H1EncodeCheckTime( &gH1Encode, encodingTime );
	//H1EncodeCheckBitrate( &gH1Encode, streamSize );
}

int H1EncodeReadStatus( char * buffer )
{
	int len = 0;
	const static char *COMMAND_STATUS[4] = {
		"STOP",
		"STOP_PREPARE",
		"START",
		"START_PREPARE"
	};

	if ( buffer == NULL )
	{
		return 0;
	}

	len += sprintf( buffer + len, "#### VENC Status ####\n");

	if ( gH1Encode.eStatus < 4 )
	{
		len += sprintf( buffer + len, "Command [%s]\n", COMMAND_STATUS[gH1Encode.eStatus] );
	}

	if ( gH1Encode.eStatus == H1ENCODE_STATUS_TYPE_START && gH1Encode.pstEncoder != NULL )
	{
		H1ENCODE_API_T *pstEncoder = gH1Encode.pstEncoder;

		len += sprintf( buffer + len, "Input Source Type: %s\n", (gH1Encode.eInputSource == LX_VENC_INPUT_SRC_DTV) ? "DTV":"OTHERS" );
		len += sprintf( buffer + len, "Output Format Type: %s\n", (gH1Encode.outputType == LX_VENC_ENCODE_TYPE_H264) ? "H.264":"VP8");
		len += sprintf( buffer + len, "Target Bitrate: %d bits/sec\n", gH1Encode.stInput.targetBitrate);
		len += sprintf( buffer + len, "Resolution: %d x %d, Framerate: %d/%d\n", pstEncoder->width, pstEncoder->height, pstEncoder->frameRateDenom, pstEncoder->frameRateNum);
		len += sprintf( buffer + len, "GOP Length: %d\n", pstEncoder->gopLength);

	#ifdef H1ENCODE_ENABLE_SAR
		len += sprintf( buffer + len, "SAR: %d:%d\n", pstEncoder->decWidth, pstEncoder->decHeight );
	#endif
	}
	len += sprintf( buffer + len, "\n");
	
	len += sprintf( buffer + len, "Buffer Status:\n");
	if ( gH1Encode.pstEncoder != NULL )
	{
		H1ENCODE_API_T *pstEncoder = gH1Encode.pstEncoder;
	
		len += sprintf( buffer + len, "     ES Buffer: 0x%08X ~ 0x%08X [0x%08X]\n", gH1Encode.memESB.u32Phys, gH1Encode.memESB.u32Phys + gH1Encode.memESB.u32Size, pstEncoder->memESB.u32WriteOffset);
		len += sprintf( buffer + len, "    AUI Buffer: 0x%08X ~ 0x%08X [0x%08X]\n", gH1Encode.memAUI.u32Phys, gH1Encode.memAUI.u32Phys + gH1Encode.memAUI.u32Size, pstEncoder->memAUI.u32WriteOffset);
	}
	else
	{
		len += sprintf( buffer + len, "     ES Buffer: 0x%08X ~ 0x%08X\n", gH1Encode.memESB.u32Phys, gH1Encode.memESB.u32Phys + gH1Encode.memESB.u32Size);
		len += sprintf( buffer + len, "    AUI Buffer: 0x%08X ~ 0x%08X\n", gH1Encode.memAUI.u32Phys, gH1Encode.memAUI.u32Phys + gH1Encode.memAUI.u32Size);
	}

	len += sprintf( buffer + len, "Vsync Status: Total[%d], Skip[%d]\n", gH1Encode.ui32TotalVsync, gH1Encode.ui32CountFrameSkip);
	len += sprintf( buffer + len, "\n");

	len += HXENC_MemallocReadStatus( buffer + len );
	
#ifdef H1ENCODE_DEBUG_MEM_LEAK
	extern atomic_t aui_count;
	extern atomic_t api_count;

	len += sprintf( buffer + len, "DEBUG Check MEM Leak:\n");
	len += sprintf( buffer + len, "    ENC PARAM COUNT: %d\n", encparams_count.counter);
	len += sprintf( buffer + len, "          API COUNT: %d\n", api_count.counter);
	len += sprintf( buffer + len, "          AUI COUNT: %d\n", aui_count.counter);
	len += sprintf( buffer + len, "\n");
#endif

	len += HXENC_ReadVersion( buffer + len );

	return len;
}

static u32 H1EncodeReadSTC( LX_VENC_TIMESTAMP_TYPE_T eTimestamp )
{
	u32 stc;
	
	switch( eTimestamp )
	{
		case LX_VENC_TIMESTAMP_PTS:
		{
			VENC_PREP_RdFL( info );
			stc = VENC_PREP_Rd( info );
		}
		break;
#ifdef H1ENCODE_USE_PREP_STC		
		case LX_VENC_TIMESTAMP_STC:
		{
			VENC_PREP_RdFL( stc );
			stc = VENC_PREP_Rd( stc );
		}
		break;
#endif
		case LX_VENC_TIMESTAMP_GSTC:
		default:
		{
			VENC_PREP_RdFL(vdec_gstcc0);
			VENC_PREP_Rd01(vdec_gstcc0, gstcc0, stc);
		}
		break;
	}

	return stc;

}

static void H1EncodeUpdateTimestamp( void )
{
#ifdef H1ENCODE_USE_VDEC_PTS
	if ( gH1Encode.eInputSource == LX_VENC_INPUT_SRC_DTV )
	{
		//gH1Encode.u32CurrentPTS = H1EncodeReadSTC( LX_VENC_TIMESTAMP_PTS ) & 0x0FFFFFFF;	// use 28bits
		gH1Encode.u32CurrentPTS = H1EncodeReadSTC( LX_VENC_TIMESTAMP_PTS ) & 0x7FFFFFFF;	// use 31bits(2013.06.25)
	}
	else
#endif
	{
		gH1Encode.u32CurrentPTS = H1EncodeReadSTC( LX_VENC_TIMESTAMP_GSTC );
	}
}

u32 H1EncodeGetTimestamp( void )
{
	return gH1Encode.u32CurrentPTS;
}

int H1EncodeCheckIPCRegister( void )
{
	H1ENCODE_PREP_STATUS_T stPrepStatus;
	
	H1EncodeReadPREP(&stPrepStatus);

	if ( stPrepStatus.decWidth == 0 && stPrepStatus.decHeight == 0 )
	{
		return RET_ERROR;
	}

	if ( stPrepStatus.decWidth > 1920 && stPrepStatus.decHeight > 1088 )
	{
		// Can't support frame size.
		return RET_ERROR;
	}

#if 0
	if ( (gH1Encode.stInput.width != stPrepStatus.decWidth) ||
		(gH1Encode.stInput.height != stPrepStatus.decHeight) )
	{
		VENC_WARN("Different between setting value and ipc register.\n");
	}
#endif

	return RET_OK;	
}

#ifdef H1ENCODE_ENABLE_AUTO_RESTART
/**
 * Diff two H1ENCODE_PREP_STATUS_T value.
 *
 * @param pstPrepA first H1ENCODE_PREP_STATUS_T
 * @param pstPrepB second H1ENCODE_PREP_STATUS_T
 * @retrun 0: not chaged, non-zero: something chaged.
 */
static BOOLEAN H1EncodeDiffPREP( H1ENCODE_PREP_STATUS_T *pstPrepA, H1ENCODE_PREP_STATUS_T *pstPrepB )
{
	if ( pstPrepA == NULL || pstPrepB == NULL )
	{
		return TRUE;
	}

	if ( pstPrepA->inWidth != pstPrepB->inWidth )
	{
		return TRUE;
	}

	if ( pstPrepA->inHeight != pstPrepB->inHeight )
	{
		return TRUE;
	}

	if ( pstPrepA->yStride != pstPrepB->yStride )
	{
		return TRUE;
	}
	
	if ( pstPrepA->cStride != pstPrepB->cStride )
	{
		return TRUE;
	}
	
	if ( pstPrepA->frameType == LX_VENC_FRAME_TYPE_PROGRESSIVE && 
		pstPrepB->frameType != LX_VENC_FRAME_TYPE_PROGRESSIVE )
	{
		return TRUE;
	}
 	else if ( pstPrepA->frameType != LX_VENC_FRAME_TYPE_PROGRESSIVE &&
		pstPrepB->frameType == LX_VENC_FRAME_TYPE_PROGRESSIVE )
 	{
		return TRUE;
	}

	return FALSE; // size is not chagned.
}

#endif

#ifdef H1ENCODE_ENABLE_SAR
static BOOLEAN H1EncodeDiffPREP_SAR( H1ENCODE_PREP_STATUS_T *pstPrepA, H1ENCODE_PREP_STATUS_T *pstPrepB )
{
	if ( pstPrepA == NULL || pstPrepB == NULL )
	{
		return TRUE;
	}

	if ( pstPrepA->decWidth != pstPrepB->decWidth )
	{
		return TRUE;
	}
	
	if ( pstPrepA->decHeight != pstPrepB->decHeight )
	{
		return TRUE;
	}	

	return FALSE; // size is not chagned.
}
#endif

static void _PREPReadStatusInternal( H1ENCODE_PREP_STATUS_T *pstPrepStatus )
{
	BOOLEAN _enableFlipTB = FALSE;
	
	// 사이즈, 포맷 정보를 PREP 레지스터에서 읽어옴.	 (DE/VDEC에서 기록하는 내용)
	u32 yBase, cBase;
	int inHeight, inWidth;
	int yStride, cStride;
	int frm_rate;
	H1ENCODE_FRAME_TYPE_T frameType;
	BOOLEAN	topFieldFirst = FALSE;
		
#ifdef H1ENCODE_ENABLE_SAR	
	int decHeight, decWidth;
#endif

	if ( pstPrepStatus == NULL )
	{
		return;
	}

	// Frame base Address (Y)
	VENC_PREP_RdFL(y_addr);
	VENC_PREP_Rd01(y_addr, addr, yBase);

	// Frame base Address (CbCr)	
	VENC_PREP_RdFL(c_addr);
	VENC_PREP_Rd01(c_addr, addr, cBase);

	// Size
	VENC_PREP_RdFL(de_dsize);
	VENC_PREP_Rd02(de_dsize, vsize, inHeight, hsize, inWidth);

#ifdef H1ENCODE_ENABLE_SAR
	// SAR Infomation
	VENC_PREP_RdFL(decsize);
	VENC_PREP_Rd02(decsize, dec_vsize, decHeight, dec_hsize, decWidth);
#endif

	// Frame Type (Field/Frame)
	VENC_PREP_RdFL(pic_info);
	VENC_PREP_Rd01(pic_info, frm_struct, frameType);

#ifdef H1ENCODE_ENABLE_FIELDORDER
	// Top Field First (True/False)
	VENC_PREP_RdFL(pic_info);
	VENC_PREP_Rd01(pic_info, topfield_first, topFieldFirst);
#else
	// Always set top-field first flag.
	topFieldFirst = TRUE;
#endif

	// Stride (Y/C)
	VENC_PREP_RdFL(y_stride);
	VENC_PREP_Rd01(y_stride, stride, yStride);
	VENC_PREP_RdFL(c_stride);
	VENC_PREP_Rd01(c_stride, stride, cStride);

	// Sequence Info
	VENC_PREP_RdFL(seqinfo);
	VENC_PREP_Rd01(seqinfo, frm_rate, frm_rate);

#ifdef H1ENCODE_DEBUG_SWAP_TB
	_enableFlipTB = TRUE;
#else
	_enableFlipTB = GET_VENC_CFG_CTRL_INDEX( VENC_CFG_CTRL_FLIP_TB );
#endif

	if ( _enableFlipTB )
	{
		if ( frameType == LX_VENC_FRAME_TYPE_TOP )
		{
			frameType = LX_VENC_FRAME_TYPE_BOTTOM;
		}
		else if ( frameType == LX_VENC_FRAME_TYPE_BOTTOM )
		{
			frameType = LX_VENC_FRAME_TYPE_TOP;
		}
	}

	pstPrepStatus->yBase = yBase;
	pstPrepStatus->cBase = cBase;

	pstPrepStatus->inWidth = inWidth;
	pstPrepStatus->inHeight = inHeight;
	pstPrepStatus->yStride = yStride;
	pstPrepStatus->cStride = cStride;
	pstPrepStatus->frameType = frameType;
	pstPrepStatus->frameRateCode = frm_rate;
	pstPrepStatus->topFieldFirst = topFieldFirst;

#ifdef H1ENCODE_ENABLE_SAR
	pstPrepStatus->decWidth = decWidth;
	pstPrepStatus->decHeight = decHeight;
#endif

#if 0
	VENC_PRINT("[PREP] width: %d, height: %d\n", inWidth, inHeight);
	VENC_PRINT("[PREP] yStride: %d, cStride: %d\n", yStride, cStride);
	VENC_PRINT("[PREP] frameType: %d\n", frameType);
#endif

#if 0
	if ( inWidth == 0 || inHeight == 0 )
	{
		VENC_WARN("Resolution error.\n");
		//return RET_ERROR;
	}

	if ( yStride == 0 || cStride == 0)
	{
		VENC_WARN("Stride error.\n");
		//return RET_ERROR;
	}
#endif

	//return RET_OK;
}

static void H1EncodeReadPREP(H1ENCODE_PREP_STATUS_T *pstPrepStatus)
{
// INCLUDE_M14_CHIP_KDRV
	if ( lx_chip_rev() >= LX_CHIP_REV(M14, B0) && lx_chip_rev() < LX_CHIP_REV(H14, A0) )
	{
		UINT32 IPCInternalBase = (UINT32)gpRealRegVENCPREP + 0xA0;
		UINT32 IPCExternalBase = (UINT32)gpIPCExternal;
		
		//printk("IPCInternalBase=0x%08X, IPCExternalBase=0x%08X\n", IPCInternalBase, IPCExternalBase);

		// Copy ipc register values.
		memcpy( (void *)IPCInternalBase, (void *)IPCExternalBase, 0x40 );
	}

	_PREPReadStatusInternal( pstPrepStatus );
}

static void H1EncodeUpdatePREP( void )
{
	H1ENCODE_PREP_STATUS_T stPrepStatus = {0};

	//memset( &stPrepStatus, 0x0, sizeof(H1ENCODE_PREP_STATUS_T) );

	// Read input frame infomation from PREP register (DE or VDEC writed)
	H1EncodeReadPREP( &stPrepStatus );

#ifdef H1ENCODE_ENABLE_AUTO_RESTART
	if ( H1EncodeDiffPREP( &gH1Encode.stPrepStatus, &stPrepStatus ) )
	{
		// if chagend input format (size, frame type, ... )
		// Restart encoding ( stop -> start )
	}
#endif

#ifdef H1ENCODE_ENABLE_SAR
	if ( H1EncodeDiffPREP_SAR( &gH1Encode.stPrepStatus, &stPrepStatus ) )
	{
		// NEED Modify
	}
#endif

#ifdef H1ENCODE_USE_STRUCT_ASSIGNMENT
	gH1Encode.stPrepStatus = stPrepStatus;
#else
	memcpy( &gH1Encode.stPrepStatus, &stPrepStatus, sizeof(H1ENCODE_PREP_STATUS_T) );
#endif

	_EventPost( H1ENCODE_EVENT_THUMBNAIL );
	
}

static void	H1EncodeMeasureFramerate( void )
{
	if ( gH1Encode.stFramerate.count > 5 )
	{
		int i = 0;
		u32 nFramerate = 0;
		u32 diff_stc = 0;

		// drop first stc.
		for ( i = 2; i < 6; i++ )
		{
			diff_stc += gH1Encode.stFramerate.gstc[i] - gH1Encode.stFramerate.gstc[i-1];
		}

		diff_stc = diff_stc >> 2;
		nFramerate = 90000/diff_stc;

		if ( (90000%diff_stc) > (diff_stc>>1) )
			nFramerate += 1;

		VENC_ERROR("avg_diff_stc = %d, nFramerate = %d\n", diff_stc, nFramerate);

		H1EncodeSetFramerate( nFramerate );

		_EventPost( H1ENCODE_EVENT_FRAMERATE );
	}
	else
	{
		gH1Encode.stFramerate.gstc[gH1Encode.stFramerate.count] = H1EncodeReadSTC( LX_VENC_TIMESTAMP_GSTC );
		gH1Encode.stFramerate.count++;
	}
}

static void H1EncodeInputDetect( void )
{
	H1ENCODE_PREP_STATUS_T stPrepStatus;
	
	H1EncodeReadPREP( &stPrepStatus );

	if ( stPrepStatus.inWidth > 0 && stPrepStatus.inHeight > 0 )
	{
		_EventPost( H1ENCODE_EVENT_INPUT );
	}

	return;
}

#define CALC_SIZE_YUV420( width, height ) ( ((width) * (height)) + ((width) * (height) / 2) )

static int _FrameCopy420( void *pY, void *pC, void *pTarget, u16 width, u16 height )
{
	int ySize = width * height;
	int cSize = width * height / 2;

	//ySize = MROUND( ySize, 32 );
	//cSize = MROUND( cSize, 32 );

	if ( pY == NULL || pC == NULL || pTarget == NULL )
	{
		return RET_ERROR;
	}

	// Copy Y Plane
	memcpy( (void *)pTarget, (void *)pY, ySize );

	// Copy C Plane
	memcpy( (void *)pTarget + ySize, (void *)pC, cSize );

	return RET_OK;
}

//
// Convert scan type from Interlaced to progressive
//
static int _ConvertScanI2P420( void *pSource, void *pTarget, u16 width, u16 height )
{
	//int totalLine = height + (height >> 1);
	u32 sourceOffset, targetOffset;
	u32 ySize = 0;
	void *tmp = NULL;
	int i;

	if ( pSource == NULL || pTarget == NULL )
	{
		return RET_ERROR;
	}

	if ( pSource == pTarget )
	{
		int totalSize = CALC_SIZE_YUV420( width, height );
		tmp = kmalloc( totalSize, GFP_KERNEL );

		memcpy( tmp, pSource, totalSize );

		pSource = tmp;
	}

	ySize = width * height;

	// Convert Y,C Plane
	for ( i = 0; i < (height / 2 + height /4); i++ )
	{
		sourceOffset = width * i;
		targetOffset = width * 2 * i;

		// odd
		memcpy( pTarget + targetOffset, pSource + sourceOffset, width );

		// even
		memcpy( pTarget + targetOffset + width, pSource + sourceOffset, width );
	}

	if ( tmp != NULL )
	{
		kfree( tmp );
	}

	return RET_OK;
}

//
// Convert image format (YUV420SemiPlanar -> YUV420Planar)
//
static int _ConvertYUV420_SP2P( void *pSource, void *pTarget, u16 width, u16 height )
{
	void *tmp = NULL;
	int i, j;
	int ySize, cbSize, crSize;
	u8 *cb, *cr;

	if ( pSource == NULL || pTarget == NULL )
	{
		return RET_ERROR;
	}

	if ( pSource == pTarget )
	{
		int totalSize = CALC_SIZE_YUV420( width, height );
		tmp = kmalloc( totalSize, GFP_KERNEL );

		memcpy( tmp, pSource, totalSize );

		pSource = tmp;
	}

	ySize = width * height;
	cbSize = (ySize >> 2);
	crSize = (ySize >> 2);

	cb = (u8 *)(pTarget + ySize + 0);
	cr = (u8 *)(pTarget + ySize + cbSize);

	// Copy Y Plane
	memcpy( pSource, pTarget, ySize );

	// Convert C Plane
	for ( i = 0; i < height / 4; i++ )
	{
		UINT32 sourceOffset = ySize + (i * width * 2);
		UINT32 targetOffset = i * width;

		for ( j = 0; j < width; j ++ )
		{
			cb[targetOffset + j] = *(char *)(pSource + sourceOffset++);
			cr[targetOffset + j] = *(char *)(pSource + sourceOffset++);
		}
	}

	if ( tmp != NULL )
	{
		kfree( tmp );
	}

	return RET_OK;
}

#if 0
static int _FrameRemovePadding420( void *pSource, void *pTarget, u16 width, u16 height, u16 stride )
{
	int totalLine = height + (height >> 1);
	int i;

	if ( pSource == NULL || pTarget == NULL )
	{
		return RET_ERROR;
	}

	for ( i = 0; i < totalLine; i++ )
	{
		u32 sourceOffset = stride * i;
		u32 targetOffset = width * i;

		memcpy( pTarget + targetOffset, pSource + sourceOffset, width );
	}

	return RET_OK;

}
#endif

int H1EncodeGetFrameImage( LX_VENC_RECORD_FRAME_IMAGE_T *pstFrameImage )
{
	int ret = RET_ERROR;

	if ( pstFrameImage == NULL )
	{
		return ret;
	}

	ret = _EventWait( H1ENCODE_EVENT_THUMBNAIL, 100 );

	if ( ret != RET_OK )
	{
		VENC_WARN("Event timeout!!!\n");
		ret = RET_ERROR;
	}
	else
	{
		u32 yBase = gH1Encode.stPrepStatus.yBase;
		u32 cBase = gH1Encode.stPrepStatus.cBase;
		u16 width = gH1Encode.stPrepStatus.inWidth;
		u16 height = gH1Encode.stPrepStatus.inHeight;
		u16 stride = gH1Encode.stPrepStatus.yStride;
		u32 size = height * stride;

		LX_VENC_RECORD_FRAME_TYPE_T frameType = gH1Encode.stPrepStatus.frameType;

		void *pY = NULL;
		void *pC = NULL;

		pY = ioremap( yBase, size );
		pC = ioremap( cBase, size );

		if ( pY == NULL || pC == NULL )
		{
			goto Release;
		}
		else
		{
			u32 cbOffset, crOffset;

			if ( frameType == LX_VENC_RECORD_FRAME_TYPE_PROGRESSIVE )
			{
				_FrameCopy420( pY, pC, (void *)gH1Encode.memTFB.u32Virt, stride, height );
			}
			else
			{
				_FrameCopy420( pY, pC, (void *)gH1Encode.memTFB.u32Virt, stride, height/2 );
				_ConvertScanI2P420( (void *)gH1Encode.memTFB.u32Virt, (void *)gH1Encode.memTFB.u32Virt, stride, height );
			}

			_ConvertYUV420_SP2P( (void *)gH1Encode.memTFB.u32Virt, (void *)gH1Encode.memTFB.u32Virt, stride, height );

			cbOffset = (stride * height);
			crOffset = cbOffset + (stride * height / 4);

			pstFrameImage->type = LX_VENC_RECORD_FRAME_IMAGE_TYPE_420_PLANAR;
			pstFrameImage->ui32Size = CALC_SIZE_YUV420( stride, height );
			pstFrameImage->ui32Height = height;
			pstFrameImage->ui32Width = width;
			pstFrameImage->ui32Stride = stride;
			pstFrameImage->ui32YPhyAdd = gH1Encode.memTFB.u32Phys;
			pstFrameImage->ui32UPhyAdd = gH1Encode.memTFB.u32Phys + cbOffset;
			pstFrameImage->ui32VPhyAdd = gH1Encode.memTFB.u32Phys + crOffset;

			 ret = RET_OK;
		}

Release:
		if ( pY != (void *)-1 )
		{
			iounmap( pY );
		}

		if ( pC != (void *)-1 )
		{
			iounmap( pC );
		}

	}

	return ret;
}

static void _CheckFramerateInformation( void )
{
	H1ENCODE_PREP_STATUS_T stPrepStatus;
			
	if ( gH1Encode.stInput.frameRateDenom > 0 && gH1Encode.stInput.frameRateNum > 0 )
	{
		return;
	}

	H1EncodeReadPREP( &stPrepStatus );

	if ( stPrepStatus.frameRateCode == LX_VENC_FRAME_RATE_15HZ )
	{
		// Read ipc again..
		msleep_interruptible( 150 );
		H1EncodeReadPREP( &stPrepStatus );
	}

	if ( stPrepStatus.frameRateCode != LX_VENC_FRAME_RATE_NONE )
	{
		return;
	}
	else
	{
		// In the normal situatin, can't reach to this code block
		// because frame rate information from DE IPC are always valid.		
		int ev_ret = RET_ERROR;
		
		gH1Encode.stFramerate.count = 0;
		memset( gH1Encode.stFramerate.gstc, 0x0, sizeof(gH1Encode.stFramerate.gstc) );

		gH1Encode.enableFramerateDetect = TRUE;
		
		// Auto-detected Framerate.
		_InterruptEnableVsync( TRUE );

		// Wait until measure framerate
		ev_ret = _EventWait( H1ENCODE_EVENT_FRAMERATE, 500 );

		gH1Encode.enableFramerateDetect = FALSE;

		_InterruptEnableVsync( FALSE );
	}
	
}

static int _MakeFramerateInformation( H1ENCODE_API_T * pstEncoder, LX_VENC_RECORD_FRAME_RATE_T eFramerate )
{
	if ( pstEncoder == NULL )
	{
		return RET_ERROR;
	}
	
	// 20130207: Fix for frame rate problem.
	if ( gH1Encode.stInput.frameRateDenom == 0 || gH1Encode.stInput.frameRateNum == 0 )
	{
		VENC_PRINT("The framerate set to a value from PREP register.\n");
		
		pstEncoder->SetFrameRate( pstEncoder, eFramerate );
	}
	else
	{
		VENC_PRINT("The framerate set to a value from KADP. [%d,%d,%d]\n", 
			gH1Encode.stInput.frameRateCode, gH1Encode.stInput.frameRateDenom, gH1Encode.stInput.frameRateNum);
		
		pstEncoder->frameRateCode = gH1Encode.stInput.frameRateCode;
		pstEncoder->frameRateDenom = gH1Encode.stInput.frameRateDenom;
		pstEncoder->frameRateNum = gH1Encode.stInput.frameRateNum;
	}

	if ( pstEncoder->frameRateDenom == 0 || pstEncoder->frameRateNum == 0 )
	{
		int ev_ret = RET_ERROR;
		
		gH1Encode.stFramerate.count = 0;
		memset( gH1Encode.stFramerate.gstc, 0x0, sizeof(gH1Encode.stFramerate.gstc) );

		gH1Encode.enableFramerateDetect = TRUE;
		
		// Auto-detected Framerate.
		_InterruptEnableVsync( TRUE );

		// Wait until measure framerate
		ev_ret = _EventWait( H1ENCODE_EVENT_FRAMERATE, 500 );

		gH1Encode.enableFramerateDetect = FALSE;

		_InterruptEnableVsync( FALSE );

		if ( gH1Encode.stInput.frameRateCode > 0 
			&& gH1Encode.stInput.frameRateDenom > 0 
			&& gH1Encode.stInput.frameRateNum > 0 )
		{
			VENC_PRINT("The framerate set to Auto-Dectection. [%d,%d,%d]\n", 
				gH1Encode.stInput.frameRateCode, gH1Encode.stInput.frameRateDenom, gH1Encode.stInput.frameRateNum);
			
			pstEncoder->frameRateCode = gH1Encode.stInput.frameRateCode;
			pstEncoder->frameRateDenom = gH1Encode.stInput.frameRateDenom;
			pstEncoder->frameRateNum = gH1Encode.stInput.frameRateNum;
		}
		else
		{
			H1ENCODE_PREP_STATUS_T stPrepStatus;

			H1EncodeReadPREP( &stPrepStatus );
			
			// Set Defatult framerate
			if ( stPrepStatus.frameType == LX_VENC_FRAME_TYPE_PROGRESSIVE )
			{
				H1EncodeSetFramerate( 30 );
			}
			else
			{
				H1EncodeSetFramerate( 60 );
			}
		}
	}

	return RET_OK;
}

static UINT32 _GetResolutionType( const UINT32 width, const UINT32 height )
{
	enum {
		RESOLUTION_UNKNOWN = 0,
		RESOLUTION_SD,
		RESOLUTION_HD,
		RESOLUTION_FHD,
	};
	UINT32 res = width * height;
	UINT32 resType = RESOLUTION_UNKNOWN;

	if ( res == 0 )
	{
		resType = RESOLUTION_UNKNOWN;
	}
	else if ( res <= 720 * 576 )
	{
		resType = RESOLUTION_SD;
	}
	else if ( res <= 1280 * 720 )
	{
		resType = RESOLUTION_HD;
	}
	else if ( res <= 1920 * 1088 )
	{
		resType = RESOLUTION_FHD;
	}

	return resType;
}

static BOOLEAN _CheckInputResolution( H1ENCODE_PREP_STATUS_T *pstPrepStatus )
{
	int retry = 5;
	int i = 0;
	UINT8 resType = 0;
	BOOLEAN matched = FALSE;
	
	if ( pstPrepStatus == NULL )
	{
		return FALSE;
	}

	resType = _GetResolutionType( gH1Encode.stInput.width, gH1Encode.stInput.height );

	if ( resType == 0  )
	{
#if 0	
		// Wait static delay for input transition.
		msleep_interruptible( 150 );
		H1EncodeReadPREP( pstPrepStatus );
#else
		int ev_ret = RET_ERROR;

		msleep_interruptible( 150 );
		
		// Wait until valid sync interrupt.
		gH1Encode.enableInputDetect = TRUE;

		_EventClear();
		_InterruptEnableVsync( TRUE );

		// Wait until vsync occured.
		ev_ret = _EventWait( H1ENCODE_EVENT_INPUT, 300 );

		gH1Encode.enableInputDetect = FALSE;

		_InterruptEnableVsync( FALSE );

		H1EncodeReadPREP( pstPrepStatus );
#endif

		return FALSE;
	}

	// 20131031(jaeseop.so)
	// WebOS에서 HAL을 통해서 width,height정보가 내려오지 않기 때문에
	// 아래 비교하는 로직으로 들어가는 경우는 없음.
	// 향후 사용할 수도 있기에 남겨둠.
	for( i = 0; i < retry; i++ )
	{
		H1ENCODE_PREP_STATUS_T stPrepStatus;
		UINT32 resTypeIPC = 0;
		
		H1EncodeReadPREP( &stPrepStatus );

		resTypeIPC = _GetResolutionType( stPrepStatus.inWidth, stPrepStatus.inHeight );

		if( resType == resTypeIPC )
		{
			// Copy ipc register values to input parameter.
			*pstPrepStatus = stPrepStatus;
			
			matched = TRUE;
			break;
		}

		VENC_PRINT("SET[%d,%d] INPUT[%d,%d]\n", 
			gH1Encode.stInput.width, gH1Encode.stInput.height,
			stPrepStatus.inWidth, stPrepStatus.inHeight);
		VENC_INFO("Delay 50ms for input transition.\n");
		
		msleep_interruptible( 50 );
	}

	return matched;
}

static int H1EncodeStart( H1ENCODE_PREP_STATUS_T stPrepStatus )
{
	int ret = RET_ERROR;
	H1ENCODE_API_T * pstEncoder;

	TRACE_ENTER();

	if ( gH1Encode.pstEncoder != NULL )
	{
		// Already allocate encoder instance
		return RET_ERROR;
	}

	if ( stPrepStatus.inWidth == 0 || stPrepStatus.inHeight == 0 )
	{
		VENC_WARN("Input resolution error.(%dx%d)\n", stPrepStatus.inWidth, stPrepStatus.inHeight );
		return RET_ERROR;
	}
	
	H1ENCODE_LOCK();

	if (gH1Encode.outputType == LX_VENC_ENCODE_TYPE_VP8)
	{
		gH1Encode.pstEncoder = VP8Alloc();
	}
	else
	{
		gH1Encode.pstEncoder = H264Alloc();
	}

	gH1Encode.ui32TotalVsync = 0;
#ifdef H1ENCODE_ENABLE_DROP_FIRST_FRAME		
	gH1Encode.remain_drop = H1ENCODE_DROP_FIRST_FRAME_COUNT;
#endif

	H1ENCODE_UNLOCK();
	
	pstEncoder = gH1Encode.pstEncoder;

	if ( pstEncoder == NULL )
	{
		return RET_ERROR;
	}

	// Check function pointers
	if ( pstEncoder->SetMemOSB == NULL || pstEncoder->SetMemESB == NULL || pstEncoder->SetMemAUI == NULL )
	{
		VENC_PRINT("Memory setter function is NULL.\n");
		ret = RET_ERROR;
		goto func_exit;
	}

	if ( pstEncoder->SetFrameType == NULL || pstEncoder->SetFrameSize == NULL || 
		pstEncoder->SetGOPLength == NULL || pstEncoder->SetFrameRate == NULL ||
		pstEncoder->SetBitrate == NULL )
	{
		VENC_PRINT("Config setter function is NULL.\n");
		ret = RET_ERROR;
		goto func_exit;
	}

	if ( pstEncoder->Open == NULL || pstEncoder->Close == NULL )
	{
		VENC_PRINT("Open or Close function is NULL.\n");
		ret = RET_ERROR;
		goto func_exit;
	}

	// Set buffer infomation
	pstEncoder->SetMemOSB( pstEncoder, gH1Encode.memOSB );
	pstEncoder->SetMemESB( pstEncoder, gH1Encode.memESB );
	pstEncoder->SetMemAUI( pstEncoder, gH1Encode.memAUI );

	// Init configuration
	pstEncoder->eFrameType = stPrepStatus.frameType;
	pstEncoder->topFieldFirst = stPrepStatus.topFieldFirst;
	pstEncoder->width = stPrepStatus.inWidth;
	pstEncoder->height = stPrepStatus.inHeight;
	pstEncoder->stride = stPrepStatus.yStride;
	if (  gH1Encode.stInput.gopLength < 1 || gH1Encode.stInput.gopLength > 300)
	{
		pstEncoder->gopLength = H1ENCODE_GOPLENGTH_DEFAULT;
	}
	else
	{
		pstEncoder->gopLength = gH1Encode.stInput.gopLength;
	}
	
	_MakeFramerateInformation( pstEncoder, stPrepStatus.frameRateCode );

#ifdef	H1ENCODE_ENABLE_DELAYED_ENCODING
	if ( pstEncoder->frameRateDenom > 0 && pstEncoder->frameRateNum > 0 )
	{
		maxDelayedTime = ( pstEncoder->frameRateDenom * 1000 / pstEncoder->frameRateNum );
		//maxDelayedTime = (maxDelayedTime * 2) + (maxDelayedTime>>1);	// x2.5
		maxDelayedTime = (maxDelayedTime * 2) - (maxDelayedTime>>3);	// x1.875
	}
	else
	{
		maxDelayedTime = 32;
	}
#endif

	if ( gH1Encode.stInput.targetBitrate > 0 )
	{
		pstEncoder->bitrate = gH1Encode.stInput.targetBitrate;
	}
	else if ( pstEncoder->bitrate == 0 )
	{
		if ( pstEncoder->width == 1920 && pstEncoder->height == 1080 )
		{
			pstEncoder->bitrate = H1ENCODE_BITRATE_FHD;
		}
		else
		{
			pstEncoder->bitrate = H1ENCODE_BITRATE_HD;
		}
	}

	if ( pstEncoder->bitrate > 0 )
	{
		pstEncoder->bEnableCBR = gH1Encode.stInput.bEnableCBR;
	}

#ifdef H1ENCODE_ENABLE_SAR
	// Set SAR Infomation
	if ( gH1Encode.stInput.sarWidth > 0 && gH1Encode.stInput.sarHeight > 0 )
	{
		pstEncoder->decWidth = gH1Encode.stInput.sarWidth;
		pstEncoder->decHeight = gH1Encode.stInput.sarHeight;
	}
	else
	{
		//pstEncoder->decWidth = stPrepStatus.decWidth;
		//pstEncoder->decHeight = stPrepStatus.decHeight;
		pstEncoder->decWidth = 0;
		pstEncoder->decHeight = 0;
	}
#endif

	H1ENCODE_LOCK();

	// Open encoder instance ( h.264 or VP8 )
	ret = pstEncoder->Open( pstEncoder );

	H1ENCODE_UNLOCK();

func_exit:
	if ( ret != RET_OK && pstEncoder != NULL )
	{
		if (gH1Encode.outputType == LX_VENC_ENCODE_TYPE_VP8)
		{
			VP8Release( pstEncoder );
		}
		else
		{
			H264Release( pstEncoder );
		}
		
		gH1Encode.pstEncoder = NULL;
	}
	
	return ret;
}

static int H1EncodeStop( void )
{
	int ret = RET_ERROR;
	int wait_count = 50;
	
	H1ENCODE_API_T *pstEncoder;

	TRACE_ENTER();

	if ( gH1Encode.pstEncoder == NULL )
	{
		return RET_ERROR;
	}

	while( gH1Encode.encode_count > 0 ) 
	{
		if ( wait_count == 0 )
		{
			VENC_PRINT("wait_count is zero.\n");
			break;
		}
		
		// Wait until idle state
		msleep_interruptible(10);
		wait_count--;
	}
	
	pstEncoder = gH1Encode.pstEncoder;

	if ( pstEncoder->Close == NULL )
	{
		VENC_PRINT("Close function is NULL.\n");
		return RET_ERROR;
	}

	H1ENCODE_LOCK();

#ifdef H1ENCODE_ENABLE_SCD
	// free resource to avoid memory leak. 
	if ( gH1Encode.pstEncParamsPrev != NULL )
	{
		_EncParamFree( gH1Encode.pstEncParamsPrev );
		gH1Encode.pstEncParamsPrev = NULL;
	}
#endif

	ret = pstEncoder->Close( pstEncoder );

	H1ENCODE_UNLOCK();

	// Flush workqueue to avoid NULL pointer access after H1EncodeClose().
	_WorkQFlush();

	H1ENCODE_LOCK();

	if (gH1Encode.outputType == LX_VENC_ENCODE_TYPE_VP8)
	{
		VP8Release( pstEncoder );
	}
	else
	{
		H264Release( pstEncoder );
	}

	gH1Encode.stInput.frameRateCode = 0;
	gH1Encode.stInput.frameRateDenom = 0;
	gH1Encode.stInput.frameRateNum = 0;
	gH1Encode.stInput.width = 0;
	gH1Encode.stInput.height = 0;
	gH1Encode.pstEncoder = NULL;
	
#ifdef	H1ENCODE_ENABLE_DELAYED_ENCODING
	maxDelayedTime = 0;
#endif

	H1ENCODE_UNLOCK();
	
	return ret;
}

#ifdef H1ENCODE_DEBUG_INTERLACED_DUMP
static void _DebugSaveField( void )
{
	u32 yBase;
	H1ENCODE_FRAME_TYPE_T frameType;
	u32 topBase = gH1Encode.memTFB.u32Virt;
	u32 bottomBase = gH1Encode.memTFB.u32Virt + 0x220000;

	yBase = gH1Encode.stPrepStatus.yBase;
	frameType = gH1Encode.stPrepStatus.frameType;

	u32 yVirt = (u32)ioremap( yBase, 2048 * 100 );

	static int top;
	static int bottom;

	if ( top == 0 && frameType == LX_VENC_FRAME_TYPE_TOP )
	{
		VENC_PRINT("[T] 0x%08x\n", gH1Encode.memTFB.u32Phys);
		memcpy( (void *)topBase, (void *)yVirt, 2048*100 );

		top = 1;
	}
	else if ( bottom == 0 && frameType == LX_VENC_FRAME_TYPE_BOTTOM )
	{
		VENC_PRINT("[B] 0x%08x\n", gH1Encode.memTFB.u32Phys + 0x220000);
		memcpy( (void *)bottomBase, (void *)yVirt, 2048*100 );

		bottom = 1;
	}

	iounmap( (void *)yVirt );
}
#endif

static int H1EncodeEncode( H1ENCODE_PREP_STATUS_T *pstPrep, u32 gstc )
{
	int ret;
	H1ENCODE_FRAME_TYPE_T frameType;
	H1ENCODE_API_T *pstEncoder;
	H1ENCODE_ENC_PARAMS_T *pstEncParams;

	if ( gH1Encode.pstEncoder == NULL || pstPrep == NULL )
	{
		return RET_ERROR;
	}

	pstEncoder = gH1Encode.pstEncoder;
	
	pstEncParams = _EncParamAlloc();

	if ( pstEncParams == NULL )
	{
		VENC_PRINT("H1EncodeEncode: Can't allocate pstEncParams.\n");
		return RET_ERROR;
	}
	
	pstEncParams->u32YBase = pstPrep->yBase;
	pstEncParams->u32CBase = pstPrep->cBase;
	pstEncParams->u32Timestamp 	= gstc;
	pstEncParams->frameType = pstPrep->frameType;	// for determining field or frame

#ifdef H1ENCODE_ENABLE_SCD
	if ( GET_VENC_CFG_CTRL_INDEX( VENC_CFG_CTRL_SCD ) )
	{
		if ( gH1Encode.pstEncParamsPrev == NULL )
		{
			gH1Encode.pstEncParamsPrev = pstEncParams;
			return RET_OK;
		}
		else
		{
			H1ENCODE_ENC_PARAMS_T *pstEncParamsTmp;
			
			// Swap current pstEncParams to gH1Encode.pstEncParamsPrev
			pstEncParamsTmp = pstEncParams;
			pstEncParams = gH1Encode.pstEncParamsPrev;
			gH1Encode.pstEncParamsPrev = pstEncParamsTmp;

			if ( pstEncParams != NULL )
			{
				pstEncParams->u32YBaseNext = gH1Encode.pstEncParamsPrev->u32YBase;
			}
		}
	}
#endif

	if ( pstEncParams == NULL )
	{
		return RET_ERROR;
	}

#ifdef H1ENCODE_CHECK_INTERLACED_INPUT
	frameType = pstEncParams->frameType;

	if ( frameType != LX_VENC_FRAME_TYPE_PROGRESSIVE )
	{
#ifndef H1ENCODE_ENABLE_FIELDORDER
	#ifdef H1ENCODE_ENABLE_INTERLACED_BF
		if ( pstEncoder->bIsFirstFrame && frameType == LX_VENC_FRAME_TYPE_TOP )
	#else
		// if input frame is interaced, check frame type is top field first.
		if ( pstEncoder->bIsFirstFrame && frameType == LX_VENC_FRAME_TYPE_BOTTOM )
	#endif
#else
		if ( pstEncoder->bIsFirstFrame == TRUE )
		{
			if ( pstPrep->topFieldFirst ) 
			{
				gH1Encode.prevFrameType = LX_VENC_FRAME_TYPE_BOTTOM;
			}
			else
			{
				gH1Encode.prevFrameType = LX_VENC_FRAME_TYPE_TOP;
			}
		}

		if ( pstEncoder->bIsFirstFrame == TRUE && frameType == gH1Encode.prevFrameType )
#endif
		{
			// Skip first unexpected field.
			//ret = RET_ERROR;
			_EncParamFree( pstEncParams );
			ret = RET_OK;
			goto ENCODE_EXIT;
		}

		if ( gH1Encode.prevFrameType == frameType )
		{
			if ( GET_VENC_CFG_CTRL_INDEX( VENC_CFG_CTRL_FIELD_REPEAT ) )
			{
				H1ENCODE_ENC_PARAMS_T *pstEncParamsRepeat = NULL;
				pstEncParamsRepeat = _EncParamAlloc();

				VENC_WARN("Field repeating.. (%d)\n", frameType);

				if ( pstEncParamsRepeat != NULL )
				{
					*pstEncParamsRepeat = *pstEncParams;
					
					if ( frameType == LX_VENC_FRAME_TYPE_BOTTOM )
						pstEncParamsRepeat->frameType = LX_VENC_FRAME_TYPE_TOP;
					else 
						pstEncParamsRepeat->frameType = LX_VENC_FRAME_TYPE_BOTTOM;
						
					ret = pstEncoder->Encode( pstEncoder, pstEncParamsRepeat );

					if ( ret != RET_OK )
						_EncParamFree( pstEncParamsRepeat );
				}
			}
			else
			{
				VENC_WARN("Unexpected field type. [%d]\n", frameType);
				// Skip the duplicated type of field.
				ret = RET_ERROR;
				goto ENCODE_END;
			}
		}

		//gH1Encode.prevFrameType = frameType;
	}
#endif

	H1ENCODE_TIME( NULL );
	ret = pstEncoder->Encode( pstEncoder, pstEncParams );
	H1ENCODE_TIME( NULL );

ENCODE_END:
	if ( ret != RET_OK )
	{
		VENC_WARN("Encode error!!!\n");

		_EncParamFree( pstEncParams );
	}
#ifdef H1ENCODE_CHECK_INTERLACED_INPUT
	else
	{
		gH1Encode.prevFrameType = frameType;
	}
#endif

#ifdef SUPPORT_VENC_DEVICE_FASYNC_FOPS
	if ( ret == RET_OK )
	{
		VENC_KillFasync( SIGIO );
	}
#endif
ENCODE_EXIT:
	return ret;
}

int H1EncodeCommand( LX_VENC_RECORD_COMMAND_T cmd )
{
	int ret;
	
	if ( gH1Encode.eStatus == H1ENCODE_STATUS_TYPE_STOP_PREPARE ||
		gH1Encode.eStatus == H1ENCODE_STATUS_TYPE_START_PREPARE )
	{
		VENC_PRINT( "Status is STOP/START PREPARE.\n");
		return RET_ERROR;
	}

	if (cmd == LX_VENC_COMMAND_REFRESH)
	{
		gstOutputStatus.ui32ESRptr = gstOutputStatus.ui32ESWptr = gH1Encode.memESB.u32Phys;
		gstOutputStatus.ui32AUIRptr = gstOutputStatus.ui32AUIWptr = gH1Encode.memAUI.u32Phys;
	}
	else
	{
		gstOutputStatus.ui32ESRptr = gstOutputStatus.ui32ESWptr;
		gstOutputStatus.ui32AUIRptr = gstOutputStatus.ui32AUIWptr;
	}

	if ( cmd == LX_VENC_COMMAND_REFRESH && gH1Encode.eStatus != H1ENCODE_STATUS_TYPE_START )
	{
		H1ENCODE_PREP_STATUS_T stPrepStatus = {};

		H1ENCODE_LOCK();
		gH1Encode.eStatus = H1ENCODE_STATUS_TYPE_START_PREPARE;
		H1ENCODE_UNLOCK();

	#if 0
		msleep( 100 );
		
		// Init 전에 picture size 및 format 정보를 읽어 옴.
		H1EncodeReadPREP( &stPrepStatus );
	#else
		// Read ipc registers and Check input resolution.
		// If the DE send vsync late, IPC register values are invalid.
		// So, wait until valid ipc and check frame information.
		_CheckInputResolution( &stPrepStatus );
	#endif
	
		_CheckFramerateInformation();
		
		// Index 및 offset 초기화
		H1EncodeReset();

		// H1 Init 및 Config 정보 설정
		ret = H1EncodeStart( stPrepStatus );

		if ( ret == RET_OK )
		{
			_EncParamClear();
			_InterruptEnableVsync( TRUE );
			
			H1ENCODE_LOCK();
			gH1Encode.encode_count = 0;
			gH1Encode.eStatus = H1ENCODE_STATUS_TYPE_START;
			H1ENCODE_UNLOCK();

#ifdef H1ENCODE_ENABLE_DELAYED_ENCODING
			_EncodeStartPost();
#endif

			VENC_NOTI("Video encoding start..\n");
		}
		else
		{
			H1ENCODE_LOCK();
			gH1Encode.eStatus = H1ENCODE_STATUS_TYPE_STOP;
			H1ENCODE_UNLOCK();
		}
	}
	else if ( cmd == LX_VENC_COMMAND_PAUSE && gH1Encode.eStatus != H1ENCODE_STATUS_TYPE_STOP )
	{
		H1ENCODE_LOCK();
		gH1Encode.eStatus = H1ENCODE_STATUS_TYPE_STOP_PREPARE;
		H1ENCODE_UNLOCK();
		
		_InterruptEnableVsync( FALSE );

		ret = H1EncodeStop();

		H1ENCODE_LOCK();
		gH1Encode.eStatus = H1ENCODE_STATUS_TYPE_STOP;
		H1ENCODE_UNLOCK();

		VENC_NOTI("Video encoding stop..\n");
	}
	else
	{
		VENC_PRINT("Not valid command.\n");

		ret = RET_ERROR;
	}

	if ( ret == RET_OK )
	{
		gH1Encode.cmd = cmd;
	}
	else
	{
		VENC_PRINT( "%s: RET_ERROR\n", __F__ );
	}

	return ret;
}

void H1EncodeAUIWrite( H1ENCODE_MEM_T *pMemAUI, const H1ENCODE_AUI_T stAUI )
{
	// au_type
	//	0: IDR picture
	//	1: non-IDR I picture
	// 	2: P picture
	//	3: B picture
	// unit_size: encoded size
	// unit_start: offset from output buffer base
	// timestamp: time at incoming time

	// +----------------------------------------------------------------+
	// |8bit|24bit      |32bit          |32bit          |32bit          | <= 16byte
	// +----+-----------+---------------+---------------+---------------+
	// |AU  |UINT_SIZE  |UINT_START     |INDEX          |TIMESTAMP      |
	// +----------------------------------------------------------------+
	H1ENCODE_AUI_T *pstAUI;

	pstAUI = (H1ENCODE_AUI_T *)(pMemAUI->u32Virt + pMemAUI->u32WriteOffset );

	//VENC_DEBUG("[%s] pstAUI: 0x%08x\n", __FUNCTION__, (u32)pstAUI);

#ifdef H1ENCODE_USE_STRUCT_ASSIGNMENT
	*pstAUI = stAUI;
#else
	memcpy( pstAUI, &stAUI, sizeof(H1ENCODE_AUI_T) );
#endif

	pMemAUI->u32WriteOffset += sizeof(H1ENCODE_AUI_T);	// 128bits = 16bytes

	if ( pMemAUI->u32WriteOffset >= pMemAUI->u32Size )
	{
		pMemAUI->u32WriteOffset = 0;
	}

	return;
}

static void H1EncodePostDone( void )
{
	//TRACE_ENTER();

	gH1Encode.eventEncodeDoneWakeup++;
	wake_up_interruptible( &gH1Encode.wqEncodeDone );

}

static int H1EncodeWaitDoneTimeout( int timeout )
{
	int ret = RET_OK;

	//TRACE_ENTER();

	gH1Encode.eventEncodeDoneWakeup = 0;

	if ( timeout > 0 )
	{
		ret = wait_event_interruptible_timeout( gH1Encode.wqEncodeDone, gH1Encode.eventEncodeDoneWakeup > 0, timeout * HZ / 1000 );

		if ( ret == 0 )
		{
			ret = RET_TIMEOUT;
		}
		else
		{
			ret = RET_OK;
		}
	}
	else
	{
		wait_event_interruptible( gH1Encode.wqEncodeDone, gH1Encode.eventEncodeDoneWakeup > 0 );
	}

	return ret;
}

void H1EncodeNotifyDone( H1ENCODE_MEM_T *pstESB, H1ENCODE_MEM_T *pstAUI )
{
	UINT32 ui32ESWptr, ui32ESRptr;
	UINT32 ui32AUIWptr, ui32AUIRptr;

	if ( pstESB == NULL || pstAUI == NULL )
	{
		VENC_PRINT("%s: pstESB or pstAUI is NULL.\n", __F__);
		return;
	}

	ui32ESWptr = pstESB->u32Phys + pstESB->u32WriteOffset;
	ui32ESRptr = pstESB->u32Phys + pstESB->u32ReadOffset;
	ui32AUIWptr = pstAUI->u32Phys + pstAUI->u32WriteOffset;
	ui32AUIRptr = pstAUI->u32Phys + pstAUI->u32ReadOffset;

#if 0
	if ( gui32NumberOfFrames < VENC_BIT_RATE_BUFFER ) { ++gui32NumberOfFrames; }

	gaFrameBytes[gui32BufferIndex] = (ui32ESRptr<ui32ESWptr)?
									(ui32ESWptr-ui32ESRptr) : (pstESB->u32Size-(ui32ESRptr-ui32ESWptr));
	gui32BufferIndex = (gui32BufferIndex+1)%VENC_BIT_RATE_BUFFER;
#endif

	// Flash ES buffer when buffer overflow
	if ( ( gstOutputStatus.ui32ESRptr <= ui32ESWptr) && (ui32ESWptr <= gstOutputStatus.ui32ESWptr) )
	{
		gstOutputStatus.ui32ESRptr = ui32ESRptr;
	}
	gstOutputStatus.ui32ESWptr = ui32ESWptr;

	// Flash AUI buffer when buffer overflow
	if ( ( gstOutputStatus.ui32AUIRptr <= ui32AUIWptr ) && (ui32AUIWptr <= gstOutputStatus.ui32AUIWptr) )
	{
		gstOutputStatus.ui32AUIRptr = ui32AUIRptr;
	}
	gstOutputStatus.ui32AUIWptr = ui32AUIWptr;

#if 0
	VENC_DEBUG("[%s] gui32ESRptr : 0x%8x, gui32ESWptr : 0x%08x\n",
		__FUNCTION__, gstOutputStatus.ui32ESRptr, gstOutputStatus.ui32ESWptr);
	VENC_DEBUG("[%s] gui32AUIRptr: 0x%8x, gui32AUIWptr: 0x%08x\n",
		__FUNCTION__, gstOutputStatus.ui32AUIRptr, gstOutputStatus.ui32AUIRptr);
#endif

	// 기존 venc와 호환을 위해서 GetOutput ioctl용 이벤트 kick
	//OS_SendEvent( &gstEvent, VENC_EVENT_ES_WPTR );
	H1EncodePostDone();
}

#ifdef H1ENCODE_ENABLE_DELAYED_ENCODING

typedef struct {
	H1ENCODE_PREP_STATUS_T stPrepStatus;
	UINT32 timestamp;
	UINT32 inputTime;

	struct list_head encodeList;

} H1ENCODE_ENCODE_LIST_T;

struct list_head gEncodeList;

static void _EncodeQueueInit( void )
{
	INIT_LIST_HEAD( &gEncodeList );
}

static void _EncodeQueueClear( void )
{
	struct list_head *pos;
	struct list_head *q;

	list_for_each_safe( pos, q, &gEncodeList )
	{
		H1ENCODE_ENCODE_LIST_T *item;

		item = list_entry( pos, H1ENCODE_ENCODE_LIST_T, encodeList );
		list_del( pos );

		if ( item != NULL )
		{
			kfree( item );
		}
	}
}

static void _EncodeQueueDestory( void )
{
	//VENC_DEBUG("%s: 0x%08x\n",  __F__, (u32)pStreamList);

	if ( !list_empty( &gEncodeList ) )
	{
		// if list is not empty, clear the list entry
		_EncodeQueueClear( );
	}
	
}

static void _EncodeCheckQueue( void )
{
	while ( list_empty( &gEncodeList ) )
	{
		// wait until list not empty
		msleep_interruptible(2);
	}

	return;
}


static void _EncodeEnqueue( H1ENCODE_PREP_STATUS_T *pstPrepStatus, UINT32 timestamp, UINT32 inputTime )
{
	H1ENCODE_ENCODE_LIST_T *item = NULL;

	if ( pstPrepStatus == NULL )
	{
		return;
	}

	while ( item == NULL )
	{
		item = (H1ENCODE_ENCODE_LIST_T *) kmalloc( sizeof(H1ENCODE_ENCODE_LIST_T), GFP_ATOMIC );
	}

#ifdef H1ENCODE_USE_STRUCT_ASSIGNMENT
	item->stPrepStatus = *pstPrepStatus;
#else
	memcpy( &item->stPrepStatus, pstPrepStatus, sizeof(H1ENCODE_PREP_STATUS_T) );
#endif

	item->timestamp = timestamp;
	item->inputTime = inputTime;

	list_add_tail( &item->encodeList, &gEncodeList );
}

static int _EncodeDequeue( H1ENCODE_PREP_STATUS_T *pstPrepStatus, UINT32 *pTimestamp )
{
	struct list_head *pos;
	struct list_head *q;

	if ( pstPrepStatus == NULL || pTimestamp == NULL )
	{
		return RET_ERROR;
	}
	
	list_for_each_safe( pos, q, &gEncodeList )
	{
		H1ENCODE_ENCODE_LIST_T *item;

		item = list_entry( pos, H1ENCODE_ENCODE_LIST_T, encodeList );

		if ( item != NULL )
		{
#ifdef H1ENCODE_USE_STRUCT_ASSIGNMENT
			*pstPrepStatus = item->stPrepStatus;
#else
			memcpy( pstPrepStatus, &item->stPrepStatus, sizeof(H1ENCODE_PREP_STATUS_T) );
#endif
			*pTimestamp = item->timestamp;
		}
		
		list_del( pos );

		if ( item != NULL )
		{
			kfree( item );
			return RET_OK;
		}
		else
		{
			return RET_ERROR;
		}
		
	}

	return RET_ERROR;
}

// Choose discard input by input time
static int _EncodeDiscardQueue( UINT32 currentTime )
{
	struct list_head *pos;
	struct list_head *q;

	int discardCount = 0;
	
	list_for_each_safe( pos, q, &gEncodeList )
	{
		H1ENCODE_ENCODE_LIST_T *item;
		int waitTime;

		item = list_entry( pos, H1ENCODE_ENCODE_LIST_T, encodeList );

		if ( currentTime > item->timestamp )
		{
			waitTime = currentTime - item->timestamp;
		}
		else
		{
			waitTime = currentTime + ((UINT32)0xFFFFFFFF - item->timestamp);
		}
		
		if ( waitTime > maxDelayedTime )
		{
			list_del( pos );
			discardCount++;

			if ( item != NULL )
			{
				kfree( item );
			}
		}
	}

	if ( discardCount )
	{
		VENC_WARN("discardCount [%d], encodedCount [%d]\n", discardCount, gH1Encode.encodedCountBeforeDiscard );
		
		H1ENCODE_LOCK();
		gH1Encode.encodedCountBeforeDiscard = 0;
		H1ENCODE_UNLOCK();
	}
	
	return discardCount;
}

static void _EncodeStartWait( void )
{
	if ( gH1Encode.eStatus != H1ENCODE_STATUS_TYPE_START )
	{
		interruptible_sleep_on( &gH1Encode.wqEncodeStart );
	} 
}

static void _EncodeStartPost( void )
{
	wake_up_interruptible( &gH1Encode.wqEncodeStart );
}

static int H1EncodeTask( void* pParam )
{
	int ret;
	
	_EncodeQueueInit();
	
	while(1)
	{
		if ( kthread_should_stop() )
		{
			VENC_PRINT("H1EncodeTask - exit!\n");

			break;
		}

		_EncodeStartWait();

		H1ENCODE_TIME( NULL );

		if ( gH1Encode.pstEncoder != NULL )
		{
			H1ENCODE_PREP_STATUS_T stPrepStatus = {0};
			UINT32 gstc = 0;
			
			if ( gH1Encode.eStatus != H1ENCODE_STATUS_TYPE_START )
			{
				msleep(5);
				continue;
			}

			_EncodeCheckQueue();
			if ( _EncodeDequeue( &stPrepStatus, &gstc ) != RET_OK )
			{
				continue;
			}
			
			H1ENCODE_LOCK();
			gH1Encode.encode_count++;
			H1ENCODE_UNLOCK();

			H1ENCODE_TIME( NULL );

			ret = H1EncodeEncode( &stPrepStatus, gstc );

			H1ENCODE_TIME( NULL );

			H1ENCODE_LOCK();
			if ( ret == RET_OK )
			{ 
				gH1Encode.encodedCountBeforeDiscard++;
			}
			gH1Encode.encode_count--;
			H1ENCODE_UNLOCK();

			_EncodeDiscardQueue( jiffies );
		}

		H1ENCODE_TIME_END();
	}

	_EncodeQueueDestory();

	return 0;
}
#else
static int H1EncodeTask( void* pParam )
{
	int ret;
	
	while(1)
	{
		if ( kthread_should_stop() )
		{
			VENC_PRINT("H1EncodeTask - exit!\n");

			break;
		}

		_VsyncWait();

		H1ENCODE_TIME( NULL );

		if ( gH1Encode.pstEncoder != NULL )
		{
			u32 gstc = H1EncodeGetTimestamp();
			H1ENCODE_PREP_STATUS_T stPrepStatus = {0};
			
			if ( gH1Encode.eStatus != H1ENCODE_STATUS_TYPE_START )
			{
				continue;
			}
			
			H1ENCODE_LOCK();
			gH1Encode.encode_count++;
			H1ENCODE_UNLOCK();

			H1EncodeReadPREP( &stPrepStatus );

			H1ENCODE_TIME( NULL );

			ret = H1EncodeEncode( &stPrepStatus, gstc );

			H1ENCODE_TIME( NULL );

			H1ENCODE_LOCK();
			if ( ret == RET_OK )
			{
				gH1Encode.encodedCountBeforeDiscard++;
			}
			gH1Encode.encode_count--;
			H1ENCODE_UNLOCK();

			#ifdef H1ENCODE_DEBUG_INTERLACED_DUMP
			if ( gH1Encode.stPrepStatus.frameType == LX_VENC_FRAME_TYPE_TOP ||
				gH1Encode.stPrepStatus.frameType == LX_VENC_FRAME_TYPE_BOTTOM ) 
			{
				_DebugSaveField();
			}
			#endif
		}
		else if ( gH1Encode.enableInputDetect )
		{
			H1EncodeInputDetect();
		}
		else if ( gH1Encode.enableFramerateDetect )
		{
			H1EncodeMeasureFramerate();
		}

		H1ENCODE_TIME_END();
	}

	return 0;
}
#endif

static void _InterruptEnable( void )
{
	VENC_PREP_RdFL(vdecintr_e_en);

#if !defined(H1ENCODE_USE_POLLING)
//#warning "H1Encode: Use interrupt."
	VENC_PREP_Wr01(vdecintr_e_en, h1_intr_e_en, 1);
#else
//#warning "H1Encode: Use polling."
	VENC_PREP_Wr01(vdecintr_e_en, h1_intr_e_en, 0);
#endif

	//VENC_PREP_Wr01(vdecintr_e_en, vdec_intr_e_en, 1);
	//VENC_PREP_Wr01(vdecintr_e_en, de_intr_e_en, 1);

	VENC_PREP_WrFL(vdecintr_e_en);
}

static void _InterruptDisable( void )
{
	VENC_PREP_RdFL(vdecintr_e_en);

	VENC_PREP_Wr01(vdecintr_e_en, h1_intr_e_en, 0);
	//VENC_PREP_Wr01(vdecintr_e_en, vdec_intr_e_en, 0);
	//VENC_PREP_Wr01(vdecintr_e_en, de_intr_e_en, 0);

	VENC_PREP_WrFL(vdecintr_e_en);
}

static u32 _InterruptRead( void )
{
	u32 status;

	VENC_PREP_RdFL(vdecintr_e_st);
	status = VENC_PREP_Rd(vdecintr_e_st);

	//VENC_INTER("status: 0x%08x\n", status);

	return status;
}

static void _InterruptClear( void )
{
	u32 enableIntr;

	VENC_PREP_RdFL(vdecintr_e_en);
	enableIntr = VENC_PREP_Rd(vdecintr_e_en);

	VENC_PREP_RdFL(vdecintr_e_cl);

	if ( enableIntr & 0x1 )
	{
		// clear vdec
		VENC_PREP_Wr01(vdecintr_e_cl, vdec_intr_e_cl, 1);
	}

	if ( enableIntr & 0x2 )
	{
		// clear de vsync
		VENC_PREP_Wr01(vdecintr_e_cl, de_intr_e_cl, 1);
	}

	if ( enableIntr & 0x4 )
	{
		// H1Encoder Interrupt

	}

	VENC_PREP_WrFL(vdecintr_e_cl);

}

static void _InterruptEnableVsync( BOOLEAN enable )
{
	//VENC_DEBUG("H1EncodeVsync ( enable=%s ) \n", enable ? "TRUE": "FALSE");

	VENC_PREP_RdFL(vdecintr_e_en);

	if ( enable )
	{
		VENC_PREP_Wr01(vdecintr_e_en, vdec_intr_e_en, 1);
		VENC_PREP_Wr01(vdecintr_e_en, de_intr_e_en, 1);
	}
	else
	{
		VENC_PREP_Wr01(vdecintr_e_en, vdec_intr_e_en, 0);
		VENC_PREP_Wr01(vdecintr_e_en, de_intr_e_en, 0);
	}

	VENC_PREP_WrFL(vdecintr_e_en);

#if defined(H1ENCODE_DEBUG_DUMP)
	if ( enable == FALSE )
	{
		H1EncodeDumpStop();
	}
#endif

}

#ifdef H1ENCODE_USE_TIMER_VSYNC
static void _TimerCheckEncode( unsigned long arg );
static void _TimerAdd( unsigned long arg )
{
	if ( arg > 4 )
	{
		return;
	}
	
	// Add timer
	encode_timer.data = arg + 1;

	if ( encode_timer.function == NULL )
	{
		encode_timer.function = _TimerCheckEncode;
	}
	
#ifdef	DEFINE_TARGET_FPGA
	encode_timer.expires = jiffies + 10000;	// +2ms
#else
	encode_timer.expires = jiffies + msecs_to_jiffies(1);
#endif

	//VENC_PRINT( "Add timer: expires[%lu]\n", encode_timer.expires);
	add_timer( &encode_timer );
}

static void _TimerCheckEncode( unsigned long arg )
{
	if ( gH1Encode.encode_count > 0 )
	{
		// Add (arg)th timer
		_TimerAdd( arg );
	}
	else
	{
		//VENC_PRINT( "Call wake_up_interruptible() @ timer function\n");		
		wake_up_interruptible( &gH1Encode.wqEncodeVsync );
	}
}
#endif

static void _VsyncWait( void )
{
	//int ret;
	int discardCount = 0;
	
#ifdef H1ENCODE_USE_VSYNC_BUSYWAIT
	while ( 1 )
	{
		ktime_t until = ktime_set( 0, 5000000 );	// 5ms
		ret = wait_event_interruptible_hrtimeout( gH1Encode.wqEncodeDone, gH1Encode.ui32CountVsync != 0, until );
		
		if ( gH1Encode.ui32CountVsync != 0 )
		{
			break;
		}
	}
#else
	wait_event_interruptible( gH1Encode.wqEncodeVsync, gH1Encode.ui32CountVsync != 0);
#endif

	if ( gH1Encode.ui32CountVsync > 1 )
	{
		u32 before = gH1Encode.ui32CountFrameSkip;
		gH1Encode.ui32CountFrameSkip += (gH1Encode.ui32CountVsync - 1);

		VENC_VSYNC("%d vsync skip. (Total vsync skip: %d)\n", gH1Encode.ui32CountFrameSkip - before, gH1Encode.ui32CountFrameSkip);

		if ( before < gH1Encode.ui32CountFrameSkip )
		{
			discardCount = gH1Encode.ui32CountFrameSkip - before;
		}
		else if ( before > gH1Encode.ui32CountFrameSkip )
		{
			discardCount = gH1Encode.ui32CountFrameSkip + ((UINT32)0xFFFFFFFF - before);
		}
	}

	if ( discardCount > 0 )
	{
		VENC_WARN("discardCount [%d], encodedCount [%d]\n", discardCount, gH1Encode.encodedCountBeforeDiscard );

		H1ENCODE_LOCK();
		gH1Encode.encodedCountBeforeDiscard = 0;
		H1ENCODE_UNLOCK();
	}
	
	gH1Encode.ui32CountVsync = 0;
}

static void _VsyncPost( void )
{
	gH1Encode.ui32CountVsync++;

#ifdef H1ENCODE_USE_TIMER_VSYNC
	if ( gH1Encode.encode_count > 0 )
	{
		// Add 1st timer ( count=0 )
		_TimerAdd( 0 );
	}
	else
#endif
	{
		wake_up_interruptible( &gH1Encode.wqEncodeVsync );
	}
}

static int _EventWait( H1ENCODE_EVENT_T event, int timeout )
{
	int ret = RET_ERROR;
	u32 recvEvent;

	if ( event == 0 )
	{
		return ret;
	}

	if ( timeout == 0 )
	{
		timeout = 100;
	}
	
	ret = OS_RecvEvent( &gstEventH1, event, &recvEvent, OS_EVENT_RECEIVE_ANY, timeout);

	return ret;
}

static void _EventPost( H1ENCODE_EVENT_T event )
{
	if ( event == 0 )
	{
		return;
	}
	
	OS_SendEvent( &gstEventH1, event );
}

static void _EventClear( void )
{
	OS_ClearEvent( &gstEventH1 );
}

#if !defined(H1ENCODE_USE_POLLING)

static void H1EncodeHX280ISR( unsigned long temp )
{

	// reference from hx286enc_isr() at hx280enc.c:413
	u32 irq_status;

	// 1. save irq status & clear irq status
	irq_status = HXENC_ReadIRQ();

	if ( irq_status & 0x01 )
	{
		// 2. Clear IRQ and sllice ready interrupt bit
		HXENC_WriteIRQ( irq_status & (~0x101) );

		// Handle slice ready interrupts. The reference implementation
		// doesn't signal slice ready interrupts to EWL.
		// The EWL will poll the slices ready register value.
		if ( (irq_status & 0x1FE) == 0x100)
		{
			// Slice ready IRQ handled
			return;
		}

		// 2. kick event
		HXENC_WakeupIRQ();

#ifdef H1ENCODE_USE_LOGFILE
		do_gettimeofday(&_irqTime);
#endif
	}

}

#endif

#ifdef H1ENCODE_UNUSE_ENCODE_THREAD
static void H1EncodeEncodeHandler( unsigned long temp )
{
	int ret;
	H1ENCODE_PREP_STATUS_T stPrepStatus;
	u32 gstc;
	
	H1EncodeUpdateTimestamp();
	gstc = H1EncodeGetTimestamp();
	H1EncodeReadPREP( &stPrepStatus );

	ret = H1EncodeEncode( &stPrepStatus, gstc );

	if ( ret != RET_OK )
	{
		VENC_PRINT("H1EncodeEncode() = %d\n", ret );
	}
}
#endif

#ifdef H1ENCODE_DEBUG_CHECK_VSYNC
static UINT32 jiffies_vsync;
#endif

static void H1EncodeVsyncHandler( unsigned long temp )
{
#ifdef H1ENCODE_DEBUG_CHECK_VSYNC	
	UINT32 jiffies_cur = jiffies;
#endif

#ifdef H1ENCODE_ENABLE_DROP_FIRST_FRAME
	if ( gH1Encode.remain_drop > 0 )
	{
		gH1Encode.remain_drop--;
		return;
	}
#endif

	// Raise vsync interrupt
#ifdef H1ENCODE_ENABLE_DELAYED_ENCODING
	H1ENCODE_PREP_STATUS_T stPrepStatus;
	u32 gstc;

	H1EncodeUpdateTimestamp();
	gstc = H1EncodeGetTimestamp();
	H1EncodeReadPREP( &stPrepStatus );

	_EncodeEnqueue( &stPrepStatus, gstc, jiffies );
#else
	H1EncodeUpdateTimestamp();
	H1EncodeUpdatePREP();
	
	// Encode wakeup (EncodeTask)
	// 이전 frame encoding시간 지연으로 Vsync 처리를 바로 할 수 없을 때, 
	// Timer interrupt를 동작시켜 5ms정도 지연효과를 기대함.
	// (Worst Cast에서 테스트 필요)
	_VsyncPost();
#endif

	gH1Encode.ui32TotalVsync++;

#ifdef H1ENCODE_DEBUG_CHECK_VSYNC
	if ( gH1Encode.stInput.frameRateNum > 0 && gH1Encode.stInput.frameRateDenom > 0 )
	{
		UINT32 interval = (jiffies_cur - jiffies_vsync);
		UINT32 expectedInterval = (gH1Encode.stInput.frameRateDenom * 1000 / gH1Encode.stInput.frameRateNum );

		if ( interval < expectedInterval - 1 )
		{
			VENC_WARN("Too short vsync interval: %d\n", interval);
		}
		else if ( interval > expectedInterval + 1 )
		{
			VENC_WARN("Too long vsync interval: %d\n", interval);
		}
	}

	jiffies_vsync = jiffies_cur;
#endif

}

int H1EncodeISRHandler( void )
{
//	BOOLEAN bVsync = FALSE;
	u32 status;
	int ret = IRQ_NONE;

//	TRACE_ENTER();

	status = _InterruptRead();

	if ( status )
	{
		_InterruptClear();

		if ( status & 0x1 || status & 0x2 )	// vdec_intr_st, de_intr_st
		{
#ifdef H1ENCODE_UNUSE_ENCODE_THREAD
			tasklet_schedule( &VENC_TASKLET_VSYNC );
#else
			H1EncodeVsyncHandler( 0UL );
#endif

#ifdef SUPPORT_VENC_DEVICE_FASYNC_FOPS
			VENC_KillFasync( SIGUSR1 );
#endif
			ret = IRQ_HANDLED;
		}

#if !defined(H1ENCODE_USE_POLLING)
		if ( status & 0x4 )	// intr_h1enc
		{
			H1EncodeHX280ISR( 0UL );

			ret = IRQ_HANDLED;
		}
#endif
	}

	return ret;
}


#ifdef SUPPORT_VENC_DEVICE_ENC_API
typedef struct {
	int			id;
	void*		inst;
	spinlock_t	lock;
	
	BOOLEAN		isFirst;
	UINT8		headerData[64];
	UINT32		headerLength;

	UINT32		width;
	UINT32		height;
	UINT32		framerate;
	UINT32		gopLength;
	UINT32		bitrate;

	LX_VENC_IMAGE_TYPE_T	imageType;
	LX_VENC_ENCODE_TYPE_T	codecType;
	BOOLEAN		interlaced;
	BOOLEAN		topFieldFirst;

	UINT32		codedFrameCount;
	UINT32		intraPreiodCount;

	BOOLEAN		memallocChanged;		//  For DEBUG
} ENC_CTX_T;

ENC_CTX_T	enc_ctx_list[] = {
	[0] = {
		.id = -1,		
	}
};

static ENC_CTX_T* _ENC_CtxNew(LX_VENC_ENCODE_TYPE_T codecType)
{
	ENC_CTX_T* handle = &enc_ctx_list[0];
		
	if ( handle->id != -1 )
	{
		return NULL;
	}
	
	handle->id = 0;
	handle->isFirst = TRUE;
	handle->codecType = codecType;

	spin_lock_init(&handle->lock);
	//enc_ctx_list[0].inst = NULL;
	
	return handle;
}

static ENC_CTX_T* _ENC_CtxGet(int id)
{
	if (enc_ctx_list[0].id == id)
	{
		return &enc_ctx_list[0];
	}
	else
	{
		return NULL;
	}
}

static int _ENC_CtxDelete(ENC_CTX_T* enc_ctx)
{
	if (enc_ctx == NULL)
	{
		return RET_INVALID_PARAMS;
	}

	enc_ctx->id = -1;
	enc_ctx->inst = NULL;
	
	return RET_OK;
}

int ENC_H264Create(ENC_CTX_T *handle)
{
	HXENC_RET_T		ret;
	HXENC_CONFIG_T	config = {};
	HXENC_PREPROCESSING_CFG_T preCfg = {};
	HXENC_RATECTRL_T rcCfg = {};
	HXENC_CODINGCTRL_T codingCfg = {};
	
	VENC_TRACE("START\n");
	
	if ( handle == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	config.width = handle->width;
	config.height = handle->height;
	config.stride = handle->width;
	config.eFrameType = handle->interlaced ? 1 : HXENC_FRAME_TYPE_PROGRESSIVE;
	config.frameRateDenom = 1;
	config.frameRateNum = handle->framerate * config.frameRateDenom;
	
	if ((ret = HXENC_H264Init( &config, (HXENC_H264INST_T *)&handle->inst )) != HXENC_OK )
	{
		VENC_ERROR("HXENC_H264Init - failed.(%d)\n", ret);
		goto func_error;
	}

	preCfg.inputWidth = handle->width;
	if ( handle->interlaced )
		preCfg.inputHeight = handle->height / 2;
	else
		preCfg.inputHeight = handle->height;
	preCfg.inputType = handle->imageType;
	preCfg.videoStabilization = 0;

	if((ret = HXENC_H264SetPreProcessing(handle->inst, &preCfg)) != HXENC_OK)
	{
		VENC_ERROR("HXENC_H264SetPreProcessing - failed.(%d)\n", ret);
		goto func_error;
	}

	if ( handle->gopLength > 0 )
	{
		rcCfg.bEnableGOPLength = TRUE;
		rcCfg.gopLength = handle->gopLength;
		//rcCfg.bitPerSecond = handle->bitrate;
	}
	
	if(( ret = HXENC_H264SetRateCtrl(handle->inst, &rcCfg)) != HXENC_OK)
	{
		VENC_ERROR("HXENC_H264SetRateCtrl - failed.(%d)\n", ret);
		goto func_error;
	}

	if ( handle->interlaced )
	{
		codingCfg.fieldOrder = handle->topFieldFirst ? 1 : 0;
		
		if((ret = HXENC_H264SetCodingCtrl(handle->inst, &codingCfg)) != HXENC_OK)
		{
			VENC_ERROR("HXENC_H264SetCodingCtrl - failed.(%d)\n", ret);
			goto func_error;
		}
	}
	
	VENC_TRACE("END\n");
	return RET_OK;
	
func_error:
	if ( handle != NULL && handle->inst != NULL )
	{
		HXENC_H264Release(handle->inst);
		handle->inst = NULL;
	}
	
	return RET_ERROR;
}

int ENC_H264Destroy(ENC_CTX_T *		handle)
{
	HXENC_RET_T		ret = HXENC_ERROR;
		
	if ( handle == NULL || handle->inst == NULL )
	{
		return RET_ERROR;
	}

	ret = HXENC_H264Release(handle->inst);
	
	return ret;
}

int ENC_H264Encode(ENC_CTX_T *handle, LX_VENC_ENC_ENCODE_T *pstEncode)
{
	HXENC_RET_T		ret = HXENC_ERROR;
	HXENC_INPUT_T		encIn = {};
	HXENC_H264_OUTPUT_T encOut = {};

	UINT32	bus_output = 0;
	UINT32	out_buf_size = 0;
	UINT32	stream_size = 0;

	unsigned long		flags;
	
	if (handle == NULL || handle->inst == NULL)
	{
		VENC_ERROR("handle or inst is NULL\n");
		return RET_ERROR;
	}

	if (pstEncode == NULL)
	{
		VENC_ERROR("pstEncode is NULL\n");
		return RET_ERROR;
	}

	spin_lock_irqsave(&handle->lock, flags);

	bus_output = pstEncode->bus_output;
	out_buf_size = pstEncode->out_buf_size;
	
	if (handle->isFirst)
	{
		encIn.pOutBuf = (u32 *)handle->headerData;
		encIn.outBufSize = sizeof(handle->headerData);

		ret = HXENC_H264StrmStart(handle->inst, &encIn, &encOut);

		if ( ret != HXENC_OK )
		{
			VENC_ERROR("Error - HXENC_H264StrmStart\n");

			spin_unlock_irqrestore(&handle->lock, flags);
			
			return RET_ERROR;
		}

		memcpy(handle->headerData, encIn.pOutBuf, encOut.streamSize);
		handle->headerLength = encOut.streamSize;
		
		memset(&encIn, 0x0, sizeof(encIn));
		memset(&encOut, 0x0, sizeof(encOut));

		handle->isFirst = FALSE;		
		encIn.codingType	= HXENC_INTRA_FRAME;
		encIn.timeIncrement = 0;
	}
	else
	{
		encIn.codingType	= HXENC_PREDICTED_FRAME;
		encIn.timeIncrement = pstEncode->timeIncrement;
	}

	if ( handle->gopLength != 0 && (handle->intraPreiodCount >= handle->gopLength) )
	{
		encIn.codingType	= HXENC_INTRA_FRAME;
	}

	if ( encIn.codingType == HXENC_INTRA_FRAME )
	{
		void * pOutBuf = NULL;
		
		handle->intraPreiodCount = 0;

		pOutBuf = ioremap(bus_output, out_buf_size);
		
		if ( pOutBuf != NULL )
		{
			memset(pOutBuf, 0x0, sizeof(handle->headerData));
			memcpy(pOutBuf, handle->headerData, sizeof(handle->headerData));
			iounmap(pOutBuf);
		
			bus_output += sizeof(handle->headerData);
			out_buf_size -= sizeof(handle->headerData);
			stream_size += sizeof(handle->headerData);
		}
	}
	
	encIn.busLuma		= pstEncode->bus_luma;
	encIn.busChromaU	= pstEncode->bus_chroma_u;
	encIn.busChromaV	= pstEncode->bus_chroma_v;
	
	encIn.busOutBuf 	= bus_output;
	encIn.outBufSize	= out_buf_size;
	
	VENC_TRACE("bus_output=0x%08X, out_buf_size=%d\n", encIn.busOutBuf, encIn.outBufSize);
	
	encIn.pOutBuf = ioremap( encIn.busOutBuf, encIn.outBufSize );

	spin_unlock_irqrestore(&handle->lock, flags);

	ret = HXENC_H264StrmEncode( handle->inst, &encIn, &encOut );

	spin_lock_irqsave(&handle->lock, flags);

	iounmap( encIn.pOutBuf );
	
	if ( ret != HXENC_OK && ret != HXENC_FRAME_READY )
	{
		goto func_exit;
	}

	handle->codedFrameCount++;
	handle->intraPreiodCount++;
	
	stream_size += encOut.streamSize;
	pstEncode->stream_size = stream_size;
	pstEncode->codingType = encOut.codingType;

func_exit:
	spin_unlock_irqrestore(&handle->lock, flags);
	
	return ret;	
}

int ENC_VP8Create(ENC_CTX_T *handle)
{
	HXENC_RET_T		ret;
	HXENC_CONFIG_T	config = {};
	HXENC_PREPROCESSING_CFG_T preCfg = {};
	HXENC_RATECTRL_T rcCfg = {};
	HXENC_CODINGCTRL_T codingCfg = {};
	
	VENC_TRACE("START\n");
	
	if ( handle == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	config.width = handle->width;
	config.height = handle->height;
	config.stride = handle->width;
	config.eFrameType = handle->interlaced ? 1 : HXENC_FRAME_TYPE_PROGRESSIVE;
	config.frameRateDenom = 1;
	config.frameRateNum = handle->framerate * config.frameRateDenom;
	
	if ((ret = HXENC_VP8Init( &config, (HXENC_VP8INST_T *)&handle->inst )) != HXENC_OK )
	{
		VENC_ERROR("HXENC_H264Init - failed.(%d)\n", ret);
		goto func_error;
	}

	preCfg.inputWidth = handle->width;
	if ( handle->interlaced )
		preCfg.inputHeight = handle->height / 2;
	else
		preCfg.inputHeight = handle->height;
	preCfg.inputType = handle->imageType;
	preCfg.videoStabilization = 0;

	if((ret = HXENC_VP8SetPreProcessing(handle->inst, &preCfg)) != HXENC_OK)
	{
		VENC_ERROR("HXENC_H264SetPreProcessing - failed.(%d)\n", ret);
		goto func_error;
	}

#if 0	
	if ( handle->gopLength > 0 )
	{
		rcCfg.bEnableGOPLength = TRUE;
		rcCfg.gopLength= handle->gopLength;
	}
	
	if((ret = HXENC_VP8SetRateCtrl((HXENC_VP8INST_T)handle->inst, &rcCfg)) != HXENC_OK)
	{
		VENC_ERROR("HXENC_VP8SetRateCtrl - failed.(%d)\n", ret);
		goto func_error;
	}
#endif

	if ( handle->interlaced )
	{
		codingCfg.fieldOrder = handle->topFieldFirst ? 1 : 0;
		
		if((ret = HXENC_VP8SetCodingCtrl(handle->inst, &codingCfg)) != HXENC_OK)
		{
			VENC_ERROR("HXENC_H264SetCodingCtrl - failed.(%d)\n", ret);
			goto func_error;
		}
	}

	VENC_TRACE("END\n");
	return RET_OK;
	
func_error:
	if ( handle != NULL && handle->inst != NULL )
	{
		HXENC_VP8Release(handle->inst);
		handle->inst = NULL;
	}
	
	return RET_ERROR;

}

int ENC_VP8Destroy(ENC_CTX_T *		handle)
{
	HXENC_RET_T		ret = HXENC_ERROR;
		
	if ( handle == NULL || handle->inst == NULL )
	{
		return RET_ERROR;
	}

	ret = HXENC_VP8Release(handle->inst);
	
	return ret;
}

int ENC_VP8Encode(ENC_CTX_T *handle, LX_VENC_ENC_ENCODE_T *pstEncode)
{
	HXENC_RET_T		ret = HXENC_ERROR;
	HXENC_INPUT_T		encIn = {};
	HXENC_VP8_OUTPUT_T	encOut = {};
	
	UINT32	stream_size = 0;
	UINT32	offset = 0;
	void *stream = NULL;

	int i = 0;
	
	if (handle == NULL || handle->inst == NULL)
	{
		VENC_ERROR("handle or inst is NULL\n");
		return RET_ERROR;
	}

	if (pstEncode == NULL)
	{
		VENC_ERROR("pstEncode is NULL\n");
		return RET_ERROR;
	}
	
	if (handle->isFirst)
	{
		encIn.codingType	= HXENC_INTRA_FRAME;
		handle->isFirst 	= FALSE;
	}
	else
	{
		encIn.codingType	= HXENC_PREDICTED_FRAME;
	}

	if ( handle->gopLength != 0 && (handle->intraPreiodCount >= handle->gopLength) )
	{
		encIn.codingType	= HXENC_INTRA_FRAME;
	}

	if ( encIn.codingType == HXENC_INTRA_FRAME )
	{
		handle->intraPreiodCount = 0;
	}

	encIn.busLuma		= pstEncode->bus_luma;
	encIn.busChromaU	= pstEncode->bus_chroma_u;
	encIn.busChromaV	= pstEncode->bus_chroma_v;
	
	encIn.busOutBuf 	= pstEncode->bus_output;
	encIn.outBufSize	= pstEncode->out_buf_size;
	
	VENC_TRACE("codingType=%d, bus_output=0x%08X, out_buf_size=%d\n", 
		encIn.codingType, encIn.busOutBuf, encIn.outBufSize);
	
	encIn.pOutBuf		= ioremap( encIn.busOutBuf, encIn.outBufSize );
	ret = HXENC_VP8StrmEncode( handle->inst, &encIn, &encOut );

	if ( ret != HXENC_OK && ret != HXENC_FRAME_READY )
	{
		goto func_exit;
	}

	handle->codedFrameCount++;
	handle->intraPreiodCount++;
	
	for ( i = 0; i < 9; i++ )
	{
		if ( encOut.streamSize[i] > 0 )
		{
			stream_size += encOut.streamSize[i];
		}
	}

	stream = kmalloc(stream_size, GFP_KERNEL);

	// copy to tmp buffer
	for ( i = 0; i < 9; i++ )
	{
		if ( encOut.streamSize[i] > 0 )
		{
			memcpy( stream + offset, encOut.pOutBuf[i], encOut.streamSize[i] );
			offset += encOut.streamSize[i];

			VENC_TRACE("encOut={ .streamSize[%d]=%d }\n", i, encOut.streamSize[i]);
		}
	}

	VENC_TRACE("encOut={ .frameSize=%d }\n", encOut.frameSize);

	// copy to result buffer
	memcpy( encIn.pOutBuf, stream, stream_size );
	pstEncode->stream_size = stream_size;
	pstEncode->codingType = encOut.codingType;

func_exit:
	if ( stream != NULL )
	{
		kfree(stream);
	}
	iounmap( encIn.pOutBuf );

	return ret;
}

int ENC_Create(LX_VENC_ENC_CREATE_T *pstCreate)
{
	int ret = RET_ERROR;
	ENC_CTX_T *		handle = NULL;
	
	if ( pstCreate == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	handle = _ENC_CtxNew(pstCreate->codecType);
	if ( handle == NULL )
	{
		VENC_ERROR("Can't allocate new context\n");
		return RET_ERROR;
	}

	handle->width 		= pstCreate->width;
	handle->height 		= pstCreate->height;
	handle->imageType	= pstCreate->imageType;
	handle->framerate 	= pstCreate->framerate;
	handle->gopLength 	= pstCreate->gopLength;
	handle->bitrate 	= pstCreate->bitrate;
	handle->interlaced 	= pstCreate->interlaced;
	handle->topFieldFirst = pstCreate->topFieldFirst;

	if ( pstCreate->memallocStart > 0 && pstCreate->memallocEnd > 0 )
	{
		// must use for debug only
		handle->memallocChanged = TRUE;
		HXENC_MemallocInit( pstCreate->memallocStart, pstCreate->memallocEnd );
	}

	if ( handle->codecType == LX_VENC_ENCODE_TYPE_VP8 )
	{
		ret = ENC_VP8Create(handle);
	}
	else
	{
		ret = ENC_H264Create(handle);
	}

	// return handle id
	pstCreate->id = handle->id;

	return ret;	
}

int ENC_Destroy(LX_VENC_ENC_DESTROY_T *pstDestroy)
{
	int ret = RET_ERROR;
	ENC_CTX_T *		handle = NULL;

	if ( pstDestroy == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	handle  = _ENC_CtxGet(pstDestroy->id);

	if ( handle == NULL )
	{
		return RET_ERROR;
	}

	if ( handle->codecType == LX_VENC_ENCODE_TYPE_VP8 )
	{
		ret = ENC_VP8Destroy(handle);
	}
	else
	{
		ret = ENC_H264Destroy(handle);
	}

	_ENC_CtxDelete(handle);

	if ( handle->memallocChanged )
	{
		HXENC_MemallocInit( gpstVencMemConfig->uiH1EncBufBase, gpstVencMemConfig->uiH1EncBufBase + gpstVencMemConfig->uiH1EncBufSize);
	}

	return ret;
}

int ENC_Encode(LX_VENC_ENC_ENCODE_T *pstEncode)
{
	int ret = RET_ERROR;
	ENC_CTX_T *		handle = NULL;
	unsigned long	flags;
	
	if ( pstEncode == NULL )
	{
		return RET_INVALID_PARAMS;
	}

	handle = _ENC_CtxGet(pstEncode->id);

	if ( handle == NULL )
	{
		VENC_ERROR("handle is NULL\n");
		
		return RET_ERROR;
	}

	spin_lock_irqsave(&handle->lock, flags);

	H1EncodeSetTime( 0 );

	spin_unlock_irqrestore(&handle->lock, flags);

	if ( handle->codecType == LX_VENC_ENCODE_TYPE_VP8 )
	{
		ret = ENC_VP8Encode(handle, pstEncode);
	}
	else
	{
		ret = ENC_H264Encode(handle, pstEncode);
	}

	spin_lock_irqsave(&handle->lock, flags);

	pstEncode->duration = H1EncodeGetTime();

	spin_unlock_irqrestore(&handle->lock, flags);
	
	return ret;

}

#endif

