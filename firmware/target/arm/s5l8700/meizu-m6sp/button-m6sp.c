/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Bertrik Sikken
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

#include <stdbool.h>
#include "config.h"

#include "inttypes.h"
#include "s5l8700.h"
#include "button-target.h"

/*  Button driver for the meizu M6SP
    
    Future improvements:
    * touch strip support
    * left/right buttons (probably part of touch strip)
    * unify with m3/m6sl button driver if possible

 */


void button_init_device(void)
{
    PCON0 &= ~(0x3 << 10);  /* P0.5 hold switch */
    PCON0 &= ~(0x3 << 14);  /* P0.7 enter button */
    PCON1 &= ~(0xF << 12);  /* P1.3 menu button */
    PCON1 &= ~(0xF << 16);  /* P1.4 play button */
}

int button_read_device(void)
{
    int buttons = 0;

    if (button_hold()) {
        return 0;
    }

    if ((PDAT0 & (1 << 7)) == 0) {
        buttons |= BUTTON_ENTER;
    }
    if ((PDAT1 & (1 << 3)) == 0) {
        buttons |= BUTTON_MENU;
    }
    if ((PDAT1 & (1 << 4)) == 0) {
        buttons |= BUTTON_PLAY;
    }
    
    return buttons;
}

bool button_hold(void)
{
    return ((PDAT0 & (1 << 5)) != 0);
}

