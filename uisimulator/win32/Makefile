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

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
     MACHINEDIR = $(RECDIR)
     # not very nice to set RTC like this, but...
     RTC += -DHAVE_RTC
else
     MACHINEDIR = $(PLAYDIR)
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

# where to put all output files
OBJDIR = .
LANGUAGE = english

TARGET = $(OBJDIR)/uisw32.exe

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR \
$(KEYPAD) $(DISPLAY) -DAPPSVERSION=\"$(VERSION)\" $(EXTRA_DEFINES) $(RTC)

LDFLAGS =  -lgdi32 -luser32

# Use this for simulator-only files
INCLUDES = -I. -I$(EXPORT) -I$(APPDIR) -I$(MACHINEDIR) -I$(SIMCOMMON) -I$(MACHINEDIR) -I$(OBJDIR)

# The true Rockbox Applications should use this include path:
APPINCLUDES =  $(INCLUDES)

DEFINES += -DWIN32

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) -W -Wall -mno-cygwin

APPCFLAGS = $(DEBUG) $(DEFINES) $(APPINCLUDES) -W -Wall -mno-cygwin
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

UNAME := $(shell uname|sed -e "s/CYGWIN.*/CYGWIN/")
ifeq ($(UNAME),CYGWIN)
  CC = gcc
  WINDRES = windres
  DLLTOOL = dlltool
  DLLWRAP = dllwrap
  DEFINES += -DNOCYGWIN
  CFLAGS += -mno-cygwin
  LDFLAGS += -mno-cygwin
  APPCFLAGS += -mno-cygwin
else
  CC = i386-mingw32msvc-gcc
  WINDRES = i386-mingw32msvc-windres
  DLLTOOL = i386-mingw32msvc-dlltool
  DLLWRAP = i386-mingw32msvc-dllwrap
  CFLAGS += -mwindows
  LDFLAGS += -mwindows
  APPCFLAGS += -mwindows
endif

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
	LCDSRSC = lcd-recorder.c sysfont.c font.c
else
	LCDSRSC = lcd-playersim.c lcd-player.c lcd-player-charset.c font-player.c
endif
FIRMSRCS = $(LCDSRSC) id3.c mp3data.c usb.c mpeg.c mp3_playback.c \
	powermgmt.c power.c sprintf.c buffer.c lcd-common.c strtok.c random.c

APPS = main.c tree.c menu.c credits.c main_menu.c icons.c language.c \
	playlist.c wps.c wps-display.c settings.c status.c \
	screens.c peakmeter.c sleeptimer.c keyboard.c onplay.c\
	misc.c plugin.c playlist_viewer.c

MENUS = settings_menu.c sound_menu.c playlist_menu.c

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
   APPS += bmp.c widgets.c
endif

SRCS = button.c dir-win32.c lcd-win32.c panic-win32.c thread-win32.c \
	debug-win32.c kernel.c string-win32.c uisw32.c stubs.c \
	$(APPS) $(MENUS) $(FIRMSRCS) sim_icons.c io.c

OBJS := $(OBJDIR)/lang.o $(SRCS:%.c=$(OBJDIR)/%.o) $(OBJDIR)/uisw32-res.o

ROCKSRCS := $(patsubst $(PLUGINDIR)/%.c,%.c,$(wildcard $(PLUGINDIR)/*.c))

ROCKS := $(ROCKSRCS:%.c=$(OBJDIR)/%.rock)

all: $(TOOLSDIR)/convbdf $(TARGET) $(ROCKS)

$(TOOLSDIR)/convbdf:
	$(MAKE) -C $(TOOLSDIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/uisw32-res.o: uisw32.rc
	$(WINDRES) -i $< -o $@

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS) $(OBJDIR)/uisw32-res.o \
	$(OBJDIR)/lang.[cho] $(OBJDIR)/build.lang $(OBJDIR)/*.o \
	$(OBJDIR)/*.rock $(OBJDIR)/*.def $(OBJDIR)/sysfont.c \
	$(OBJDIR)/credits.raw
	$(RM) -r $(DEPS)

distclean: clean
	$(RM) config.cache

$(OBJDIR)/credits.raw: $(DOCSDIR)/CREDITS
	perl $(APPDIR)/credits.pl < $< > $@

$(OBJDIR)/credits.o: $(APPDIR)/credits.c $(APPDIR)/credits.h $(OBJDIR)/credits.raw
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/menu.o: $(APPDIR)/menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/main_menu.o: $(APPDIR)/main_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/language.o: $(APPDIR)/language.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/sound_menu.o: $(APPDIR)/sound_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/settings_menu.o: $(APPDIR)/settings_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/playlist_menu.o: $(APPDIR)/playlist_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/icons.o: $(MACHINEDIR)/icons.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/widgets.o: $(RECDIR)/widgets.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/keyboard.o: $(MACHINEDIR)/keyboard.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/main.o: $(APPDIR)/main.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/misc.o: $(APPDIR)/misc.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/wps.o: $(APPDIR)/wps.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/sleeptimer.o: $(APPDIR)/sleeptimer.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/wps-display.o: $(APPDIR)/wps-display.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/bmp.o: $(RECDIR)/bmp.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/tree.o: $(APPDIR)/tree.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/onplay.o: $(APPDIR)/onplay.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/playlist.o: $(APPDIR)/playlist.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/playlist_viewer.o: $(APPDIR)/playlist_viewer.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/plugin.o: $(APPDIR)/plugin.c plugin-win32.h
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/build.lang: $(APPDIR)/lang/$(LANGUAGE).lang
	perl $(TOOLSDIR)/uplang $(APPDIR)/lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang
	perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

$(OBJDIR)/lcd-recorder.o: $(DRIVERS)/lcd-recorder.c
	$(CC) $(APPCFLAGS) -DHAVE_LCD_BITMAP -c $< -o $@

$(OBJDIR)/power.o: $(DRIVERS)/power.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/usb.o: $(FIRMWAREDIR)/usb.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/panic.o: $(FIRMWAREDIR)/panic.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/powermgmt.o: $(FIRMWAREDIR)/powermgmt.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/lcd-player-charset.o: $(DRIVERS)/lcd-player-charset.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/settings.o: $(APPDIR)/settings.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/id3.o: $(FIRMWAREDIR)/id3.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mp3data.o: $(FIRMWAREDIR)/mp3data.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/font.o: $(FIRMWAREDIR)/font.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sysfont.o: $(FIRMWAREDIR)/fonts/clR6x8.bdf
	$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	$(CC) $(APPCFLAGS) -c $(OBJDIR)/sysfont.c -o $@

$(OBJDIR)/status.o: $(APPDIR)/status.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/screens.o: $(APPDIR)/screens.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/peakmeter.o: $(RECDIR)/peakmeter.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/version.o: $(FIRMWAREDIR)/version.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/debug.o: $(FIRMWAREDIR)/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mpeg.o: $(FIRMWAREDIR)/mpeg.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mp3_playback.o: $(FIRMWAREDIR)/mp3_playback.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/buffer.o: $(FIRMWAREDIR)/buffer.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/random.o: $(COMMON)/random.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sprintf.o: $(COMMON)/sprintf.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/strtok.o: $(COMMON)/strtok.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/stubs.o: $(SIMCOMMON)/stubs.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lcd-common.o: $(SIMCOMMON)/lcd-common.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/font-player.o: $(SIMCOMMON)/font-player.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sim_icons.o: $(SIMCOMMON)/sim_icons.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lcd-playersim.o: $(SIMCOMMON)/lcd-playersim.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lcd-player.o: $(DRIVERS)/lcd-player.c
	$(CC) $(CFLAGS) -c $< -o $@

# these ones are simulator-specific

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.po : $(PLUGINDIR)/%.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.rock : $(OBJDIR)/%.po
	$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $<
	$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $< -o $@

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
