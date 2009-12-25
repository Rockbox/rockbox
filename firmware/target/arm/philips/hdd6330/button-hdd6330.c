/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#include "button.h"
#include "backlight.h"
#include "synaptics-mep.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

static int int_btn = BUTTON_NONE;

/*
 * Generate a click sound from the player (not in headphones yet)
 * TODO: integrate this with the "key click" option
 */
void button_click(void)
{
    GPO32_ENABLE |= 0x2000;
    GPO32_VAL |= 0x2000;
    udelay(1000);
    GPO32_VAL &= ~0x2000;
}

#ifndef BOOTLOADER
void button_init_device(void)
{
    /* The touchpad is powered on and initialized in power-hdd1630.c
       since it needs to be ready for both buttons and button lights. */
}

/*
 * Button interrupt handler
 */
void button_int(void)
{
    char data[4];
    int val;

    int_btn = BUTTON_NONE;

    val = touchpad_read_device(data, 4);
    
    if (val == MEP_BUTTON_HEADER)
    {
        /* Buttons packet */
        if (data[1] & 0x1)
            int_btn |= BUTTON_LEFT;
        if (data[1] & 0x2)
            int_btn |= BUTTON_RIGHT;
    }
    else if (val == MEP_ABSOLUTE_HEADER)
    {
        /* Absolute packet - the finger is on the vertical strip.
           Position ranges from 1-4095, with 1 at the bottom. */
        val = ((data[1] >> 4) << 8) | data[2]; /* position */

        if ((val > 0) && (val <= 1365))
            int_btn |= BUTTON_DOWN;
        else if ((val > 1365) && (val <= 2730))
            int_btn |= BUTTON_SELECT;
        else if ((val > 2730) && (val <= 4095))
            int_btn |= BUTTON_UP;
    }
}
#else
void button_init_device(void){}
#endif /* bootloader */

bool button_hold(void)
{
    return !(GPIOJ_INPUT_VAL & 0x8);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    static int btn_old = BUTTON_NONE;
    int btn = int_btn;

    /* Hold */
    if(button_hold())
        return BUTTON_NONE;

    /* Device buttons */
    if (!(GPIOA_INPUT_VAL & 0x01)) btn |= BUTTON_MENU;
    if (!(GPIOA_INPUT_VAL & 0x02)) btn |= BUTTON_VOL_UP;
    if (!(GPIOA_INPUT_VAL & 0x04)) btn |= BUTTON_VOL_DOWN;
    if (!(GPIOA_INPUT_VAL & 0x08)) btn |= BUTTON_VIEW;
    if (!(GPIOD_INPUT_VAL & 0x20)) btn |= BUTTON_PLAYLIST;
    if (!(GPIOD_INPUT_VAL & 0x40)) btn |= BUTTON_POWER;

    if ((btn != btn_old) && (btn != BUTTON_NONE))
        button_click();

    btn_old = btn;

    return btn;
}

bool headphones_inserted(void)
{
    return (GPIOE_INPUT_VAL & 0x80) ? true : false;
}
