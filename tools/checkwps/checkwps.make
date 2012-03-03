#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

FLAGS=-g -D__PCTOOL__ $(TARGET) -Wall

SRC= $(call preprocess, $(TOOLSDIR)/checkwps/SOURCES)

INCLUDES = -I$(ROOTDIR)/apps/gui \
           -I$(ROOTDIR)/apps/gui/skin_engine \
           -I$(ROOTDIR)/firmware/export \
           -I$(ROOTDIR)/firmware/include \
           -I$(ROOTDIR)/firmware/target/hosted \
           -I$(ROOTDIR)/firmware/target/hosted/sdl \
           -I$(ROOTDIR)/apps \
           -I$(ROOTDIR)/apps/recorder \
           -I$(ROOTDIR)/apps/radio \
           -I$(APPSDIR) \
           -I$(BUILDDIR)

# Makes mkdepfile happy
GCCOPTS+=-D__PCTOOL__ -DCHECKWPS -g

.SECONDEXPANSION: # $$(OBJ) is not populated until after this

$(BUILDDIR)/$(BINARY): $$(OBJ) $$(SKINLIB)
	@echo LD $(BINARY)
	$(SILENT)$(HOSTCC) $(INCLUDE) $(FLAGS) -L$(BUILDDIR)/lib -lskin_parser -o $@ $+
