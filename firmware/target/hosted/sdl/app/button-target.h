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
/*

#define HAS_BUTTON_HOLD

bool button_hold(void);
*/
void button_init_device(void);
#ifdef HAVE_BUTTON_DATA
int button_read_device(int *data);
#else
int button_read_device(void);
#endif

/* Main unit's buttons */
#define BUTTON_UP           0x00000001
#define BUTTON_DOWN         0x00000002
#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_SELECT       0x00000010
#define BUTTON_MENU         0x00000020
#define BUTTON_BACK         0x00000040
#define BUTTON_SCROLL_FWD   0x00000100
#define BUTTON_SCROLL_BACK  0x00000200

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

#define BUTTON_MAIN 0x1FFF

/* No remote */
#define BUTTON_REMOTE 0

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10
                
#endif /* _BUTTON_TARGET_H_ */
