#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# We use the SB code available in the Rockbox utils/sbtools directory

IMXTOOLS_DIR=../../utils/imxtools/sbtools/

COMPILEFLAGS := -Wall -g -O3 -I$(IMXTOOLS_DIR)

# std=gnu99 is required by MinGW on Windows (c99 is sufficient for Linux / MXE)
CFLAGS += -std=gnu99 $(COMPILEFLAGS)

TOMCRYPT_DIR := ../../utils/tomcrypt

CFLAGS += -I$(TOMCRYPT_DIR)/src/headers
LDOPTS += -lpthread $(TOMCRYPT_DIR)/librbtomcrypt.a

OUTPUT = mkimxboot

# inputs for lib
IMXTOOLS_SOURCES = misc.c sb.c crypto.c crc.c elf.c
LIBSOURCES := dualboot.c mkimxboot.c md5.c \
	      $(addprefix $(IMXTOOLS_DIR),$(IMXTOOLS_SOURCES))

# for now build tomcrypt as part of the lib.
LIBSOURCES += $(addprefix $(TOMCRYPT_DIR),$(TOMCRYPT_SOURCES))

# inputs for binary only
SOURCES := $(LIBSOURCES) main.c
# dependencies for binary
EXTRADEPS :=

include ../libtools.make

# explicit dependencies on dualboot.{c,h} and mkimxboot.h
$(OBJDIR)mkimxboot.o: dualboot.h dualboot.c mkimxboot.c mkimxboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mkimxboot.h
$(BINARY): librbtomcrypt.a

librbtomcrypt.a:
	$(MAKE) -C ../../utils/tomcrypt

