#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
CC := gcc
CFLAGS += -Wall -Wextra

OUTPUT = mks5lboot

# inputs for lib
LIBSOURCES := dualboot.c mkdfu.c ipoddfu.c
# inputs for binary only
SOURCES := $(LIBSOURCES) main.c
# dependencies for binary
EXTRADEPS :=

CPPDEFINES := $(shell echo foo | $(CROSS)$(CC) -dM -E -)

ifeq ($(findstring WIN32,$(CPPDEFINES)),WIN32)
LDOPTS += -lsetupapi
# optional libusb support (needed for WinUSB and libusbK drivers)
ifeq ($(findstring MINGW,$(CPPDEFINES)),MINGW)
ifeq ($(USE_LIBUSBAPI),1)
CFLAGS += -DUSE_LIBUSBAPI
LDOPTS += -Wl,-Bstatic -lusb-1.0
endif
endif
else
ifeq ($(findstring APPLE,$(CPPDEFINES)),APPLE)
LDOPTS += -L/usr/local/lib -framework IOKit -framework CoreFoundation
else # Linux
CFLAGS += -DUSE_LIBUSBAPI
LDOPTS += -lusb-1.0
endif
endif

include ../libtools.make

# explicit dependencies on dualboot.{c,h} and mks5lboot.h
$(OBJDIR)mks5lboot.o: dualboot.h dualboot.c mks5lboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mks5lboot.h
