#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2002 by Daniel Stenberg
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

TARGET = libdmalloc.a

LIBOBJS = dmalloc.o bmalloc.o bysize.o

# define this to talk a lot in runtime
# -DDEBUG_VERBOSE 
CFLAGS = -g -W -Wall -DDEBUG_MALLOC
CC = gcc
AR = ar

LDFLAGS = -L. -ldmalloc

all: $(TARGET)

clean:
	rm -f core *~ $(TARGET) $(LIBOBJS)

$(TARGET): $(LIBOBJS)
	$(AR) ruv $(TARGET) $(LIBOBJS)

bmalloc.o: bmalloc.c bysize.h
bysize.o: bysize.c
dmalloc.o: dmalloc.c
