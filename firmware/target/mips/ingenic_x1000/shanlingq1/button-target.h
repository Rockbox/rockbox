/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __BUTTON_TARGET_H__
#define __BUTTON_TARGET_H__

#include <stdbool.h>

/* physical buttons */
#define BUTTON_POWER        0x00000001
#define BUTTON_VOL_UP       0x00000002 /* up = wheel clockwise */
#define BUTTON_VOL_DOWN     0x00000004
#define BUTTON_PLAY         0x00000008 /* circle */
#define BUTTON_NEXT         0x00000010 /* down */
#define BUTTON_PREV         0x00000020 /* up */

/* compatibility hacks */
#define BUTTON_LEFT         BUTTON_MIDLEFT
#define BUTTON_RIGHT        BUTTON_MIDRIGHT

/* touchscreen "buttons" */
#define BUTTON_TOPLEFT      0x00000040
#define BUTTON_TOPMIDDLE    0x00000080
#define BUTTON_TOPRIGHT     0x00000100
#define BUTTON_MIDLEFT      0x00000200
#define BUTTON_CENTER       0x00000400
#define BUTTON_MIDRIGHT     0x00000800
#define BUTTON_BOTTOMLEFT   0x00001000
#define BUTTON_BOTTOMMIDDLE 0x00002000
#define BUTTON_BOTTOMRIGHT  0x00004000

#define BUTTON_MAIN (BUTTON_POWER|BUTTON_VOL_UP|BUTTON_VOL_DOWN|\
                     BUTTON_PLAY|BUTTON_NEXT|BUTTON_PREV)

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT  30

#endif /* __BUTTON_TARGET_H__ */
