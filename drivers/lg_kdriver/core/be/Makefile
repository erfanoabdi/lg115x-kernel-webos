
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
#	AUTHOR		:	justine.jeong@lge.com
#	DATE		:	2012.02.13
#	DESCRIPTION	:	Makefile for building be module                      #
#******************************************************************************#
include $(KDRV_TOP_DIR)/modules.mk

be_MODULE			:= be

KDRV_CORE_TOP_DIR="$(PLATFORM_TOP)/os/$(PLATFORM_CHIP_FAMILY)_linux/drivers/kdriver/core"

#-------------------------------------------------------------------------------
# TODO: define your driver source file
#-------------------------------------------------------------------------------
kdrv_be-objs		:= be_drv.o
kdrv_be-objs		+= be_cfg.o


kdrv_be-objs		+= top/be_top_drv.o
kdrv_be-objs		+= top/be_top_hal.o
kdrv_be-objs		+= reg/be_reg.o
kdrv_be-objs		+= fwi/fwl_drv.o
kdrv_be-objs		+= fwi/fwl_hal.o
kdrv_be-objs		+= fwi/ipc_drv.o
kdrv_be-objs		+= fwi/ipc_hal.o
kdrv_be-objs		+= fwi/int_drv.o
kdrv_be-objs		+= fwi/int_hal.o
kdrv_be-objs		+= led/led_drv.o
kdrv_be-objs		+= led/led_hal.o
kdrv_be-objs		+= frc/frc_drv.o
kdrv_be-objs		+= frc/frc_hal.o

kdrv_be-objs		+= h3d/h3d_drv.o
kdrv_be-objs		+= h3d/h3d_hal.o
kdrv_be-objs		+= pwm_pe/pwm_pe_drv.o
kdrv_be-objs		+= pwm_pe/pwm_hal.o
kdrv_be-objs		+= sre/sre_drv.o
kdrv_be-objs		+= sre/sre_hal.o

kdrv_be-objs		+= fwi/mcu/mcu_ipc_test.o
kdrv_be-objs		+= video_status_keeper.o

ifeq ($(INCLUDE_H13_CHIP_KDRV), YES)
kdrv_be-objs		+= ../../chip/h13/be/top/be_top_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/frc/be_frc_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/led/be_led_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/sre/be_sre_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/h3d/be_h3d_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/fwi/be_fwl_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/fwi/be_ipc_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/fwi/be_int_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/pwm_pe/be_pwm_hw_h13.o
kdrv_be-objs		+= ../../chip/h13/be/reg/be_reg_hw_h13.o
endif
ifeq ($(INCLUDE_H14_CHIP_KDRV), YES)
kdrv_be-objs		+= ../../chip/h14/be/top/be_top_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/frc/be_frc_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/led/be_led_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/sre/be_sre_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/h3d/be_h3d_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/fwi/be_fwl_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/fwi/be_ipc_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/fwi/be_int_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/pwm_pe/be_pwm_hw_h14.o
kdrv_be-objs		+= ../../chip/h14/be/reg/be_reg_hw_h14.o
endif
ifeq ($(INCLUDE_M14_CHIP_KDRV), YES)
kdrv_be-objs		+= ../../chip/m14/be/top/be_top_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/frc/be_frc_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/led/be_led_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/sre/be_sre_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/h3d/be_h3d_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/fwi/be_fwl_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/fwi/be_ipc_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/fwi/be_int_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/pwm_pe/be_pwm_hw_m14.o
kdrv_be-objs		+= ../../chip/m14/be/reg/be_reg_hw_m14.o
endif

#------------------------------------------------------------------------------
# REVISION maker using MD5
# -----------------------------------------------------------------------------
BE_HEAD_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_DATE     :=  $(shell if [ -z "$(BE_HEAD_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_HEAD_REV)";fi)

BE_LED_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be/led | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_LED_DATE := $(shell if [ -z "$(BE_LED_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_LED_REV)";fi)

BE_H3D_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be/h3d | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_H3D_DATE      := $(shell if [ -z "$(BE_H3D_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_H3D_REV)";fi)

BE_PWM_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be/pwm_pe | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_PWM_DATE      := $(shell if [ -z "$(BE_PWM_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_PWM_REV)";fi)

BE_TOP_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be/top | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_TOP_DATE      := $(shell if [ -z "$(BE_TOP_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_TOP_REV)";fi)

BE_FWI_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be/fwi | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_FWI_DATE      := $(shell if [ -z "$(BE_FWI_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_FWI_REV)";fi)

BE_REG_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/be/reg | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_REG_DATE      := $(shell if [ -z "$(BE_REG_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_REG_REV)";fi)

BE_CHIP_H13_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h13/be | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_CHIP_H13_DATE      := $(shell if [ -z "$(BE_CHIP_H13_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_CHIP_H13_REV)";fi)

BE_CHIP_H14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h14/be | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_CHIP_H14_DATE      := $(shell if [ -z "$(BE_CHIP_H14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_CHIP_H14_REV)";fi)

BE_CHIP_M14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/m14/be | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
BE_CHIP_M14_DATE      := $(shell if [ -z "$(BE_CHIP_M14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(BE_CHIP_M14_REV)";fi)


DE_HEAD_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/de | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
DE_DATE      := $(shell if [ -z "$(DE_HEAD_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(DE_HEAD_REV)";fi)

DE_CHIP_H13_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h13/de | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
DE_CHIP_H13_DATE      := $(shell if [ -z "$(DE_CHIP_H13_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(DE_CHIP_H13_REV)";fi)

DE_CHIP_H14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h14/de | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
DE_CHIP_H14_DATE      := $(shell if [ -z "$(DE_CHIP_H14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(DE_CHIP_H14_REV)";fi)

DE_CHIP_M14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/m14/de | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
DE_CHIP_M14_DATE      := $(shell if [ -z "$(DE_CHIP_M14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(DE_CHIP_M14_REV)";fi)

AFE_HEAD_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/afe | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
AFE_DATE      := $(shell if [ -z "$(AFE_HEAD_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(AFE_HEAD_REV)";fi)

AFE_CHIP_H13_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h13/afe | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
AFE_CHIP_H13_DATE      := $(shell if [ -z "$(AFE_CHIP_H13_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(AFE_CHIP_H13_REV)";fi)

AFE_CHIP_H14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/h14/afe | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
AFE_CHIP_H14_DATE      := $(shell if [ -z "$(AFE_CHIP_H14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(AFE_CHIP_H14_REV)";fi)

AFE_CHIP_M14_REV = $(shell LC_ALL=C cd $(KDRV_CORE_TOP_DIR) && git log -1 $(KDRV_CORE_TOP_DIR)/../chip/m14/afe | awk /"Date"/'{print $$2 " " $$3 " " $$4 " " $$5 " " $$6}')
AFE_CHIP_M14_DATE      := $(shell if [ -z "$(AFE_CHIP_M14_REV)" ]; then echo "COULD NOT GET GIT LOG"; else echo "$(AFE_CHIP_M14_REV)";fi)



#-------------------------------------------------------------------------------
# TODO: define your driver specific CFLAGS
#-------------------------------------------------------------------------------
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/top
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/reg
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/fwi
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/h3d
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/frc
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/sre
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/led
kdrv_be-CFLAGS   += -I$(KDRV_TOP_DIR)/be/pwm_pe
kdrv_be-CFLAGS	 += -DBE_DRV_VER="TO_STR($(BE_DATE))"

kdrv_be-CFLAGS	 += -DBE_LED_VER="TO_STR($(BE_LED_DATE))"
kdrv_be-CFLAGS	 += -DBE_H3D_VER="TO_STR($(BE_H3D_DATE))"
kdrv_be-CFLAGS	 += -DBE_PWM_VER="TO_STR($(BE_PWM_DATE))"
kdrv_be-CFLAGS	 += -DBE_TOP_VER="TO_STR($(BE_TOP_DATE))"
kdrv_be-CFLAGS	 += -DBE_FWI_VER="TO_STR($(BE_FWI_DATE))"
kdrv_be-CFLAGS	 += -DBE_REG_VER="TO_STR($(BE_REG_DATE))"

kdrv_be-CFLAGS	 += -DBE_CHIP_H13_VER="TO_STR($(BE_CHIP_H13_DATE))"
kdrv_be-CFLAGS	 += -DBE_CHIP_H14_VER="TO_STR($(BE_CHIP_H14_DATE))"
kdrv_be-CFLAGS	 += -DBE_CHIP_M14_VER="TO_STR($(BE_CHIP_M14_DATE))"

kdrv_be-CFLAGS	 += -DDE_DRV_VER="TO_STR($(DE_DATE))"
kdrv_be-CFLAGS	 += -DDE_CHIP_H13_VER="TO_STR($(DE_CHIP_H13_DATE))"
kdrv_be-CFLAGS	 += -DDE_CHIP_H14_VER="TO_STR($(DE_CHIP_H14_DATE))"
kdrv_be-CFLAGS	 += -DDE_CHIP_M14_VER="TO_STR($(DE_CHIP_M14_DATE))"

kdrv_be-CFLAGS	 += -DAFE_DRV_VER="TO_STR($(AFE_DATE))"
kdrv_be-CFLAGS	 += -DAFE_CHIP_H13_VER="TO_STR($(AFE_CHIP_H13_DATE))"
kdrv_be-CFLAGS	 += -DAFE_CHIP_H14_VER="TO_STR($(AFE_CHIP_H14_DATE))"
kdrv_be-CFLAGS	 += -DAFE_CHIP_M14_VER="TO_STR($(AFE_CHIP_M14_DATE))"


#-------------------------------------------------------------------------------
# DO NOT change the below part
#-------------------------------------------------------------------------------
EXTRA_CFLAGS		+= $(kdrv_be-CFLAGS)

ifeq ($(KDRV_GLOBAL_LINK), YES)
# KDRV_MODULE_OBJS is used for KDRV_GLOBAL_LINK mode
KDRV_MODULE_OBJS	+= $(addprefix $(be_MODULE)/,$(kdrv_be-objs))
else
obj-m				:= kdrv_be.o
endif

