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
#ifndef LOGFUNNEL


#if defined(__linux__)
#if !defined(CHIP_NAME_d13) && !defined(CHIP_NAME_d14)
#define LOGMAPPED
#endif
#endif


#ifdef LOGMAPPED
#include "logm.h"
#else
#include "log_kmsg.h"
#endif

#else
#include "../logfunnel/log.h"
#endif
