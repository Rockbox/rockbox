#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
CFLAGS += -Wall -Wextra

OUTPUT = mks5lboot

# inputs for lib
LIBSOURCES := dualboot.c mkdfu.c ipoddfu.c
# inputs for binary only
SOURCES := $(LIBSOURCES) main.c
# dependencies for binary
EXTRADEPS :=

ifeq ($(findstring MINGW,$(shell uname)),MINGW)
LDOPTS += -lsetupapi
# optional libusb support on Windows
ifdef DISABLE_LIBUSBAPI
CFLAGS += -DNO_LIBUSBAPI
else
LDOPTS += -Wl,-Bstatic -lusb-1.0
endif
else
# Linux, OS X
LDOPTS += -lusb-1.0
endif

include ../libtools.make

# explicit dependencies on dualboot.{c,h} and mks5lboot.h
$(OBJDIR)mks5lboot.o: dualboot.h dualboot.c mks5lboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mks5lboot.h
