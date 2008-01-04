#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
OPT_FLAGS=-O3 -funroll-loops

INCLUDES = -I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR) -I. $(TARGET_INC)
CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN $(OPT_FLAGS) $(PROFILE_OPTS)

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-zxbox

# This sets up 'SRC' based on the files mentioned in SOURCES
include $(TOOLSDIR)/makesrc.inc

SOURCES = $(SRC)
OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS = .

ifndef SIMVER
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    LDS := archos.lds
    OUTPUT = $(OUTDIR)/zxbox.ovl
else  ## iRiver/iPod/... targets
    LDS := ../plugin.lds
    OUTPUT = $(OUTDIR)/zxbox.rock
endif
else  ## simulators
    OUTPUT = $(OUTDIR)/zxbox.rock
endif

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/zxbox.elf: $(OBJS) $(LINKFILE)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lgcc \
           -T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/zxbox.map

$(OUTPUT): $(OBJDIR)/zxbox.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else

ifeq ($(SIMVER), sdl)
###################################################
# This is the SDL simulator version

$(OUTPUT): $(OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

endif
endif # end of simulator section


include $(TOOLSDIR)/make.inc

# MEMORYSIZE should be passed on to this makefile with the chosen memory size
# given in number of MB
$(LINKFILE): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) \
	$(DEFINES) -E -P - >$@

clean:
	$(call PRINTS,cleaning zxbox)rm -rf $(OBJDIR)/zxbox
	$(SILENT)rm -f $(OBJDIR)/zxbox.* $(DEPFILE)

-include $(DEPFILE)

