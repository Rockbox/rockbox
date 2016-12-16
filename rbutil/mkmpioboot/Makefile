#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

TARGET_DIR ?= $(shell pwd)/
# We use the UCL code available in the Rockbox tools/ directory
CFLAGS += -I../../tools/ucl/include -Wall

OUTPUT = mkmpioboot

# inputs
LIBSOURCES = mkmpioboot.c
SOURCES = main.c
EXTRADEPS = $(LIBUCL)

include ../libtools.make

# explicit dependencies
$(OBJDIR)mkmpioboot.o: mkmpioboot.c mkmpioboot.h

