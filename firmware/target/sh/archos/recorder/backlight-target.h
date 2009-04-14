/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
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

#include "config.h"
#include "rtc.h"

#define _backlight_init() true

static inline void _backlight_on(void)
{
    rtc_write(0x13, 0x10); /* 32 kHz square wave */
    rtc_write(0x0a, rtc_read(0x0a) | 0x40); /* Enable square wave */
}

static inline void _backlight_off(void)
{
    /* While on, backlight is flashing at 32 kHz.  If the square wave output
       is disabled while the backlight is lit, it will become constantly lit,
       (brighter) and slowly fade.  This resets the square wave counter and
       results in the unlit state */
    unsigned char rtc_0a = rtc_read(0x0a) & ~0x40;
    rtc_write(0x0a, rtc_0a);        /* Disable square wave */
    rtc_write(0x13, 0xF0);          /* 1 Hz square wave */
    rtc_write(0x0a, rtc_0a | 0x40); /* Enable square wave */

    /* When the square wave output is disabled in the unlit state,
       the backlight stays off */
    rtc_write(0x0a, rtc_0a);
}

#endif
