############################################################################
#             __________               __   ___.                  
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/ 
# $Id$
#
# Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

APPDIR = ../../apps
RECDIR = $(APPDIR)/recorder
PLAYDIR = $(APPDIR)/player
PLUGINDIR = $(APPDIR)/plugins
SIMCOMMON = ../common

ISONDIO := $(findstring ONDIO, $(TARGET))
ISIRIVER := $(findstring IRIVER, $(TARGET))
ISPLAYER := $(findstring PLAYER, $(TARGET))
ifeq ($(ISPLAYER), PLAYER)
     MACHINEDIR = $(PLAYDIR)
else
     MACHINEDIR = $(RECDIR)
endif

PREVAPPDIR= ..
FIRMWAREDIR = ../../firmware

# build some sources from these dirs
DRIVERS = $(FIRMWAREDIR)/drivers
COMMON = $(FIRMWAREDIR)/common

# include here:
EXPORT = $(FIRMWAREDIR)/export
TOOLSDIR = ../../tools
DOCSDIR = ../../docs

RM = rm -f
DEBUG = -g

ifndef OBJDIR
no_configure:
	@echo "Don't run make here. Run the tools/configure script from your own build"
	@echo "directory, then run make there."
	@echo
	@echo "More help on how to build rockbox can be found here:"
	@echo "http://rockbox.haxx.se/docs/how_to_compile.html"
endif

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR \
$(TARGET) -DAPPSVERSION=\"$(VERSION)\" -DMEM=${MEMORYSIZE} $(EXTRA_DEFINES)

LDFLAGS =  -lgdi32 -luser32

# Use this for simulator-only files
INCLUDES = -I. -I$(EXPORT) -I$(APPDIR) -I$(MACHINEDIR) -I$(SIMCOMMON) -I$(OBJDIR) -I$(PLUGINDIR)/lib

# The true Rockbox Applications should use this include path:
APPINCLUDES =  $(INCLUDES)

DEFINES += -DWIN32

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) -W -Wall -mno-cygwin

APPCFLAGS = $(DEBUG) $(DEFINES) $(APPINCLUDES) -W -Wall -mno-cygwin
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

UNAME := $(shell uname|sed -e "s/CYGWIN.*/CYGWIN/")
EXEFILE := $(OBJDIR)/rockboxui.exe

ifeq ($(UNAME),CYGWIN)
  CC = gcc
  WINDRES = windres
  DLLTOOL = dlltool
  DLLWRAP = dllwrap
  RANLIB = ranlib
  DEFINES += -DNOCYGWIN
  LDFLAGS += -mno-cygwin
else
  CC = i386-mingw32msvc-gcc
  WINDRES = i386-mingw32msvc-windres
  DLLTOOL = i386-mingw32msvc-dlltool
  DLLWRAP = i386-mingw32msvc-dllwrap
  RANLIB = i386-mingw32msvc-ranlib
  CFLAGS += -mwindows
  LDFLAGS += -mwindows
  APPCFLAGS += -mwindows
endif

ifeq ($(ISPLAYER),PLAYER)
	LCDSRSC = lcd-playersim.c lcd-player.c lcd-player-charset.c font-player.c
else
ifeq ($(ISIRIVER),IRIVER)
	LCDSRSC = lcd-h100.c sysfont.c font.c
else
	LCDSRSC = lcd-recorder.c sysfont.c font.c
endif
endif
COMMONSRCS = io.c stubs.c lcd-common.c sim_icons.c fmradio.c

FIRMSRCS = $(LCDSRSC) id3.c mp3data.c usb.c mpeg.c mp3_playback.c \
	powermgmt.c power.c sprintf.c buffer.c strtok.c random.c \
	timefuncs.c strcasestr.c

APPS = main.c tree.c menu.c credits.c main_menu.c icons.c language.c \
	playlist.c wps.c wps-display.c settings.c status.c \
	screens.c sleeptimer.c keyboard.c onplay.c\
	misc.c plugin.c playlist_viewer.c bookmark.c filetypes.c \
	settings_menu.c sound_menu.c playlist_menu.c filetree.c dbtree.c

ifneq ($(ISPLAYER),PLAYER)
   APPS += peakmeter.c bmp.c widgets.c radio.c
endif

SRCS = button.c lcd-win32.c panic-win32.c thread-win32.c \
	debug-win32.c kernel.c string-win32.c uisw32.c \
	$(APPS) $(FIRMSRCS) $(COMMONSRCS)

OBJS := $(OBJDIR)/lang.o $(SRCS:%.c=$(OBJDIR)/%.o) $(OBJDIR)/uisw32-res.o

ifdef ENABLEDPLUGINS
  ROCKS := $(patsubst $(PLUGINDIR)/%.c,$(OBJDIR)/%.rock,$(wildcard $(PLUGINDIR)/*.c))
  PLUGINLIBOBJS := $(patsubst $(PLUGINDIR)/lib/%.c,$(OBJDIR)/%.o,$(wildcard $(PLUGINDIR)/lib/*.c))
endif

all: $(TOOLSDIR)/convbdf $(EXEFILE) $(ROCKS)

$(ROCKS): $(OBJDIR)/libplugin.a

$(TOOLSDIR)/convbdf:
	$(MAKE) -C $(TOOLSDIR)

$(EXEFILE): $(OBJDIR)/lang.o $(OBJS)
	@echo LD $@
	@$(CC) $(OBJS) -o $(EXEFILE) $(LDFLAGS)

clean:
	$(RM) $(OBJS) *~ core $(EXEFILE) $(CLIENTS) $(OBJDIR)/uisw32-res.o \
	$(OBJDIR)/lang.[cho] $(OBJDIR)/build.lang $(OBJDIR)/*.o \
	$(OBJDIR)/*.rock $(OBJDIR)/*.def $(OBJDIR)/sysfont.c \
	$(OBJDIR)/credits.raw $(OBJDIR)/libplugin.a
	$(RM) -f $(OBJDIR)/UI256.bmp
	$(RM) -r $(DEPS)

################## Specific dependencies ##################
$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl $(APPDIR)/credits.pl < $< > $@

$(OBJDIR)/UI256.bmp: UI-$(ARCHOS).bmp
	@echo UI
	@cp $< $@

$(OBJDIR)/uisw32-res.o: uisw32.rc $(OBJDIR)/UI256.bmp
	$(WINDRES) -I$(OBJDIR) -i $< -o $@

$(OBJDIR)/credits.o: $(APPDIR)/credits.c $(APPDIR)/credits.h $(OBJDIR)/credits.raw

$(OBJDIR)/button.o: button.c
$(OBJDIR)/plugin.o: $(APPDIR)/plugin.c plugin-win32.h
$(OBJDIR)/build.lang: $(APPDIR)/lang/$(LANGUAGE).lang
	@echo UPLANG
	@perl $(TOOLSDIR)/uplang $(APPDIR)/lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang
	@echo GENLANG
	@perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	@$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

$(OBJDIR)/sysfont.o: $(FIRMWAREDIR)/fonts/clR6x8.bdf
	@echo CC $<
	@$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	@$(CC) $(APPCFLAGS) -c $(OBJDIR)/sysfont.c -o $@

################## Plugins ##################
$(OBJDIR)/libplugin.a:  $(PLUGINLIBOBJS)
	@echo AR $@
	@$(AR) ru $@ $+
	@$(RANLIB) $@

$(OBJDIR)/%.o: $(PLUGINDIR)/lib/%.c
	@echo CC $<
	@$(CC) $(CFLAGS) -DPLUGIN -c $< -o $@

$(OBJDIR)/%.po : $(PLUGINDIR)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -DPLUGIN -c $< -o $@

$(OBJDIR)/%.rock : $(OBJDIR)/%.po $(OBJDIR)/libplugin.a
	@echo DLL $@
	@$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	@$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< $(OBJDIR)/libplugin.a -o $@
	@chmod -x $@

################## Patterns for building objects ##################
$(OBJDIR)/%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SIMCOMMON)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(MACHINEDIR)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(DRIVERS)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(FIRMWAREDIR)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(COMMON)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(APPDIR)/%.c
	@echo CC $<
	@$(CC) $(APPCFLAGS) -c $< -o $@

################## Auto-dependencies ##################
DEPS:=$(OBJDIR)/.deps

$(DEPS)/%.d: %.c
		@$(SHELL) -c 'if [ ! -d $(DEPS) ]; then \
			echo Creating the dependency directory: $(DEPS); \
			mkdir -p $(DEPS); fi'
		@echo "Updating Dependencies for $<"
		@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
	|sed '\''s|\($*\)\.o[ :]*|$(OBJDIR)/\1.o $(<:%.c=%.d) : |g'\'' > $@; \
	[ -s $@ ] || rm -f $@'

-include $(SRCS:%.c=$(DEPS)/%.d)
