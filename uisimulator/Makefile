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

CC = gcc
RM = rm

CFLAGS = -g -O2 
CPPFLAGS =  -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS
LDFLAGS = -lX11 -lm -lXt -lXmu -lsocket -lnsl

DEPEND = .depends

OBJS=   alpha.o hsv.o screenhack.o yarandom.o uibasic.o resources.o visual.o\
	usleep.o lcd.o lcd-x11.o

SRCS = $(OBJS:%.o=%.c)
HDRS = $(OBJS:%.o=%.h)

all: $(DEPEND) $(TARGET)

clean:
	$(RM) -f $(OBJS) *~ core $(TARGET) $(CLIENTS) $(DEPEND)

distclean: clean
	$(RM) config.cache

.c.o:
	$(CC) $(CPPFLAGS) $(CCFLAGS) -c $<

$(DEPEND):
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPEND)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJS)

# Dependencies
include $(DEPEND)
