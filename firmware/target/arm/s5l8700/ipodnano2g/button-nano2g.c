/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-m3.c 21787 2009-07-11 20:00:07Z bertrik $
 *
 * Copyright (C) 2009 Dave Chapman
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

#include <stdbool.h>
#include "config.h"

#include "s5l8700.h"
#include "button-target.h"

#define CLICKWHEEL00      (*(volatile unsigned long*)(0x3c200000))
#define CLICKWHEEL10      (*(volatile unsigned long*)(0x3c200010))
#define CLICKWHEELINT     (*(volatile unsigned long*)(0x3c200014))
#define CLICKWHEEL_DATA   (*(volatile unsigned long*)(0x3c200018))

static int buttons = 0;

void INT_SPI(void)
{
    int clickwheel_events;
    int btn =0;
    int status;

    clickwheel_events = CLICKWHEELINT;

    if (clickwheel_events & 4) CLICKWHEELINT = 4;
    if (clickwheel_events & 2) CLICKWHEELINT = 2;
    if (clickwheel_events & 1) CLICKWHEELINT = 1;

    status = CLICKWHEEL_DATA;
        if ((status & 0x800000ff) == 0x8000001a) 
        {
            if (status & 0x00000100)
                btn |= BUTTON_SELECT;
            if (status & 0x00000200)
                btn |= BUTTON_RIGHT;
            if (status & 0x00000400)
                btn |= BUTTON_LEFT;
            if (status & 0x00000800)
                btn |= BUTTON_PLAY;
            if (status & 0x00001000)
                btn |= BUTTON_MENU;
        }

	buttons = btn;
}

void button_init_device(void)
{
    CLICKWHEEL00 = 0x280000;
    CLICKWHEEL10 = 3;
    INTMOD = 0;
    INTMSK |= (1<<26);
    PCON10 &= ~0xF00;
}

int button_read_device(void)
{
    return buttons;
}

bool button_hold(void)
{
    return ((PDAT14 & (1 << 6)) == 0);
}
