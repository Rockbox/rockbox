/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Robert Keevil
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
#include "kernel.h"
#include "piezo.h"

static unsigned int duration;
static bool beeping;

void INT_TIMERA(void)
{
    /* clear interrupt */
    TACON = TACON;
    if (!(--duration))
        piezo_stop();
}

static void piezo_gpio_enable(bool enable)
{
    PCON1 = (PCON1 & ~0xff) | (enable ? 0x53 : 0xee);
}

static void piezo_start(unsigned short cycles, unsigned short periods)
{
#ifndef SIMULATOR
    duration = periods;
    beeping = 1;
    /* select TA_OUT function on GPIO ports */
    piezo_gpio_enable(true);
//     /* configure timer for 100 kHz (12 MHz / 4 / 30) */
    /* configure timer for 100 kHz (24 MHz / 4 / 60) */
    TACMD = (1 << 1);   /* TA_CLR */
//     TAPRE = 30 - 1;     /* prescaler */
    TAPRE = 60 - 1;     /* prescaler */
    TACON = (1 << 13) | /* TA_INT1_EN */
            (0 << 12) | /* TA_INT0_EN */
            (0 << 11) | /* TA_START */
            (1 << 8) |  /* TA_CS = ECLK / 4 */
//             (1 << 6) |  /* select ECLK (12 MHz) */
            (1 << 6) |  /* select ECLK (24 MHz) */
            (1 << 4);   /* TA_MODE_SEL = PWM mode */
    TADATA0 = cycles;   /* set interval period */
    TADATA1 = cycles << 1; /* set interval period */
    TACMD = (1 << 0);   /* TA_EN */
#endif
}

void piezo_stop(void)
{
#ifndef SIMULATOR
    beeping = 0;
    TACMD = (1 << 1);   /* TA_CLR */
    /* configure GPIO for the lowest power consumption */
    piezo_gpio_enable(false);
#endif
}

void piezo_clear(void)
{
    piezo_stop();
}

bool piezo_busy(void)
{
    return beeping;
}

void piezo_init(void)
{
    piezo_stop();
}

void piezo_button_beep(bool beep, bool force)
{
    if (force)
        while (beeping)
            yield();

    if (!beeping)
    {
        if (beep)
            piezo_start(22, 457);
        else
            piezo_start(40, 4);
    }
}

#ifdef BOOTLOADER
void piezo_tone(uint32_t period /*uS*/, int32_t duration /*ms*/)
{
    int32_t stop = USEC_TIMER + duration*1000;
    uint32_t level = 0;

    piezo_gpio_enable(true);
    while ((int32_t)USEC_TIMER - stop < 0)
    {
        level ^= 1;
        GPIOCMD = 0x1000e | level;
        udelay(period >> 1);
    }
    piezo_gpio_enable(false);
}

void piezo_seq(uint16_t *seq)
{
    uint16_t period;

    while ((period = *seq++) != 0)
    {
        piezo_tone(period, *seq++);
        udelay(*seq++ * 1000);
    }
}
#endif
