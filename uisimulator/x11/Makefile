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

APPDIR= ..
FIRMWAREDIR = ../../firmware
DRIVERS = $(FIRMWAREDIR)/drivers
COMMON = $(FIRMWAREDIR)/common

CC = gcc
RM = rm -f
DEBUG = -g

#DISPLAY = -DHAVE_LCD_CHARCELLS
DISPLAY = -DHAVE_LCD_BITMAP

DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR	\
-DHAVE_RECORDER_KEYPAD $(DISPLAY)

LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl

INCLUDES = -I. -I$(DRIVERS) -I$(FIRMWAREDIR)

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
  INCLUDES += -I/usr/X11R6/include
  LIBDIRS = -L/usr/X11R6/lib
else
  LIBDIRS = 
endif

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES)

#SRCS = $(wildcard *.c)

FIRMSRCS = chartables.c lcd.c sprintf.c id3.c debug.c

APPS = tetris.c screensaver.c tree.c app.c play.c menu.c icons.c

SRCS = screenhack.c uibasic.c resources.c visual.c lcd-x11.c \
        button-x11.c io.c sleep.c $(APPS) $(FIRMSRCS)

OBJS := $(SRCS:c=o)

all: $(TARGET)

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS)
	$(RM) -r $(DEPS)

distclean: clean
	$(RM) config.cache

menu.o: $(APPDIR)/menu.c
	$(CC) $(CFLAGS) -c $< -o $@

icons.o: $(APPDIR)/icons.c
	$(CC) $(CFLAGS) -c $< -o $@

tetris.o: $(APPDIR)/tetris.c
	$(CC) $(CFLAGS) -c $< -o $@

screensaver.o: $(APPDIR)/screensaver.c
	$(CC) $(CFLAGS) -c $< -o $@

app.o: $(APPDIR)/app.c
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
	$(CC) -g -o $(TARGET) $(LIBDIRS) $(LDFLAGS) $(OBJS)

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
