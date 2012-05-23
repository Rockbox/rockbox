#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/

# We use the SB code available in the Rockbox utils/sbtools directory
IMXTOOLS_DIR=../../utils/imxtools/sbtools/
CFLAGS += -I$(IMXTOOLS_DIR) -Wall
CFLAGS += -std=c99 -g

OUTPUT = mkimxboot

# inputs for lib
IMXTOOLS_SOURCES = misc.c sb.c crypto.c crc.c aes128.c sha1.c
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

$(OBJDIR)%.o: $(IMXTOOLS_DIR)%.c
	@echo CC $<
	$(SILENT)mkdir -p $(dir $@)
	$(SILENT)$(CROSS)$(CC) $(CFLAGS) -c -o $@ $<
