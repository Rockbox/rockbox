/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in December 2005
 * Original file: linux/arch/armnommu/mach-ipod/keyboard.c
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
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

/*
 * Rockbox button functions
 */

#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "serial.h"
#include "power.h"
#include "powermgmt.h"
#include "hwcompat.h"

static int int_btn = BUTTON_NONE;
#ifdef IPOD_1G2G
/* The 1st Gen wheel draws ~12mA when enabled permanently. Therefore
 * we only enable it for a very short time to check for changes every
 * tick, and only keep it enabled if there is activity. */
#define WHEEL_TIMEOUT (HZ/4)
#endif

#define WHEELCLICKS_PER_ROTATION  96
#define WHEEL_BASE_SENSITIVITY     6 /* Compute every ... clicks */
#define WHEEL_REPEAT_VELOCITY     45 /* deg/s */
#define WHEEL_SMOOTHING_VELOCITY 100 /* deg/s */

static void handle_scroll_wheel(int new_scroll)
{
    static const signed char scroll_state[4][4] = {
        {0, 1, -1, 0},
        {-1, 0, 0, 1},
        {1, 0, 0, -1},
        {0, -1, 1, 0}
    };

    static int prev_scroll = -1;
    static int direction = 0;
    static int count = 0;
    static long nextbacklight_hw_on = 0;

    static unsigned long last_wheel_usec = 0;
    static unsigned long wheel_delta = 1ul << 24;
    static unsigned long wheel_velocity = 0;
    static int prev_keypost = BUTTON_NONE;

    int wheel_keycode = BUTTON_NONE;
    int scroll;
    unsigned long usec;
    unsigned long v;

    if (prev_scroll == -1) {
        prev_scroll = new_scroll;
        return;
    }

    scroll = scroll_state[prev_scroll][new_scroll];
    prev_scroll = new_scroll;

    if (direction != scroll) {
        /* direction reversal or was hold - reset all */
        direction = scroll;
        prev_keypost = BUTTON_NONE;
        wheel_velocity = 0;
        wheel_delta = 1ul << 24;
        count = 0;
    }

    /* poke backlight every 1/4s of activity */
    if (TIME_AFTER(current_tick, nextbacklight_hw_on)) {
        backlight_on();
        reset_poweroff_timer();
        nextbacklight_hw_on = current_tick + HZ/4;
    }

    /* has wheel travelled far enough? */
    if (++count < WHEEL_BASE_SENSITIVITY) {
        return;
    }

    /* reset travel count and do calculations */
    count = 0;

    /* 1st..3rd Gen wheel has inverse direction mapping
     * compared to Mini 1st Gen wheel. */
    switch (direction) {
        case 1:
            wheel_keycode = BUTTON_SCROLL_BACK;
            break;
        case -1:
            wheel_keycode = BUTTON_SCROLL_FWD;
            break;
        default:
            /* only happens if we get out of sync */
            return;
    }

    /* have a keycode */

    usec = USEC_TIMER;
    v = usec - last_wheel_usec;

    /* calculate deg/s based upon sensitivity-adjusted interrupt period */

    if ((long)v <= 0) {
        /* timer wrapped (no activity for awhile), skip acceleration */
        v = 0;
        wheel_delta = 1ul << 24;
    }
    else {
        if (v > 0xfffffffful/WHEELCLICKS_PER_ROTATION) {
            v = 0xfffffffful/WHEELCLICKS_PER_ROTATION; /* check overflow below */
        }

        v = 360000000ul*WHEEL_BASE_SENSITIVITY / (v*WHEELCLICKS_PER_ROTATION);

        if (v > 0xfffffful)
            v = 0xfffffful; /* limit to 24 bits */
    }

    if (v < WHEEL_SMOOTHING_VELOCITY) {
        /* very slow - no smoothing */
        wheel_velocity = v;
        /* ensure backlight never gets stuck for an extended period if tick
         * wrapped such that next poke is very far ahead */
        nextbacklight_hw_on = current_tick - 1;
    }
    else {
        /* some velocity filtering to smooth things out */
        wheel_velocity = (7*wheel_velocity + v) / 8;
    }

    if (queue_empty(&button_queue)) {
        int key = wheel_keycode;

        if (v >= WHEEL_REPEAT_VELOCITY && prev_keypost == key) {
            /* quick enough and same key is being posted more than once in a
             * row - generate repeats - use unsmoothed v to guage */
            key |= BUTTON_REPEAT;
        }

        prev_keypost = wheel_keycode;

        /* post wheel keycode with wheel data */
        queue_post(&button_queue, key,
                   (wheel_velocity >= WHEEL_ACCEL_START ? (1ul << 31) : 0)
                    | wheel_delta | wheel_velocity);
        /* message posted - reset delta */
        wheel_delta = 1ul << 24;
    }
    else {
        /* skipped post - increment delta and limit to 7 bits */
        wheel_delta += 1ul << 24;

        if (wheel_delta > (0x7ful << 24))
            wheel_delta = 0x7ful << 24;
    }

    last_wheel_usec = usec;
}

static int ipod_3g_button_read(void)
{
    unsigned char source, state;
    int btn = BUTTON_NONE;
    
    /* get source of interupts */
    source = GPIOA_INT_STAT;

    /* get current keypad status */
    state = GPIOA_INPUT_VAL;
    
    /* toggle interrupt level */
    GPIOA_INT_LEV = ~state;

    /* ack any active interrupts */
    GPIOA_INT_CLR = source;

#ifdef IPOD_3G
    static bool was_hold = false;

    if (was_hold && source == 0x40 && state == 0xbf) {
        return BUTTON_NONE;
    }
    was_hold = false;

    if ((state & 0x20) == 0) {
        /* 3g hold switch is active low */
        was_hold = true;
        /* hold switch on 3g causes all outputs to go low */
        /* we shouldn't interpret these as key presses */
        return BUTTON_NONE;
    }
#elif defined IPOD_1G2G
    if (state & 0x20) {
        /* 1g/2g hold switch is active high */
        return BUTTON_NONE;
    }
#endif
    if ((state & 0x1) == 0) {
        btn |= BUTTON_RIGHT;
    }
    if ((state & 0x2) == 0) {
        btn |= BUTTON_SELECT;
    }
    if ((state & 0x4) == 0) {
        btn |= BUTTON_PLAY;
    }
    if ((state & 0x8) == 0) {
        btn |= BUTTON_LEFT;
    }
    if ((state & 0x10) == 0) {
        btn |= BUTTON_MENU;
    }

    if (source & 0xc0) {
        handle_scroll_wheel((state & 0xc0) >> 6);
    }

    return btn;
}

void ipod_3g_button_int(void)
{
    CPU_INT_DIS = GPIO_MASK;
    int_btn = ipod_3g_button_read();
    CPU_INT_EN = GPIO_MASK;
}

void button_init_device(void)
{
    GPIOA_ENABLE = 0xff;
    GPIOA_OUTPUT_EN = 0;

    GPIOA_INT_LEV = ~GPIOA_INPUT_VAL;
    GPIOA_INT_CLR = GPIOA_INT_STAT;

#ifdef IPOD_1G2G
    if ((IPOD_HW_REVISION >> 16) != 2)
    {   /* enable scroll wheel */
        GPIOB_ENABLE |= 0x01;
        GPIOB_OUTPUT_EN |= 0x01;
        GPIOB_OUTPUT_VAL |= 0x01;
    }
#endif
    GPIOA_INT_EN  = 0xff;

    CPU_INT_EN = GPIO_MASK;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    static bool hold_button = false;
    bool hold_button_old;
#ifdef IPOD_1G2G
    static int wheel_timeout = 0;
    static unsigned char last_wheel_value = 0;
    unsigned char wheel_value;

    if ((IPOD_HW_REVISION >> 16) != 2)
    {
        if (!hold_button && (wheel_timeout == 0))
        {
            GPIOB_OUTPUT_VAL |= 0x01; /* enable wheel */
            udelay(50);               /* let the voltage settle */
        }
        wheel_value = GPIOA_INPUT_VAL >> 6;
        if (wheel_value != last_wheel_value)
        {
            last_wheel_value = wheel_value;
            wheel_timeout = WHEEL_TIMEOUT; /* keep wheel enabled */
            GPIOA_INT_EN = 0xff;      /* enable wheel interrupts */
        }
        if (wheel_timeout)
            wheel_timeout--;
        else
        {
            GPIOA_INT_EN = 0x3f;       /* disable wheel interrupts */
            GPIOB_OUTPUT_VAL &= ~0x01; /* disable wheel */
        }
    }
#endif

    /* normal buttons */
    hold_button_old = hold_button;
    hold_button = button_hold();

    if (hold_button != hold_button_old)
        backlight_hold_changed(hold_button);

    return int_btn;
}

bool button_hold(void)
{
#ifdef IPOD_1G2G
    return (GPIOA_INPUT_VAL & 0x20);
#else
    return !(GPIOA_INPUT_VAL & 0x20);
#endif
}

bool headphones_inserted(void)
{
#ifdef IPOD_1G2G
    if ((IPOD_HW_REVISION >> 16) == 2)
    {
        /* 2G uses GPIO B bit 0 */
        return (GPIOB_INPUT_VAL & 0x1)?true:false;
    }
    else
    {
        /* 1G has no headphone detection, so fake insertion */
        return (true);
    }
#else
    /* 3G uses GPIO C bit 0 */
    return (GPIOC_INPUT_VAL & 0x1)?true:false;
#endif
}


