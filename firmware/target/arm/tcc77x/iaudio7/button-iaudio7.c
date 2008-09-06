/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Vitja Makarov
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
#include "cpu.h"
#include "button.h"
#include "adc.h"

#include "button-target.h"
#include "ata2501.h"

void button_init_device(void)
{
    ata2501_init();
}

/*
 touchpad:
  0: stop
  1-8: between next & prev
  9: play
  10: next
  11: prev
*/

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int adc;
    int sensor;

    if (button_hold())
        return BUTTON_NONE;

    adc = adc_read(0);
    sensor = ata2501_read();

    if (0 == (GPIOA & 4))
        btn |= BUTTON_POWER;

    /* seems they can't be hold together */
    if (adc < 0x120)
        btn |= BUTTON_VOLUP;
    else if (adc < 0x270)
        btn |= BUTTON_VOLDOWN;
    else if (adc < 0x300)
        btn |= BUTTON_MENU;

    if (sensor & (1 << 0))
        btn |= BUTTON_STOP;
    if (sensor & (1 << 9))
        btn |= BUTTON_PLAY;
    if (sensor & ((1 << 10) | 0x1c0))
        btn |= BUTTON_RIGHT;
    if (sensor & ((1 << 11) | 0xe))
        btn |= BUTTON_LEFT;

    return btn;
}

bool button_hold(void)
{
    return !(GPIOA & 0x2);
}
