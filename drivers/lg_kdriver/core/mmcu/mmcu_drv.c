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
 * main driver implementation for media MCU device. The media MCU may be used for
 * 	VDEC & VENC
 *
 * author	  taewoong.kim (taewoong.kim@lge.com)
 * version	  1.0
 * date		  2013.05.02
 * note		  Additional information.
 *
 * @addtogroup lg1154_media4_media_mcu
 * @{
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>    /**< For isr */
#include <linux/irqreturn.h>
#include <linux/delay.h>
#include "base_device.h"
#include "hma_alloc.h"
#include "os_util.h"
#include "base_types.h"

#include "mmcu_drv.h"
#include "mmcu_hal.h"
#include "mmcu_fw.h"
#include "mmcu_pipe.h"
#include "mmcu_lock.h"
#include "mmcu_proc.h"
#include "../sys/sys_regs.h"

#include "log.h"

logm_define (mmcu_drv, log_level_warning);

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

/*----------------------------------------------------------------------------------------
 *   Function Define
 *---------------------------------------------------------------------------------------*/

static void _MMCU_LoadMcuFw(void)
{
	UINT32		ui32Rom, ui32Ram;
	UINT32		mem_size, mem_base;
	SINT32		ret;

	mem_size = 0x80000 + 0x80000 + 0x100;
	mem_base = hma_alloc_user ("ddr0", mem_size, 1<<8, "mmcu");
	if (mem_base == 0)
	{
		logm_error(mmcu_drv, "no memory for mmcu driver %d\n", mem_size);
		return;
	}

	ret = hma_pool_register ("mmcu", mem_base, mem_size);
	if (ret < 0)
	{
		logm_error(mmcu_drv, "pool register fail %d\n", ret);
		return;
	}

	ui32Rom = hma_alloc_user("mmcu", 0x80000, 1 << 8, "mcu_rom");
	ui32Ram = hma_alloc_user("mmcu", 0x80000, 1 << 8, "mcu_ram");

	logm_noti(mmcu_drv, "ROM 0x%X	RAM 0x%X\n", ui32Rom, ui32Ram);

	MCU_HAL_InitSystemMemory(ui32Rom, ui32Ram, ui32Ram, 0);
	MCU_HAL_CodeDown(ui32Rom, 0x80000, aui8VdecCodec, sizeof(aui8VdecCodec));
}

void MMCU_PreInit(void)
{
}


static void _MMCU_Uart0_Switch_H14(int whichUart)
{
	switch (whichUart)
	{
	case 0:
		CTOP_CTRL_H14_RdFL(ctr58);
		CTOP_CTRL_H14_Wr01(ctr58, uart0_sel, 2);			// UART0 -> CPU0
		CTOP_CTRL_H14_Wr01(ctr58, rx_sel_vdec, 1);			// UART1 -> DE
		CTOP_CTRL_H14_WrFL(ctr58);
		break;
	case 1:
		CTOP_CTRL_H14_RdFL(ctr58);
		CTOP_CTRL_H14_Wr01(ctr58, uart0_sel, 4);			// UART0 -> MMCU
		CTOP_CTRL_H14_Wr01(ctr58, rx_sel_vdec, 0);			// UART0 RX -> VDEC
		CTOP_CTRL_H14_WrFL(ctr58);
		break;
	default:
		break;
	}
}

static void _MMCU_Uart0_Switch_M14(int whichUart)
{
	logm_noti(mmcu_drv, "set uart mode %d\n", whichUart);

	switch (whichUart)
	{
	case 0:
		CTOP_CTRL_M14_RdFL(ctr58);
		CTOP_CTRL_M14_Wr01(ctr58, uart0_sel, 2);			// UART0 -> CPU0
		CTOP_CTRL_M14_Wr01(ctr58, rx_sel_vdec, 1);			// UART1 -> DE
		CTOP_CTRL_M14_WrFL(ctr58);
		break;
	case 1:
		CTOP_CTRL_M14_RdFL(ctr58);
		CTOP_CTRL_M14_Wr01(ctr58, uart0_sel, 4);			// UART0 -> MMCU
		CTOP_CTRL_M14_Wr01(ctr58, rx_sel_vdec, 0);			// UART0 RX -> VDEC
		CTOP_CTRL_M14_WrFL(ctr58);
		break;
	default:
		break;
	}
}

static int _MMCU_Uart0_Switch_H13(int whichUart)
{
	int ret = RET_OK;

	logm_noti(mmcu_drv, "set uart mode %d\n", whichUart);
	CTOP_CTRL_H13_RdFL(ctr58);

	switch (whichUart)
	{
		case 0 :
			CTOP_CTRL_H13_Wr01(ctr58, uart0_sel, 2); // UART0 = cpu0
			CTOP_CTRL_H13_Wr01(ctr58, uart1_sel, 4); // UART1 = mmcu
			CTOP_CTRL_H13_Wr01(ctr58, rx_sel_vdec, 1); // 2:MMCU from UART1
			break;
		case 1 :
			CTOP_CTRL_H13_Wr01(ctr58, uart0_sel, 4); // UART0 = mmcu
			//CTOP_CTRL_H13_Wr01(ctr58, uart1_sel, 2); // UART1 = cpu0
			CTOP_CTRL_H13_Wr01(ctr58, rx_sel_vdec, 0); // 0:MMCU from UART0
			break;
		case 2 :
			CTOP_CTRL_H13_Wr01(ctr58, uart2_sel, 4); // UART2 = mmcu
			CTOP_CTRL_H13_Wr01(ctr58, rx_sel_vdec, 2); // 0:MMCU from UART2
			CTOP_CTRL_H13_RdFL(ctr95);
			CTOP_CTRL_H13_Wr01(ctr95, uart_en, 1); //
			CTOP_CTRL_H13_WrFL(ctr95);

			break;

		default :
			//BREAK_WRONG(whichUart);
			break;
	}
	CTOP_CTRL_H13_WrFL(ctr58);

	logm_noti(mmcu_drv, "set uart done\n");
	return ret;
}

static void _MMCU_Uart0_Switch(int whichUart)
{
	UINT32 chip_rev;

	chip_rev = lx_chip_rev();
	logm_noti(mmcu_drv, "set uart mode %d\n", whichUart);

	if (chip_rev>=LX_CHIP_REV(L9,0) && chip_rev<=LX_CHIP_REV(L9,FF))
	{
	}
	else if (chip_rev>=LX_CHIP_REV(H13,0) && chip_rev<=LX_CHIP_REV(H13,FF))
	{
		_MMCU_Uart0_Switch_H13(whichUart);
	}
	else if (chip_rev>=LX_CHIP_REV(M14,0) && chip_rev<=LX_CHIP_REV(M14,FF))
	{
		_MMCU_Uart0_Switch_M14(whichUart);
	}
	else if (chip_rev>=LX_CHIP_REV(H14,0) && chip_rev<=LX_CHIP_REV(H14,FF))
	{
		_MMCU_Uart0_Switch_H14(whichUart);
	}
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	UINT32 type;
	void *pData;

	//if (VDEC_ISR_Handler() < 0)
	//	return IRQ_NONE;

	pData = MCU_HAL_GetIpcData();
	type = ((reg_mcu_ipc_t*)pData)->cmd;

	switch(type)
	{
		case MMCU_IPC_PIPECMD:
			MPIPE_Isr(pData);
			break;
	}

	MCU_HAL_ClearExtIntr(MCU_INTR_IPC);

	return IRQ_HANDLED;
}

static UINT32 _MMCU_InitInt(MMCU_HAL_ISR_FN fpIsrFn)
{
	int ret = FALSE;

	ret = request_irq(MMCU_IRQ_NUM,	(irq_handler_t)irq_handler,
			IRQF_SHARED, "mmcu", (void*)0xdecdec2);

	//MCU_HAL_RegisterIsrFn(fpIsrFn);
	MCU_HAL_EnableExtIntr(MCU_INTR_IPC);

	return ret;
}


#define CMD_PIPE_BUF_SIZE	0x1000
void	MCU_HAL_SetInterIntrStatus(MCU_INTR_MODE eMode, BOOLEAN bValue);
void	MCU_HAL_SetExtIntrEvent(void);
void IPC_CMD_Init(void);



UINT32 MMCU_Read(UINT8* pu8Buf, UINT32 u32Size)
{
//	MPIPE_Read(pu8Buf, u32Size);		!!!!!!!!!!

	return 0;
}
EXPORT_SYMBOL(MMCU_Read);

void MMCU_HandleIsr(UINT32 type, void *data)
{
	switch(type)
	{
		case MMCU_IPC_PIPECMD:
			MPIPE_Isr(data);
			break;
	}

	return;
}

int MMCU_Init(void)
{
	logm_noti(mmcu_drv, "Media MCU Init!!\n");

	mmcu_proc_init();

	MCU_HAL_Init();

	_MMCU_LoadMcuFw();

	//_MMCU_Uart0_Switch(1);

	_MMCU_InitInt(MMCU_HandleIsr);

	msleep(100);

	//_MMCU_InitIpc();

	MLOCK_Init(NULL, 0);
	MPIPE_Init();

	//IPC_CMD_Init();

	//_MMCU_ShmInit(pPhShmAddr);

	logm_noti(mmcu_drv, "Init OK\n");
	return 0;
}

void MMCU_Cleanup(void)
{
}
/*
void MMCU_Isr(void)
{
	switch()
	{
	case IPC_READY_CMDQ:
		break;
	default:
		break;
	}
}
*/

void MMCU_DebugCmd(UINT32 u32nParam, CHAR *apParams[])
{
	UINT32      u32CmdType;
	UINT32      au32Params[10];
	UINT32      i;

	if (u32nParam>ARRAY_SIZE(au32Params))
	{
		logm_warning(mmcu_drv, "too many params %d\n", u32nParam);
		u32nParam=10;
	}

	for(i=0;i<u32nParam;i++)
		au32Params[i] = simple_strtoul(apParams[i], NULL, 0);

	if (!strcmp(apParams[0],"uart"))
		u32CmdType = 1;
	else
		u32CmdType = au32Params[0];

	logm_warning(mmcu_drv, "debug cmd : %d  nParam %d\n", u32CmdType, u32nParam);
	switch (u32CmdType)
	{
		case 0:
			break;
		case 1:
			{
				_MMCU_Uart0_Switch(au32Params[1]);
			}
			break;
		default:
			break;
	}

	return;
}
