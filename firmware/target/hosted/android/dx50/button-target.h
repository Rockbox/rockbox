/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.r
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>

#define HAS_BUTTON_HOLD

/* Main unit's buttons */
#define BUTTON_LEFT        0x00000001
#define BUTTON_RIGHT       0x00000002
#define BUTTON_PLAY        0x00000004
#define BUTTON_POWER       0x00000008
#define BUTTON_VOL_UP      0x00000010
#define BUTTON_VOL_DOWN    0x00000020
#define BUTTON_POWER_LONG  0x00000040

#define BUTTON_MAIN (BUTTON_LEFT|BUTTON_VOL_UP|BUTTON_VOL_DOWN\
                    |BUTTON_RIGHT|BUTTON_PLAY|BUTTON_POWER|BUTTON_POWER_LONG)

#define KEYCODE_LINEOUT 113
#define KEYCODE_SPDIF 114
#define KEYCODE_HOLD 115
#define KEYCODE_PWR 116
#define KEYCODE_PWR_LONG 117
#define KEYCODE_SD 143
#define KEYCODE_VOLPLUS 158
#define KEYCODE_VOLMINUS 159
#define KEYCODE_PREV 160
#define KEYCODE_NEXT 162
#define KEYCODE_PLAY 161
#define STATE_UNLOCKED 16
#define STATE_SPDIF_UNPLUGGED 32
#define STATE_LINEOUT_UNPLUGGED 64

/* Touch Screen Area Buttons */
#define BUTTON_TOPLEFT      0x00001000
#define BUTTON_TOPMIDDLE    0x00002000
#define BUTTON_TOPRIGHT     0x00004000
#define BUTTON_MIDLEFT      0x00008000
#define BUTTON_CENTER       0x00010000
#define BUTTON_MIDRIGHT     0x00020000
#define BUTTON_BOTTOMLEFT   0x00040000
#define BUTTON_BOTTOMMIDDLE 0x00080000
#define BUTTON_BOTTOMRIGHT  0x00100000


/* Software power-off */
#define POWEROFF_BUTTON         BUTTON_POWER_LONG
#define POWEROFF_COUNT          0

#endif /* _BUTTON_TARGET_H_ */
