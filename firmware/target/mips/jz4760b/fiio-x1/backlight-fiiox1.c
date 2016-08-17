/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"
#include "gpio-jz4760b.h"
#include "system-fiiox1.h"

/* there are two versions of the backlight hardware, they use different pins */
static int backlight_type;

bool backlight_hw_init(void)
{
    /* use PA29 to detect backlight type: GPIO out, no pullup, set to 1 to see if there is a pulldown */
    jz_gpio_setup_std_out(0, 29, true);
    backlight_type = jz_gpio_get_input(0, 29);
    jz_gpio_set_output(0, 29, false);
    /* configure both possible backlight pins as GPIOs: PE1 and PF3 */
    jz_gpio_setup_std_out(4, 1, false);
    jz_gpio_setup_std_out(5, 3, false);

    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

void backlight_hw_on(void)
{
}

void backlight_hw_off(void)
{
    /* there is no real on/off but we can set to 0 brightness */
    backlight_hw_brightness(0);
}

/* send a stream of pulses to the backlight controller */
static void backlight_pulse(unsigned bank, unsigned pin, int low_pulse, int high_pulse,
    int stream_pulse, int count)
{
    /* send a low pulse */
    jz_gpio_set_output(bank, pin, false);
    if(count == 0)
        return;
    udelay(low_pulse);
    jz_gpio_set_output(bank, pin, true);
    udelay(high_pulse);
    /* send a stream of pulse, assume 50% duty ratio for now */
    while(--count > 0)
    {
        jz_gpio_set_output(bank, pin, false);
        udelay(stream_pulse);
        jz_gpio_set_output(bank, pin, true);
        udelay(stream_pulse);
    }
}

void backlight_hw_brightness(int brightness)
{
    int count = (brightness == 0) ? 0 : 17 - brightness;
    /* NOTE: the OF uses the hardware version to adjust its backlight brightness table, we do not
     * do that for now */
    if(fiiox1_get_hw_version() == 2)
    {
        if(backlight_type == 1)
        {
            /* because of the large involved delays, the OF uses the TCU with PWM and counts the
            * interrupts to get the timing right, maybe we should implement it like that */
            backlight_pulse(4, 1, 4000, 300, 400, count);  /* PE1 */
        }
        else
            backlight_pulse(4, 1, 4000, 40, 1, count); /* PF3 */
    }
    else
        backlight_pulse(4, 1, 4000, 40, 1, count); /* PE1 */
}
