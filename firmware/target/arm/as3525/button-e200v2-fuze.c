/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Thomas Martitz
 * Copyright (C) 2008 by Dominik Wenger
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
#include "button-target.h"
#include "backlight.h"
#include "dbop-as3525.h"


#if defined(SANSA_FUZE) || defined(SANSA_FUZEV2)
#define DBOP_BIT15_BUTTON       BUTTON_HOME
#define WHEEL_REPEAT_INTERVAL   (HZ/5)
#define WHEEL_COUNTER_DIV       4
#define ACCEL_INCREMENT         2
#define ACCEL_SHIFT             2
#endif

#ifdef SANSA_E200V2
#define DBOP_BIT15_BUTTON       BUTTON_REC
#define WHEEL_REPEAT_INTERVAL   (HZ/5)
#define WHEEL_COUNTER_DIV       2
#define ACCEL_INCREMENT         3
#define ACCEL_SHIFT             1
#endif

/* Buttons */
static bool hold_button     = false;
#ifndef BOOTLOADER
static bool hold_button_old = false;
#endif

void button_init_device(void)
{
    GPIOA_DIR |= (1<<1);     
    GPIOA_PIN(1) = (1<<1);
}

#if !defined(BOOTLOADER) && defined(HAVE_SCROLLWHEEL)
static void scrollwheel(unsigned int wheel_value)
{
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

    if(hold_button)
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
}
#endif /* !defined(BOOTLOADER) && defined(SCROLLWHEEL) */

bool button_hold(void)
{
    return hold_button;
}

unsigned short button_read_dbop(void)
{
    unsigned dbop_din = dbop_read_input();
#if defined(HAVE_SCROLLWHEEL) && !defined(BOOTLOADER)
    /* scroll wheel handling */
    scrollwheel((dbop_din >> 13) & (1<<1|1<<0));
#endif
    return dbop_din;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
#ifdef SANSA_FUZE
    static unsigned power_counter = 0;
#endif
    unsigned short dbop_din;
    int btn = BUTTON_NONE;

    dbop_din = button_read_dbop();

    /* hold button handling */
    hold_button = ((dbop_din & (1<<12)) != 0);
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */
    if (hold_button) {
#ifdef SANSA_FUZE
        power_counter = HZ;
#endif
        return 0;
    }

    /* push button handling */
    if ((dbop_din & (1 << 2)) == 0)
        btn |= BUTTON_UP;
    if ((dbop_din & (1 << 3)) == 0)
        btn |= BUTTON_LEFT;
    if ((dbop_din & (1 << 4)) == 0)
        btn |= BUTTON_SELECT;
    if ((dbop_din & (1 << 5)) == 0)
        btn |= BUTTON_RIGHT;
    if ((dbop_din & (1 << 6)) == 0)
        btn |= BUTTON_DOWN;
    if ((dbop_din & (1 << 8)) != 0)
        btn |= BUTTON_POWER;
    if ((dbop_din & (1 << 15)) == 0)
        btn |= DBOP_BIT15_BUTTON;

#ifdef SANSA_FUZE
    /* read power on bit 8, but not if hold button was just released, since
     * you basically always hit power due to the slider mechanism after releasing
     * (fuze only)
     */
    if (power_counter > 0) {
            power_counter--;
        btn &= ~BUTTON_POWER;
    }
#endif

    return btn;
}

