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

FIRMWAREDIR=../firmware

CC = gcc
RM = rm

CFLAGS = -g
CPPFLAGS =  -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR -DHAVE_LCD_BITMAP -I$(FIRMWAREDIR)
LDFLAGS = -lX11 -lm -lXt -lXmu -lnsl
INCLUDES = 
LIBDIRS = 

OBJS=   screenhack.o uibasic.o resources.o visual.o lcd.o lcd-x11.o chartables.o

SRCS = $(OBJS:%.o=%.c)
HDRS = $(OBJS:%.o=%.h)

all: $(TARGET)

clean:
	$(RM) -f $(OBJS) *~ core $(TARGET) $(CLIENTS)

distclean: clean
	$(RM) config.cache

lcd.o: $(FIRMWAREDIR)/lcd.c
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -c $< -o $@

chartables.o: $(FIRMWAREDIR)/chartables.c
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -c $<

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(LIBDIRS) $(LDFLAGS) $(OBJS)


lcd-x11.o: lcd-x11.c screenhack.h config.h resources.h visual.h lcd-x11.h
resources.o: resources.c utils.h resources.h
screenhack.o: screenhack.c xmu.h screenhack.h config.h resources.h \
  visual.h version.h vroot.h
uibasic.o: uibasic.c screenhack.h config.h resources.h visual.h version.h
visual.o: visual.c utils.h resources.h visual.h
