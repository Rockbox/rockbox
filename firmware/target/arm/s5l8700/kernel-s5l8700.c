/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Bertrik Sikken
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

/*  S5L8700 driver for the kernel timer

    Timer B is configured as a 10 kHz timer (assuming PCLK = 48 MHz)
 */

void INT_TIMERB(void)
{
    /* clear interrupt */
    TBCON = TBCON;

    call_tick_tasks();  /* Run through the list of tick tasks */
}

void tick_start(unsigned int interval_in_ms)
{
    int cycles = 10 * interval_in_ms;
    
    /* enable timer clock */
    PWRCON &= ~(1 << 4);
    
    /* configure timer for 10 kHz */
    TBCMD = (1 << 1);   /* TB_CLR */
    TBPRE = 300 - 1;    /* prescaler for 48 MHz PCLK */
    TBCON = (0 << 13) | /* TB_INT1_EN */
            (1 << 12) | /* TB_INT0_EN */
            (0 << 11) | /* TB_START */
            (2 << 8) |  /* TB_CS = PCLK / 16 */
            (0 << 4);   /* TB_MODE_SEL = interval mode */
    TBDATA0 = cycles;   /* set interval period */
    TBCMD = (1 << 0);   /* TB_EN */

    /* enable timer interrupt */
    INTMSK |= INTMSK_TIMERB;
}

