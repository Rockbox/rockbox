############################################################################
#             __________               __   ___.                  
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/ 
# $Id$
#
# Copyright (C) 2002 by Felix Arends
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

TARGET = rockboxui

FIRMWAREDIR = ../../firmware
DRIVERS = $(FIRMWAREDIR)/drivers
COMMON = $(FIRMWAREDIR)/common

CC = cl
DEFINES = -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR -DARCHOS_RECORDER -D_WIN32 -DWIN32
LDFLAGS = /ouisw32.exe /link -subsystem:windows
INCLUDES = -I$(FIRMWAREDIR) -I$(DRIVERS) -I$(COMMON)
LIBS = gdi32.lib user32.lib


CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) /MD /Fd"Release/vc70.pdb"

#SRCS = $(wildcard *.c)

SRCS = lcd-win32.c tetris.c uisw32.c lcd.c button.c \
  chartables.c kernel.c Release/uisw32.res 

OBJS = $(SRCS:.c=.obj)

uisw32.exe: $(OBJS)
 $(CC) $(CFLAGS) $(LIBS) $(LDFLAGS) $(OBJS)
 
lcd.obj: $(DRIVERS)/lcd.c
 $(CC) $(CFLAGS) -c $(DRIVERS)/lcd.c -olcd.obj
 
chartables.obj: $(FIRMWAREDIR)/chartables.c
 $(CC) $(CFLAGS) -c $(FIRMWAREDIR)/chartables.c -ochartables.obj