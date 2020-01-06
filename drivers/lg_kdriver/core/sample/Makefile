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
#                                                                              #
#	FILE NAME	:	makefile                                                   #
#	VERSION		:	1.0                                                        #
#	AUTHOR		:	root
#	DATE        :	2009.12.07
#	DESCRIPTION	:	Makefile for building sample module                      #
#******************************************************************************#
include $(KDRV_TOP_DIR)/modules.mk

sample_MODULE			:= sample

#-------------------------------------------------------------------------------
# TODO: define your driver source file
#-------------------------------------------------------------------------------
kdrv_sample-objs		:= sample_cfg.o
kdrv_sample-objs		+= sample_drv.o
kdrv_sample-objs		+= sample_proc.o
kdrv_sample-objs		+=

#-------------------------------------------------------------------------------
# TODO: define your driver specific CFLAGS
#-------------------------------------------------------------------------------
kdrv_sample-CFLAGS	+=
kdrv_sample-CFLAGS	+=
kdrv_sample-CFLAGS	+=

#-------------------------------------------------------------------------------
# DO NOT change the below part
#-------------------------------------------------------------------------------
EXTRA_CFLAGS		+= $(kdrv_sample-CFLAGS)

ifeq ($(KDRV_GLOBAL_LINK), YES)
# KDRV_MODULE_OBJS is used for KDRV_GLOBAL_LINK mode
KDRV_MODULE_OBJS	+= $(addprefix $(sample_MODULE)/,$(kdrv_sample-objs)) 
else
obj-m				:= kdrv_sample.o
endif
