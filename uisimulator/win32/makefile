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
SIMDIR = ../win32/
APPDIR = ../../apps/
RECDIR = $(APPDIR)recorder/
RM = del

DISPLAY = -DHAVE_LCD_CHARCELLS
#DISPLAY = -DHAVE_LCD_BITMAP

KEYPAD = -DHAVE_PLAYER_KEYPAD
#KEYPAD = -DHAVE_RECORDER_KEYPAD

CC = cl
RC = rc
LINK = link
DEFINES = -DWIN32 -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR $(KEYPAD) $(DISPLAY)
LDFLAGS = /OUT:uisw32.exe /SUBSYSTEM:windows
INCLUDES = -I$(FIRMWAREDIR) -I$(DRIVERS) -I$(COMMON) -I$(SIMDIR) -I$(APPDIR) -I$(RECDIR)
LIBS = /DEFAULTLIB:gdi32.lib /DEFAULTLIB:user32.lib

CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) /MD /Fd"Release/vc70.pdb" /c

SRCS = *.c \
	$(DRIVERS)/lcd.c \
	$(APPDIR)*.c \
	$(FIRMWAREDIR)/chartables.c $(FIRMWAREDIR)/id3.c $(FIRMWAREDIR)/settings.c $(FIRMWAREDIR)/backlight.c \
        $(COMMON)/sprintf.c $(COMMON)/strtok.c

!IF ("$(DISPLAY)" == "-DHAVE_LCD_BITMAP")
SRCS = $(SRCS) $(RECDIR)*.c
!ENDIF

OBJS = *.obj uisw32.res

uisw32.exe: $(SRCS:.c=.obj)
 $(CC) $(CFLAGS) $(SRCS)
 $(RC) /r uisw32.rc
 $(LINK) $(LIBS) $(LDFLAGS) $(OBJS)

clean:
 $(RM) *.obj
 $(RM) uisw32.res