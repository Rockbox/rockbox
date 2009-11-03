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

#ifndef BOOTLOADER
void button_init_device(void)
{
    /* The touchpad is powered on and initialized in power-sa9200.c
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
        if (data[1] & 0x1) int_btn |= BUTTON_RIGHT;
        if (data[1] & 0x2) int_btn |= BUTTON_NEXT;
        if (data[1] & 0x4) int_btn |= BUTTON_PREV;
        if (data[1] & 0x8) int_btn |= BUTTON_LEFT;
        if (data[2] & 0x1) int_btn |= BUTTON_MENU;
    }
    else if (val == MEP_ABSOLUTE_HEADER)
    {
        /* Absolute packet - the finger is on the vertical strip.
           Position ranges from 1-4095, with 1 at the bottom. */
        val = ((data[1] >> 4) << 8) | data[2]; /* position */

        if ((val > 0) && (val <= 1365))
            int_btn |= BUTTON_DOWN;
        else if ((val > 1365) && (val <= 2730))
            int_btn |= BUTTON_PLAY;
        else if ((val > 2730) && (val <= 4095))
            int_btn |= BUTTON_UP;
    }
}
#else
void button_init_device(void){}
#endif /* bootloader */

bool button_hold(void)
{
    return !(GPIOL_INPUT_VAL & 0x40);
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = int_btn;

    if (button_hold())
        return BUTTON_NONE;

    if (!(GPIOB_INPUT_VAL & 0x20)) btn |= BUTTON_POWER;
    if (!(GPIOF_INPUT_VAL & 0x10)) btn |= BUTTON_VOL_UP;
    if (!(GPIOF_INPUT_VAL & 0x04)) btn |= BUTTON_VOL_DOWN;

    return btn;
}

bool headphones_inserted(void)
{
    return (GPIOB_INPUT_VAL & 0x10) ? false : true;
}
