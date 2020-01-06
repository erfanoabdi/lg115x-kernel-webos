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


#ifndef __MODULE_CMD_DTSM6_H__
#define __MODULE_CMD_DTSM6_H__

#include "imc/adec_imc_cmd.h"

#define DTSM6_CMD_SET_PARAM         ADEC_CMD(0x2000, DtsM6CmdSetParam)

#define DTSM6_CMD_DEBUG_STATUS      ADEC_CMD_SIMP(0x2020)

#ifdef IC_CERTIFICATION
typedef struct _CertiParam0Field
{
    unsigned int spkrout                    :4;
    unsigned int DRCpercent                 :2;
    unsigned int AudioPresentIndex          :2;
    unsigned int activesecondaryassetmask   :8;
    unsigned int activeassetmask            :3;
    unsigned int replaceset_channel         :3;
    unsigned int replaceset_group           :2;
    unsigned int spdifoutput                :1;
    unsigned int multipleassetdecode        :1;
    unsigned int transencode                :1;
    unsigned int analogcomp                 :1;
    unsigned int nodecdefaultspkrremap      :1;
    unsigned int primarydeconly             :1;
    unsigned int decodeonlycore             :1;
    unsigned int nodialnorm                 :1;
} CertiParam0Field;

typedef struct _CertiParam1Field
{
    unsigned int lfemix_mode                :4;
    unsigned int multdmixoutput             :4;
    unsigned int exception_mode             :4;
    unsigned int reserved                   :20;
} CertiParam1Field;

typedef union _CertiParam0Flag
{
    unsigned int        value;
    CertiParam0Field    field;
} CertiParam0Flag;

typedef union _CertiParam1Flag
{
    unsigned int        value;
    CertiParam1Field    field;
} CertiParam1Flag;

#endif // #ifdef IC_CERTIFICATION

typedef struct _DtsM6CmdSetParam
{
    unsigned int isSubDec;              // Sub Decoding           : 0 = Main Dec(SPDIF ES Support), 1 = Sub Dec(SPDIF ES Not Support)
    unsigned int dts_type;              // 0 : DTS-CA, 1 : DTS-HD, 2 : DTS-LBR

#ifdef IC_CERTIFICATION
    CertiParam0Flag certi_param0;       // IC Certification parameter0
    CertiParam1Flag certi_param1;       // IC Certification parameter1
#endif // #ifdef IC_CERTIFICATION
} DtsM6CmdSetParam;

#endif // #ifndef __MODULE_CMD_DTSM6_H__

