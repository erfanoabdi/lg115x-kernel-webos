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

#ifndef __MODULE_CMD_BUF_MONITOR_H__
#define __MODULE_CMD_BUF_MONITOR_H__

#include "imc/adec_imc_cmd.h"

#define DBG_CMD_BUF_REPORT			ADEC_CMD_SIMP(0xD00B)
#define DBG_CMD_SYS_DELAY_REPORT	ADEC_CMD_SIMP(0xD00D)
#define DBG_CMD_PROF_REPORT			ADEC_CMD_SIMP(0xD00F)
#define DBG_CMD_ALLOC_SIZE          ADEC_CMD_SIMP(0xD0A0)
#define DBG_CMD_ALLOC_LIST          ADEC_CMD_SIMP(0xD0A1)
#define DBG_CMD_CHECK_MEM           ADEC_CMD_SIMP(0xD0C0)


#endif
