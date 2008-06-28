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
#include "system.h"
#include "button.h"
#include "backlight.h"
#include "adc.h"
#include "lcd-remote-target.h"

/* have buttons scan by default */
static bool button_scan_on     = true;
static bool hold_button        = false;
static bool remote_hold_button = false;

void button_init_device(void)
{
    /* Power, Remote Play & Hold switch */
    GPIO_FUNCTION |= 0x0e000000;
    GPIO_ENABLE &= ~0x0e000000;
}

void button_enable_scan(bool enable)
{
    button_scan_on = enable;
}

bool button_scan_enabled(void)
{
    return button_scan_on;
}

bool button_hold(void)
{
    return (GPIO_READ & 0x08000000) == 0;
}

bool remote_button_hold(void)
{
    return remote_hold_button;
}

int button_read_device(void)
{
    int  btn = BUTTON_NONE;
    bool hold_button_old;
    bool remote_hold_button_old;
    static int prev_data = 0xff;
    static int last_valid = 0xff;
    int  data;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    /* give BL notice if HB state chaged */
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    if (button_scan_on && !hold_button)
    {
        data = adc_scan(ADC_BUTTONS);

        /* ADC debouncing: Only accept new reading if it's
         * stable (+/-1). Use latest stable value otherwise. */
        if ((unsigned)(data - prev_data + 1) <= 2)
            last_valid = data;
        prev_data = data;
        data = last_valid;
        
        if (data < 0xf0)
        {
            if(data < 0x7c)
                if(data < 0x42)
                    btn = BUTTON_LEFT;
                else
                    if(data < 0x62)
                        btn = BUTTON_RIGHT;
                    else
                        btn = BUTTON_SELECT;
            else
                if(data < 0xb6)
                    if(data < 0x98)
                        btn = BUTTON_REC;
                    else
                        btn = BUTTON_PLAY;
                else
                    if(data < 0xd3)
                        btn = BUTTON_DOWN;
                    else
                        btn = BUTTON_UP;
        }
    }

    /* remote buttons */
    data = remote_detect() ? adc_scan(ADC_REMOTE) : 0xff;

    remote_hold_button_old = remote_hold_button;
    remote_hold_button = data < 0x17;

#ifndef BOOTLOADER
    if (remote_hold_button != remote_hold_button_old)
        remote_backlight_hold_changed(remote_hold_button);
#endif

    if (!remote_hold_button)
    {
        if (data < 0xee)
        {
            if(data < 0x7a)
                if(data < 0x41)
                    btn |= BUTTON_RC_FF;
                else
                    if(data < 0x61)
                        btn |= BUTTON_RC_REW;
                    else
                        btn |= BUTTON_RC_MODE;
            else
                if(data < 0xb4)
                    if(data < 0x96)
                        btn |= BUTTON_RC_REC;
                    else
                        btn |= BUTTON_RC_MENU;
                else
                    if(data < 0xd1)
                        btn |= BUTTON_RC_VOL_UP;
                    else
                        btn |= BUTTON_RC_VOL_DOWN;
        }
    }

    data = GPIO_READ;

    /* hold and power are mutually exclusive */
    if (!(data & 0x04000000))
        btn |= BUTTON_POWER;

    /* remote play button should be dead if hold */
    if (!remote_hold_button && !(data & 0x02000000))
        btn |= BUTTON_RC_PLAY;

    return btn;
}
