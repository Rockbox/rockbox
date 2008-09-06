/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Vitja Makarov
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
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

#include <stdbool.h>
#include "tcc77x.h"

void power_touch_panel(bool on);

static inline bool _backlight_init(void)
{
    GPIOD_DIR |= 0x2;
    return true;
}

static inline void _backlight_on(void)
{
    GPIOD |= 0x2;
    power_touch_panel(true);
}

static inline void _backlight_off(void)
{
    GPIOD &= ~0x2;
    power_touch_panel(false);
}
#endif /* BACKLIGHT_TARGET_H */
