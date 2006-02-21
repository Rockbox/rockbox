#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES= -I$(FIRMDIR)/include -I$(FIRMDIR)/export -I. -I$(BUILDDIR) \
          -I$(OBJDIR)
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES)

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
CSRC := $(SRC:%.bmp=$(OBJDIR)/%.c)
OBJS := $(CSRC:%.c=%.o)
DEPFILE = $(OBJDIR)/dep-bitmaps-mono

BMP2RB = $(BMP2RB_MONO)
OUTPUT = $(BUILDDIR)/libpluginbitmapsmono.a
BMPINCDIR = $(BUILDDIR)/pluginbitmaps

include $(TOOLSDIR)/makebmp.inc

clean:
	@echo "cleaning plugins/bitmaps/mono"
	@rm -f $(CSRC) $(OBJS) $(OUTPUT) $(DEPFILE)
	@rmdir $(OBJDIR)

-include $(DEPFILE)
