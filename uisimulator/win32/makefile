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

#DISPLAY = -DHAVE_LCD_CHARCELLS
DISPLAY = -DHAVE_LCD_BITMAP

#KEYPAD = -DHAVE_PLAYER_KEYPAD
KEYPAD = -DHAVE_RECORDER_KEYPAD

CC = cl
DEFINES = -DWIN32 -DHAVE_CONFIG_H -DGETTIMEOFDAY_TWO_ARGS -DSIMULATOR	\
$(KEYPAD) $(DISPLAY)
LDFLAGS = /ouisw32.exe /link -subsystem:windows
INCLUDES = -I$(FIRMWAREDIR) -I$(DRIVERS) -I$(COMMON) -I$(SIMDIR) -I$(APPDIR) -I$(RECDIR)
LIBS = gdi32.lib user32.lib


CFLAGS = $(DEBUG) $(DEFINES) $(INCLUDES) /MD /Fd"Release/vc70.pdb"

SRCS = lcd-win32.c uisw32.c lcd.c button.c tree.c main.c \
  chartables.c kernel.c uisw32.res dir-win32.c main_menu.c \
  play.c debug-win32.c menu.c credits.c sound_menu.c mpeg.c \
  id3.c settings.c sprintf.c

!IF ("$(DISPLAY)" == "-DHAVE_LCD_BITMAP")
SRCS = $(SRCS) tetris.c screensaver.c icons.c bmp.c
!ENDIF

OBJS = $(SRCS:.c=.obj)

uisw32.exe: $(OBJS)
 $(CC) $(CFLAGS) $(LIBS) $(LDFLAGS) $(OBJS)
 
lcd.obj: $(DRIVERS)/lcd.c
 $(CC) $(CFLAGS) -c $(DRIVERS)/lcd.c -olcd.obj
 
chartables.obj: $(FIRMWAREDIR)/chartables.c
 $(CC) $(CFLAGS) -c $(FIRMWAREDIR)/chartables.c -ochartables.obj

tetris.obj: ../../apps/recorder/tetris.c
 $(CC) $(CFLAGS) -c ../../apps/recorder/tetris.c -otetris.obj

tree.obj: ../../apps/tree.c
 $(CC) $(CFLAGS) -c ../../apps/tree.c -otree.obj

main.obj: ../../apps/main.c
 $(CC) $(CFLAGS) -c ../../apps/main.c -omain.obj
 
main_menu.obj: ../../apps/main_menu.c
 $(CC) $(CFLAGS) -c ../../apps/main_menu.c -omain_menu.obj

play.obj: ../../apps/play.c
 $(CC) $(CFLAGS) -c ../../apps/play.c -oplay.obj

bmp.obj: ../../apps/recorder/bmp.c
 $(CC) $(CFLAGS) -c ../../apps/recorder/bmp.c -obmp.obj

screensaver.obj: ../../apps/recorder/screensaver.c
 $(CC) $(CFLAGS) -c ../../apps/recorder/screensaver.c -oscreensaver.obj

menu.obj: ../../apps/menu.c
 $(CC) $(CFLAGS) -c ../../apps/menu.c -omenu.obj

credits.obj: ../../apps/credits.c
 $(CC) $(CFLAGS) -c ../../apps/credits.c -ocredits.obj

icons.obj: ../../apps/recorder/icons.c
 $(CC) $(CFLAGS) -c ../../apps/recorder/icons.c -oicons.obj

sound_menu.obj: ../../apps/sound_menu.c
 $(CC) $(CFLAGS) -c ../../apps/sound_menu.c -osound_menu.obj

id3.obj: $(FIRMWAREDIR)/id3.c
 $(CC) $(CFLAGS) -c $(FIRMWAREDIR)/id3.c -oid3.obj

settings.obj: $(FIRMWAREDIR)/settings.c
 $(CC) $(CFLAGS) -c $(FIRMWAREDIR)/settings.c -osettings.obj

sprintf.obj: $(FIRMWAREDIR)/common/sprintf.c
 $(CC) $(CFLAGS) -c $(FIRMWAREDIR)/common/sprintf.c -osprintf.obj

clean:
 $(RM) *.obj