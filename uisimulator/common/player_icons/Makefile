############################################################################
#             __________               __   ___.                  
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/ 
# $Id$
#
# Copyright (C) 2002 by Mats Lidell <matsl@contactor.se>
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################


BMP2RB = ../../../tools/bmp2rb

SUFFIXES := .bmp

%.h: %.bmp
	$(BMP2RB) $< > $@

BITMAPS = battery.bmp battery_bit.bmp \
	pause.bmp play.bmp record.bmp \
	volume.bmp volume1.bmp volume2.bmp volume3.bmp volume4.bmp volume5.bmp \
	usb.bmp audio.bmp param.bmp \
	repeat.bmp repeat1.bmp

SRCS := $(BITMAPS:%.bmp=%.h)

all:	$(SRCS)
	cat $(SRCS) > grand_total_of_player_icons.c

clean:
	rm $(SRCS) grand_total_of_player_icons.c

