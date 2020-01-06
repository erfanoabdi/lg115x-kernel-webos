#******************************************************************************#
# SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
# Copyright(c) 2009 by LG Electronics Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#------------------------------------------------------------------------------#
#																			   #
#	FILE NAME	:	makefile												   #
#	VERSION		:	1.0														   #
#	AUTHOR		:	dongho7.park (dongho7.park@lge.com)
#	DATE		:	2009.12.30
#	DESCRIPTION :	Makefile for building mmcu module						 #
#******************************************************************************#
include $(KDRV_TOP_DIR)/modules.mk

mmcu_MODULE			:= mmcu

#-------------------------------------------------------------------------------
# TODO: define your driver source file
#-------------------------------------------------------------------------------
kdrv_mmcu-objs		:= mmcu_drv.o
kdrv_mmcu-objs		+= mmcu_hal.o
kdrv_mmcu-objs		+= mmcu_pipe.o
kdrv_mmcu-objs		+= mmcu_lock.o
kdrv_mmcu-objs		+= mmcu_proc.o
#kdrv_mmcu-objs		+= 
ifeq ($(INCLUDE_H13_CHIP_KDRV), YES)
endif

#-------------------------------------------------------------------------------
# TODO: define your driver specific CFLAGS
#-------------------------------------------------------------------------------
kdrv_mmcu-CFLAGS	+= -I$(KDRV_TOP_DIR)/logfunnel
kdrv_mmcu-CFLAGS	+= -I$(KDRV_TOP_DIR)/vdec
kdrv_mmcu-CFLAGS	+=

#-------------------------------------------------------------------------------
# DO NOT change the below part
#-------------------------------------------------------------------------------
EXTRA_CFLAGS		+= $(kdrv_mmcu-CFLAGS)

ifeq ($(KDRV_GLOBAL_LINK), YES)
# KDRV_MODULE_OBJS is used for KDRV_GLOBAL_LINK mode
KDRV_MODULE_OBJS	+= $(addprefix $(mmcu_MODULE)/,$(kdrv_mmcu-objs))
else
obj-m				:= kdrv_mmcu.o
endif
