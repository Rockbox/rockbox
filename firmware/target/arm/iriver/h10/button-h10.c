/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

/* Custom written for the H10 based on analysis of the GPIO data */


#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"


void button_init_device(void)
{
    /* Enable REW, FF, Play, Left, Right, Hold buttons */
    GPIO_SET_BITWISE(GPIOA_ENABLE, 0xfc);
    
    /* Enable POWER button */
    GPIO_SET_BITWISE(GPIOB_ENABLE, 0x01);
    
    /* We need to output to pin 6 of GPIOD when reading the scroll pad value */
    GPIO_SET_BITWISE(GPIOD_ENABLE, 0x40);
    GPIO_SET_BITWISE(GPIOD_OUTPUT_EN, 0x40);
    GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x40);
}

bool button_hold(void)
{
    return (GPIOA_INPUT_VAL & 0x4)?false:true;
}

bool remote_button_hold(void)
{
    return adc_scan(ADC_REMOTE) < 0x2B;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;
    unsigned char state;
    static bool hold_button = false;
    static bool remote_hold_button = false;
    bool hold_button_old;
    bool remote_hold_button_old;

    /* Hold */
    hold_button_old = hold_button;
    hold_button = button_hold();

#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        backlight_hold_changed(hold_button);
    }
#endif

    /* device buttons */
    if (!hold_button)
    {
        /* Read normal buttons */
        state = GPIOA_INPUT_VAL & 0xf8;
        if ((state & 0x8) == 0) btn |= BUTTON_FF;
        if ((state & 0x10) == 0) btn |= BUTTON_PLAY;
        if ((state & 0x20) == 0) btn |= BUTTON_REW;
        if ((state & 0x40) == 0) btn |= BUTTON_RIGHT;
        if ((state & 0x80) == 0) btn |= BUTTON_LEFT;
        
        /* Read power button */
        if (GPIOB_INPUT_VAL & 0x1) btn |= BUTTON_POWER;
        
        /* Read scroller */
        if ( GPIOD_INPUT_VAL & 0x20 )
        {
            GPIO_CLEAR_BITWISE(GPIOD_OUTPUT_VAL, 0x40);
            udelay(250);
            data = adc_scan(ADC_SCROLLPAD);
            GPIO_SET_BITWISE(GPIOD_OUTPUT_VAL, 0x40);
            
            if(data < 0x224)
            {
                btn |= BUTTON_SCROLL_DOWN;
            } else {
                btn |= BUTTON_SCROLL_UP;
            }
        }
    }
    
    /* remote buttons */
    remote_hold_button_old = remote_hold_button;

    data = adc_scan(ADC_REMOTE);
    remote_hold_button = data < 0x2B;

#ifndef BOOTLOADER
    if (remote_hold_button != remote_hold_button_old)
        backlight_hold_changed(remote_hold_button);
#endif

    if(!remote_hold_button)
    {
        if (data < 0x3FF)
        {
            if(data < 0x204)
                if(data < 0x155)
                    btn |= BUTTON_RC_FF;
                else
                    btn |= BUTTON_RC_REW;
            else
                if(data < 0x2D0)
                   btn |= BUTTON_RC_VOL_DOWN;
                else
                    btn |= BUTTON_RC_VOL_UP;
        }
    }

    /* remote play button should be dead if hold */
    if (!remote_hold_button && !(GPIOA_INPUT_VAL & 0x1))
        btn |= BUTTON_RC_PLAY;
    
    return btn;
}
