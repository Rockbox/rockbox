#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# We use the SB code available in the Rockbox utils/sbtools directory
UPGTOOLS_DIR=../../utils/nwztools/upgtools/
CFLAGS += -I$(UPGTOOLS_DIR) -Wall
# std=gnu99 is required by MinGW on Windows (c99 is sufficient for Linux / MXE)
CFLAGS += -std=gnu99 -g -O3
# dependencies
# FIXME make it work for windows and maybe embed crypto++

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

OUTPUT = mknwzboot

# inputs for lib
UPGTOOLS_SOURCES = misc.c upg.c fwp.c mg.cpp md5.cpp
LIBSOURCES := mknwzboot.c install_script.c uninstall_script.c \
	      $(addprefix $(UPGTOOLS_DIR),$(UPGTOOLS_SOURCES))
# inputs for binary only
SOURCES := $(LIBSOURCES) main.c
# dependencies for binary
EXTRADEPS :=

include ../libtools.make

install_script.c install_script.h: install_script.sh $(BIN2C)
	$(BIN2C) install_script.sh install_script

uninstall_script.c uninstall_script.h: uninstall_script.sh $(BIN2C)
	$(BIN2C) uninstall_script.sh uninstall_script

# explicit dependencies on install_script.{c,h} and mknwzboot.h
$(OBJDIR)mknwzboot.o: install_script.h install_script.c uninstall_script.h uninstall_script.c mknwzboot.h
$(OBJDIR)main.o: main.c mknwzboot.h
