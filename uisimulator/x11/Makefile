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

TARGET = rockboxui

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

#DISPLAY = -DHAVE_LCD_CHARCELLS
DISPLAY = -DHAVE_LCD_BITMAP

#KEYPAD = -DHAVE_PLAYER_KEYPAD
KEYPAD = -DHAVE_RECORDER_KEYPAD

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR	\
$(KEYPAD) $(DISPLAY)

LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl

INCLUDES = -I. -I$(DRIVERS) -I$(COMMON) -I$(FIRMWAREDIR) -I$(APPDIR) -I$(RECDIR)

LIBS = 

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
  INCLUDES += -I/usr/X11R6/include
  LIBDIRS = -L/usr/X11R6/lib
  DEFINES += -DLINUX
 # MPEG_PLAY = 1
else
  DEFINES += -DBIG_ENDIAN
  LIBDIRS = 
endif

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES)

#SRCS = $(wildcard *.c)

FIRMSRCS = chartables.c lcd.c sprintf.c id3.c debug.c

APPS = main.c tree.c play.c menu.c credits.c main_menu.c

ifeq ($(DISPLAY),-DHAVE_LCD_BITMAP)
   APPS += tetris.c screensaver.c icons.c bmp.c
endif

SRCS = screenhack.c uibasic.c resources.c visual.c lcd-x11.c \
        button-x11.c io.c sleep.c $(APPS) $(FIRMSRCS)

ifdef MPEG_PLAY
  SRCS += mpegplay.c oss_sound.c bit.c decoder.c fixed.c frame.c huffman.c layer12.c layer3.c stream.c synth.c timer.c version.c
  DEFINES += -DMPEG_PLAY -DFPM_DEFAULT -DHAVE_CONFIG_H
  INCLUDES += -I$(LIBMADDIR)
endif

OBJS := $(SRCS:c=o)

all: $(TARGET)

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS)
	$(RM) -r $(DEPS)

distclean: clean
	$(RM) config.cache

ifdef MPEG_PLAY
bit.o: $(LIBMADDIR)/bit.c
	$(CC) $(CFLAGS) -c $< -o $@

decoder.o: $(LIBMADDIR)/decoder.c
	$(CC) $(CFLAGS) -c $< -o $@

fixed.o: $(LIBMADDIR)/fixed.c
	$(CC) $(CFLAGS) -c $< -o $@

frame.o: $(LIBMADDIR)/frame.c
	$(CC) $(CFLAGS) -c $< -o $@

huffman.o: $(LIBMADDIR)/huffman.c
	$(CC) $(CFLAGS) -c $< -o $@

layer12.o: $(LIBMADDIR)/layer12.c
	$(CC) $(CFLAGS) -c $< -o $@

layer3.o: $(LIBMADDIR)/layer3.c
	$(CC) $(CFLAGS) -c $< -o $@

stream.o: $(LIBMADDIR)/stream.c
	$(CC) $(CFLAGS) -c $< -o $@

synth.o: $(LIBMADDIR)/synth.c
	$(CC) $(CFLAGS) -c $< -o $@

timer.o: $(LIBMADDIR)/timer.c
	$(CC) $(CFLAGS) -c $< -o $@

version.o: $(LIBMADDIR)/version.c
	$(CC) $(CFLAGS) -c $< -o $@

mpegplay.o: $(PREVAPPDIR)/common/mpegplay.c
	$(CC) $(CFLAGS) -c $< -o $@

endif

credits.o: $(APPDIR)/credits.c
	$(CC) $(CFLAGS) -c $< -o $@

menu.o: $(APPDIR)/menu.c
	$(CC) $(CFLAGS) -c $< -o $@

main_menu.o: $(APPDIR)/main_menu.c
	$(CC) $(CFLAGS) -c $< -o $@

icons.o: $(RECDIR)/icons.c
	$(CC) $(CFLAGS) -c $< -o $@

tetris.o: $(RECDIR)/tetris.c
	$(CC) $(CFLAGS) -c $< -o $@

screensaver.o: $(RECDIR)/screensaver.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: $(APPDIR)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

bmp.o: $(RECDIR)/bmp.c
	$(CC) $(CFLAGS) -c $< -o $@

play.o: $(APPDIR)/play.c
	$(CC) $(CFLAGS) -c $< -o $@

tree.o: $(APPDIR)/tree.c
	$(CC) $(CFLAGS) -c $< -o $@

lcd.o: $(DRIVERS)/lcd.c
	$(CC) $(CFLAGS) -c $< -o $@

chartables.o: $(FIRMWAREDIR)/chartables.c
	$(CC) $(CFLAGS) -c $< -o $@

id3.o: $(FIRMWAREDIR)/id3.c
	$(CC) $(CFLAGS) -c $< -o $@

debug.o: $(FIRMWAREDIR)/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

sprintf.o: $(COMMON)/sprintf.c
	$(CC) $(CFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJS)
	$(CC) -g -o $(TARGET) $(LIBDIRS) $(LDFLAGS) $(OBJS) $(LIBS)

DEPS:=.deps

$(DEPS)/%.d: %.c
		@$(SHELL) -c 'if [ ! -d $(DEPS) ]; then \
			echo Creating the dependency directory: $(DEPS); \
			mkdir $(DEPS); fi'
		@echo "Updating Dependencies for $<"
		@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $< \
	|sed '\''s/\($*\)\.o[ :]*/\1.o $(<:%.c=%.d) : /g'\'' > $@; \
	[ -s $@ ] || rm -f $@'

-include $(SRCS:%.c=$(DEPS)/%.d)
