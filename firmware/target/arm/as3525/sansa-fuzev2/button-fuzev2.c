/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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
#include "backlight.h"

#ifdef HAS_BUTTON_HOLD
static bool hold_button = false;
#endif

#ifdef HAVE_SCROLLWHEEL
#define SCROLLWHEEL_BITS        (1<<7|1<<6)
                                                      /* TIMER units */
#define TIMER_TICK              (KERNEL_TIMER_FREQ/HZ)/* how long a tick lasts */
#define TIMER_MS                (TIMER_TICK/(1000/HZ))/* how long a ms lasts */

#define WHEEL_REPEAT_INTERVAL   (300*TIMER_MS)      /* 300ms */
#define WHEEL_FAST_ON_INTERVAL  ( 20*TIMER_MS)      /*  20ms */
#define WHEEL_FAST_OFF_INTERVAL ( 60*TIMER_MS)      /*  60ms */
/* phsyical clicks per rotation * wheel value changes per phys click */
#define WHEEL_CHANGES_PER_CLICK         4
#define WHEELCLICKS_PER_ROTATION        (12*WHEEL_CHANGES_PER_CLICK)

/*
 * based on button-e200.c, adjusted to the AMS timers and fuzev2's
 * scrollwheel and cleaned up a little
 */
static void scrollwheel(unsigned int wheel_value)
{
    /* wheel values and times from the previous irq */
    static unsigned int  old_wheel_value   = 0;
    static unsigned int  wheel_repeat      = BUTTON_NONE;
    static          long last_wheel_post   = 0;

    /* We only post every 4th action, as this matches better with the physical
     * clicks of the wheel */
    static unsigned int  wheel_click_count = 0;
    /* number of items to skip in lists, 1 in slow mode */
    static unsigned int  wheel_delta       = 0;
    /* accumulated wheel rotations per second */
    static unsigned long wheel_velocity    = 0;
    /* fast or slow mode? */
    static          int  wheel_fast_mode   = 0;

    /* Read wheel
     * Bits 6 and 7 of GPIOA change as follows (Gray Code):
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

    unsigned int btn = BUTTON_NONE;

    if (old_wheel_value == wheel_tbl[0][wheel_value])
        btn = BUTTON_SCROLL_FWD;
    else if (old_wheel_value == wheel_tbl[1][wheel_value])
        btn = BUTTON_SCROLL_BACK;

    if (btn == BUTTON_NONE)
    {
        old_wheel_value = wheel_value;
        return;
    }

    int  repeat = 1; /* assume repeat */
    long time = TIMER2_VALUE + current_tick*TIMER_TICK; /* to timer unit */
    long v = (time - last_wheel_post);

   /* interpolate velocity in timer_freq/timer_unit == 1/s */
    if (v) v = TIMER_FREQ / v;

    /* accumulate velocities over time with each v */
    wheel_velocity = (7*wheel_velocity + v) / 8;

    if (btn != wheel_repeat)
    {
        /* direction reversals nullify all fast mode states */
        wheel_repeat      = btn;
        repeat            =
        wheel_velocity    =
        wheel_click_count = 0;
    }

    if (wheel_fast_mode != 0)
    {
        /* fast OFF happens immediately when velocity drops below
           threshold */
        if (TIME_AFTER(time,
                last_wheel_post + WHEEL_FAST_OFF_INTERVAL))
        {
            /* moving out of fast mode */
            wheel_fast_mode = 0;
            /* reset velocity */
            wheel_velocity = 0;
            /* wheel_delta is always 1 in slow mode */
            wheel_delta = 1;
        }
    }
    else
    {
        /* fast ON gets filtered to avoid inadvertent jumps to fast mode */
        if (repeat && wheel_velocity > TIMER_FREQ/WHEEL_FAST_ON_INTERVAL)
        {
            /* moving into fast mode */
            wheel_fast_mode = 1 << 31;
            wheel_click_count = 0;
            wheel_velocity = TIMER_FREQ/WHEEL_FAST_OFF_INTERVAL;
        }
        else if (++wheel_click_count < WHEEL_CHANGES_PER_CLICK)
        {   /* skip some wheel changes, so that 1 post represents
             * 1 item in lists */
            btn = BUTTON_NONE;
        }

        /* wheel_delta is always 1 in slow mode */
        wheel_delta = 1;
    }

    if (btn != BUTTON_NONE)
    {
        wheel_click_count = 0;

        /* generate repeats if quick enough */
        if (repeat && TIME_BEFORE(time,
                last_wheel_post + WHEEL_REPEAT_INTERVAL))
            btn |= BUTTON_REPEAT;
            
        last_wheel_post = time;

        if (queue_empty(&button_queue))
        {
            queue_post(&button_queue, btn, wheel_fast_mode |
                   (wheel_delta << 24) | wheel_velocity*360/WHEELCLICKS_PER_ROTATION);
            /* message posted - reset delta and poke backlight on*/
            wheel_delta = 1;
            backlight_on();
            buttonlight_on();
        }
        else
        {
            /* skipped post - increment delta */
            if (++wheel_delta > 0x7f)
                wheel_delta = 0x7f;
        }
    }

    old_wheel_value = wheel_value;
}
#endif

void button_init_device(void)
{
#if defined(HAVE_SCROLLWHEEL)
    GPIOA_DIR &= ~(1<<6|1<<7);
    GPIOC_DIR = 0;
    GPIOB_DIR |= (1<<4)|(1<<0);

    GPIOB_PIN(4) = 1<<4; /* activate the wheel */

    /* setup scrollwheel isr */
    /* clear previous irq if any */
    GPIOA_IC = SCROLLWHEEL_BITS;
    /* enable edge detecting */
    GPIOA_IS &= ~SCROLLWHEEL_BITS;
    /* detect both raising and falling edges */
    GPIOA_IBE |= SCROLLWHEEL_BITS;
    /* lastly, enable the interrupt */
    GPIOA_IE |= SCROLLWHEEL_BITS;
#endif
}

    /* read the 2 bits at the same time */
#define GPIOA_PIN76_offset ((1<<(6+2)) | (1<<(7+2)))
#define GPIOA_PIN76 (*(volatile unsigned char*)(GPIOA_BASE+GPIOA_PIN76_offset))

void button_gpioa_isr(void)
{
#if defined(HAVE_SCROLLWHEEL)
    /* scroll wheel handling */
    if (GPIOA_MIS & SCROLLWHEEL_BITS)
        scrollwheel(GPIOA_PIN76 >> 6);

    /* ack interrupt */
    GPIOA_IC = SCROLLWHEEL_BITS;
#endif
}


/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = 0;
    static bool hold_button_old = false;
    static long power_counter = 0;
    unsigned gpiod6;

    /* if we don't wait for the fifo to empty, we'll see screen corruption
     * (the higher the CPU frequency the higher the corruption) */
    while ((DBOP_STAT & (1<<10)) == 0);

    int delay = 30;
    while(delay--) nop;

    bool ccu_io_bit12 = CCU_IO & (1<<12);
    CCU_IO &= ~(1<<12);

    /* B1 is shared with FM i2c */
    bool gpiob_pin0_dir = GPIOB_DIR & (1<<1);
    GPIOB_DIR &= ~(1<<1);

    GPIOB_PIN(0) = 1<<0;
    udelay(4);

    gpiod6 = GPIOD_PIN(6);

    GPIOB_PIN(0) = 0;
    udelay(2);

    if (GPIOC_PIN(1) & 1<<1)
        btn |= BUTTON_DOWN;
    if (GPIOC_PIN(2) & 1<<2)
        btn |= BUTTON_UP;
    if (GPIOC_PIN(3) & 1<<3)
        btn |= BUTTON_LEFT;
    if (GPIOC_PIN(4) & 1<<4)
        btn |= BUTTON_SELECT;
    if (GPIOC_PIN(5) & 1<<5)
        btn |= BUTTON_RIGHT;
    if (GPIOB_PIN(1) & 1<<1)
        btn |= BUTTON_HOME;
    if (gpiod6 & 1<<6)
    {   /* power/hold is on the same pin. we know it's hold if the bit isn't
         * set now anymore */
        if (GPIOD_PIN(6) & 1<<6)
        {
            hold_button = false;
            btn |= BUTTON_POWER;
        }
        else
        {
            hold_button = true;
        }
    }

    if(gpiob_pin0_dir)
        GPIOB_DIR |= 1<<1;

    if(ccu_io_bit12)
        CCU_IO |= 1<<12;

#ifdef HAS_BUTTON_HOLD
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
        /* mask scrollwheel irq so we don't need to check for
         * the hold button in the isr */
        if (hold_button)
            GPIOA_IE &= ~SCROLLWHEEL_BITS;
        else
            GPIOA_IE |= SCROLLWHEEL_BITS;
    }
#else
    (void)hold_button_old;
#endif
    if (hold_button)
    {
        power_counter = HZ;
        return 0;
    }
    /* read power, but not if hold button was just released, since
     * you basically always hit power due to the slider mechanism after releasing
     * (fuze only)
     */
    else if (power_counter > 0)
    {
        power_counter--;
        btn &= ~BUTTON_POWER;
    }
#endif
    return btn;
}

#ifdef HAS_BUTTON_HOLD
bool button_hold(void)
{
    return hold_button;
}
#endif
