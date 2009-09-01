#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FLAGS=-g -D__PCTOOL__ -DDEBUG -DROCKBOX_DIR_LEN=9 -DWPS_DIR=\".\" $(TARGET)

SRC = $(ROOTDIR)/apps/gui/skin_engine/wps_debug.c \
      $(ROOTDIR)/apps/gui/skin_engine/skin_parser.c \
      $(ROOTDIR)/apps/gui/skin_engine/skin_buffer.c \
      $(ROOTDIR)/apps/misc.c \
      $(ROOTDIR)/apps/recorder/bmp.c \
      $(ROOTDIR)/firmware/common/strlcpy.c \
      $(APPSDIR)/checkwps.c

INCLUDES = -I$(ROOTDIR)/apps/gui \
           -I$(ROOTDIR)/apps/gui/skin_engine \
           -I$(ROOTDIR)/firmware/export \
           -I$(ROOTDIR)/apps \
           -I$(ROOTDIR)/apps/recorder \
           -I$(APPSDIR)

# Makes mkdepfile happy
GCCOPTS+=-D__PCTOOL__

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/$(BINARY): $$(OBJ)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(INCLUDE) $(FLAGS) -o $@ $+
