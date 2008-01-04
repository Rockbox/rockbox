#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $$Id: $$
#

INCLUDES = -I$(APPSDIR) -I.. -I. $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR) \
 -I$(BUILDDIR)/pluginbitmaps
CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-shortcuts

SOURCES := shortcuts_common.c shortcuts_view.c shortcuts_append.c
VIEW_OBJS := $(OBJDIR)/shortcuts_common.o $(OBJDIR)/shortcuts_view.o
APPEND_OBJS := $(OBJDIR)/shortcuts_common.o $(OBJDIR)/shortcuts_append.o
DIRS = .

ifndef SIMVER
    LDS := ../plugin.lds
endif

OUTPUT = $(OUTDIR)/shortcuts_view.rock $(OUTDIR)/shortcuts_append.rock

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/shortcuts_view.elf: $(VIEW_OBJS) $(LINKFILE) $(BITMAPLIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -O -nostdlib -o $@ $(VIEW_OBJS) -L$(BUILDDIR) -lplugin -lgcc \
           $(LINKBITMAPS) -Wl,--gc-sections -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/shortcuts_view.map

$(OUTDIR)/shortcuts_view.rock: $(OBJDIR)/shortcuts_view.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@

$(OBJDIR)/shortcuts_append.elf: $(APPEND_OBJS) $(LINKFILE) $(BITMAPLIBS)
	$(call PRINTS,LD $(@F))$(CC) $(GCCOPTS) -O -nostdlib -o $@ $(APPEND_OBJS) -L$(BUILDDIR) -lplugin -lgcc \
           $(LINKBITMAPS) -Wl,--gc-sections -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/shortcuts_append.map

$(OUTDIR)/shortcuts_append.rock: $(OBJDIR)/shortcuts_append.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else

###################################################
# This is the SDL simulator version

$(OUTDIR)/shortcuts_view.rock: $(VIEW_OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(VIEW_OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

$(OUTDIR)/shortcuts_append.rock: $(APPEND_OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(APPEND_OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

endif # end of simulator section


include $(TOOLSDIR)/make.inc

# MEMORYSIZE should be passed on to this makefile with the chosen memory size
# given in number of MB
$(LINKFILE): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) \
	$(DEFINES) -E -P - >$@

clean:
	$(call PRINTS,cleaning shortcuts)rm -rf $(OBJDIR)/shortcuts
	$(SILENT)rm -f $(OBJDIR)/shortcuts* $(DEPFILE)

-include $(DEPFILE)
