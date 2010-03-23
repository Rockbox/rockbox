/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "kernel.h"
#include "panic.h"
#include "timer.h"

#ifdef HAVE_SCROLLWHEEL
/* let the timer interrupt twice as often for the scrollwheel polling */
#define KERNEL_TIMER_FREQ (TIMER_FREQ/2)
#else
#define KERNEL_TIMER_FREQ TIMER_FREQ
#endif

#ifdef HAVE_SCROLLWHEEL
#include "button-target.h"
/* The scrollwheel is polled every 5 ms (the tick tasks only every 10) */
static int poll_scrollwheel = 0;

static inline void do_scrollwheel(void)
{
    if (!poll_scrollwheel)
        call_tick_tasks();      /* Run through the list of tick tasks
                                 * (that includes reading the scrollwheel) */
    else
    {
        if (!button_hold())
            button_read_dbop(); /* Read the scrollwheel */
    }

    poll_scrollwheel ^= 1;
}
#else
static inline void do_scrollwheel(void)
{
    call_tick_tasks();      /* Run through the list of tick tasks */
}
#endif

#if defined(SANSA_C200V2)
#include "backlight-target.h"

static int timer2_cycles_per_tick = 0;
static int timer2_cycles_pwmon = 0;
static int timer2_cycles_pwmoff = 0;
static int timer2_pwm_state = 0;
static int timer2_pwm_on = 0;

void _set_timer2_pwm_ratio(int ratio)
{
    int cycles = timer2_cycles_per_tick;

    /*
     * Rather arbitrary limits, but since the CPU
     * needs some to time in the interrupt handler
     * there sure is some limit.
     * More specifically, if the cycles needed to do
     * the pwm handling are more than the reloaded counter needs
     * to reach 0 again it will reload to the old value most
     * likely leading to a (slight) slowdown in tick rate.
     */

    if (ratio < 10) {
        /*
         * Permanent off, reduce interrupt rate to save power
         */
        TIMER2_BGLOAD = cycles;
        timer2_pwm_on = 0;
        _backlight_pwm(0);
        return;
    }

    if (ratio > 990) {
        /*
         * Permanent on, reduce interrupt rate to save power
         */
        TIMER2_BGLOAD = cycles;
        timer2_pwm_on = 0;
        _backlight_pwm(1);
        return;
    }

    timer2_cycles_pwmon = cycles*ratio/1000;
    timer2_cycles_pwmoff = cycles*(1000-ratio)/1000;

    if (timer2_pwm_on == 0) {
        timer2_pwm_state = 0;
        timer2_pwm_on = 1;
        TIMER2_BGLOAD = timer2_cycles_pwmoff;
    }
}

static void set_timer2_cycles_per_tick(int cycles)
{
    timer2_cycles_per_tick = cycles;
}

static inline void do_sw_pwm(void)
{
    if (!timer2_pwm_on) {
        do_scrollwheel();  /* Handle scrollwheel and tick tasks */
        TIMER2_INTCLR = 0;  /* clear interrupt */
        return;
    }

    timer2_pwm_state ^= 1;
    if (timer2_pwm_state) {
        TIMER2_BGLOAD = timer2_cycles_pwmoff;
        _backlight_pwm(1);
        /*
         * Always do scrollwheel and tick tasks during the longer cycle for safety,
         * since the short cycle can be quite short.
         * (minimum: 1us if ratio is 10 or 990 or 0.5us with scrollwheel,
         *  or just about 6000 clock cycles at 60MHz)
         */
        if (timer2_cycles_pwmon > timer2_cycles_pwmoff)
            do_scrollwheel();  /* Handle scrollwheel and tick tasks */
    } else {
        TIMER2_BGLOAD = timer2_cycles_pwmon;
        _backlight_pwm(0);
        if (!(timer2_cycles_pwmon > timer2_cycles_pwmoff))
            do_scrollwheel();  /* Handle scrollwheel and tick tasks */
    }

    TIMER2_INTCLR = 0;  /* clear interrupt */
}
#else
static inline void do_sw_pwm(void)
{
    do_scrollwheel();  /* Handle scrollwheel and tick tasks */
}

static void set_timer2_cycles_per_tick(int cycles)
{
    (void)cycles;
}
#endif


void INT_TIMER2(void)
{
    /*
     * Timer is stacked as follows:
     * Lowest layer: Software PWM (if configured)
     *   Alternates timer2 reload value to implement
     *   software pwm at 100Hz (no scrollwheel)
     *   or 200Hz (scrollwheel) with variable pulse width 1% to 99%
     * Middle layer: Scrollwheel handling (if configured, 200Hz)
     *   Alternate between polling scrollwheel and running tick
     *   tasks (includes scrollwheel polling).
     * Top layer: Run tick tasks at 100Hz
     */
    do_sw_pwm();

    TIMER2_INTCLR = 0;  /* clear interrupt */
}

void tick_start(unsigned int interval_in_ms)
{
    int cycles = KERNEL_TIMER_FREQ / 1000 * interval_in_ms;

    CGU_PERI |= CGU_TIMER2_CLOCK_ENABLE;    /* enable peripheral */
    VIC_INT_ENABLE = INTERRUPT_TIMER2;     /* enable interrupt */

    set_timer2_cycles_per_tick(cycles);
    TIMER2_LOAD = TIMER2_BGLOAD = cycles;   /* timer period */

    /* /!\ bit 4 (reserved) must not be modified
     * periodic mode, interrupt enabled, no prescale, 32 bits counter */
    TIMER2_CONTROL = (TIMER2_CONTROL & (1<<4)) |
                     TIMER_ENABLE |
                     TIMER_PERIODIC |
                     TIMER_INT_ENABLE |
                     TIMER_32_BIT;
}
