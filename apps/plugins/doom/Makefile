################################################################
#
# $Id$
#
# $Log: Makefile,v $
# Revision 1.13  2006-12-13 06:52:09  kkurbjun
# Free more plugin memory
#
# Revision 1.12  2006-12-13 05:46:51  kkurbjun
# Will it fit?
#
# Revision 1.11  2006-12-13 04:44:17  kkurbjun
# Dehacked and BEX support for Doom - currently only supports a DEHACKED file in a WAD (not as a standalone file yet).
#
# Revision 1.10  2006-10-27 21:47:55  amiconn
# Next step of Makefile tuning: * Use 'make' internal commands for printing messages. Saves build time especially on cygwin. * SILENT variable used in more places. * Bitmap build system uses one Makefille less.
#
# Revision 1.9  2006-10-23 22:33:36  amiconn
# Consistent style of 'make' messages. Always use ranlib after ar.
#
# Revision 1.8  2006-10-23 14:09:28  markun
# rearrange compiler options to avoid conflicts with header files on FreeBSD
#
# Revision 1.7  2006-10-22 00:21:56  amiconn
# Speed up build process in general by using internal functions of make instead of spawning sub-shells where possible.
#
# Revision 1.6  2006-09-29 20:04:35  barrywardell
# Cleaner implementation of the recent OSX simulator build fix. No need to define SHARED_FLAG in each Makefile. Just have configure create it in the root Makefile instead.
#
# Revision 1.5  2006-09-29 16:15:08  barrywardell
# Allow UI simulator to be build on OS (Slightly updated version of FS5767)
#
# Revision 1.4  2006-07-27 13:27:19  linus
# Moved the X5 button driver to the target tree
#
# Revision 1.3  2006-04-14 21:07:56  kkurbjun
# Start of profiling support for doom.
#
# Revision 1.2  2006-03-29 21:16:45  kkurbjun
# Use rockbox endian defines
#
# Revision 1.1  2006-03-28 15:44:01  dave
# Patch #2969 - Doom!  Currently only working on the H300.
#
# 

INCLUDES = -I$(APPSDIR) -I.. -I. $(TARGET_INC) -I$(FIRMDIR)/include -I$(FIRMDIR)/export \
 -I$(FIRMDIR)/common -I$(FIRMDIR)/drivers -I$(OUTDIR) -I$(BUILDDIR)
CFLAGS = $(INCLUDES) $(GCCOPTS) $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN \
 -Wno-strict-prototypes $(PROFILE_OPTS)

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

# The arm code compiles too large for the plugin buffer when set to optimize for speed
ifeq ($(CPU), arm)
    CFLAGS += -Os
ifndef SIMVER
ifeq ($(TARGET), IRIVER_H100)
    CFLAGS += -mstructure-size-boundary=8
endif
endif
else
    CFLAGS += -O2
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
   w_wad.c rockdoom.c d_deh.c ../../../firmware/common/sscanf.c

# sscanf isn't built into the core right now - should remove from here if that
# changes

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
#OBJS2 := $(SRC:%.c=$(OBJDIR)/%.o)
#OBJS = $(patsubst %.S, $(OBJDIR)/%.o, $(OBJS2))
DIRS = .


ifndef SIMVER
    LDS := ../plugin.lds
    OUTPUT = $(OUTDIR)/doom.rock
else  ## simulators
    OUTPUT = $(OUTDIR)/doom.rock
endif

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/doom.elf: $(OBJS) $(LINKFILE)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lgcc\
           -T$(LINKFILE) -Wl,--gc-sections -Wl,-Map,$(OBJDIR)/doom.map

$(OUTPUT): $(OBJDIR)/doom.elf
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
	$(call PRINTS,cleaning doom)rm -rf $(OBJDIR)/doom
	$(SILENT)rm -f $(OBJDIR)/doom.* $(DEPFILE)

-include $(DEPFILE)
