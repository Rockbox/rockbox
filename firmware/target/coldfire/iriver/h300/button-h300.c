/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jonathan Gordon
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
#include "system.h"
#include "button.h"
#include "backlight.h"
#include "adc.h"
#include "lcd-remote.h"

/* have buttons scan by default */
static bool button_scan_on = true;

void button_init_device(void)
{
    /* Set GPIO9 and GPIO15 as general purpose inputs */
    GPIO_ENABLE &= ~0x00008200;
    GPIO_FUNCTION |= 0x00008200;
    /* Set GPIO33, GPIO37, GPIO38  and GPIO52 as general purpose inputs */
    GPIO1_ENABLE &= ~0x00100060;
    GPIO1_FUNCTION |= 0x00100062;
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
    return (GPIO1_READ & 0x00000002)?true:false;
}

bool remote_button_hold_only(void)
{
    if(remote_type() == REMOTETYPE_H300_NONLCD)
        return adc_scan(ADC_REMOTE)<0x0d; /* hold should be 0x00 */
    else
        return (GPIO1_READ & 0x00100000)?true:false;
}

/* returns true only if there is remote present */
bool remote_button_hold(void)
{
    /* H300's NON-LCD remote doesn't set the "remote present" bit */
    if(remote_type() == REMOTETYPE_H300_NONLCD)
        return remote_button_hold_only();
    else
        return ((GPIO_READ & 0x40000000) == 0)?remote_button_hold_only():false;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;
    static bool hold_button = false;
    static bool remote_hold_button = false;
    static int prev_data = 0xff;
    static int last_valid = 0xff;
    bool hold_button_old;
    bool remote_hold_button_old;

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();
    
    
#ifndef BOOTLOADER
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
        
        if (data < 0xba)
        {
            if (data < 0x54)
                if (data < 0x30)
                    if (data < 0x10)
                        btn = BUTTON_SELECT;
                    else
                        btn = BUTTON_UP;
                else
                    btn = BUTTON_LEFT;
            else
                if (data < 0x98)
                    if (data < 0x76)
                        btn = BUTTON_DOWN;
                    else
                        btn = BUTTON_RIGHT;
                else
                    btn = BUTTON_OFF;
        }
    }

    /* remote buttons */
    remote_hold_button_old = remote_hold_button;
    remote_hold_button = remote_button_hold_only();

#ifndef BOOTLOADER
    if (remote_hold_button != remote_hold_button_old)
        remote_backlight_hold_changed(remote_hold_button);
#endif

    if (!remote_hold_button)
    {
        data = adc_scan(ADC_REMOTE);
        switch (remote_type())
        {
            case REMOTETYPE_H100_LCD:
                if (data < 0xf5)
                {
                    if (data < 0x73)
                        if (data < 0x3f)
                            if (data < 0x25)
                                if(data < 0x0c)
                                    btn |= BUTTON_RC_STOP;
                                else
                                    btn |= BUTTON_RC_VOL_DOWN;
                            else
                                btn |= BUTTON_RC_MODE;
                        else
                            if (data < 0x5a)
                                btn |= BUTTON_RC_VOL_UP;
                            else
                                btn |= BUTTON_RC_BITRATE;
                    else
                        if (data < 0xa8)
                            if (data < 0x8c)
                                btn |= BUTTON_RC_REC;
                            else
                                btn |= BUTTON_RC_SOURCE;
                        else
                            if (data < 0xdf)
                                if(data < 0xc5)
                                    btn |= BUTTON_RC_FF;
                                else
                                    btn |= BUTTON_RC_MENU;
                            else
                                btn |= BUTTON_RC_REW;
                }
                break;
            case REMOTETYPE_H300_LCD:
                if (data < 0xf5)
                {
                    if (data < 0x73)
                        if (data < 0x42)
                            if (data < 0x27)
                                if(data < 0x0c)
                                    btn |= BUTTON_RC_VOL_DOWN;
                                else
                                    btn |= BUTTON_RC_FF;
                            else
                                btn |= BUTTON_RC_STOP;
                        else
                            if (data < 0x5b)
                                btn |= BUTTON_RC_MODE;
                            else
                                btn |= BUTTON_RC_REC;
                    else
                        if (data < 0xab)
                            if (data < 0x8e)
                                btn |= BUTTON_RC_ON;
                            else
                                btn |= BUTTON_RC_BITRATE;
                        else
                            if (data < 0xde)
                                if(data < 0xc5)
                                    btn |= BUTTON_RC_SOURCE;
                                else
                                    btn |= BUTTON_RC_VOL_UP;
                            else
                                btn |= BUTTON_RC_REW;
                }
                break;
            case REMOTETYPE_H300_NONLCD:
                if (data < 0xf1)
                {
                    if (data < 0x7d)
                        if (data < 0x25)
                            btn |= BUTTON_RC_FF;
                        else
                            btn |= BUTTON_RC_REW;
                    else
                        if (data < 0xd5)
                            btn |= BUTTON_RC_VOL_DOWN;
                        else
                            btn |= BUTTON_RC_VOL_UP;
                }
                break;
        }
    }

    if (!hold_button)
    {
        data = GPIO_READ;
        if ((data & 0x0200) == 0)
            btn |= BUTTON_MODE;
        if ((data & 0x8000) == 0)
            btn |= BUTTON_REC;
    }

    data = GPIO1_READ;
    if (!hold_button && ((data & 0x20) == 0))
        btn |= BUTTON_ON;
    if (!remote_hold_button && ((data & 0x40) == 0))
        switch(remote_type())
        {
            case REMOTETYPE_H100_LCD:
            case REMOTETYPE_H300_NONLCD:
                btn |= BUTTON_RC_ON;
                break;
            case REMOTETYPE_H300_LCD:
                btn |= BUTTON_RC_MENU;
                break;
        }
        
    return btn;
}
