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

FIRMWAREDIR = ../firmware
DRIVERS = $(FIRMWAREDIR)/drivers

CC = gcc
RM = rm -f
DEBUG = -g
DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR -DHAVE_LCD_BITMAP 
LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl


UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
  INCLUDES = -I/usr/X11R6/include -I$(DRIVERS) -I$(FIRMWAREDIR)
  LIBDIRS = -L/usr/X11R6/lib
else
  INCLUDES = -I$(FIRMWAREDIR) 
  LIBDIRS = 
endif

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES)

#SRCS = $(wildcard *.c)

SRCS = screenhack.c uibasic.c resources.c visual.c lcd.c lcd-x11.c \
        button-x11.c chartables.c tetris.c app.c

OBJS := $(SRCS:c=o)

all: $(TARGET)

clean:
	$(RM) $(OBJS) *~ core $(TARGET) $(CLIENTS)
	$(RM) -r $(DEPS)

distclean: clean
	$(RM) config.cache

lcd.o: $(DRIVERS)/lcd.c
	$(CC) $(CFLAGS) -c $< -o $@

chartables.o: $(FIRMWAREDIR)/chartables.c
	$(CC) $(CFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(LIBDIRS) $(LDFLAGS) $(OBJS)

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
