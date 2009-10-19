/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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
#include <stdbool.h>
#include <stdio.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "led-mini2440.h"

void power_init(void)
{
    /* Nothing to do */
}

unsigned int power_input_status(void)
{
    unsigned int status = 0;

    /* Always on*/
    status = POWER_INPUT_MAIN;
    return status;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return false;
}

void power_off(void)
{
    /* we don't have any power control, user must do it */
    led_flash (LED_NONE, LED_ALL);
    while (1);
}
