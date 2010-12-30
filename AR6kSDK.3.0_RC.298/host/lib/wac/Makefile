#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2010 Atheros Corporation.  All rights reserved.
#
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
#
# </copyright>
# 
# <summary>
# 	WAC (Wireless Auto Configuration)
# </summary>
#
#------------------------------------------------------------------------------
#==============================================================================
# Author(s): ="Atheros"
#==============================================================================
CC := $(ATH_CROSS_COMPILE_TYPE)gcc
AR = $(ATH_CROSS_COMPILE_TYPE)ar
RANLIB = $(ATH_CROSS_COMPILE_TYPE)ranlib

KERNEL_SRC :=$(ATH_LINUXPATH)

BUILD_STATIC = y

# BUILD_STRIPPING = y

ifndef CFLAGS
CFLAGS = -Os -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow \
	-Wpointer-arith -Wcast-qual
endif

DEPFLAGS=-MMD

CFLAGS += -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_CTRL_IFACE -DWAPI_ENABLE -DWAC_ENABLE

#WPA_SUPPLICANT_IN_USE_PATH=opensrc_0_6_9

CFLAGS += -I./ -I$(KERNEL_SRC)/include -I../../include -I../../../include -I../../wlan/include \
	    -I../../os/linux/include -I../../3rdparty/supplicant/$(WPA_SUPPLICANT_IN_USE_PATH)/src/common \
	    -I../../3rdparty/supplicant/$(WPA_SUPPLICANT_IN_USE_PATH)/src/utils

STATIC=libwac.a
DYNAMIC=libwac.so

PICFLAG=-fPIC

OBJS_LIB = wac_lib.o wac_utils.o

ifdef BUILD_STATIC
 WACLIB=$(STATIC)
else
 WACLIB=$(DYNAMIC)
endif

ifdef BUILD_STRIPPING
  STRIPFLAGS= -Wl,-s
else
  STRIPFLAGS=
endif


RM = rm -f

LIBS = -lpthread

XCFLAGS=$(CFLAGS) $(DEPFLAGS)

all: $(WACLIB) wac_app

%: %.o
	$(CC) $(LDFLAGS) $(STRIPFLAGS) $(XCFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(XCFLAGS) -c $<

%.so: %.c
	$(CC) $(XCFLAGS) $(PICFLAG) -c -o $@ $<

wac_app: wac_app.o $(WACLIB)
	$(CC) $(LDFLAGS) $(STRIPFLAGS) $(XCFLAGS) -o $@ $^ $(LIBS) $(WACLIB)

$(DYNAMIC): $(OBJS_LIB:.o=.so)
	$(CC) -shared -o $@ -Wl,-soname,$@ $(STRIPFLAGS) $(LIBS) -lc $^

$(STATIC): $(OBJS_LIB:.o=.so)
	$(RM) $@
	$(AR) cru $@ $^
#	$(RANLIB) $@
	
clean:
	$(RM) wac_app
	$(RM) *.o *.d *.so *.a
