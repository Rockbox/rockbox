#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

INCLUDES = -I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export	\
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers
CFLAGS = $(GCCOPTS) -O3 $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) \
 -DMEM=${MEMORYSIZE} -DPLUGIN

ifdef APPEXTRA
    INCLUDES += -I$(APPSDIR)/$(APPEXTRA)
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-rockboy
SRC = cpu.c emu.c events.c exports.c fastmem.c hw.c lcd.c lcdc.c loader.c \
	main.c mem.c nosound.c rccmds.c rcvars.c rtc.c save.c sound.c split.c \
	sys_rockbox.c rockboy.c menu.c

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
$(OBJDIR)/rockboy.elf: $(OBJS) $(LINKFILE) $(OUTDIR)/libplugin.a
	@echo "LD $@"
	@$(CC) $(GCCOPTS) -O -nostdlib -o $@ $(OBJS) -L$(OUTDIR) -lplugin -lgcc \
           -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/rockboy.map

$(OUTPUT): $(OBJDIR)/rockboy.elf
	@echo "OBJCOPY $<"
	@$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OUTPUT): $(OBJS) $(OUTDIR)/libplugin.a
	@echo "LD $@"
	@$(CC) $(CFLAGS) -shared $(OBJS) -L$(OUTDIR) -lplugin -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of x11-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OUTPUT): $(OBJS) $(OUTDIR)/libplugin.a
	@echo "DLL $@"
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $(OBJS)
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $(OBJS) \
                $(OUTDIR)/libplugin.a -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif
endif # end of win32-simulator

endif # end of simulator section


include $(TOOLSDIR)/make.inc

# MEM should be passed on to this makefile with the chosen memory size given
# in number of MB
$(LINKFILE): $(LDS)
	@echo "build $@"
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) \
         -E -P - >$@

clean:
	@echo "cleaning rockboy"
	@rm -rf $(OBJDIR)/rockboy
	@rm -f $(OBJDIR)/rockboy.*

-include $(DEPFILE)

