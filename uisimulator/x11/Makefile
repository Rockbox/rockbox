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

LANGUAGE = english

TARGET = $(OBJDIR)/rockboxui

DEFINES := -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR \
$(TARGET) -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) 

# Use this for simulator-only files
INCLUDES = -I. -I$(EXPORT) -I$(APPDIR) -I$(MACHINEDIR) -I$(SIMCOMMON) -I$(OBJDIR) -I$(PLUGINDIR)/lib

# The true Rockbox Applications should use this include path:
APPINCLUDES =  $(INCLUDES)

SRCDIRS = . $(DRIVERS) $(FIRMWAREDIR)/export $(APPDIR) $(MACHINEDIR)

# The true Rockbox Applications should use this include path:
APPINCLUDES = $(INCLUDES)

LIBS = -lpthread

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) -W -Wall

APPCFLAGS = $(DEBUG) $(DEFINES) $(APPINCLUDES) -W -Wall

UNAME := $(shell uname|sed -e "s/CYGWIN.*/CYGWIN/")
EXEFILE = $(OBJDIR)/rockboxui
ifeq ($(UNAME),Linux)
  LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl -ldl
  INCLUDES += -I/usr/X11R6/include
  LIBDIRS = -L/usr/X11R6/lib
  DEFINES += -DHAVE_OSS
else
ifeq ($(UNAME),CYGWIN)
  LDFLAGS = -lSM -lICE -lXt -lX11 -lXmu -lSM -lICE -lX11
  INCLUDES += -I/usr/X11R6/include
  LIBDIRS = -L/usr/X11R6/lib
  DEFINES += -DHAVE_OSS -D_SSIZE_T_
  EXEFILE = $(OBJDIR)/rockboxui.exe
else
ifeq ($(UNAME),FreeBSD)
  LIBS = -pthread
  LDFLAGS = -lX11 -lm -lXt -lXmu
  INCLUDES += -I/usr/X11R6/include
  LIBDIRS = -L/usr/X11R6/lib
  DEFINES += -DHAVE_OSS
else
  LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl -ldl
  DEFINES += -DBIG_ENDIAN
  LIBDIRS = 
endif
endif
endif

ifeq ($(HAVE_MPEG_PLAY),1)
	SOUNDSRC = mpegplay.c oss_sound.c
	LDFLAGS += $(SOUND_LDFLAGS)
	CFLAGS += $(SOUND_CFLAGS) -DHAVE_MPEG_PLAY
else
	SOUNDSRC = 
endif

ifeq ($(ISPLAYER), PLAYER)
	LCDSRSC = lcd-playersim.c lcd-player.c lcd-player-charset.c font-player.c
else
	LCDSRSC = lcd-recorder.c sysfont.c font.c
endif
COMMONSRCS = io.c stubs.c lcd-common.c sim_icons.c fmradio.c

FIRMSRCS = $(LCDSRSC) id3.c mp3data.c usb.c mpeg.c mp3_playback.c \
	powermgmt.c power.c sprintf.c buffer.c strtok.c random.c \
	timefuncs.c panic.c debug.c

APPS = main.c tree.c menu.c credits.c main_menu.c icons.c language.c \
	playlist.c wps.c wps-display.c settings.c status.c \
	screens.c sleeptimer.c keyboard.c onplay.c\
	misc.c plugin.c playlist_viewer.c bookmark.c filetypes.c \
	settings_menu.c sound_menu.c playlist_menu.c

ifneq ($(ISPLAYER), PLAYER)
   APPS += peakmeter.c bmp.c widgets.c radio.c
endif

SRCS = screenhack.c uibasic.c resources.c visual.c lcd-x11.c \
       button-x11.c thread.c $(APPS) $(MENUS) $(FIRMSRCS) \
       $(COMMONSRCS) $(SOUNDSRC)

OBJS := $(OBJDIR)/lang.o $(SRCS:%.c=$(OBJDIR)/%.o)

ROCKS := $(patsubst $(PLUGINDIR)/%.c,$(OBJDIR)/%.rock,$(wildcard $(PLUGINDIR)/*.c))
PLUGINLIBOBJS := $(patsubst $(PLUGINDIR)/lib/%.c,$(OBJDIR)/%.o,$(wildcard $(PLUGINDIR)/lib/*.c))

all: $(TOOLSDIR)/convbdf $(EXEFILE) $(ROCKS)

$(ROCKS): $(OBJDIR)/libplugin.a

$(TOOLSDIR)/convbdf:
	$(MAKE) -C $(TOOLSDIR)

clean:
	$(RM) $(OBJS) *~ core $(EXEFILE) $(CLIENTS) $(OBJDIR)/lang.[cho] \
	$(OBJDIR)/build.lang $(OBJDIR)/*.o $(OBJDIR)/sysfont.c \
	$(ROCKS) $(OBJDIR)/credits.raw
	$(RM) -r $(DEPS)
	$(RM) -f $(OBJDIR)/libplugin.a $(OBJDIR)/rockbox.zip

################## Specific dependencies ##################
$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl $(APPDIR)/credits.pl < $< > $@

$(OBJDIR)/uisw32-res.o: uisw32.rc
	$(WINDRES) -i $< -o $@

$(OBJDIR)/credits.o: $(APPDIR)/credits.c $(APPDIR)/credits.h $(OBJDIR)/credits.raw

$(OBJDIR)/thread.o: ./thread.c
$(OBJDIR)/plugin.o: $(APPDIR)/plugin.c
$(OBJDIR)/build.lang: $(APPDIR)/lang/$(LANGUAGE).lang
	@echo "UPLANG"
	@perl $(TOOLSDIR)/uplang $(APPDIR)/lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang
	@echo GENLANG
	@perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	@echo "CC lang.c"
	@$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

$(OBJDIR)/sysfont.o: $(FIRMWAREDIR)/fonts/clR6x8.bdf
	@echo CC $<
	@$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	@$(CC) $(APPCFLAGS) -c $(OBJDIR)/sysfont.c -o $@

################## Plugins ##################
$(OBJDIR)/libplugin.a:  $(PLUGINLIBOBJS)
	@echo AR $<
	@$(AR) ru $@ $+

$(OBJDIR)/%.o: $(PLUGINDIR)/lib/%.c
	@echo CC $<
	@$(CC) $(CFLAGS) -DPLUGIN -c $< -o $@

$(OBJDIR)/%.rock: $(APPDIR)/plugins/%.c $(APPDIR)/plugin.h
	@echo CC $<
	@$(CC) $(APPCFLAGS) -DPLUGIN -L$(OBJDIR) -lplugin -shared $< -o $@
	@chmod -x $@

################## Patterns for building objects ##################
$(OBJDIR)/%.o: ../x11/%.c
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

# these ones are simulator-specific

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

ifeq ($(UNAME),CYGWIN)
$(EXEFILE): $(OBJS) $(OBJDIR)/libplugin.a
	@echo LD $@
	@$(CC) -g -o $(EXEFILE) $(OBJS) $(LIBDIRS) $(LDFLAGS) $(LIBS)
else
$(EXEFILE): $(OBJS) $(OBJDIR)/libplugin.a
	@echo LD $@
	@$(CC) -g -o $(EXEFILE) $(LIBDIRS) $(LDFLAGS) $(OBJS) $(LIBS)
endif

tags:
	@$(SHELL) -c 'for d in $(SRCDIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

