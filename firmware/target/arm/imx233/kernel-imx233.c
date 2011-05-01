/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "kernel.h"
#include "timrot-imx233.h"
#include "clkctrl-imx233.h"

static void tick_timer(void)
{
    /* Run through the list of tick tasks */
    call_tick_tasks();
}

void tick_start(unsigned int interval_in_ms)
{
    /* use the 1-kHz XTAL clock source */
    imx233_setup_timer(0, true, interval_in_ms,
        HW_TIMROT_TIMCTRL__SELECT_1KHZ_XTAL, HW_TIMROT_TIMCTRL__PRESCALE_1,
        false, &tick_timer);
}
