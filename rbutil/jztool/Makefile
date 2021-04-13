#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

CFLAGS += -Wall -Wextra -Iinclude
OUTPUT = jztool

ifdef RELEASE
CFLAGS += -Os -DNDEBUG
else
CFLAGS += -O0 -ggdb
endif

LIBSOURCES := src/buffer.c src/context.c src/device_info.c \
			  src/fiiom3k.c src/identify_file.c src/paramlist.c \
			  src/usb.c src/x1000.c
SOURCES    := $(LIBSOURCES) jztool.c
EXTRADEPS  :=

CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)

ifeq ($(findstring WIN32,$(CPPDEFINES)),WIN32)
# TODO: support Windows
else
ifeq ($(findstring APPLE,$(CPPDEFINES)),APPLE)
# TODO: support OSX
else
# Linux
CFLAGS += `pkg-config --cflags libusb-1.0`
LDOPTS += `pkg-config --libs libusb-1.0`
endif
endif

include ../libtools.make
