#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2010 Atheros Corporation.  All rights reserved.
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
# MII HIF makefile for Linux
#  

EXTRA_CFLAGS += -DHIF_MESSAGE_BASED
ATH_HIF_MESSAGE_BASED =yes

ifneq ($(ATH_OS_SUB_TYPE),linux_2_4)
ar6000-objs := ../../hif/mii/linux/hif_mii.o

endif

all:

clean:
