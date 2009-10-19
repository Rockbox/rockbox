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
#include "system.h"
#include "button.h"
#include "kernel.h"

void button_init_device(void)
{
    /* Configure port directions and enable internal pullups on button inputs */

    /* These are the standard 6 buttons on the Mini2440 */
    S3C2440_GPIO_CONFIG (GPGCON, 0, GPIO_INPUT);
    S3C2440_GPIO_CONFIG (GPGCON, 3, GPIO_INPUT);
    S3C2440_GPIO_CONFIG (GPGCON, 5, GPIO_INPUT);
    S3C2440_GPIO_CONFIG (GPGCON, 6, GPIO_INPUT);
    S3C2440_GPIO_CONFIG (GPGCON, 7, GPIO_INPUT);
    S3C2440_GPIO_CONFIG (GPGCON, 11, GPIO_INPUT);

    S3C2440_GPIO_PULLUP (GPGUP, 0, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPGUP, 3, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPGUP, 5, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPGUP, 6, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPGUP, 7, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPGUP, 11, GPIO_PULLUP_ENABLE);
    
    /* These are additional buttons on my add on keypad */
    S3C2440_GPIO_CONFIG (GPGCON, 9, GPIO_INPUT);
    S3C2440_GPIO_CONFIG (GPGCON, 10, GPIO_INPUT);
    S3C2440_GPIO_PULLUP (GPGUP, 9, GPIO_PULLUP_ENABLE);
    S3C2440_GPIO_PULLUP (GPGUP, 10, GPIO_PULLUP_ENABLE);
    
}

inline bool button_hold(void)
{
    return 0;
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;

    /* Read the buttons - active low */
    btn = (GPGDAT & BUTTON_MAIN) ^ BUTTON_MAIN;
    
    return btn;
}

void touchpad_set_sensitivity(int level)
{
    (void)level;
    /* No touchpad */
}

bool headphones_inserted(void)
{
    /* No detect */
    return false;
}
