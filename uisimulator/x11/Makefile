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

PREVAPPDIR= ..
FIRMWAREDIR = ../../firmware
DRIVERS = $(FIRMWAREDIR)/drivers
COMMON = $(FIRMWAREDIR)/common
LIBMADDIR = $(PREVAPPDIR)/common/libmad

CC = gcc
RM = rm -f
DEBUG = -g

# where to put all output files
OBJDIR = .

TARGET = $(OBJDIR)/rockboxui

#DISPLAY = -DHAVE_LCD_CHARCELLS
DISPLAY = -DHAVE_LCD_BITMAP

#KEYPAD = -DHAVE_PLAYER_KEYPAD
KEYPAD = -DHAVE_RECORDER_KEYPAD

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR	\
$(KEYPAD) $(DISPLAY) #-DLCD_PROPFONTS

LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl

# Use this for simulator-only files
INCLUDES = -I. -I$(DRIVERS) -I$(COMMON) -I$(FIRMWAREDIR) -I$(APPDIR) -I$(RECDIR)

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

FIRMSRCS = chartables.c lcd.c sprintf.c id3.c debug.c usb.c mpeg.c power.c

APPS = main.c tree.c menu.c credits.c main_menu.c\
	playlist.c showtext.c wps.c settings.c status.c

MENUS = games_menu.c screensavers_menu.c settings_menu.c sound_menu.c

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
   APPS += tetris.c sokoban.c bounce.c boxes.c icons.c bmp.c
endif

SRCS = screenhack.c uibasic.c resources.c visual.c lcd-x11.c stubs.c \
        button-x11.c io.c thread.c $(APPS) $(MENUS) $(FIRMSRCS)

ifdef MPEG_PLAY
  SRCS += mpegplay.c oss_sound.c bit.c decoder.c fixed.c frame.c huffman.c layer12.c layer3.c stream.c synth.c timer.c version.c
  DEFINES += -DMPEG_PLAY -DFPM_DEFAULT -DHAVE_CONFIG_H
  INCLUDES += -I$(LIBMADDIR)
endif

OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(TARGET)

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS)
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

$(OBJDIR)/credits.o: $(APPDIR)/credits.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/menu.o: $(APPDIR)/menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/main_menu.o: $(APPDIR)/main_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/sound_menu.o: $(APPDIR)/sound_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/games_menu.o: $(APPDIR)/games_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/screensavers_menu.o: $(APPDIR)/screensavers_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/settings_menu.o: $(APPDIR)/settings_menu.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/icons.o: $(RECDIR)/icons.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/tetris.o: $(RECDIR)/tetris.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/sokoban.o: $(RECDIR)/sokoban.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/bounce.o: $(RECDIR)/bounce.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/boxes.o: $(RECDIR)/boxes.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/main.o: $(APPDIR)/main.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/wps.o: $(APPDIR)/wps.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/bmp.o: $(RECDIR)/bmp.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/tree.o: $(APPDIR)/tree.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/playlist.o: $(APPDIR)/playlist.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/showtext.o: $(APPDIR)/showtext.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/lcd.o: $(DRIVERS)/lcd.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/chartables.o: $(FIRMWAREDIR)/chartables.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/settings.o: $(APPDIR)/settings.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/status.o: $(APPDIR)/status.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/id3.o: $(FIRMWAREDIR)/id3.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/debug.o: $(FIRMWAREDIR)/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mpeg.o: $(FIRMWAREDIR)/mpeg.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sprintf.o: $(COMMON)/sprintf.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/usb.o: $(FIRMWAREDIR)/usb.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/stubs.o: ../common/stubs.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/power.o: $(DRIVERS)/power.c
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
