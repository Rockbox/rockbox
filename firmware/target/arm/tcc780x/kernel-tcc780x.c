/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Rob Purchase
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
#include "timer.h"
#include "thread.h"

/* NB: PCK_TCT must previously have been set to 2Mhz by calling clock_init() */
void tick_start(unsigned int interval_in_ms)
{
    /* disable Timer0 */
    TCFG0 &= ~1;

    /* set counter reference value based on 1Mhz tick */
    TREF0 = interval_in_ms * 1000;

    /* Timer0 = reset to 0, divide=2, IRQ enable, enable (continuous) */
    TCFG0 = (1<<8) | (0<<4) | (1<<3) | 1;

    /* Unmask timer IRQ */
    IEN |= TIMER0_IRQ_MASK;
}

/* NB: Since we are using a single timer IRQ, tick tasks are dispatched as
       part of the central timer IRQ processing in timer-tcc780x.c */
