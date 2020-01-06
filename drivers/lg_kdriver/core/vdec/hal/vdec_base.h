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

#if defined (CHIP_NAME_h13vdec)
#include "h13b0vdec/vdec_base.h"
#elif defined (CHIP_NAME_d13)
#include "d13/vdec_base.h"
#elif defined (CHIP_NAME_d14)
#include "d14/vdec_base.h"
#elif defined (CHIP_NAME_h13) || defined (CHIP_NAME_h14)
#include "h13/vdec_base.h"
#elif defined (CHIP_NAME_m14)
#include "m14/vdec_base.h"
#elif defined (CHIP_NAME_h15)
#include "h15/vdec_base.h"
#else
#warning "unknown chip name..."
#endif

