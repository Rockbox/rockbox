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
    {
        beeping = 0;
        TACMD = (1 << 1);   /* TA_CLR */
    }
}

static void piezo_start(unsigned short cycles, unsigned short periods)
{
#ifndef SIMULATOR
    duration = periods;
    beeping = 1;
    /* configure timer for 100 kHz */
    TACMD = (1 << 1);   /* TA_CLR */
    TAPRE = 30 - 1;     /* prescaler */         /* 12 MHz / 4 / 30 = 100 kHz */
    TACON = (1 << 13) | /* TA_INT1_EN */
            (0 << 12) | /* TA_INT0_EN */
            (0 << 11) | /* TA_START */
            (1 << 8) |  /* TA_CS = PCLK / 4 */
            (1 << 6) |  /* UNKNOWN bit */       /* external 12 MHz clock ??? */
            (1 << 4);   /* TA_MODE_SEL = PWM mode */
    TADATA0 = cycles;   /* set interval period */
    TADATA1 = cycles << 1; /* set interval period */
    TACMD = (1 << 0);   /* TA_EN */
#endif
}

void piezo_stop(void)
{
#ifndef SIMULATOR
    TACMD = (1 << 1);   /* TA_CLR */
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
    beeping = 0;
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
