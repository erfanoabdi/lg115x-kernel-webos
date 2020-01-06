
# SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
# Copyright(c) 2013 by LG Electronics Inc.

# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
# GNU General Public License for more details.
 

#   ---------------------------------------------------------------------------#
#                                                                              #
#	FILE NAME	:	makefile                                                   #
#	VERSION		:	1.0                                                        #
#	AUTHOR		:	dj911.kim@lge.com
#	DATE        :	2012.09.09
#	DESCRIPTION	:	Makefile for building ovi module                           #
#******************************************************************************#
include $(KDRV_TOP_DIR)/modules.mk

ovi_MODULE			:= ovi

KDRV_CORE_TOP_DIR="$(PLATFORM_TOP)/os/$(PLATFORM_CHIP_FAMILY)_linux/drivers/kdriver/core"

#-------------------------------------------------------------------------------
# TODO: define your driver source file
#-------------------------------------------------------------------------------
kdrv_ovi-objs		+= ovi_drv.o
kdrv_ovi-objs		+= ovi_cfg.o
kdrv_ovi-objs		+= ovi_hal.o
kdrv_ovi-objs		+= ovi_proc.o
ifeq ($(INCLUDE_H13_CHIP_KDRV), YES)
kdrv_ovi-objs		+= ../../chip/h13/ovi/ovi_hw_h13.o
endif
ifeq ($(INCLUDE_H14_CHIP_KDRV), YES)
kdrv_ovi-objs		+= ../../chip/h14/ovi/ovi_hw_h14.o
endif
ifeq ($(INCLUDE_M14_CHIP_KDRV), YES)
kdrv_ovi-objs		+= ../../chip/m14/ovi/ovi_hw_m14.o
endif

#------------------------------------------------------------------------------
# REVISION maker using MD5
# -----------------------------------------------------------------------------
OVI_HEAD_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/ovi | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
OVI_DATE      := $(shell if [ -z "$(OVI_HEAD_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(OVI_HEAD_REV)";fi)

OVI_CHIP_H13_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h13/ovi | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
OVI_CHIP_H13_DATE      := $(shell if [ -z "$(OVI_CHIP_H13_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(OVI_CHIP_H13_REV)";fi)

OVI_CHIP_H14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h14/ovi | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
OVI_CHIP_H14_DATE      := $(shell if [ -z "$(OVI_CHIP_H14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(OVI_CHIP_H14_REV)";fi)

OVI_CHIP_M14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/m14/ovi | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
OVI_CHIP_M14_DATE      := $(shell if [ -z "$(OVI_CHIP_M14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(OVI_CHIP_M14_REV)";fi)



#-------------------------------------------------------------------------------
# TODO: define your driver specific CFLAGS
#-------------------------------------------------------------------------------
kdrv_ovi-CFLAGS   += -I$(KDRV_TOP_DIR)/ovi
ifeq ($(INCLUDE_H13_CHIP_KDRV), YES)
kdrv_ovi-CFLAGS   += -I$(KDRV_TOP_DIR)/../chip/h13/ovi
endif
ifeq ($(INCLUDE_H14_CHIP_KDRV), YES)
kdrv_ovi-CFLAGS   += -I$(KDRV_TOP_DIR)/../chip/h14/ovi
endif
ifeq ($(INCLUDE_M14_CHIP_KDRV), YES)
kdrv_ovi-CFLAGS   += -I$(KDRV_TOP_DIR)/../chip/m14/ovi
endif

kdrv_ovi-CFLAGS	 += -DOVI_DRV_VER="TO_STR($(OVI_DATE))"
kdrv_ovi-CFLAGS	 += -DOVI_CHIP_H13_VER="TO_STR($(OVI_CHIP_H13_DATE))"
kdrv_ovi-CFLAGS	 += -DOVI_CHIP_H14_VER="TO_STR($(OVI_CHIP_H14_DATE))"
kdrv_ovi-CFLAGS	 += -DOVI_CHIP_M14_VER="TO_STR($(OVI_CHIP_M14_DATE))"



#-------------------------------------------------------------------------------
# DO NOT change the below part
#-------------------------------------------------------------------------------
EXTRA_CFLAGS		+= $(kdrv_ovi-CFLAGS)

ifeq ($(KDRV_GLOBAL_LINK), YES)
# KDRV_MODULE_OBJS is used for KDRV_GLOBAL_LINK mode
KDRV_MODULE_OBJS	+= $(addprefix $(ovi_MODULE)/,$(kdrv_ovi-objs))
else
obj-m				:= kdrv_ovi.o
endif
