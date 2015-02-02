/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_


/* Hardware buttons. */
#define BUTTON_LEFT        0x00000001
#define BUTTON_RIGHT       0x00000002
#define BUTTON_PLAY        0x00000004
#define BUTTON_POWER       0x00000008
#define BUTTON_VOL_UP      0x00000010
#define BUTTON_VOL_DOWN    0x00000020
#define BUTTON_POWER_LONG  0x00000040

#define BUTTON_MAIN (  BUTTON_LEFT  | BUTTON_VOL_UP | BUTTON_VOL_DOWN  | BUTTON_RIGHT \
                     | BUTTON_PLAY  | BUTTON_POWER  | BUTTON_POWER_LONG)


#define STATE_SPDIF_UNPLUGGED   32
#define STATE_LINEOUT_UNPLUGGED 64


/* Touchscreen area buttons 3x3 grid. */
#define BUTTON_TOPLEFT      0x00001000
#define BUTTON_TOPMIDDLE    0x00002000
#define BUTTON_TOPRIGHT     0x00004000
#define BUTTON_MIDLEFT      0x00008000
#define BUTTON_CENTER       0x00010000
#define BUTTON_MIDRIGHT     0x00020000
#define BUTTON_BOTTOMLEFT   0x00040000
#define BUTTON_BOTTOMMIDDLE 0x00080000
#define BUTTON_BOTTOMRIGHT  0x00100000


/* Power-off */
#define POWEROFF_BUTTON BUTTON_POWER_LONG
#define POWEROFF_COUNT  0


#endif
