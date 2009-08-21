#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FLAGS=-g -D__PCTOOL__ -DDEBUG -DROCKBOX_DIR_LEN=9 -DWPS_DIR=\".\"

COMMON = $(ROOTDIR)/apps/gui/skin_engine/wps_debug.c \
         $(ROOTDIR)/apps/gui/skin_engine/skin_parser.c \
         $(ROOTDIR)/apps/gui/skin_engine/skin_buffer.c \
         $(ROOTDIR)/apps/misc.c \
         $(ROOTDIR)/apps/recorder/bmp.c \
         $(ROOTDIR)/firmware/common/strlcpy.c

INCLUDE = -I $(ROOTDIR)/apps/gui \
          -I $(ROOTDIR)/apps/gui/skin_engine \
          -I $(ROOTDIR)/firmware/export \
          -I $(ROOTDIR)/apps \
          -I $(ROOTDIR)/apps/recorder \
          -I $(APPSDIR)

# Makes mkdepfile happy
GCCOPTS+=-D__PCTOOL__
SRC=$(APPSDIR)/checkwps.c $(COMMON)
OTHER_SRC=$(SRC)
ASMDEFS_SRC=$(SRC)

$(BUILDDIR)/$(BINARY): $(APPSDIR)/checkwps.c  $(COMMON)
	@echo CC $(BINARY)
	@$(HOSTCC) $(INCLUDE) $(FLAGS) $(COMMON) $(TARGET) $(APPSDIR)/checkwps.c -o $@
