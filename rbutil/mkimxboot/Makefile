#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# We use the SB code available in the Rockbox utils/sbtools directory
IMXTOOLS_DIR=../../utils/imxtools/sbtools/
CFLAGS += -I$(IMXTOOLS_DIR) -Wall
# std=gnu99 is required by MinGW on Windows (c99 is sufficient for Linux / MXE)
CFLAGS += -std=gnu99 -g -O3
LDFLAGS += -lcrypto++

OUTPUT = mkimxboot

# inputs for lib
IMXTOOLS_SOURCES = misc.c sb.c crypto.cpp crc.c elf.c
LIBSOURCES := dualboot.c mkimxboot.c md5.c \
	      $(addprefix $(IMXTOOLS_DIR),$(IMXTOOLS_SOURCES))
# inputs for binary only
SOURCES := $(LIBSOURCES) main.c
# dependencies for binary
EXTRADEPS :=

include ../libtools.make

# explicit dependencies on dualboot.{c,h} and mkimxboot.h
$(OBJDIR)mkimxboot.o: dualboot.h dualboot.c mkimxboot.c mkimxboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mkimxboot.h

