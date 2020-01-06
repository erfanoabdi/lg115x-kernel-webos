#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
//#include <Common/DDI_DTV.h>

#include <pvr_drv.h>
#include <pvr_kapi.h>
#include <pvr_core_api.h>
#include <pvr_reg.h>
#include <pvr_dev.h>
#include <pvr_drv.h>

#define dvr_isr_err_print(format, arg...) PVR_ISR_PRINT("PVR_ISR %d> "format, __LINE__ , ## arg)

extern volatile DVR_REG_T *gpPvrReg;
extern DVR_MOD_T astDvrControl[];
extern DVR_PIE_DATA_T astDvrPieData[];
extern DVR_BUFFER_STATE_T astDvrBufferState[];

DVR_ErrorStatus_T astDvrErrStat[8];	//8 just in case for nchannel

/*
 * Function to calculate the current download buffer level based on
 * memorized write and read pointers
 */
UINT32	DVR_DN_GetAvailableSpace (LX_PVR_CH_T enDvrCh)
{
	UINT32	ui32WritePtr, ui32ReadPtr;
	UINT32	ui32Base, ui32End;
	UINT32	ui32AvailableSize;

	ui32WritePtr = astDvrMemMap[enDvrCh].stDvrDnBuff.ui32WritePtr;
	ui32ReadPtr =  astDvrMemMap[enDvrCh].stDvrDnBuff.ui32ReadPtr;

	ui32Base = astDvrMemMap[enDvrCh].stDvrDnBuff.ui32BufferBase;
	ui32End = astDvrMemMap[enDvrCh].stDvrDnBuff.ui32BufferEnd;

	if ( ui32WritePtr == ui32ReadPtr )
    {
		//Full buffer size
		ui32AvailableSize = ui32End - ui32Base;
//		PVR_PRINT("W==R");
    }
    else if (ui32WritePtr > ui32ReadPtr)
    {
		//Normal case
		ui32AvailableSize = (ui32End - ui32WritePtr) + (ui32ReadPtr - ui32Base);
//		PVR_PRINT("W>R");
    }
	else
	{
		//Wrap around case (ui32ReadPtr > ui32WritePtr)
		ui32AvailableSize = ui32ReadPtr - ui32WritePtr;
//		PVR_PRINT("W<R");
	}
//	PVR_PRINT(" %u\n",ui32AvailableSize);
	return ui32AvailableSize;
}


#define		_PVR_GET_PICTURE(data)		((data&0x00C00000) == 0x00C00000) ? "SI" :\
										((data&0x00800000) == 0x00800000) ? "S" :\
										((data&0x00400000) == 0x00400000) ? "I" :\
										((data&0x00200000) == 0x00200000) ? "P" :\
										((data&0x00100000) == 0x00100000) ? "B" : "NONE"



/* Event notifications from ISR */
void DVR_ISR_NotifyEvent ( LX_PVR_CH_T enDvrCh, DVR_EVENT_T enEvent, UINT32 ui32Data1, UINT32 ui32Data2 )
{
	switch ( enEvent )
	{
		case DVR_EVT_UNIT_BUFF:
			{
				UINT32	ui32DnSize = 0, ui32BufferNum = 0;
				ui32DnSize = (DVR_DN_MAX_PKT_CNT * 192);

				astDvrErrStat[enDvrCh].ui32DnUnitBuf++;
				if ( DVR_DN_GetAvailableSpace(enDvrCh) > ui32DnSize )
				{
					//There is enough space in the download buffer to accomodate the new
					//data, hence no overflow
				}
				else
				{
					/* Set the current DVR DN state to overflow */
					astDvrBufferState[enDvrCh].eDnBufState = LX_PVR_BUF_STAT_Full;
					/* Increase the overflow error count */
					astDvrErrStat[enDvrCh].ui32DnOverFlowErr++;
				}
				//Update the current write pointer after the overflow calculation
				DVR_DN_GetWritePtrReg(enDvrCh, &astDvrMemMap[enDvrCh].stDvrDnBuff.ui32WritePtr);

				ui32BufferNum = astDvrControl[enDvrCh].ui32CurrBufNum;

				++ui32BufferNum;
				if (ui32BufferNum == astDvrControl[enDvrCh].ui32TotalBufCounter)
				{
					/* Wrap around */
					ui32BufferNum = 0;
				}

				if(astDvrMemMap[enDvrCh].stDvrDnBuff.ui32WritePtr < 
					(astDvrMemMap[enDvrCh].stDvrDnBuff.ui32BufferBase + ui32DvrDnMinPktCount * 192))
					ui32BufferNum = 0;

				astDvrControl[enDvrCh].ui32CurrBufNum = ui32BufferNum;

				PVR_KDRV_LOG( PVR_PIE_DEBUG ,"ISR: UNIT_BUF Ch[%d] BufNum[%d]", (UINT32)enDvrCh, ui32BufferNum);
			}

			break;

		case DVR_EVT_UL_FULL:
			//dvr_isr_err_print ( "I-UP-AL-FULL\n" );
			/* Not handled as of now, need to stop UL if buffer full */
			break;

		case DVR_EVT_UL_ERR_SYNC:
			dvr_isr_err_print ( "I-UP-ERR-SYNC--\n" );
			/* Not handled as of now, needed just for checking hw */
			break;

		case DVR_EVT_UL_EPTY:
			astDvrErrStat[enDvrCh].ui32UpAlmostEmpty++;
			break;

		case DVR_EVT_UL_OVERLAP:
			astDvrErrStat[enDvrCh].ui32UpOverlapErr++;
			break;

		case DVR_EVT_PIE_SCD:
			{
				UINT32 ui32WrIdx = 0, ui32Cnt = 0;
				UINT32 ui32DataWrIdx = 0, ui32BufNum = 0;
				PIE_IND_T 	*pstPieReg = NULL;

				pstPieReg = (PIE_IND_T *)ui32Data1;

				ui32WrIdx = astDvrControl[enDvrCh].ui32PieIndex;
				ui32Cnt = ui32Data2;		//The number of indices is passed in second argument

				ui32DataWrIdx = astDvrPieData[enDvrCh].ui32PieWrIndex;
				ui32BufNum = astDvrControl[enDvrCh].ui32CurrBufNum;

				while ( ui32Cnt )
				{
					PVR_KDRV_LOG( PVR_PIE_DEBUG ,"ISR: Wr[%d] D[0x%x] [%s] Buf[%d]", ui32WrIdx, pstPieReg->ui32Val, _PVR_GET_PICTURE(pstPieReg->ui32Val), ui32BufNum);
					
					astDvrControl[enDvrCh].astPieTable[ui32WrIdx].ui32Val = pstPieReg->ui32Val;
					astDvrControl[enDvrCh].astBufTable[ui32WrIdx] = ui32BufNum;
					++ui32WrIdx;
					if (ui32WrIdx == PIE_MAX_ENTRIES_LOCAL)
					{
						/* Wrap around */
						ui32WrIdx = 0;
					}

					astDvrPieData[enDvrCh].astPieData[ui32DataWrIdx].ui32Val = pstPieReg->ui32Val;
 					++ui32DataWrIdx;
					if (ui32DataWrIdx == PIE_MAX_DATA_LOCAL)
					{
						/* Wrap around */
						ui32DataWrIdx = 0;
					}

					++pstPieReg;
					--ui32Cnt;
				}

				astDvrControl[enDvrCh].ui32PieIndex = ui32WrIdx;
				astDvrPieData[enDvrCh].ui32PieWrIndex = ui32DataWrIdx;
			}
			break;
	}
}



irqreturn_t DVR_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	//UINT32	ui32Status;
	INTR_STAT_T	unIntrStat;

	/* Read the interrupt status register */
	//Memorize the status register for clearing only the serviced interrupts
//	ui32Status = gpPvrReg->intr_stat.ui32Val;

	// Murugan-17.Aug.09
	// Service the interrupts active in the memorized intr_stat field only. If we directly
	//use the register interrupt status while servicing the interrupts, we get chance of
	//some interrupts being serviced twice
	unIntrStat.ui32Val = gpPvrReg->intr_stat.ui32Val;

//	PVR_PRINT ( "I-DVR-IRQ\n" );

	/* Service the interrupt */

	/* UPLOAD */
	if ( unIntrStat.ui32Val &  0x000000FF )	//Byte-0 LSB, UP block
	{
		LX_PVR_CH_T	enDvrCh;

		/* Upload interrupt for channel A */
		if ( unIntrStat.le.almost )
		{
			enDvrCh = LX_PVR_CH_A;

			if ( gpPvrReg->up_buf_stat_a.le.al_ist & 1 )
			{
				// Signal to consumer
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_EPTY, 0, 0);
				//Disable the Almost intr before ack, it will be enabled after updating write ptr
				gpPvrReg->intr_en.le.almost		= 0;
			}

			if ( gpPvrReg->up_buf_stat_a.le.al_ist & 2 )
			{
				// Signal to consumer
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_FULL, 0, 0);
				gpPvrReg->up_conf_a.le.iack = 1;
			}

			//Disable the Almost intr before ack, it will be enabled after updating write ptr
//			gpPvrReg->intr_en.le.almost		= 0;

			//Ack the low level UP Block - ALMOST
//			gpPvrReg->up_conf_a.le.iack = 1;
		}

		/* Upload interrupt for channel B */
		if ( unIntrStat.le.almost1 )
		{
			enDvrCh = LX_PVR_CH_B;

			if ( gpPvrReg->up_buf_stat_b.le.al_ist & 1 )
			{
				// Signal to consumer
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_EPTY, 0, 0);
				gpPvrReg->intr_en.le.almost1	= 0;
			}

			if ( gpPvrReg->up_buf_stat_b.le.al_ist & 2 )
			{
				// Signal to consumer
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_FULL, 0, 0);
				gpPvrReg->up_conf_b.le.iack = 1;
			}

			//Ack the low level UP Block - ALMOST
//			gpPvrReg->up_conf_b.le.iack = 1;

			//Disable the Almost intr before ack, it will be enabled after updating write ptr
//			gpPvrReg->intr_en.le.almost1	= 0;
		}

		/* UP Error interrupt for channel-A */
		if ( unIntrStat.le.up_err )
		{
			enDvrCh = LX_PVR_CH_A;
			if ( gpPvrReg->up_err_stat_a.le.overlap )
			{
				// Signal to consumer
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_OVERLAP, 0, 0);
				gpPvrReg->intr_en.le.up_err = 0;
			}
			else
			{
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_ERR_SYNC, 0, 0);
				//ACK in case of interrupts not handled
				gpPvrReg->up_conf_a.le.iack = 4;
			}
		}

		/* UP Error interrupt for channel-B */
		if ( unIntrStat.le.up_err1 )
		{
			enDvrCh = LX_PVR_CH_B;
			if ( gpPvrReg->up_err_stat_b.le.overlap )
			{
				// Signal to consumer
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_OVERLAP, 0, 0);
				gpPvrReg->intr_en.le.up_err1 = 0;
			}
			else
			{
				DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_ERR_SYNC, 0, 0);
				//ACK in case of interrupts not handled
				gpPvrReg->up_conf_b.le.iack = 4;
			}
		}

		/* UP REP for channel-A */
		if ( unIntrStat.le.rep )
		{
			enDvrCh = LX_PVR_CH_A;
//			DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_OVERLAP, 0, 0);
			gpPvrReg->intr_en.le.rep = 0;
		}

		/* UP REP for channel-B */
		if ( unIntrStat.le.rep1 )
		{
			enDvrCh = LX_PVR_CH_B;
//			DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UL_OVERLAP, 0, 0);
			gpPvrReg->intr_en.le.rep1 = 0;
		}

	}
	
	/* PIE */
	if ( unIntrStat.ui32Val & 0x00FF0000 )
	{
		LX_PVR_CH_T enDvrCh;

//		PVR_PRINT ( "I-PIE--[0x%08X]\n", unIntrStat.ui32Val );

		/* PIE interrupt for channel A */
		if ( unIntrStat.le.scd )
		{
			enDvrCh = LX_PVR_CH_A;

			DVR_ISR_NotifyEvent ( enDvrCh,
					DVR_EVT_PIE_SCD,
					(UINT32) &gpPvrReg->pie_ind_a[0],	//Address for copying the index data
					(UINT32) gpPvrReg->pie_stat_a.le.pind_cnt); //Number of valid entries to copy

			// Ack the pie interrupt
			gpPvrReg->pie_conf_a.le.iack = 1;
		}

		/* PIE interrupt for channel B */
		if ( unIntrStat.le.scd1 )
		{
			enDvrCh = LX_PVR_CH_B;

			DVR_ISR_NotifyEvent ( enDvrCh,
					DVR_EVT_PIE_SCD,
					(UINT32) &gpPvrReg->pie_ind_b[0],	//Address for copying the index data
					(UINT32) gpPvrReg->pie_stat_b.le.pind_cnt); //Number of valid entries to copy

			// Ack the pie interrupt
			gpPvrReg->pie_conf_b.le.iack = 1;
		}
	}

	/* DOWNLOAD */
	if (  unIntrStat.ui32Val &  0x0000FF00 )
	{
		LX_PVR_CH_T	enDvrCh;

//		PVR_PRINT ( "I-DN--[0x%08X]\n", unIntrStat.ui32Val );
		/* Memorize the write pointer of DN buffer and signal the consumer */
		/* Download interrupt for channel A */
		if ( unIntrStat.le.unit_buf )
		{
			enDvrCh = LX_PVR_CH_A;
			DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UNIT_BUFF, 0, 0);
		}

		/* Download interrupt for channel B */
		if ( unIntrStat.le.unit_buf1 )
		{
			enDvrCh = LX_PVR_CH_B;
			DVR_ISR_NotifyEvent ( enDvrCh, DVR_EVT_UNIT_BUFF, 0, 0);
		}
	}

	/* Ignore all other unhandled interrupts */

	//Clear the interrupts
	gpPvrReg->intr_rstat.ui32Val = unIntrStat.ui32Val;

	return IRQ_HANDLED;
}

