/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
#include "backlight-target.h"
#include "lcd.h"

bool backlight_hw_init(void)
{
    return true;
}

void backlight_hw_on(void)
{
    lcd_enable(true);
}

void backlight_hw_off(void)
{
    lcd_enable(false);
}

void backlight_hw_brightness(int brightness)
{
    lcd_set_contrast(brightness*16-1);
}

void lcd_sleep(void)
{
    backlight_hw_off();
}
