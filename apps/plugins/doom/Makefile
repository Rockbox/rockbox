################################################################
#
# $Id$
#
# $Log$
# Revision 1.2  2006/03/29 21:16:45  kkurbjun
# Use rockbox endian defines
#
# Revision 1.1  2006-03-28 15:44:01  dave
# Patch #2969 - Doom!  Currently only working on the H300.
#
# 

INCLUDES = -I$(APPSDIR) -I.. -I. -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR)
CFLAGS = $(GCCOPTS) $(INCLUDES) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN \
 -Wno-strict-prototypes -O2

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

ifneq (,$(strip $(foreach tgt,IPOD_NANO IPOD_COLOR IPOD_VIDEO,$(findstring $(tgt),$(TARGET)))))
ifndef SIMVER
    CFLAGS += -mstructure-size-boundary=8
endif
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-doom
SRC = info.c doomdef.c doomstat.c dstrings.c tables.c \
	f_finale.c f_wipe.c d_net.c d_items.c g_game.c m_menu.c m_argv.c \
	m_cheat.c m_random.c am_map.c p_ceilng.c p_doors.c p_genlin.c \
   p_enemy.c p_floor.c p_inter.c p_lights.c p_map.c p_maputl.c p_plats.c \
   p_pspr.c p_setup.c p_sight.c p_spec.c p_switch.c p_mobj.c p_telept.c \
   p_tick.c p_saveg.c p_user.c r_bsp.c r_data.c r_draw.c r_main.c \
   r_plane.c r_segs.c r_sky.c r_things.c wi_stuff.c v_video.c st_lib.c \
   st_stuff.c hu_stuff.c hu_lib.c s_sound.c z_zone.c z_bmalloc.c sounds.c \
   d_main.c m_misc.c m_bbox.c i_system.c i_sound.c i_video.c \
   w_wad.c rockdoom.c
# fixmath2.S

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
#OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
#OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS = .


ifndef SIMVER
ifneq (,$(findstring RECORDER,$(TARGET))) ## Archos recorder targets
    LDS := archos.lds
    OUTPUT = $(OUTDIR)/doom.ovl
else  ## iRiver target
    LDS := ../plugin.lds
    OUTPUT = $(OUTDIR)/doom.rock
endif
else  ## simulators
    OUTPUT = $(OUTDIR)/doom.rock
endif

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/doom.elf: $(OBJS) $(LINKFILE)
	@echo "LD "`basename $@`
	@$(CC) $(GCCOPTS) $(LDFLAGS) -nostdlib -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lgcc\
           -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/doom.map

$(OUTPUT): $(OBJDIR)/doom.elf
	@echo "OBJCOPY "`basename $@`
	@$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OUTPUT): $(OBJS)
	@echo "LD $@"
	@$(CC) $(CFLAGS) -shared $(OBJS) -L$(BUILDDIR) -lplugin -o $@
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
	@echo "LD $@"
	@$(CC) $(CFLAGS) -shared $(OBJS) -L$(BUILDDIR) -lplugin -o $@
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
	@echo "DLL "`basename $@`
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $(OBJS)
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $(OBJS) \
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
	@echo "build "`basename $@`
	@cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) $(DEFINES) \
         -E -P - >$@

clean:
	@echo "cleaning doom"
	@rm -rf $(OBJDIR)/doom
	@rm -f $(OBJDIR)/doom.* $(DEPFILE)

-include $(DEPFILE)
