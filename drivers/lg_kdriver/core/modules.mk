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
#	FILE NAME	:	modules.mk                                                 #
#	VERSION		:	1.0                                                        #
#	AUTHOR		:	raxis@lge.com                                              #
#	DATE        :	2009/11/10                                                 #
#	DESCRIPTION	:	Makefile for building LG1150 kernel driver                 #
#******************************************************************************#
# MODULE_MK_INCLUDED variable prevents the recursive inclusion of module.mk
# if module.mk is included MODULE_MK_INCLUDED is set to YES.
# so if another inclusion of module.mk is safely ignored.
#
ifeq ($(MODULE_MK_INCLUDED),)

ifeq ($(KDRV_PLATFORM_CFG),)
$(error KDRV_PLATFORM_CFG is not defined, please define this variable above )
endif

include $(KDRV_TOP_DIR)/incs.mk
include $(KDRV_TOP_DIR)/../platform/$(KDRV_PLATFORM_CFG)/platform_dev.mk

#------------------------------------------------------------------------------
# define module directory
#
# TODO: add your module directory
#------------------------------------------------------------------------------
BASE_MODULE		:=
BASE_MODULE		+= base
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_SAMPLE,sample,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_AAD, 	aad,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_AUDIO, audio,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_AFE, 	afe,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_CI, 	ci,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_DE, 	de,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_DEMOD, demod,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_DENC, 	denc,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_FBDEV, fbdev,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_GFX, 	gfx,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_GPU, 	gpu,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_GPIO, 	gpio,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_HDMI, 	hdmi,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_I2C, 	i2c,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_SPI, 	spi,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_PE, 	pe,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_PVR, 	pvr,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_SDEC, 	sdec,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_VBI, 	vbi,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_MMCU, 	mmcu,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_VENC, 	venc,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_PNG, 	png,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_SE, 	se,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_SYS, 	sys,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_SCI, 	sci,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_CLK, 	clk,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_MONITOR,monitor,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_MJPEG, mjpeg,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_APR,	apr,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_MEMPROT, memprot,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_LOGM, 	logm,)

BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_BE, 	be,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_LGBUS,	lgbus,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_OVI,	ovi,)
BASE_MODULE		+= $(call cond_assign, YES, INCLUDE_KDRV_PM,	pm,)

#------------------------------------------------------------------------------
# Define default CFLAGS for kernel driver
#
# TODO : add common CFLAGS
#------------------------------------------------------------------------------
EXTRA_CFLAGS	+= -D__ARM__
#EXTRA_CFLAGS	+= -mfpu=neon -mfloat-abi=softfp	# arm VFPv3

# 20100709 seokjoo.lee for OS_DEBUG_Print format check.
EXTRA_CFLAGS	+= -Wformat

# include platform definition file(s)
EXTRA_CFLAGS	+= -include $(KDRV_TOP_DIR)/../platform/$(KDRV_PLATFORM_CFG)/platform_config.h
EXTRA_CFLAGS	+= -include $(KDRV_TOP_DIR)/../platform/$(KDRV_PLATFORM_CFG)/platform_mem_map.h

# include kdriver main header file(s) and HAL file(s)
EXTRA_CFLAGS	+= -I$(KDRV_TOP_DIR)/../include
EXTRA_CFLAGS	+= -I$(KDRV_TOP_DIR)/../chip
EXTRA_CFLAGS	+= -I$(KDRV_TOP_DIR)/../platform

EXTRA_CFLAGS	+= $(call cond_assign, y, DEBUG, -DKDRV_DEBUG_BUILD,)
EXTRA_CFLAGS	+= -DKDRV_TOP_DIR=\"$(KDRV_TOP_DIR)\"

EXTRA_CFLAGS	+= -I$(KDRV_TOP_DIR)/hma
EXTRA_CFLAGS	+= -I$(KDRV_TOP_DIR)/mmcu
EXTRA_CFLAGS	+= $(addprefix -I$(KDRV_TOP_DIR)/,$(BASE_MODULE))

ifeq ($(PLATFORM_CHIP_TYPE),FPGA)
EXTRA_CFLAGS	+= -DPLATFORM_FPGA
endif

EXTRA_CFLAGS	+= $(call cond_define, YES, KDRV_GLOBAL_LINK)
EXTRA_CFLAGS	+= $(call cond_define, YES, KDRV_CONFIG_PM)
EXTRA_CFLAGS	+= $(call cond_define, YES, KDRV_MODULE_BUILD)

EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_SAMPLE)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_SYS)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_AAD)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_AUDIO)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_AFE)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_CI)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_DE)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_DEMOD)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_DENC)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_FBDEV)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_GFX)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_GPIO)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_HDMI)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_I2C)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_SPI)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_PE)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_PVR)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_SDEC)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_VBI)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_MMCU)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_VDEC)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_VENC)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_PNG)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_SE)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_SCI)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_CLK)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_MONITOR)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_MJPEG)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_BE)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_APR)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_OVI)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_LOGM)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_PM)

EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_LGBUS)

EXTRA_CFLAGS    += $(call cond_define, YES, INCLUDE_KDRV_GPU)
EXTRA_CFLAGS    += $(call cond_define, YES, INCLUDE_GPU_MALI)

EXTRA_CFLAGS	+= $(call cond_define, YES, USE_VIDEO_UART2_FOR_MCU)
#USE_VIDEO_UART2_FOR_MCU = YES
#EXTRA_CFLAGS	+= -DUSE_VIDEO_UART2_FOR_MCU

EXTRA_CFLAGS	+= -DUSE_VIDEO_IOCTL_CALLING

EXTRA_CFLAGS	+= -DKDRV_PLATFORM=$(call cond_assign, y, CONFIG_MACH_L8_ANDROID, KDRV_COSMO_PLATFORM, KDRV_GP_PLATFORM)
EXTRA_CFLAGS	+= -DKDRV_GP_PLATFORM=0x1000 -DKDRV_COSMO_PLATFORM=0x1001
EXTRA_CFLAGS	+= $(call cond_define, YES, SIC_LAB_BOARD)

EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_L8_CHIP_KDRV)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_L9_CHIP_KDRV)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_H13_CHIP_KDRV)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_M14_CHIP_KDRV)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_H14_CHIP_KDRV)
EXTRA_CFLAGS	+= $(call cond_define, YES, INCLUDE_KDRV_VER_FPGA)

#
# Below lines are orignally defined in GP2.
#
#EXTRA_CFLAGS	+= -DCHIP_LG1150=3008
#EXTRA_CFLAGS	+= -DMAIN_CHIP_TYPE=CHIP_LG1150
# [L8] minjun.kim 2010-09-30 �Ʒ� ���� �������� ���ÿ�
KDRV_OUT_DIR        := $(OUT_DIR)/res/lglib/kdrv
CHANGE_FILE_LIST    := $(OUT_DIR)/changed_file_list.txt
CHANGE_FILE_FLAG    := $(OUT_DIR)/changed_file_flag.txt

# Add platform dependent Makefile
# -include $(KDRV_TOP_DIR)/base/platform/$(KDRV_PLATFORM_CFG)/platform_module.mk

#
# See the comment in ifeq($(MODULE_MK_INCLUDED,) part
#
MODULE_MK_INCLUDED = YES


endif

