/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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

void button_init_device(void)
{
    /* GPIO56 (main PLAY) 
     * GPIO41 (remote PLAY)
     * as general purpose inputs 
     */
    or_l((1<<24)|(1<<9),&GPIO1_FUNCTION);
    and_l(~((1<<24)|(1<<9)),&GPIO1_ENABLE);
}

bool button_hold(void)
{
    /* GPIO36 active high */
    return (GPIO1_READ & (1<<4))?true:false;
}

bool remote_button_hold(void)
{
    return adc_scan(ADC_REMOTE)<50?true:false;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data = 0;
    static bool hold_button = false;
    static bool remote_hold_button = false;

    bool hold_button_old;

    /* read hold buttons status */
    hold_button_old = hold_button;
    hold_button = button_hold();
    remote_hold_button = remote_button_hold();
    
#ifndef BOOTLOADER
    /* Only main hold affects backlight */
    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);
#endif

    if (!hold_button)
    {
        data = adc_scan(ADC_BUTTONS);

        if (data < 2250) /* valid button */
        {
	    if (data < 900) /* middle */
            {
                if (data < 500)
                {
                    if (data > 200)
                        /* 200 - 500 */
                        btn = BUTTON_REC;
                }
                else /* 900 - 500 */
                    btn = BUTTON_VOL_DOWN;
            }
            else /* 2250 - 900 */
            {
                if (data < 1600)
                {
                    /* 1600 - 900 */
                    if (data < 1200)
                        /* 1200 - 900 */
                        btn = BUTTON_VOL_UP;
                    else /* 1600 - 1200 */
                        btn = BUTTON_NEXT;
                }
                else /* 1600 - 2250 */
                {
                    if (data < 1900)
                        /* 1900 - 1600 */
                        btn = BUTTON_PREV;
                    else /* 1900 - 2250 */
                        btn = BUTTON_SELECT;
                }
	    }	    
        }
    }

    if (!remote_hold_button)
    {
        data = adc_scan(ADC_REMOTE);

        if (data < 2050) /* valid button */
        {
	    if (data < 950) /* middle */
            {
                if (data < 650)
                {
                    if (data < 400)
                    {
                        if (data > 250)
                            /* 250 - 400 */
                            btn = BUTTON_RC_VOL_DOWN;
                    }
                    else /* 650 - 400 */
                        btn = BUTTON_RC_VOL_UP;
                }
                else /* 950 - 650 */
                    btn = BUTTON_RC_NEXT;
            }
            else /* 2050 - 950 */
            {
                if (data < 1900)
                {
                    if (data < 1350)
                        /* 1350 - 900 */
                        btn = BUTTON_RC_PREV;
                }
                else /* 2050 - 1900 */
                    btn = BUTTON_RC_SELECT;
            }
        }
    }

    data = GPIO1_READ;

    /* GPIO56 active high main PLAY/PAUSE/ON */
    if (!hold_button && ((data & (1<<24))))
        btn |= BUTTON_PLAY;

    /* GPIO41 active high remote PLAY/PAUSE/ON */
    if (!remote_hold_button && ((data & (1<<9))))
        btn |= BUTTON_RC_PLAY;
        
    return btn;
}
