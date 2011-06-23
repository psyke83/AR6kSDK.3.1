#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2005-2010 Atheros Corporation.  All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
#
#------------------------------------------------------------------------------
#==============================================================================
# Author(s): ="Atheros"
#==============================================================================

ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ATH_ANDROID_SRC_BASE:= $(BOARD_WLAN_ATHEROS_SDK)

# WOW GPIO
ifneq ($(BOARD_WLAN_ATHEROS_WOW_GPIO),)
board_wow_gpio = $(BOARD_WLAN_ATHEROS_WOW_GPIO)
else
board_wow_gpio = 0
endif

export  ATH_BUILD_TYPE=ANDROID_ARM_NATIVEMMC
export  ATH_BUS_TYPE=sdio
export  ATH_OS_SUB_TYPE=linux_2_6

ATH_ANDROID_ROOT:= $(CURDIR)
export ATH_SRC_BASE:=$(ATH_ANDROID_ROOT)/$(BOARD_WLAN_ATHEROS_SDK)/host
ATH_CROSS_COMPILE_TYPE:=$(ATH_ANDROID_ROOT)/$(TARGET_TOOLS_PREFIX)
#ATH_CROSS_COMPILE_TYPE:=$(ATH_ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-4.3.1/bin/arm-eabi-
ATH_TARGET_OUTPUT:=$(ATH_ANDROID_ROOT)

ifeq ($(TARGET_PRODUCT),$(filter $(TARGET_PRODUCT),qsd8250_surf qsd8250_ffa msm7627_surf msm7627_ffa msm7625_ffa msm7625_surf msm7630_surf GT-I5500 ))
export  ATH_LINUXPATH=$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
ATH_CROSS_COMPILE_TYPE:=$(ATH_ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-4.3.1/bin/arm-eabi-
endif
export ATH_LINUXPATH := $(ATH_ANDROID_ROOT)/kernel_imx
ifndef ATH_LINUXPATH
#check for Nvidia-base platform
ifeq ($(TARGET_PRODUCT),$(filter $(TARGET_PRODUCT),harmony ventana))
export ATH_LINUXPATH=$(ATH_ANDROID_ROOT)/$(TARGET_OUT_INTERMEDIATES)/KERNEL
ATH_CROSS_COMPILE_TYPE:=$(ATH_ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
endif
endif

ifndef ATH_LINUXPATH
# Comment in the following variable for your platform and comment out $(error)
# Link your kernel into android SDK directory as 'kernel' directory
# export  ATH_LINUXPATH= [Your android/kernel path ]
$(error define your kernel path here for ATH_LINUXPATH $(TARGET_PRODUCT))

endif 

export  ATH_ARCH_CPU_TYPE=arm
export  ATH_BUS_SUBTYPE=linux_sdio
export  ATH_ANDROID_ENV=yes
export  ATH_SOFTMAC_FILE_USED=no
export  ATH_CFG80211_ENV=no
export  ATH_DEBUG_DRIVER=yes
export  ATH_HTC_RAW_INT_ENV=yes
export  ATH_AR6K_OTA_TEST_MODE=no

ATH_HIF_TYPE:=sdio

ifneq ($(PLATFORM_VERSION),$(filter $(PLATFORM_VERSION),1.5 1.6))

ifeq ($(TARGET_PRODUCT),$(filter $(TARGET_PRODUCT),msm7627_surf msm7627_ffa GT-I5500))
ATH_ANDROID_BUILD_FLAGS=-D__LINUX_ARM_ARCH__=6 -march=armv6 
#-fstack-check="generic"
endif

ifeq ($(TARGET_PRODUCT),$(filter $(TARGET_PRODUCT),qsd8250_surf qsd8250_ffa msm7630_surf smdkc100))
ATH_ANDROID_BUILD_FLAGS=-D__LINUX_ARM_ARCH__=7 -march=armv7-a 
#-fstack-check="generic"
endif

endif  # ECLAIR

ifeq ($(TARGET_PRODUCT),$(filter $(TARGET_PRODUCT),smdk6410))
ATH_ANDROID_BUILD_FLAGS += -DATH6KL_CONFIG_HIF_VIRTUAL_SCATTER 
endif 

#Uncomment the following define in order to enable OTA mode
#ATH_ANDROID_BUILD_FLAGS += -DATH6K_CONFIG_OTA_MODE

export ATH_ANDROID_BUILD_FLAGS

mod_cleanup := $(ATH_TARGET_OUTPUT)/$(ATH_ANDROID_SRC_BASE)/dummy

$(mod_cleanup) :
	rm -f `find $(ATH_TARGET_OUTPUT)/$(ATH_ANDROID_SRC_BASE) -name "*.o"`
	mkdir -p $(TARGET_OUT)/wifi/ath6k/AR6003/hw2.0/

mod_file := $(TARGET_OUT)/wifi/ar6000.ko
$(mod_file) : $(mod_cleanup) $(TARGET_PREBUILT_KERNEL) $(ACP)
	$(MAKE) ARCH=arm CROSS_COMPILE=$(ATH_CROSS_COMPILE_TYPE) -C $(ATH_LINUXPATH) ATH_HIF_TYPE=$(ATH_HIF_TYPE) PLAT_WOW_GPIO_PIN=$(board_wow_gpio) SUBDIRS=$(ATH_SRC_BASE)/os/linux modules
	$(ACP) $(ATH_TARGET_OUTPUT)/$(ATH_ANDROID_SRC_BASE)/host/os/linux/ar6000.ko $(TARGET_OUT)/wifi/

ALL_PREBUILT += $(mod_file)

include $(LOCAL_PATH)/tools/Android.mk

endif
