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

/* Taken from button-h10.c by Barry Wardell and reverse engineering by MrH. */

#include "system.h"
#include "button.h"
#include "backlight.h"
#include "powermgmt.h"

#define WHEEL_REPEAT_VELOCITY       35 /* deg/s */
#define WHEEL_SMOOTHING_VELOCITY    50 /* deg/s */
#define WHEEL_FAST_ON_VELOCITY     350 /* deg/s */
#define WHEEL_FAST_OFF_VELOCITY    150 /* deg/s */
#define WHEEL_BASE_SENSITIVITY       2
#define WHEELCLICKS_PER_ROTATION    48 /* wheelclicks per full rotation */

#ifndef BOOTLOADER
/* Wheel */
static int read_wheel_keycode(void);
/* Buttons */
static bool hold_button     = false;
static bool hold_button_old = false;
#define _button_hold() hold_button
#else
#define _button_hold() ((GPIOF_INPUT_VAL & 0x80) != 0)
#endif /* BOOTLOADER */
static int  int_btn         = BUTTON_NONE;

void button_init_device(void)
{
    /* Enable all buttons */
    GPIO_CLEAR_BITWISE(GPIOF_OUTPUT_EN, 0xff);
    GPIO_SET_BITWISE(GPIOF_ENABLE, 0xff);

    /* Scrollwheel light - enable control through GPIOG pin 7 and set timeout */
    GPIO_SET_BITWISE(GPIOG_OUTPUT_EN, 0x80);
    GPIO_SET_BITWISE(GPIOG_ENABLE, 0x80);

#ifndef BOOTLOADER
    /* Mask these before performing init ... because init has possibly
       occurred before */
    GPIO_CLEAR_BITWISE(GPIOF_INT_EN, 0xff);
    GPIO_CLEAR_BITWISE(GPIOH_INT_EN, 0xc0);

    GPIO_CLEAR_BITWISE(GPIOH_OUTPUT_EN, 0xc0);
    GPIO_SET_BITWISE(GPIOH_ENABLE, 0xc0);

    /* Read initial buttons */
    button_int();

    /* Read initial wheel value (bit 6-7 of GPIOH) */
    read_wheel_keycode();

    /* Enable button interrupts */
    GPIO_SET_BITWISE(GPIOF_INT_EN, 0xff);
    GPIO_SET_BITWISE(GPIOH_INT_EN, 0xc0);

    CPU_INT_EN = HI_MASK;
    CPU_HI_INT_EN = GPIO1_MASK;
#endif /* BOOTLOADER */
}

bool button_hold(void)
{
    return _button_hold();
}

/* clickwheel */
#ifndef BOOTLOADER
static int read_wheel_keycode(void)
{
    /* Read wheel 
     * Bits 6 and 7 of GPIOH change as follows:
     * Clockwise rotation   01 -> 00 -> 10 -> 11
     * Counter-clockwise    11 -> 10 -> 00 -> 01
     *
     * This is equivalent to wheel_value of:
     * Clockwise rotation   0x40 -> 0x00 -> 0x80 -> 0xc0
     * Counter-clockwise    0xc0 -> 0x80 -> 0x00 -> 0x40
     */
    static const unsigned char wheel_tbl[2][4] =
    {
       /* 0x00  0x40  0x80  0xc0 */ /* Wheel value        */
        { 0x40, 0xc0, 0x00, 0x80 }, /* Clockwise rotation */
        { 0x80, 0x00, 0xc0, 0x40 }, /* Counter-clockwise  */ 
    };

    static unsigned long prev_value;

    int keycode = BUTTON_NONE;

    unsigned long value = GPIOH_INPUT_VAL & 0xc0;
    GPIO_WRITE_BITWISE(GPIOH_INT_LEV, value ^ 0xc0, 0xc0);
    GPIOH_INT_CLR = GPIOH_INT_STAT & 0xc0;

    if (!hold_button)
    {
        unsigned long prev = prev_value;
        int scroll = value >> 6;

        if (prev == wheel_tbl[0][scroll])
            keycode = BUTTON_SCROLL_FWD;
        else if (prev == wheel_tbl[1][scroll])
            keycode = BUTTON_SCROLL_BACK;
    }

    prev_value = value;

    return keycode;
}

void clickwheel_int(void)
{
    static int prev_keycode = BUTTON_NONE;
    static int prev_keypost = BUTTON_NONE;
    static int count = 0;
    static int fast_mode = 0;
    static long next_backlight_on = 0;

    static unsigned long prev_usec[WHEEL_BASE_SENSITIVITY] = { 0 };
    static unsigned long delta = 1ul << 24;
    static unsigned long velocity = 0; /* Velocity smoothed or unsmoothed */

    unsigned long usec = USEC_TIMER;
    unsigned long v; /* Raw velocity */

    int keycode = read_wheel_keycode();

    if (keycode == BUTTON_NONE)
        return;

    /* Spurious wheel "reversals" are not uncommon. Resetting also helps
     * cover them up. As such, prev_keypost is also not reset or else false
     * non-repeats are generated when it happens. */
    if (keycode != prev_keycode)
    {
        /* Direction reversals reset state */
        unsigned long usec_back = 360000000ul /
                        (WHEEL_REPEAT_VELOCITY*WHEELCLICKS_PER_ROTATION);
        prev_keycode = keycode;
        count = 0;
        fast_mode = 0;
        prev_usec[0] = usec - usec_back;
        prev_usec[1] = prev_usec[0] - usec_back;
        delta = 1ul << 24;
        velocity = 0;
    }

    if (TIME_AFTER(current_tick, next_backlight_on))
    {
        /* Poke backlight to turn it on or maintain it no more often
         * than every 1/4 second */
        next_backlight_on = current_tick + HZ/4;
        backlight_on();
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on();
#endif
        reset_poweroff_timer();
    }

    /* Calculate deg/s based upon interval and number of clicks in that
     * interval - FIR moving average */
    v = usec - prev_usec[1];

    if ((long)v <= 0)
    {
        /* timer wrapped (no activity for awhile), skip acceleration */
        v = 0;
        delta = 1ul << 24;
    }
    else
    {
        /* Check overflow below */
        if (v > 0xfffffffful / WHEELCLICKS_PER_ROTATION)
            v = 0xfffffffful / WHEELCLICKS_PER_ROTATION;

        v = 360000000ul*WHEEL_BASE_SENSITIVITY / (v*WHEELCLICKS_PER_ROTATION);

        if (v > 0xfffffful)
            v = 0xfffffful; /* limit to 24 bits */
    }

    prev_usec[1] = prev_usec[0];
    prev_usec[0] = usec;

    if (v < WHEEL_SMOOTHING_VELOCITY)
    {
        /* Very slow - no smoothing */
        velocity = v;
        /* Ensure backlight never gets stuck for an extended period if tick
         * wrapped such that next poke is very far ahead */
        next_backlight_on = current_tick - 1;
    }
    else
    {
        /* Some velocity filtering to smooth things out */
        velocity = (7*velocity + v) / 8;
    }

    if (fast_mode != 0)
    {
        /* Fast OFF happens immediately when velocity drops below
           threshold */
        if (v < WHEEL_FAST_OFF_VELOCITY)
        {
            fast_mode = 0; /* moving out of fast mode */
            velocity = v;

            /* delta is always 1 in slow mode */
            delta = 1ul << 24;
        }
    }
    else
    {
        /* Fast ON gets filtered to avoid inadvertent jumps to fast mode */
        if (velocity >= WHEEL_FAST_ON_VELOCITY)
            fast_mode = 1; /* moving into fast mode */

        /* delta is always 1 in slow mode */
        delta = 1ul << 24;
    }

    count += fast_mode + 1;
    if (count < WHEEL_BASE_SENSITIVITY)
        return;

    count = 0;

    if (queue_empty(&button_queue))
    {
        /* Post wheel keycode with wheel data */
        int key = keycode;

        if (v >= WHEEL_REPEAT_VELOCITY && prev_keypost == key)
        {
            /* Quick enough and same key is being posted more than once in a
             * row - generate repeats - use unsmoothed v */
            key |= BUTTON_REPEAT;
        }

        prev_keypost = keycode;

        queue_post(&button_queue, key, (fast_mode << 31) | delta | velocity);
        /* Message posted - reset delta */
        delta = 1ul << 24;
    }
    else
    {
        /* Skipped post - increment delta and limit to 7 bits */
        delta += 1ul << 24;

        if (delta > (0x7ful << 24))
            delta = 0x7ful << 24;
    }
}
#else
void clickwheel_int(void)
{
}
#endif /* BOOTLOADER */

/* device buttons */
void button_int(void)
{
    unsigned long state = GPIOF_INPUT_VAL & 0xff;

#ifndef BOOTLOADER
    unsigned long status = GPIOF_INT_STAT;

    GPIO_WRITE_BITWISE(GPIOF_INT_LEV, state ^ 0xff, 0xff);
    GPIOF_INT_CLR = status;

    hold_button = (state & 0x80) != 0;
#endif

    int_btn = BUTTON_NONE;

    if (!_button_hold())
    {
        /* Read normal buttons */
        if ((state & 0x01) == 0) int_btn |= BUTTON_REC;
        if ((state & 0x02) == 0) int_btn |= BUTTON_DOWN;
        if ((state & 0x04) == 0) int_btn |= BUTTON_RIGHT;
        if ((state & 0x08) == 0) int_btn |= BUTTON_LEFT;
        if ((state & 0x10) == 0) int_btn |= BUTTON_SELECT; /* The centre button */
        if ((state & 0x20) == 0) int_btn |= BUTTON_UP; /* The "play" button */
        if ((state & 0x40) != 0) int_btn |= BUTTON_POWER;
    }
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
#ifdef BOOTLOADER
    /* Read buttons directly in the bootloader */
    button_int();
#else
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */

    /* The int_btn variable is set in the button interrupt handler */
    return int_btn;
}
