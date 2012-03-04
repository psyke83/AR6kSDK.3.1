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
#
# link or copy whole olca driver into external/athwlan/olca/
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BT_ALT_STACK),true)
# Define AR6K_PREBUILT_HCIUTILS_LIB:= true if you want to force to enable HCIUTILS
# Otherwise, we will try to load the hciutil.so dynamically to determinate whether we use hciutils or hci socket
# Uncomment the following if you want to use static library

#AR6K_PREBUILT_HCIUTILS_LIB := true

endif

include $(CLEAR_VARS)

LOCAL_SRC_FILES := abtfilt_bluez_dbus.c \
		abtfilt_core.c \
		abtfilt_main.c \
		abtfilt_utils.c \
		abtfilt_wlan.c \
		btfilter_action.c \
		btfilter_core.c

ifeq ($(BT_ALT_STACK),true)
LOCAL_SRC_FILES += abtfilt_bluez_hciutils.c

ifeq ($(AR6K_PREBUILT_HCIUTILS_LIB),true)
LOCAL_STATIC_LIBRARIES := hciutils
LOCAL_CFLAGS += -DSTATIC_LINK_HCILIBS
endif
AR6K_PREBUILT_HCIUTILS_LIB :=

else
LOCAL_CFLAGS += -DCONFIG_NO_HCILIBS
endif

LOCAL_SHARED_LIBRARIES := \
	 	libdbus \
		libbluetooth \
		libcutils \
		libdl

ifeq ($(AR6K_PREBUILT_HCIUTILS_LIB),true)
LOCAL_STATIC_LIBRARIES := hciutils
LOCAL_CFLAGS += -DSTATIC_LINK_HCILIBS
endif
AR6K_PREBUILT_HCIUTILS_LIB :=

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../../../include \
	$(LOCAL_PATH)/../../../tools/athbtfilter/bluez \
	$(LOCAL_PATH)/../../../../include \
	$(LOCAL_PATH)/../../../os/linux/include \
        $(LOCAL_PATH)/../../../btfilter \
        $(LOCAL_PATH)/../../.. \
	$(call include-path-for, dbus) \
	$(call include-path-for, bluez-libs)

ifneq ($(PLATFORM_VERSION),$(filter $(PLATFORM_VERSION),1.5 1.6))
LOCAL_C_INCLUDES += external/bluetooth/bluez/include/bluetooth external/bluetooth/bluez/lib/bluetooth 
LOCAL_CFLAGS+=-DBLUEZ4_3
else
LOCAL_C_INCLUDES += external/bluez/libs/include/bluetooth
endif

LOCAL_CFLAGS+= \
	-DDBUS_COMPILATION -DABF_DEBUG


LOCAL_MODULE := abtfilt
LOCAL_MODULE_TAGS := debug eng user optional
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)
