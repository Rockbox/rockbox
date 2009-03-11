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

#ifndef BACKLIGHT_SW_FADING_H
#define BACKLIGHT_SW_FADING_H


/* total fading time will be current brightness level * FADE_DELAY * 10ms */
#if (MAX_BRIGHTNESS_SETTING >= 25)
#define FADE_DELAY 2 /* =HZ/50 => 20ms */
#elif (MAX_BRIGHTNESS_SETTING >= 16)
#define FADE_DELAY 3 /* =HZ/33 => 30ms */
#else
#define FADE_DELAY 4 /* =HZ/25 => 40ms*/
#endif


void _backlight_fade_update_state(int brightness);
bool _backlight_fade_step(int direction);

/* enum used for both, fading state and fading type selected through the settings */

enum {
    NOT_FADING = 0,
    FADING_UP,
    FADING_DOWN,
};

#endif /* BACKLIGHT_SW_FADING_H */
