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

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
	MACHINEDIR = $(RECDIR)
        # not very nice to set RTC like this, but...
        RTC += -DHAVE_RTC
else
	MACHINEDIR = $(PLAYDIR)
endif

PREVAPPDIR= ..
FIRMWAREDIR = ../../firmware
DRIVERS = $(FIRMWAREDIR)/drivers
COMMON = $(FIRMWAREDIR)/common
LIBMADDIR = $(PREVAPPDIR)/common/libmad
TOOLSDIR = ../../tools
DOCSDIR = ../../docs

CC = gcc
RM = rm -f
DEBUG = -g

# where to put all output files
OBJDIR = .

TARGET = $(OBJDIR)/rockboxui
LANGUAGE = english

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR	\
$(KEYPAD) $(DISPLAY) $(EXTRA_DEFINES) $(RTC)

LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl

# Use this for simulator-only files
INCLUDES = -I. -I$(DRIVERS) -I$(COMMON) -I$(FIRMWAREDIR) -I$(APPDIR) -I$(MACHINEDIR) -I../common -I$(OBJDIR)
SRCDIRS = . $(DRIVERS) $(COMMON) $(FIRMWAREDIR) $(APPDIR) $(MACHINEDIR)

# The true Rockbox Applications should use this include path:
APPINCLUDES = -I$(FIRMWAREDIR)/include $(INCLUDES)

LIBS = -lpthread

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
  INCLUDES += -I/usr/X11R6/include
  LIBDIRS = -L/usr/X11R6/lib
  DEFINES += -DLINUX
  #MPEG_PLAY = 1
else
  DEFINES += -DBIG_ENDIAN
  LIBDIRS = 
endif

ifdef MPEG_PLAY
  INCLUDES += -I$(PREVAPPDIR)/common
endif

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) -W -Wall

APPCFLAGS = $(DEBUG) $(DEFINES) -DAPPSVERSION=\"$(VERSION)\" $(APPINCLUDES) -W -Wall

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
	LCDSRSC = lcd-recorder.c sysfont.c font.c
else
	LCDSRSC = lcd-playersim.c lcd-player.c font-player.c
endif
FIRMSRCS = $(LCDSRSC) sprintf.c id3.c debug.c usb.c mpeg.c power.c\
  powermgmt.c panic.c ctype.c

APPS = main.c tree.c menu.c credits.c main_menu.c language.c\
	playlist.c wps.c wps-display.c settings.c status.c icons.c\
	screens.c peakmeter.c viewer.c

MENUS = games_menu.c demo_menu.c settings_menu.c sound_menu.c

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
   APPS += tetris.c sokoban.c bounce.c snow.c bmp.c widgets.c wormlet.c cube.c
endif

SRCS = screenhack.c uibasic.c resources.c visual.c lcd-x11.c stubs.c \
        button-x11.c io.c thread.c sim_icons.c $(APPS) $(MENUS) $(FIRMSRCS)

ifdef MPEG_PLAY
  SRCS += mpegplay.c oss_sound.c bit.c decoder.c fixed.c frame.c huffman.c layer12.c layer3.c stream.c synth.c timer.c version.c
  DEFINES += -DMPEG_PLAY -DFPM_DEFAULT -DHAVE_CONFIG_H
  INCLUDES += -I$(LIBMADDIR)
endif

OBJS := $(OBJDIR)/lang.o $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(TARGET) $(EXTRA_TARGETS)

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS) $(OBJDIR)/lang.[cho] \
	$(OBJDIR)/build.lang $(OBJDIR)/*.o $(OBJDIR)/sysfont.c
	$(RM) -r $(DEPS)

distclean: clean
	$(RM) config.cache

ifdef MPEG_PLAY
$(OBJDIR)/bit.o: $(LIBMADDIR)/bit.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/decoder.o: $(LIBMADDIR)/decoder.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/fixed.o: $(LIBMADDIR)/fixed.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/frame.o: $(LIBMADDIR)/frame.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/huffman.o: $(LIBMADDIR)/huffman.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/layer12.o: $(LIBMADDIR)/layer12.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/layer3.o: $(LIBMADDIR)/layer3.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/stream.o: $(LIBMADDIR)/stream.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/synth.o: $(LIBMADDIR)/synth.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/timer.o: $(LIBMADDIR)/timer.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/version.o: $(LIBMADDIR)/version.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mpegplay.o: $(PREVAPPDIR)/common/mpegplay.c
	$(CC) $(CFLAGS) -c $< -o $@

endif

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

$(OBJDIR)/games_menu.o: $(APPDIR)/games_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/demo_menu.o: $(APPDIR)/demo_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/settings_menu.o: $(APPDIR)/settings_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/icons.o: $(MACHINEDIR)/icons.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/widgets.o: $(RECDIR)/widgets.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/tetris.o: $(RECDIR)/tetris.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/wormlet.o: $(RECDIR)/wormlet.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/sokoban.o: $(RECDIR)/sokoban.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/bounce.o: $(RECDIR)/bounce.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/cube.o: $(RECDIR)/cube.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/snow.o: $(RECDIR)/snow.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/main.o: $(APPDIR)/main.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/wps.o: $(APPDIR)/wps.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/viewer.o: $(APPDIR)/viewer.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/wps-display.o: $(APPDIR)/wps-display.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/bmp.o: $(RECDIR)/bmp.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/tree.o: $(APPDIR)/tree.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/playlist.o: $(APPDIR)/playlist.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/build.lang: $(APPDIR)/lang/$(LANGUAGE).lang
	perl $(TOOLSDIR)/uplang $(APPDIR)/lang/english.lang $< > $@

$(OBJDIR)/lang.o: $(OBJDIR)/build.lang
	perl -s $(TOOLSDIR)/genlang -p=$(OBJDIR)/lang $<
	$(CC) $(CFLAGS) -c $(OBJDIR)/lang.c -o $@

$(OBJDIR)/lcd-recorder.o: $(DRIVERS)/lcd-recorder.c
	$(CC) $(CFLAGS) -DHAVE_LCD_BITMAP -c $< -o $@

$(OBJDIR)/sysfont.o: $(FIRMWAREDIR)/fonts/clR6x8.bdf
	$(TOOLSDIR)/convbdf -c -o $(OBJDIR)/sysfont.c $<
	$(CC) $(APPCFLAGS) -c $(OBJDIR)/sysfont.c -o $@

$(OBJDIR)/font.o: $(FIRMWAREDIR)/font.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/settings.o: $(APPDIR)/settings.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/status.o: $(APPDIR)/status.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/screens.o: $(APPDIR)/screens.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/peakmeter.o: $(RECDIR)/peakmeter.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/id3.o: $(FIRMWAREDIR)/id3.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/debug.o: $(FIRMWAREDIR)/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/panic.o: $(FIRMWAREDIR)/panic.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mpeg.o: $(FIRMWAREDIR)/mpeg.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sprintf.o: $(COMMON)/sprintf.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/ctype.o: $(COMMON)/ctype.c
	$(CC) $(CFLAGS) $(APPINCLUDES) -c $< -o $@

$(OBJDIR)/stubs.o: ../common/stubs.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sim_icons.o: ../common/sim_icons.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/usb.o: $(FIRMWAREDIR)/usb.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/powermgmt.o: $(FIRMWAREDIR)/powermgmt.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/unicode.o: $(FIRMWAREDIR)/unicode.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/ajf.o: $(FIRMWAREDIR)/ajf.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/power.o: $(DRIVERS)/power.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lcd-playersim.o: ../common/lcd-playersim.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/font-player.o: ../common/font-player.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/lcd-player.o: $(DRIVERS)/lcd-player.c
	$(CC) $(CFLAGS) -c $< -o $@

# these ones are simulator-specific

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -g -o $(TARGET) $(LIBDIRS) $(LDFLAGS) $(OBJS) $(LIBS)

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

tags:
	@$(SHELL) -c 'for d in $(SRCDIRS); do { etags -o $(OBJDIR)/TAGS -a $$d/*.[ch]; }; done'

