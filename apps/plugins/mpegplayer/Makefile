#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(APPSDIR) -I.. -I. $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(APPSDIR)/plugins/lib -I$(OUTDIR) -I$(BUILDDIR)
CFLAGS = $(INCLUDES) $(GCCOPTS) -O2 $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-mpegplayer

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS = .

LDS := ../plugin.lds
OUTPUT = $(OUTDIR)/mpegplayer.rock

all: $(OUTPUT)

.PHONY: libmad-mpegplayer

$(BUILDDIR)/libmad-mpegplayer.a: libmad-mpegplayer

libmad-mpegplayer:
	$(SILENT)mkdir -p $(OBJDIR)/libmad-mpegplayer
	$(call PRINTS,MAKE in libmad for mpegplayer)$(MAKE) -C $(APPSDIR)/codecs/libmad MPEGPLAYER=1 OBJDIR=$(OBJDIR)/libmad-mpegplayer OUTPUT=$(BUILDDIR)/libmad-mpegplayer.a

ifndef SIMVER
$(OBJDIR)/mpegplayer.elf: $(OBJS) $(LINKFILE) $(BUILDDIR)/libmad-mpegplayer.a
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lmad-mpegplayer -lgcc\
           $(LINKBITMAPS) -T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/mpegplayer.map

$(OUTPUT): $(OBJDIR)/mpegplayer.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else
# This is the SDL simulator version

$(OUTPUT): $(OBJS) $(BUILDDIR)/libmad-mpegplayer.a
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -lmad-mpegplayer -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

endif # end of sdl-simulator

include $(TOOLSDIR)/make.inc

# MEMORYSIZE should be passed on to this makefile with the chosen memory size
# given in number of MB
$(LINKFILE): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) \
	$(DEFINES) -E -P - >$@

clean:
	$(call PRINTS,cleaning mpegplayer)rm -rf $(OBJDIR)/mpegplayer
	$(SILENT)rm -f $(OBJDIR)/mpegplayer.* $(DEPFILE)

-include $(DEPFILE)

