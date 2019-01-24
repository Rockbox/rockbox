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

# Location to pkg-config binary.
PKGCONFIG := pkg-config

# Distros could use different names for the crypto library. We try a list
# of candidate names, only one of them should be the valid one.
LIBCRYPTO_NAMES = libcryptopp libcrypto++ cryptopp crypto++

$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval LDOPTS += $(shell $(PKGCONFIG) --silence-errors --libs $(l))))
$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval CFLAGS += $(shell $(PKGCONFIG) --silence-errors --cflags $(l))))
$(foreach l,$(LIBCRYPTO_NAMES),\
	$(eval CXXFLAGS += $(shell $(PKGCONFIG) --silence-errors --cflags $(l))))
LDOPTS += -lpthread

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
