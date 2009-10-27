/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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

#include <stdbool.h>
#include "config.h"


bool button_hold(void);
void button_init_device(void);
int button_read_device(int*);
void touchpad_set_sensitivity(int level);

/* Mini2440 specific button codes */

#define BUTTON_ONE          0x0001
#define BUTTON_TWO          0x0008
#define BUTTON_THREE        0x0020
#define BUTTON_FOUR         0x0040
#define BUTTON_FIVE         0x0080
#define BUTTON_SIX          0x0800

/* Add on buttons */
#define BUTTON_SEVEN        0x0200
#define BUTTON_EIGHT        0x0400

/* Touch Screen Area Buttons */
#define BUTTON_TOPLEFT      0x010000
#define BUTTON_TOPMIDDLE    0x020000
#define BUTTON_TOPRIGHT     0x040000
#define BUTTON_MIDLEFT      0x080000
#define BUTTON_CENTER       0x100000
#define BUTTON_MIDRIGHT     0x200000
#define BUTTON_BOTTOMLEFT   0x400000
#define BUTTON_BOTTOMMIDDLE 0x800000
#define BUTTON_BOTTOMRIGHT  0x100000

#define BUTTON_TOUCH        0x200000


#define BUTTON_MENU         BUTTON_ONE         
#define BUTTON_UP           BUTTON_TWO
#define BUTTON_SELECT       BUTTON_THREE
#define BUTTON_DOWN         BUTTON_FOUR
#define BUTTON_LEFT         BUTTON_FIVE
#define BUTTON_RIGHT        BUTTON_SIX

/* Add on buttons */
#define BUTTON_A            BUTTON_SEVEN
#define BUTTON_POWER        BUTTON_EIGHT

/* TODO: bodge to keep keymap-mini2440 happy */
#define BUTTON_VOL_DOWN     0x4000
#define BUTTON_VOL_UP       0x8000

#define BUTTON_MAIN (BUTTON_MENU|BUTTON_LEFT|BUTTON_RIGHT  | \
                     BUTTON_UP  |BUTTON_DOWN|BUTTON_SELECT | \
                     BUTTON_A   |BUTTON_POWER )

#define BUTTON_REMOTE       0

#define POWEROFF_BUTTON     BUTTON_MENU
#define POWEROFF_COUNT      10

#endif /* _BUTTON_TARGET_H_ */
