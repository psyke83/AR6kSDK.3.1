#------------------------------------------------------------------------------
# <copyright file="makefile" company="Atheros">
#    Copyright (c) 2005-2010 Atheros Corporation.  All rights reserved.
# 
# $ATH_LICENSE_HOSTSDK0_C$
#------------------------------------------------------------------------------
#==============================================================================
# Author(s): ="Atheros"
#==============================================================================

MAKE := make
WORKAREA = $(PKGDIR_TLA)

# Some people may set $TARGET=AR6003 instead of
# $(TARGET)=AR6002 and $(AR6002_REV)=4, but we need
# to translate here
ifeq ($(TARGET),AR6003)
    TARGET=AR6002
    AR6002_REV=4
    export TARGET
    export AR6002_REV
endif

all: tools

tools: FORCE
	$(MAKE) -C tools/bdiff/
	$(MAKE) -C tools/mkdsetimg/
	$(MAKE) -C tools/regDbGen/
	if [ -e tools/systemtools/tools/createini/ ]; \
	then \
		$(MAKE) -C tools/systemtools/ -f makefile.linux all; \
        else \
		$(MAKE) -C tools/systemtools/ -f makefile.linux.customer all; \
	fi
	$(MAKE) -C tools/tcmd/

clean: clobber

clobber:
	rm -f tools/bdiff/bdiff
	rm -f tools/mkdsetimg/mkdsetimg
	rm -f tools/regDbGen/regulatory*.bin
	rm -f tools/tcmd/athtestcmd

FORCE:
