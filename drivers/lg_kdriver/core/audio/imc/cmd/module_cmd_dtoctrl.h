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


#ifndef __MODULE_CMD_DTORATECTRL_H__
#define __MODULE_CMD_DTORATECTRL_H__

#include "imc/adec_imc_cmd.h"


#define DTOCTRL_CMD_SET_FS          ADEC_CMD(0x2003, DtoCtrlCmdSetFs)
#define DTOCTRL_CMD_SET_DTORATE     ADEC_CMD(0x2004, DtoCtrlCmdSetDtorate)
#define DTOCTRL_CMD_SET_INTINFO     ADEC_CMD(0x2005, DtoCtrlCmdSetIntInfo)
#define DTOCTRL_CMD_SET_INTFS       ADEC_CMD(0x2006, DtoCtrlCmdSetIntFs)
#define DTOCTRL_CMD_SET_INTNUM      ADEC_CMD(0x2007, DtoCtrlCmdSetIntNum)
#define DTOCTRL_CMD_SET_CLOCKTYPE	ADEC_CMD(0x2008, DtoCtrlCmdSetClockType)

#define DTOCTRL_CMD_PRINT_MODULE_STATE	ADEC_CMD_SIMP(0x2020)


typedef struct _DtoCtrlCmdSetFs
{
	unsigned int Fs;			// only ES
}DtoCtrlCmdSetFs;

typedef struct _DtoCtrlCmdSetDtorate
{
    unsigned int dtorate;
    unsigned int force_cnt_clear;
}DtoCtrlCmdSetDtorate;

typedef struct _DtoCtrlCmdSetIntFs
{
	unsigned int RefIntFs;
	unsigned int TarIntFs;
}DtoCtrlCmdSetIntFs;

typedef struct _DtoCtrlCmdSetIntNum
{
	unsigned int RefInt;
	unsigned int TarInt;
}DtoCtrlCmdSetIntNum;

typedef struct _DtoCtrlCmdSetIntInfo
{
	unsigned int RefInt;
	unsigned int RefIntFs;
	unsigned int TarInt;
	unsigned int TarIntFs;
}DtoCtrlCmdSetIntInfo;

typedef struct _DtoCtrlCmdSetClockType
{
	unsigned int clock;			//0 is DTO, 1 is AAD,  2 is just clock check
}DtoCtrlCmdSetClockType;

#endif // #ifndef __MODULE_CMD_DTORATECTRL_H__

