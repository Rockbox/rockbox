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

CC = i386-mingw32msvc-gcc
WINDRES = i386-mingw32msvc-windres
RM = rm -f
DEBUG = -g

# where to put all output files
OBJDIR = .

TARGET = $(OBJDIR)/uisw32.exe

#DISPLAY = -DHAVE_LCD_CHARCELLS
DISPLAY = -DHAVE_LCD_BITMAP

#KEYPAD = -DHAVE_PLAYER_KEYPAD
KEYPAD = -DHAVE_RECORDER_KEYPAD

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR	\
$(KEYPAD) $(DISPLAY) -DAPPSVERSION=\"$(VERSION)\"

LDFLAGS =  -lgdi32 -luser32

# Use this for simulator-only files
INCLUDES = -I. -I$(DRIVERS) -I$(COMMON) -I$(FIRMWAREDIR) -I$(APPDIR) -I$(RECDIR)

# The true Rockbox Applications should use this include path:
APPINCLUDES = -I$(FIRMWAREDIR)/include $(INCLUDES)

DEFINES += -DWIN32

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) -W -Wall

APPCFLAGS = $(DEBUG) $(DEFINES) $(APPINCLUDES) -W -Wall

FIRMSRCS = chartables.c lcd.c power.c sprintf.c id3.c backlight.c usb.c \
        mpeg.c powermgmt.c

APPS = main.c tree.c menu.c credits.c main_menu.c\
	playlist.c showtext.c wps.c settings.c status.c

MENUS = games_menu.c screensavers_menu.c settings_menu.c sound_menu.c

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
   APPS += tetris.c sokoban.c blank.c bounce.c boxes.c icons.c bmp.c widgets.c
endif

SRCS = button.c dir-win32.c lcd-win32.c panic-win32.c thread-win32.c \
	debug-win32.c kernel.c string-win32.c uisw32.c \
	$(APPS) $(MENUS) $(FIRMSRCS) strtok.c

OBJS := $(SRCS:%.c=$(OBJDIR)/%.o) $(OBJDIR)/uisw32-res.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/uisw32-res.o: uisw32.rc
	$(WINDRES) -i $< -o $@

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS) $(OBJDIR)/uisw32-res.o
	$(RM) -r $(DEPS)

distclean: clean
	$(RM) config.cache

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

$(OBJDIR)/widgets.o: $(RECDIR)/widgets.c
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
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/power.o: $(DRIVERS)/power.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/chartables.o: $(FIRMWAREDIR)/chartables.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/usb.o: $(FIRMWAREDIR)/usb.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/powermgmt.o: $(FIRMWAREDIR)/powermgmt.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/settings.o: $(APPDIR)/settings.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/id3.o: $(FIRMWAREDIR)/id3.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/status.o: $(APPDIR)/status.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/debug.o: $(FIRMWAREDIR)/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/mpeg.o: $(FIRMWAREDIR)/mpeg.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/sprintf.o: $(COMMON)/sprintf.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/strtok.o: $(COMMON)/strtok.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/backlight.o: $(FIRMWAREDIR)/backlight.c
	$(CC) $(APPCFLAGS) -c $< -o $@


# these ones are simulator-specific

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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
