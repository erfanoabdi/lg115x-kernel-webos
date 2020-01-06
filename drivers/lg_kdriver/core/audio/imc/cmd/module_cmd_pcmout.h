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

#ifndef __MODULE_CMD_PCMOUT_H__
#define __MODULE_CMD_PCMOUT_H__

#include "imc/adec_imc_cmd.h"


#define PCMOUT_CMD_SET_SPKCH	            ADEC_CMD(0x2000, PcmoutCmdSetSpkch)
#define PCMOUT_CMD_SET_TEST_DBG				ADEC_CMD(0x2001, PcmoutCmdSetTestDbg)
#define PCMOUT_CMD_GET_MASTERSYSTEMDELAY	ADEC_CMD(0x2002, PcmoutCmdGetMasterSD)

#define PCMOUT_CMD_PRINT_MODULE_STATE        ADEC_CMD_SIMP(0x2020)


typedef struct _PcmoutCmdSetSpkch
{
	unsigned int channel;			// 2, 4
}PcmoutCmdSetSpkch;

typedef struct _PcmoutCmdSetTestDbg
{
	unsigned int index;			// 0 : original, 1: 200ms tic 2: pcmout print 
}PcmoutCmdSetTestDbg;

typedef struct _PcmoutCmdGetMasterSD
{    
    unsigned int module_id;      // The module ID
}PcmoutCmdGetMasterSD;

#endif // #ifndef __MODULE_CMD_SPDIF_H__

