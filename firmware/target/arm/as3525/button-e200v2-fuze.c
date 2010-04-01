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

extern void scrollwheel(unsigned wheel_value);

#if defined(SANSA_FUZE)
#define DBOP_BIT15_BUTTON       BUTTON_HOME
#endif

#ifdef SANSA_E200V2
#define DBOP_BIT15_BUTTON       BUTTON_REC
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

