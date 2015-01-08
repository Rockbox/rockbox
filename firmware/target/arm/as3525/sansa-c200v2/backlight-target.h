/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Barry Wardell
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

bool backlight_hw_init(void);
void _backlight_pwm(int on);
void backlight_hw_on(void);
void backlight_hw_off(void);

static inline void _backlight_panic_on(void)
{
    backlight_hw_on();
    _backlight_pwm(1);
}

void backlight_hw_brightness(int brightness);
int  __backlight_is_on(void);

void buttonlight_hw_on(void);
void buttonlight_hw_off(void);

/*
 * FIXME: This may be better off in kernel.h, but...
 */
void _set_timer2_pwm_ratio(int ratio);

#endif
