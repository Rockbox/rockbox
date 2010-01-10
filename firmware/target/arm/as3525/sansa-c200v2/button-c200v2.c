/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell, (C) 2009 by Bertrik Sikken
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
#include "button-target.h"
#include "button.h"
#include "backlight.h"
#include "dbop-as3525.h"

static unsigned short _dbop_din;

static bool hold_button     = false;
#ifndef BOOTLOADER
static bool hold_button_old = false;
#endif

void button_init_device(void)
{
    GPIOA_DIR &= ~(1<<3);
}

bool button_hold(void)
{
    return hold_button;
}

/*
 * Get button pressed from hardware
 */
int button_read_device(void)
{
    int btn = BUTTON_NONE;

    _dbop_din = dbop_read_input();

    /* hold button handling */
    hold_button = ((_dbop_din & (1<<12)) == 0);
#ifndef BOOTLOADER
    /* light handling */
    if (hold_button != hold_button_old)
    {
        hold_button_old = hold_button;
        backlight_hold_changed(hold_button);
    }
#endif /* BOOTLOADER */
    if (hold_button) {
        return 0;
    }

    /* direct GPIO connections */
    if (GPIOA_PIN(3))
        btn |= BUTTON_POWER;

    /* DBOP buttons */
    if(!(_dbop_din & (1<<2)))
        btn |= BUTTON_LEFT;
    if(!(_dbop_din & (1<<3)))
        btn |= BUTTON_DOWN;
    if(!(_dbop_din & (1<<4)))
        btn |= BUTTON_SELECT;
    if(!(_dbop_din & (1<<5)))
        btn |= BUTTON_UP;
    if(!(_dbop_din & (1<<6)))
        btn |= BUTTON_RIGHT;
    if(!(_dbop_din & (1<<13)))
        btn |= BUTTON_VOL_UP;
    if(!(_dbop_din & (1<<14)))
        btn |= BUTTON_VOL_DOWN;
    if(!(_dbop_din & (1<<15)))
        btn |= BUTTON_REC;

    return btn;
}
