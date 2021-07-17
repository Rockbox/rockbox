#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

CFLAGS += -Wall -Wextra -Iinclude -I../../tools/ucl/include -I../../lib/microtar/src
OUTPUT = jztool

ifdef RELEASE
CFLAGS += -Os -DNDEBUG
else
CFLAGS += -O0 -ggdb
endif

LIBSOURCES := src/buffer.c src/context.c src/device_info.c \
			  src/identify_file.c src/ucl_unpack.c src/usb.c src/x1000.c
SOURCES    := $(LIBSOURCES) jztool.c
EXTRADEPS  := libucl.a libmicrotar.a

CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)

ifeq ($(findstring WIN32,$(CPPDEFINES)),WIN32)
# TODO: support Windows
else
ifeq ($(findstring APPLE,$(CPPDEFINES)),APPLE)
# Mac, tested on x86 only -- may need to adjust paths if building on ARM.
# paths should work with homebrew libusb.
LIBUSB_CFLAGS ?= -I/usr/local/include/libusb-1.0
ifdef STATIC
LIBUSB_LDOPTS ?= /usr/local/lib/libusb-1.0.a -framework IOKit -framework CoreFoundation
else
LIBUSB_LDOPTS ?= -L/usr/local/lib -lusb-1.0
endif
else
# Linux; note for static builds you need to build a copy of libusb without
# udev support and specify the includes / libs manually
LIBUSB_CFLAGS ?= `pkg-config --cflags libusb-1.0`
LIBUSB_LDOPTS ?= `pkg-config --libs libusb-1.0`
endif
endif

CFLAGS += $(LIBUSB_CFLAGS)
LDOPTS += $(LIBUSB_LDOPTS)

include ../libtools.make
