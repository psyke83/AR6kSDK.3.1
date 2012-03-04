#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2005-2007 Atheros Corporation.  All rights reserved.
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
CC := $(ATH_CROSS_COMPILE_TYPE)gcc
KERNEL_SRC :=$(ATH_LINUXPATH)

ifndef CFLAGS
#CFLAGS = -MMD -O2 -Wall -g
CFLAGS = -Wall
endif

CFLAGS += -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_CTRL_IFACE -DWAPI_ENABLE

all:
	$(CC) $(CFLAGS) -I./ -I$(KERNEL_SRC)/include -I../../include -I../../../include -I../../wlan/include \
					-I../../os/linux/include -I../../3rdparty/supplicant/opensrc_0_6_9/src/common \
					-I../../os/linux/include -I../../3rdparty/supplicant/opensrc_0_6_9/src/utils wac.c -o wac

#wpa_ctrl.o: os_internal.o
#	$(CC) $(CFLAGS) -I./ -I$(KERNEL_SRC)/include -I../../include -I../../../include -I../../wlan/include \
#					-I../../os/linux/include -I../../3rdparty/supplicant/opensrc_0_6_9/src/utils \
#					-c ../../3rdparty/supplicant/opensrc_0_6_9/src/common/wpa_ctrl.c
#
#os_internal.o:
#	$(CC) $(CFLAGS) -I./ -I$(KERNEL_SRC)/include -I../../include -I../../../include -I../../wlan/include \
#					-I../../os/linux/include -I../../3rdparty/supplicant/opensrc_0_6_9/src/utils \
#					-c ../../3rdparty/supplicant/opensrc_0_6_9/src/utils/os_internal.c
