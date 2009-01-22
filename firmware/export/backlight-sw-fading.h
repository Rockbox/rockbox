/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Thomas Martitz
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

#ifndef BACKLIGHT_THREAD_FADING_H
#define BACKLIGHT_THREAD_FADING_H

#include "config.h"

#ifdef USE_BACKLIGHT_SW_FADING

/* delay supposed to be MAX_BRIGHTNESS_SETTING*2 rounded to the next multiple
 * of 5, however not more than 40 */
#define _FADE_DELAY (((MAX_BRIGHTNESS_SETTING*2+4)/5)*5)
#define FADE_DELAY (HZ/(MIN(_FADE_DELAY, 40)))

void _backlight_fade_update_state(int brightness);
bool _backlight_fade_step(int direction);

/* enum used for both, fading state and fading type selected through the settings */

enum {
    NOT_FADING = 0,
    FADING_UP,
    FADING_DOWN,
};
#endif /* USE_BACKLIGHT_SW_FADING */

#endif /* _BACKLIGHT_THREAD_FADING_ */
