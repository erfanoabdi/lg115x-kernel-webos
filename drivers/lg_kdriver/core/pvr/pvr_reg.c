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
 *  register access interface for the LG1150 hardware pvr
 *
 *  author		Murugan Durairaj (murugan.d@lge.com)
 *  version		1.1
 *  date		2010.03.20
 *
 *  @addtogroup lg1150_pvr
 *	@{
 */

#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/semaphore.h>

#include <os_util.h>

//#include <Common/DDI_DTV.h>
#ifdef USE_QEMU_SYSTEM
//#include <Common/MEM_DTV.h>
#else
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#include <asm/hardware.h>
#endif
#endif /* USE_QEMU_SYSTEM */

#include <pvr_drv.h>
#include <pvr_kapi.h>
#include <pvr_reg.h>
#include <pvr_dev.h>

#ifdef USE_QEMU_SYSTEM
DVR_REG_T gstTempPvrReg;
volatile DVR_REG_T *gpPvrReg = (volatile DVR_REG_T *)&gstTempPvrReg;
#else
volatile DVR_REG_T *gpPvrReg;
DVR_REG_T *gpLocalPvrReg, stLocalPvrReg;
#endif /* USE_QEMU_SYSTEM */

/*
 * Upload specific functions
 */
#define dvr_reg_print(format, arg...) PVR_REG_PRINT("PVR_REG(%c) %d> "format, ch?'B':'A', __LINE__ , ## arg)

UINT32 DVR_RegInit(void)
{
    UINT32 rtrn = PVR_OK;
	
	if (lx_chip_rev() >= LX_CHIP_REV(H14, A0))
	{
		gpPvrReg = (DVR_REG_T *)ioremap(H14_DVR_REG_BASE, 0x500);
	}
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, B0))
	{
		gpPvrReg = (DVR_REG_T *)ioremap(M14_B0_DVR_REG_BASE, 0x500);
	} 
	else if (lx_chip_rev() >= LX_CHIP_REV(M14, A0))
	{
		gpPvrReg = (DVR_REG_T *)ioremap(M14_A0_DVR_REG_BASE, 0x500);
	} 
    else if (lx_chip_rev() >= LX_CHIP_REV(H13, A0))
	{
		gpPvrReg = (DVR_REG_T *)ioremap(H13_DVR_REG_BASE, 0x500);
	} 

	return rtrn;
}

UINT32 DVR_UP_SetSpeedReg(LX_PVR_CH_T ch ,SINT32 speed)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->up_max_jitter_a.le.al_jitter =  speed;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->up_max_jitter_b.le.al_jitter =  speed;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_DN_EnableReg(LX_PVR_CH_T ch, BOOLEAN bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->dn_ctrl_a.le.record =  bEnable & 1;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->dn_ctrl_b.le.record =  bEnable & 1;
            break;
        default :
            rtrn = PVR_FAILURE;
			break;
    }
	
    return rtrn;
}

UINT32 DVR_DN_PauseReg(LX_PVR_CH_T ch ,BOOLEAN bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->dn_ctrl_a.le.paus_rec =  bEnable & 1;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->dn_ctrl_b.le.paus_rec =  bEnable & 1;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_DN_EnableINTReg(LX_PVR_CH_T ch ,BOOLEAN bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->intr_en.le.unit_buf  = bEnable & 1;
            gpPvrReg->intr_en.le.dn_err  = bEnable & 1;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->intr_en.le.unit_buf1 = bEnable & 1;
            gpPvrReg->intr_en.le.dn_err1  = bEnable & 1;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_DN_ConfigureIntrLevel(LX_PVR_CH_T ch, UINT32 ui32BufNumLim, UINT32 ui32PktNumLim )
{
    UINT32 rtrn = PVR_OK;

	/* Configure the packet limit and buffer limit for interrupt */
	switch ( ch )
	{
		case LX_PVR_CH_A:
			gpPvrReg->dn_buf_lim_a.le.pkt_cnt_lim = ui32PktNumLim;
            gpPvrReg->dn_buf_lim_a.le.buf_num_lim = ui32BufNumLim & 0x1F;
			break;

		case LX_PVR_CH_B:
			gpPvrReg->dn_buf_lim_b.le.pkt_cnt_lim = ui32PktNumLim;
            gpPvrReg->dn_buf_lim_b.le.buf_num_lim = ui32BufNumLim & 0x1F;
			break;

		default:
            rtrn = PVR_FAILURE;
			break;
	}

    return rtrn;
}

UINT32 DVR_DN_GetPacketBufLimit(LX_PVR_CH_T ch ,UINT32 *pui32PktNumLim, UINT32 *pui32BufNumLim )
{
    UINT32 rtrn = PVR_OK;

	/* Configure the packet limit and buffer limit for interrupt */
	switch ( ch )
	{
		case LX_PVR_CH_A:
			*pui32PktNumLim  = (UINT32) gpPvrReg->dn_buf_lim_a.le.pkt_cnt_lim;
			*pui32BufNumLim  = (UINT32) gpPvrReg->dn_buf_lim_a.le.buf_num_lim;
			break;

		case LX_PVR_CH_B:
			*pui32PktNumLim  = (UINT32) gpPvrReg->dn_buf_lim_b.le.pkt_cnt_lim;
			*pui32BufNumLim  = (UINT32) gpPvrReg->dn_buf_lim_b.le.buf_num_lim;
			break;

		default:
			/* Unhandled */
            rtrn = PVR_FAILURE;
			break;
	}

    return rtrn;
}

UINT32 DVR_UP_EnableReg(LX_PVR_CH_T ch ,BOOLEAN bEnable, UINT8 ui8PacketLen)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->up_conf_a.le.upen =  bEnable & 1;
			gpPvrReg->up_mode_a.le.its_mod = (ui8PacketLen == 188);
			gpPvrReg->up_mode_a.le.num_serr = 0;
			if (ui8PacketLen == 192)
			{
				gpPvrReg->up_mode_a.le.atcp = 1;
				gpPvrReg->up_conf_a.le.tcp = 1;
			}
			else
			{
				gpPvrReg->up_mode_a.le.atcp = 0;
			}
			dvr_reg_print("UpMode[0x%08X], UpConf[0x%08X]\n", gpPvrReg->up_mode_a.ui32Val, gpPvrReg->up_conf_a.ui32Val );
            break;
        case LX_PVR_CH_B :
            gpPvrReg->up_conf_b.le.upen =  bEnable & 1;
			gpPvrReg->up_mode_b.le.its_mod = (ui8PacketLen == 188);
			gpPvrReg->up_mode_b.le.num_serr = 0;
			if (ui8PacketLen == 192)
			{
				gpPvrReg->up_mode_b.le.atcp = 1;
				gpPvrReg->up_conf_b.le.tcp = 1;
			}
			else
			{
				gpPvrReg->up_mode_b.le.atcp = 0;
			}
			dvr_reg_print("UpMode[0x%08X], UpConf[0x%08X]\n", gpPvrReg->up_mode_b.ui32Val, gpPvrReg->up_conf_b.ui32Val );
            break;
        default :
            rtrn = PVR_FAILURE;
			break;
    }
	
    return rtrn;
}

UINT32 DVR_UP_TimeStampCopyReg(LX_PVR_CH_T ch ,BOOLEAN bEnable)
{
	UINT32 rtrn = PVR_OK;

	switch (ch) {
		case LX_PVR_CH_A :
			gpPvrReg->up_mode_a.le.atcp = 1;
			gpPvrReg->up_conf_a.le.tcp = bEnable & 1;
			break;
		case LX_PVR_CH_B :
			gpPvrReg->up_mode_b.le.atcp = 1;
			gpPvrReg->up_conf_b.le.tcp = bEnable & 1;
			break;
		default :
			rtrn = PVR_FAILURE;
	}
	return rtrn;
}


UINT32 DVR_UP_TimeStampCheckDisableReg(LX_PVR_CH_T ch ,BOOLEAN bDisable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
			/* Setting 1 to the TSC register field will disable the Timestamp checking */
			gpPvrReg->up_mode_a.le.tsc = bDisable;
            break;

        case LX_PVR_CH_B :
			/* Setting 1 to the TSC register field will disable the Timestamp checking */
			gpPvrReg->up_mode_b.le.tsc = bDisable;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

/*
 * PLAY_MOD:
 *	000 - Normal play
 *	001 - X2 Faster
 *	010 - X2 Slower
 *	011 - X4 Slower
 *	100 - Trick play mode
 */
UINT32 DVR_UP_ChangePlaymode(LX_PVR_CH_T ch , UINT8 ui8PlayMode )
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->up_mode_a.le.play_mod = ui8PlayMode;
            break;

        case LX_PVR_CH_B :
			gpPvrReg->up_mode_b.le.play_mod = ui8PlayMode;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

BOOLEAN DVR_UP_GetEnableState(LX_PVR_CH_T ch)
{
	BOOLEAN	bEnable;

    switch (ch) {
        case LX_PVR_CH_A :
			bEnable = gpPvrReg->up_conf_a.le.upen;
            break;
        case LX_PVR_CH_B :
			bEnable = gpPvrReg->up_conf_b.le.upen;
            break;
        default :
            bEnable = FALSE;
    }
    return bEnable;
}

UINT8 DVR_UP_GetErrorStat(LX_PVR_CH_T ch)
{
	UINT8	ui8ErrStat;

    switch (ch) {
        case LX_PVR_CH_A :
			ui8ErrStat = gpPvrReg->up_err_stat_a.ui32Val & 0x3;
            break;
        case LX_PVR_CH_B :
			ui8ErrStat = gpPvrReg->up_err_stat_b.ui32Val & 0x3;
            break;
        default :
            ui8ErrStat = 0;
    }
    return ui8ErrStat;
}



UINT32 DVR_UP_PauseReg(LX_PVR_CH_T ch ,BOOLEAN bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->up_conf_a.le.upps =  bEnable & 1;
			gpPvrReg->up_conf_a.le.tcp = !bEnable;	//Copy timestamp when pauseis released
			//15.06.2010 - Disable flush bit when enable pause
            gpPvrReg->up_mode_a.le.flush =  !bEnable;
 			dvr_reg_print("%s -> UpMode[0x%08X], UpConf[0x%08X] Rptr[0x%08X]\n",
					bEnable ? "Pause" : "Resume",
					gpPvrReg->up_mode_a.ui32Val, gpPvrReg->up_conf_a.ui32Val, gpPvrReg->up_buf_rptr_a.ui32Val );
            break;
        case LX_PVR_CH_B :
            gpPvrReg->up_conf_b.le.upps =  bEnable & 1;
			gpPvrReg->up_conf_b.le.tcp = !bEnable;	//Copy timestamp when pauseis released
			//15.06.2010 - Disable flush bit when enable pause
            gpPvrReg->up_mode_b.le.flush =  !bEnable;
 			dvr_reg_print("%s -> UpMode[0x%08X], UpConf[0x%08X] Rptr[0x%08X]\n",
					bEnable ? "Pause" : "Resume",
					gpPvrReg->up_mode_b.ui32Val, gpPvrReg->up_conf_b.ui32Val, gpPvrReg->up_buf_rptr_b.ui32Val );
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_UP_RepPauseReg(LX_PVR_CH_T ch ,BOOLEAN bEnable, UINT32 ui32Pptr)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
			if ( bEnable )
			{
				//Update PPTR
				gpPvrReg->up_buf_pptr_a.le.up_buf_pptr = ui32Pptr;
			}
			gpPvrReg->intr_en.le.rep   = (bEnable)?1:0;
            gpPvrReg->up_conf_a.le.rep = bEnable & 1;
			gpPvrReg->up_conf_a.le.tcp = !bEnable;	//Copy timestamp when pauseis released
            break;
        case LX_PVR_CH_B :
			if ( bEnable )
			{
				//Update PPTR
				gpPvrReg->up_buf_pptr_b.le.up_buf_pptr = ui32Pptr;
			}
			gpPvrReg->intr_en.le.rep1	= (bEnable)?1:0;
            gpPvrReg->up_conf_b.le.rep =  bEnable & 1;
			gpPvrReg->up_conf_b.le.tcp = !bEnable;	//Copy timestamp when pauseis released
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_DN_GetBufBoundReg( LX_PVR_CH_T ch, UINT32 *pBase, UINT32 *pEnd )
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            *pBase = gpPvrReg->dn_buf_sptr_a.le.sptr;
            *pEnd  = gpPvrReg->dn_buf_eptr_a.le.eptr;
            break;
        case LX_PVR_CH_B :
            *pBase = gpPvrReg->dn_buf_sptr_b.le.sptr;
            *pEnd  = gpPvrReg->dn_buf_eptr_b.le.eptr;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	*pBase <<= 12;
	*pEnd  <<= 12;

    return rtrn;
}

UINT32 DVR_DN_SetBufBoundReg(LX_PVR_CH_T ch, UINT32 ptrBase, UINT32 ptrEnd)
{
    UINT32 rtrn = PVR_OK;

	dvr_reg_print("Dn Base[0x%08X] End[0x%08x]\n", ptrBase, ptrEnd );

	ptrBase >>= 12;
	ptrEnd  >>= 12;
    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->dn_buf_sptr_a.le.sptr = ptrBase;
			gpPvrReg->dn_buf_eptr_a.le.eptr = ptrEnd;
			/* dvr_reg_print("Dn[%c] sptr[0x%04X] eptr[0x%04x]\n", ch ? 'B' : 'A',
				gpPvrReg->dn_buf_sptr_a.le.sptr,
				gpPvrReg->dn_buf_eptr_a.le.eptr ); */
            break;
        case LX_PVR_CH_B :
			gpPvrReg->dn_buf_sptr_b.le.sptr = ptrBase;
			gpPvrReg->dn_buf_eptr_b.le.eptr = ptrEnd;
			/* dvr_reg_print("Dn[%c] sptr[0x%04X] eptr[0x%04x]\n", ch ? 'B' : 'A',
				gpPvrReg->dn_buf_sptr_b.le.sptr,
				gpPvrReg->dn_buf_eptr_b.le.eptr ); */
            break;
        default :
            rtrn = PVR_FAILURE;
    }

    return rtrn;
}

UINT32 DVR_DN_GetWritePtrReg( LX_PVR_CH_T ch, UINT32 *pWrite)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            *pWrite = gpPvrReg->dn_buf_wptr_a.le.dn_buf_wptr;
            break;
        case LX_PVR_CH_B :
            *pWrite = gpPvrReg->dn_buf_wptr_b.le.dn_buf_wptr;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	*pWrite <<= 3;

    return rtrn;
}

UINT32 DVR_DN_SetWritePtrReg(LX_PVR_CH_T ch, UINT32 ptrWrite)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->dn_buf_wptr_a.le.dn_buf_wptr = (ptrWrite >> 3);
			dvr_reg_print("Set Dn wptr[0x%08X]\n",
				gpPvrReg->dn_buf_wptr_a.ui32Val );
            break;
        case LX_PVR_CH_B :
            gpPvrReg->dn_buf_wptr_b.le.dn_buf_wptr = (ptrWrite >> 3);
			dvr_reg_print("Set Dn wptr[0x%08X]\n",
				gpPvrReg->dn_buf_wptr_b.ui32Val );
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_UP_SetWritePtrReg(LX_PVR_CH_T ch, UINT32 ptrWrite)
{
    UINT32 rtrn = PVR_OK;

//	dvr_reg_print("Up[%c] WritePtr[0x%08X]\n", ch ? 'B' : 'A', ptrWrite );

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->up_buf_wptr_a.le.up_buf_wptr = ptrWrite;
			dvr_reg_print("Up Set wptr_reg[0x%08X], cur rd_ptr[0x%08X]\n",
				gpPvrReg->up_buf_wptr_a.le.up_buf_wptr,
				gpPvrReg->up_buf_rptr_a.le.up_buf_rptr );
            break;
        case LX_PVR_CH_B :
            gpPvrReg->up_buf_wptr_b.le.up_buf_wptr = ptrWrite;
			dvr_reg_print("Up Set wptr_reg[0x%08X], cur rd_ptr[0x%08X]\n",
				gpPvrReg->up_buf_wptr_b.le.up_buf_wptr,
				gpPvrReg->up_buf_rptr_b.le.up_buf_rptr );
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_UP_SetReadPtrReg(LX_PVR_CH_T ch, UINT32 ptrRead)
{
    UINT32 rtrn = PVR_OK;

	dvr_reg_print("Up[%c] ReadPtr[0x%08X]\n", ch ? 'B' : 'A', ptrRead );

//	ptrRead >>= 3;
    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->up_buf_rptr_a.le.up_buf_rptr = ptrRead;
			dvr_reg_print("Up set rd_ptr[0x%08X]\n",
				gpPvrReg->up_buf_rptr_a.le.up_buf_rptr );
            break;
        case LX_PVR_CH_B :
            gpPvrReg->up_buf_rptr_b.le.up_buf_rptr = ptrRead;
			dvr_reg_print("Up set rd_ptr[0x%08X]\n",
				gpPvrReg->up_buf_rptr_b.le.up_buf_rptr );
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_UP_GetPointersReg(LX_PVR_CH_T ch, UINT32 *pWrite, UINT32 *pRead)
{
    UINT32 rtrn = PVR_OK;

    /* Read the write and read pointers & return to the caller */
    switch (ch) {
        case LX_PVR_CH_A :
            *pWrite = gpPvrReg->up_buf_wptr_a.le.up_buf_wptr;
            *pRead  = gpPvrReg->up_buf_rptr_a.le.up_buf_rptr;
            break;
        case LX_PVR_CH_B :
            *pWrite = gpPvrReg->up_buf_wptr_b.le.up_buf_wptr;
            *pRead  = gpPvrReg->up_buf_rptr_b.le.up_buf_rptr;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
/*	dvr_reg_print("Up[%c] cur_wptr_reg[0x%08X], cur_rd_ptr[0x%08X]\n", ch ? 'B' : 'A',
				*pWrite ,
				*pRead ); */

//	*pWrite <<= 3;
//	*pRead  <<= 3;

    return rtrn;
}

UINT32 DVR_UP_SetBufBoundReg(LX_PVR_CH_T ch, UINT32 ptrBase, UINT32 ptrEnd)
{
    UINT32 rtrn = PVR_OK;

	dvr_reg_print("Up Base[0x%08X] End[0x%08x]\n", ptrBase, ptrEnd );

	ptrBase >>= 12;
	ptrEnd  >>= 12;
    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->up_buf_sptr_a.le.sptr = ptrBase;
			gpPvrReg->up_buf_eptr_a.le.eptr = ptrEnd ;
			/* dvr_reg_print("Up sptr[0x%04X] eptr[0x%04x]\n",
				gpPvrReg->up_buf_sptr_a.le.sptr,
				gpPvrReg->up_buf_eptr_a.le.eptr );*/
            break;
        case LX_PVR_CH_B :
			gpPvrReg->up_buf_sptr_b.le.sptr = ptrBase;
			gpPvrReg->up_buf_eptr_b.le.eptr = ptrEnd;
			/* dvr_reg_print("Up sptr[0x%04X] eptr[0x%04x]\n",
				gpPvrReg->up_buf_sptr_b.le.sptr,
				gpPvrReg->up_buf_eptr_b.le.eptr ); */
            break;
        default :
            rtrn = PVR_FAILURE;
    }


    return rtrn;
}

UINT32 DVR_UP_GetBufBoundReg(LX_PVR_CH_T ch, UINT32 *pBase, UINT32 *pEnd)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            *pBase = gpPvrReg->up_buf_sptr_a.le.sptr;
            *pEnd  = gpPvrReg->up_buf_eptr_a.le.eptr;
            break;
        case LX_PVR_CH_B :
            *pBase = gpPvrReg->up_buf_sptr_b.le.sptr;
            *pEnd  = gpPvrReg->up_buf_eptr_b.le.eptr;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	*pBase <<= 12;
	*pEnd  <<= 12;

    return rtrn;
}

UINT32 DVR_UP_ResetBlock(LX_PVR_CH_T ch)
{
    UINT32 rtrn = PVR_OK;
	
    switch (ch) {
        case LX_PVR_CH_A :
			// Reset Upload Block with nm_rst 1-> 0
			gpPvrReg->up_conf_a.le.nm_rst = 1;
			dvr_reg_print("UL Block NM Reset [0x%08X]\n", gpPvrReg->up_conf_a.ui32Val );
			gpPvrReg->up_conf_a.le.nm_rst = 0;
            break;
        case LX_PVR_CH_B :
			// Reset Upload Block with nm_rst 1-> 0
			gpPvrReg->up_conf_b.le.nm_rst = 1;
			dvr_reg_print("UL Block NM Reset [0x%08X]\n", gpPvrReg->up_conf_b.ui32Val );
			gpPvrReg->up_conf_b.le.nm_rst = 0;
            break;
        default :
            rtrn = PVR_FAILURE;
			break;
    }
	
    return rtrn;
}

UINT32 DVR_DN_ResetBlock(LX_PVR_CH_T ch)
{
    UINT32 rtrn = PVR_OK;
	
    switch (ch) {
        case LX_PVR_CH_A :
			//Reset Block
			gpPvrReg->dn_ctrl_a.le.reset_sw = 1;
			dvr_reg_print("DN Block Reset [0x%08X]\n", gpPvrReg->dn_ctrl_a.ui32Val );
            break;
        case LX_PVR_CH_B :
			//Reset Block
			gpPvrReg->dn_ctrl_b.le.reset_sw = 1;
			dvr_reg_print("DN Block Reset [0x%08X]\n", gpPvrReg->dn_ctrl_b.ui32Val );
            break;
        default :
            rtrn = PVR_FAILURE;
			break;
    }
	
    return rtrn;
}

UINT32 DVR_UP_EnableEmptyLevelReg(LX_PVR_CH_T ch, UINT32 bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->intr_en.le.almost		= (bEnable)?1:0;
			//Error interrupt also
			gpPvrReg->intr_en.le.up_err		= (bEnable)?1:0;
			//Both full and empty interrupt masks are enabled time being
			gpPvrReg->up_buf_stat_a.le.al_imsk = (bEnable)?0:3;
//			dvr_reg_print("Intr En[0x%08X], Up_Buf_Stat[0x%08X]\n", gpPvrReg->intr_en.ui32Val, gpPvrReg->up_buf_stat_a.ui32Val );
            break;
        case LX_PVR_CH_B :
			gpPvrReg->intr_en.le.almost1		= (bEnable)?1:0;
			//Error interrupt also
			gpPvrReg->intr_en.le.up_err1		= (bEnable)?1:0;
			//Both full and empty interrupt masks are enabled time being
			gpPvrReg->up_buf_stat_b.le.al_imsk = (bEnable)?0:3;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_UP_SetEmptyLevelReg(LX_PVR_CH_T ch, UINT32 level)
{
    UINT32 rtrn = PVR_OK;

//	dvr_reg_print("Up[%c] Empty Lvl[%d] Full Lvl[%d]\n", ch ? 'B' : 'A', level, level );

	level  >>= 12;

    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->up_al_empty_a.le.level = level;
			gpPvrReg->up_al_ful_a.le.level = level;
//			dvr_reg_print("Up[%c] Empty[0x%08X] Full[0x%08X]\n", ch ? 'B' : 'A', gpPvrReg->up_al_empty_a.le.level, gpPvrReg->up_al_ful_a.le.level );
            break;
        case LX_PVR_CH_B :
			gpPvrReg->up_al_empty_b.le.level = level;
			gpPvrReg->up_al_ful_b.le.level = level;
//			dvr_reg_print("Up[%c] Empty[0x%08X] Full[0x%08X]\n", ch ? 'B' : 'A', gpPvrReg->up_al_empty_b.le.level, gpPvrReg->up_al_ful_b.le.level );
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_UP_GetTSCJitterReg(LX_PVR_CH_T ch, UINT32 *pJitter)
{
    UINT32 rtrn = PVR_OK;

//	dvr_reg_print("Up[%c] Empty Lvl[%d] Full Lvl[%d]\n", ch ? 'B' : 'A', level, level );

    switch (ch) {
        case LX_PVR_CH_A :
			*pJitter = gpPvrReg->up_tsc_jitter_a.ui32Val;
            break;
        case LX_PVR_CH_B :
			*pJitter = gpPvrReg->up_tsc_jitter_b.ui32Val;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_PIE_SetModeReg(LX_PVR_CH_T ch, UINT32 mode)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->pie_mode_a.le.pie_mode = mode & 0x3;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->pie_mode_b.le.pie_mode = mode & 0x3;
            break;
        default :
            rtrn = PVR_FAILURE;
			break;
    }
	return rtrn;
}

UINT32 DVR_DN_SetPIDReg(LX_PVR_CH_T ch, UINT32 PID)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
//            gpPvrReg->pie_mode_a.le.strm_sel = 1;
            gpPvrReg->dn_vpid_a.le.pid =  PID & 0x1fff;
            break;
        case LX_PVR_CH_B :
//            gpPvrReg->pie_mode_b.le.strm_sel = 1;
            gpPvrReg->dn_vpid_b.le.pid =  PID & 0x1fff;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}

UINT32 DVR_PIE_ConfigureSCD(LX_PVR_CH_T ch, UINT8 ui8ScdIndex, UINT32 ui32Mask, UINT32 ui32Value, UINT8 ui8Enable )
{
    UINT32 rtrn = PVR_OK;

	if ( ui8ScdIndex > DVR_SCD_INDEX_MAX )
	{
		return PVR_FAILURE;
	}

    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->pie_scd_mask_a[ui8ScdIndex].le.scd_mask = ui32Mask;
			gpPvrReg->pie_scd_value_a[ui8ScdIndex].le.scd_value = ui32Value;
			if ( ui8Enable )
			{
				/* Enable the filter */
				gpPvrReg->pie_mode_a.le.gscd_filter |= (1 << ui8ScdIndex);
			}
			else
			{
				/* Disable the filter */
				gpPvrReg->pie_mode_a.le.gscd_filter &= ~(1 << ui8ScdIndex);
			}
            break;
        case LX_PVR_CH_B :
			gpPvrReg->pie_scd_mask_b[ui8ScdIndex].le.scd_mask = ui32Mask;
			gpPvrReg->pie_scd_value_b[ui8ScdIndex].le.scd_value = ui32Value;
			if ( ui8Enable )
			{
				/* Enable the filter */
				gpPvrReg->pie_mode_b.le.gscd_filter |= (1 << ui8ScdIndex);
			}
			else
			{
				/* Disable the filter */
				gpPvrReg->pie_mode_b.le.gscd_filter &= ~(1 << ui8ScdIndex);
			}
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	return rtrn;
}

UINT32 DVR_PIE_GscdByteInfoConfig(LX_PVR_CH_T ch, UINT8 ui8ByteSel0, UINT8 ui8ByteSel1, UINT8 ui8ByteSel2, UINT8 ui8ByteSel3 )
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->pie_mode_a.le.scd0_bsel = ui8ByteSel0;
			gpPvrReg->pie_mode_a.le.scd1_bsel = ui8ByteSel1;
			gpPvrReg->pie_mode_a.le.scd2_bsel = ui8ByteSel2;
			gpPvrReg->pie_mode_a.le.scd3_bsel = ui8ByteSel3;
            break;
        case LX_PVR_CH_B :
			gpPvrReg->pie_mode_b.le.scd0_bsel = ui8ByteSel0;
			gpPvrReg->pie_mode_b.le.scd1_bsel = ui8ByteSel1;
			gpPvrReg->pie_mode_b.le.scd2_bsel = ui8ByteSel2;
			gpPvrReg->pie_mode_b.le.scd3_bsel = ui8ByteSel3;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	return rtrn;
}


UINT32 DVR_PIE_EnableSCDReg(LX_PVR_CH_T ch, BOOLEAN bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
			gpPvrReg->pie_mode_a.le.gscd_en = bEnable & 0x1;
            break;
        case LX_PVR_CH_B :
			gpPvrReg->pie_mode_b.le.gscd_en = bEnable & 0x1;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	return rtrn;
}

UINT32 DVR_PIE_EnableINTReg(LX_PVR_CH_T ch ,BOOLEAN bEnable)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->intr_en.le.scd  = bEnable & 1;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->intr_en.le.scd1  = bEnable & 1;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
    return rtrn;
}


UINT32 DVR_PIE_ClearIACKReg(LX_PVR_CH_T ch, BOOLEAN bClear)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->pie_conf_a.le.iack = bClear & 0x1;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->pie_conf_b.le.iack = bClear & 0x1;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	return rtrn;
}

UINT32 DVR_PIE_ResetBlock(LX_PVR_CH_T ch)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->pie_conf_a.le.vd_rst = 1;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->pie_conf_b.le.vd_rst = 1;
            break;
        default :
            rtrn = PVR_FAILURE;
			break;
    }
	
	return rtrn;
}

UINT32 DVR_UP_ClearIACKReg(LX_PVR_CH_T ch, UINT8 ui8AckValue)
{
    UINT32 rtrn = PVR_OK;

    switch (ch) {
        case LX_PVR_CH_A :
            gpPvrReg->up_conf_a.le.iack = ui8AckValue;
            break;
        case LX_PVR_CH_B :
            gpPvrReg->up_conf_b.le.iack = ui8AckValue;
            break;
        default :
            rtrn = PVR_FAILURE;
    }
	return rtrn;
}


UINT32 DVR_REG_ReadAllRegisters ( DVR_REG_T *pstDvrRegDest )
{
	UINT32	ui32RegAreaSize;
	UINT32	*pui32RegSrc, *pui32RegDest;

	pui32RegSrc = (UINT32 *) gpPvrReg;
	pui32RegDest = (UINT32 *) pstDvrRegDest;
	ui32RegAreaSize = sizeof ( DVR_REG_T )/4;

	/* Copy all the register contents to the given register structure */
//	PVR_PRINT ( "DVRMOD> Copy registers [0x%08X] -> [0x%08X], [%d]words\n", (UINT32)pui32RegSrc, (UINT32)pui32RegDest, (UINT32)ui32RegAreaSize );
	while ( ui32RegAreaSize-- )
	{
		*pui32RegDest++ = *pui32RegSrc++;
	}
	/* Print the first and last words of the register area here for cross checking in user space code */
//	PVR_PRINT (" Intr_en [0x%08X]\n up_stat_b_1 [0x%08X]\n", *((UINT32*)&gpPvrReg->intr_en.le), *((UINT32*)&gpPvrReg->up_stat1_b.le) );
//	PVR_PRINT ( "DVRMOD> Copy success !!\n" );
	return PVR_OK;

}

UINT32 DVR_CheckPatternMismatch ( LX_PVR_CH_T ch )
{
//	UINT32	ui32Status, ui32ErrorAddr;
#if 0
	volatile UINT32	*pui32PatternReg;
	UINT32	ui32PatternStat;

	pui32PatternReg = (volatile UINT32 *) ((UINT32)gpPvrReg + 0x18); // 0xA418 =

	ui32PatternStat = *pui32PatternReg;
	if ( ui32PatternStat & 7 ) //b0, b1, b2
	{
		PVR_PRINT ( "Pattern Mismatch [0x%08X] !!!!!\n", ui32PatternStat );
		return 1;
	}
#endif /* #if 0 */
#if 0
	switch ( ch ) {
        case LX_PVR_CH_A :
            break;
        case LX_PVR_CH_B :
            break;
        default :
            break;
    }
#endif /* #if 0 */


//	return 1;	// Pattern mismatch error
	return 0;	// No error
}
#if 1
void DVR_EnableWaitCycle (LX_PVR_CH_T ch, UINT16 ui16WaitCycle )
{
	UP_MODE_A_T	UpModeRegLocal;
    PVR_KDRV_LOG( PVR_UPLOAD ,"DVR> Wait_Cycle Mode Reg A[0x%08X] B[0x%08X]\n", gpPvrReg->up_mode_a.ui32Val,  gpPvrReg->up_mode_b.ui32Val );

    switch ( ch ) {
        case LX_PVR_CH_A :
	        UpModeRegLocal.ui32Val = gpPvrReg->up_mode_a.ui32Val;
	        UpModeRegLocal.le.wait_cycle = ui16WaitCycle;
	        gpPvrReg->up_mode_a.ui32Val = UpModeRegLocal.ui32Val;
			PVR_KDRV_LOG( PVR_UPLOAD ,"DVR> Wait_Cycle Mode Reg A[0x%08X]\n", gpPvrReg->up_mode_a.ui32Val);
            break;
        case LX_PVR_CH_B :
            UpModeRegLocal.ui32Val = gpPvrReg->up_mode_b.ui32Val;
	        UpModeRegLocal.le.wait_cycle = ui16WaitCycle;
	        gpPvrReg->up_mode_b.ui32Val = UpModeRegLocal.ui32Val;
			PVR_KDRV_LOG( PVR_UPLOAD ,"DVR> Wait_Cycle Mode Reg B[0x%08X]\n", gpPvrReg->up_mode_b.ui32Val );
            break;
        default :
            break;
    }
	return;
}
#else
void DVR_EnableWaitCycle ( UINT16 ui16WaitCycle )
{
	UP_MODE_A_T	UpModeRegLocal;

	PVR_PRINT ( "DVR> Wait_Cycle Mode Reg A[0x%08X] B[0x%08X]\n", gpPvrReg->up_mode_a.ui32Val,  gpPvrReg->up_mode_b.ui32Val );
	UpModeRegLocal.ui32Val = gpPvrReg->up_mode_a.ui32Val;
	UpModeRegLocal.le.wait_cycle = ui16WaitCycle;
	gpPvrReg->up_mode_a.ui32Val = UpModeRegLocal.ui32Val;
	UpModeRegLocal.ui32Val = gpPvrReg->up_mode_b.ui32Val;
	UpModeRegLocal.le.wait_cycle = ui16WaitCycle;
	gpPvrReg->up_mode_b.ui32Val = UpModeRegLocal.ui32Val;
	PVR_PRINT ( "DVR> Wait_Cycle Mode Reg A[0x%08X] B[0x%08X]\n", gpPvrReg->up_mode_a.ui32Val,  gpPvrReg->up_mode_b.ui32Val );
	return;
}
#endif

