# -*- Makefile -*-

# return the list of extra sub-directories to build in the SUBDIRS variable

# This uses the native 'gcc' compiler and not $(CC) since we use the -include
# option and older gcc compiler doesn't have that. We use one such older
# compiler for the win32 cross-compiles on Linux.
#
# The weird grep -v thing in here is due to Apple's stupidities and is needed
# to make this do right when used on Mac OS X.

SUBDIRS := $(shell cat SUBDIRS | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) \
$(TARGET) $(DEFINES) $(EXTRA_DEFINES) -E -P -include "config.h" - | \
grep -v "^\#")
