#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# We use the HMAC-SHA1 code from tools/
TOOLS_DIR=../../tools/
CFLAGS += -Wall -I$(TOOLS_DIR)
CFLAGS += -std=c99 -g
LDOPTS += -lz

OUTPUT = mkzenboot

# inputs for lib
TOOLS_SOURCES = hmac-sha1.c
LIBSOURCES := dualboot.c mkzenboot.c md5.c utils.c \
	      $(addprefix $(TOOLS_DIR),$(TOOLS_SOURCES))

# inputs for binary only
SOURCES := main.c
# dependencies for binary
EXTRADEPS :=

include ../libtools.make

# explicit dependencies on dualboot.{c,h} and mkimxboot.h
$(OBJDIR)mkzenboot.o: dualboot.h dualboot.c mkzenboot.c mkzenboot.h
$(OBJDIR)main.o: dualboot.h dualboot.c main.c mkzenboot.h
