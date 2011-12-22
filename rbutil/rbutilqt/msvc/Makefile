#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#

ifndef V
SILENT = @
endif
# The rbspeex Makefile has problems with msys' path mangling as well as running
# MSVC's lib tool.  Use TOP instead of pwd. This means the resulting files will
# always be placed in this folder.
# On Linux use the current folder instead.
TOP := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

LIBS = ipodpatcher sansapatcher chinachippatcher \
       mkamsboot mkimxboot mkmpioboot mktccboot \
       ucl rbspeex

ifeq ($(findstring Linux,$(shell uname)),Linux)
PWD=$(shell pwd)
all: dll
else
PWD=$(abspath $(TOP))
all: lib
endif

CC=gcc
dll: $(addsuffix .dll,$(LIBS))

ucl.dll:
	$(SILENT)$(MAKE) -C ../../../tools/ucl/src \
	    TARGET_DIR=$(PWD)/ OBJDIR=$(PWD)/build-dll/$(basename $@) \
	    CROSS=$(CROSS) CC=$(CC) APPVERSION=dll dll
rbspeex.dll:
	$(SILENT)$(MAKE) -C ../../../tools/rbspeex \
	    TARGET_DIR=$(PWD)/ OUT=$(PWD)/build-dll/$(basename $@) \
	    CROSS=$(CROSS) CC=$(CC) STATIC=1 APPVERSION=dll dll

%.dll:
	$(SILENT)$(MAKE) -C ../../$(basename $@) \
	    TARGET_DIR=$(PWD)/ BUILD_DIR=$(PWD)/build-dll/$(basename $@) \
	    CROSS=$(CROSS) CC=$(CC) APPVERSION=dll dll

lib: $(addsuffix .lib,$(LIBS))

%.lib: %.dll
	@echo LIB $@
	@lib /machine:x86 /nologo /def:$(basename $<).def

clean:
	rm -rf $(addsuffix .dll,$(LIBS))
	rm -rf $(addsuffix .def,$(LIBS))
	rm -rf $(addsuffix .exp,$(LIBS))
	rm -rf $(addsuffix .lib,$(LIBS))
	rm -rf build-dll
