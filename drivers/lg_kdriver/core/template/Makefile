#******************************************************************************#
# SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
# Copyright(c) 2012 by LG Electronics Inc.
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
#	AUTHOR		:				                                               #
#	DATE        :	                                                           #
#	DESCRIPTION	:	Makefile for building template module                      #
#******************************************************************************#
include $(KDRV_TOP_DIR)/modules.mk

template_MODULE			:= template

#-------------------------------------------------------------------------------
# TODO: define your driver source file
#-------------------------------------------------------------------------------
kdrv_template-objs		:= template_cfg.o
kdrv_template-objs		+= template_drv.o
kdrv_template-objs		+= template_proc.o
kdrv_template-objs		+=

#-------------------------------------------------------------------------------
# TODO: define your driver specific CFLAGS
#-------------------------------------------------------------------------------
kdrv_template-CFLAGS	+=
kdrv_template-CFLAGS	+=
kdrv_template-CFLAGS	+=

#-------------------------------------------------------------------------------
# DO NOT change the below part
#-------------------------------------------------------------------------------
EXTRA_CFLAGS		+= $(kdrv_template-CFLAGS)

ifeq ($(KDRV_GLOBAL_LINK), YES)
# KDRV_MODULE_OBJS is used for KDRV_GLOBAL_LINK mode
KDRV_MODULE_OBJS	+= $(addprefix $(template_MODULE)/,$(kdrv_template-objs)) 
else
obj-m				:= kdrv_template.o
endif
