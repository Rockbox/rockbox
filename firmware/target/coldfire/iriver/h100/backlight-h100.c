/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "backlight.h"
#include "lcd.h"

/* Returns the current state of the backlight (true=ON, false=OFF). */
bool _backlight_init(void)
{
    or_l(0x00020000, &GPIO1_ENABLE);
    or_l(0x00020000, &GPIO1_FUNCTION);
    
    return (GPIO1_OUT & 0x00020000) ? false : true;
}

void _backlight_hw_on(void)
{
    and_l(~0x00020000, &GPIO1_OUT);
}

void _backlight_hw_off(void)
{
    or_l(0x00020000, &GPIO1_OUT);
}

void _remote_backlight_on(void)
{
    and_l(~0x00000800, &GPIO_OUT);
}

void _remote_backlight_off(void)
{
    or_l(0x00000800, &GPIO_OUT);
}
