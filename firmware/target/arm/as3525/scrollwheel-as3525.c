/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009-2010 by Thomas Martitz
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
#include "button.h"
#include "kernel.h"
#include "backlight.h"

void scrollwheel(unsigned int wheel_value)
{
#ifndef BOOTLOADER
    /* current wheel values, parsed from dbop and the resulting button */
    unsigned         btn             = BUTTON_NONE;
    /* old wheel values */
    static unsigned old_wheel_value = 0;
    static unsigned old_btn         = BUTTON_NONE;

    /*
     * Getting BUTTON_REPEAT works like this: Remember when the btn value was
     * posted to the button_queue last, and if it was recent enough, generate
     * BUTTON_REPEAT
     */
    static long      last_wheel_post = 0;

    /*
     *  Providing wheel acceleration works as follows: We increment accel
     * by 2 if the wheel was turned, and decrement it by 1 each tick
     * (no matter if it was turned), that means: the longer and faster you turn,
     * the higher accel will be. accel>>2 will actually posted to the button_queue
     */
    static int        accel = 0;
    /* We only post every 4th action, as this matches better with the physical
     * clicks of the wheel */
    static int        counter = 0;
    /* Read wheel 
     * Bits 13 and 14 of DBOP_DIN change as follows (Gray Code):
     * Clockwise rotation   00 -> 01 -> 11 -> 10 -> 00
     * Counter-clockwise    00 -> 10 -> 11 -> 01 -> 00
     *
     * For easy look-up, actual wheel values act as indicies also,
     * which is why the table seems to be not ordered correctly
     */
    static const unsigned char wheel_tbl[2][4] =
    {
        { 2, 0, 3, 1 }, /* Clockwise rotation */
        { 1, 3, 0, 2 }, /* Counter-clockwise  */ 
    };

    if(button_hold())
    {
        accel  =  counter  = 0;
        return;
    }

    if (old_wheel_value == wheel_tbl[0][wheel_value])
        btn = BUTTON_SCROLL_FWD;
    else if (old_wheel_value == wheel_tbl[1][wheel_value])
        btn = BUTTON_SCROLL_BACK;
    else if (old_wheel_value != wheel_value && accel > ACCEL_INCREMENT)
    {   /* if no button is read and wheel_value changed, assume old_btn */
        btn = old_btn;
    }
    /* else btn = BUTTON_NONE */

    if (btn != BUTTON_NONE)
    {
        if (btn != old_btn)
        {
            /* direction reversals nullify acceleration and counters */
            old_btn =  btn;
            accel  =  counter  = 0;
        }
        /* wheel_delta will cause lists to jump over items,
         * we want this for fast scrolling, but we must keep it accurate
         * for slow scrolling */
        int wheel_delta = 0;
        /* generate BUTTON_REPEAT if quick enough, scroll slightly faster too*/
        if (TIME_BEFORE(current_tick, last_wheel_post + WHEEL_REPEAT_INTERVAL))
        {
            btn |= BUTTON_REPEAT;
            wheel_delta = accel>>ACCEL_SHIFT;
        }

        accel += ACCEL_INCREMENT;

        /* the wheel is more reliable if we don't send every change,
         * every WHEEL_COUNTER_DIVth is basically one "physical click" 
         * which should make up 1 item in lists */
        if (++counter >= WHEEL_COUNTER_DIV && queue_empty(&button_queue))
        {
            buttonlight_on();
            backlight_on();
            queue_post(&button_queue, btn, ((wheel_delta+1)<<24));
            /* message posted - reset count and remember post */
            counter = 0;
            last_wheel_post = current_tick;
        }
    }
    if (accel > 0)
        accel--;

    old_wheel_value = wheel_value;
#else
    (void)wheel_value;
#endif
}
