/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "backlight-target.h"
#include "system.h"
#include "backlight.h"
#include "pcf50606.h"
#include "pcf50635.h"
#include "tcc780x.h"
#include "pmu-target.h"

int backlight_hw_init(void)
{
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    /* set backlight on by default, since the screen is unreadable without it */
    backlight_hw_on();
    return true;
}

void backlight_hw_brightness(int brightness)
{
    int level = disable_irq_save();
    
    if (get_pmu_type() == PCF50606)
    {
        pcf50606_write(PCF5060X_PWMC1,
                       0xe1 | (MAX_BRIGHTNESS_SETTING-brightness)<<1);
        pcf50606_write(PCF5060X_GPOC1, 0x3);
    }
    else
    {
        static const int brightness_lookup[MAX_BRIGHTNESS_SETTING+1] =
            {0x1, 0x8, 0xa, 0xe, 0x12, 0x16, 0x19, 0x1b, 0x1e,
             0x21, 0x24, 0x26, 0x28, 0x2a, 0x2c};

        pcf50635_write(PCF5063X_REG_LEDOUT, brightness_lookup[brightness]);
    }
    
    restore_irq(level);
}

void backlight_hw_on(void)
{
    if (get_pmu_type() == PCF50606)
    {
        GPIOA_SET = (1<<6);
    }
    else
    {
        int level = disable_irq_save();
        pcf50635_write(PCF5063X_REG_LEDENA, 1);
        restore_irq(level);
    }
}

void backlight_hw_off(void)
{
    if (get_pmu_type() == PCF50606)
    {
        GPIOA_CLEAR = (1<<6);
    }
    else
    {
        int level = disable_irq_save();
        pcf50635_write(PCF5063X_REG_LEDENA, 0);
        restore_irq(level);
    }
}
