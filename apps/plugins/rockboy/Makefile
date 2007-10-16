#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(APPSDIR) -I.. -I. $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR)
CFLAGS = $(INCLUDES) $(GCCOPTS) -O2 $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN -finline-functions

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-rockboy
SRC = cpu.c emu.c events.c fastmem.c hw.c lcd.c lcdc.c loader.c \
	mem.c menu.c rbsound.c rockboy.c rtc.c save.c sound.c sys_rockbox.c \
	../../../firmware/common/sscanf.c

#CFLAGS += -DDYNAREC
#SRC += dynarec.c

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DIRS = .

ifndef SIMVER
ifneq (,$(findstring RECORDER,$(TARGET))) ## Archos recorder targets
    LDS := archos.lds
    OUTPUT = $(OUTDIR)/rockboy.ovl
else  ## iRiver target
    LDS := ../plugin.lds
    OUTPUT = $(OUTDIR)/rockboy.rock
endif
else  ## simulators
    OUTPUT = $(OUTDIR)/rockboy.rock
endif

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/rockboy.elf: $(OBJS) $(LINKFILE)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -fast -lgcc \
           -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockboy.map

$(OUTPUT): $(OBJDIR)/rockboy.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OUTPUT): $(OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of x11-simulator
ifeq ($(SIMVER), sdl)
###################################################
# This is the sdl simulator version

$(OUTPUT): $(OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of sdl-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OUTPUT): $(OBJS)
	$(call PRINTS,DLL $(@F))$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $(OBJS)
	$(SILENT)$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $(OBJS) \
                $(BUILDDIR)/libplugin.a -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif
endif # end of win32-simulator
endif
endif # end of simulator section


include $(TOOLSDIR)/make.inc

# MEMORYSIZE should be passed on to this makefile with the chosen memory size
# given in number of MB
$(LINKFILE): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) \
         -E -P - >$@

clean:
	$(call PRINTS,cleaning rockboy)rm -rf $(OBJDIR)/rockboy
	$(SILENT)rm -f $(OBJDIR)/rockboy.* $(DEPFILE)

-include $(DEPFILE)

