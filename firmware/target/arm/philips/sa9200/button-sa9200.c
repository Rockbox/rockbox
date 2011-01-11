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
static bool hold_button_old = false;

/* Capacitive buttons - lowest setting is still very sensitive */
static const signed char button_sensitivity = -8; /* -8 to 7 */

/* Strip - decent amount of contact needed */
static const signed char strip_sensitivity = 0; /* -8 to 7 */
static const unsigned char strip_pressure = 25; /* 0 to 255 */

void button_init_device(void)
{
    /* The touchpad is powered on and initialized in power-sa9200.c
       since it needs to be ready for both buttons and button lights. */

    /* perform button-specific inits here */

    /* Besides $00, only common parameters $20 and $21 are supported */

    /* Report modes:
     * [15:12] cap btn sens : xxxx,
     * [11: 8] pos sen sens : xxxx,
     * [ 7: 6] rate: 10 (40 rps),
     * [    5] no filter: 0,
     * [    4] reserved: 0,
     * [    3] en scr: 0,
     * [    2] en btns: 1,
     * [    1] en rel: 0,
     * [    0] en abs: 1 */
    touchpad_set_parameter(0, 0x20,
        ((button_sensitivity & 0xf) << 12) |
        ((strip_sensitivity & 0xf) << 8) |
        (0x2 << 6) |
        (0x1 << 2) |
        (0x1 << 0));

    /* Enhanced operating configuration:
     * [15: 9] reserved : 0000000
     * [    8] Min abs reporting : 0
     * [    7] reserved : 0
     * [    6] not all cap btns : 1
     * [    5] single cap btn : 0
     * [    4] en 50ms debounce : 1
     * [    3] motion reporting : 0
     * [    2] clip z if no finger : 0
     * [    1] disable decel : 0
     * [    0] enable dribble : 0 */
    touchpad_set_parameter(0, 0x21,
        (0x1 << 6) | (0x1 << 4));
}

/*
 * Button interrupt handler
 */
void button_int(void)
{
    unsigned char data[4];
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
    else if (val == MEP_ABSOLUTE_HEADER && data[3] >= strip_pressure)
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
void button_init_device(void)
{
}
#endif /* BOOTLOADER */

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
    bool hold = !(GPIOL_INPUT_VAL & 0x40);

#ifndef BOOTLOADER
    /* Backlight hold handling */
    if (hold != hold_button_old)
    {
        hold_button_old = hold;
        backlight_hold_changed(hold);
    }
#endif

    if (hold)
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
