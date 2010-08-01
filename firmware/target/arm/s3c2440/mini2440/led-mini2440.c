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
#include "kernel.h"
#include "led-mini2440.h"

/* LED functions for debug */

void led_init (void)
{
    S3C2440_GPIO_CONFIG (GPBCON, 5, GPIO_OUTPUT);
    S3C2440_GPIO_CONFIG (GPBCON, 6, GPIO_OUTPUT);
    S3C2440_GPIO_CONFIG (GPBCON, 7, GPIO_OUTPUT);
    S3C2440_GPIO_CONFIG (GPBCON, 8, GPIO_OUTPUT);
    
    S3C2440_GPIO_PULLUP (GPBUP, 5, GPIO_PULLUP_DISABLE);
    S3C2440_GPIO_PULLUP (GPBUP, 6, GPIO_PULLUP_DISABLE);
    S3C2440_GPIO_PULLUP (GPBUP, 7, GPIO_PULLUP_DISABLE);
    S3C2440_GPIO_PULLUP (GPBUP, 8, GPIO_PULLUP_DISABLE);
}

/* Turn on one or more LEDS */
void set_leds (int led_mask)
{
  GPBDAT &= ~led_mask;
}

/* Turn off one or more LEDS */
void clear_leds (int led_mask)
{
  GPBDAT |= led_mask;
}

/* Alternate flash pattern1 and pattern2 */ 
/* Never returns */
void led_flash (int led_pattern1, int led_pattern2)
{   
    while (1)
    {
        set_leds (led_pattern1);
        sleep(HZ/2);
        clear_leds (led_pattern1);
        
        set_leds(led_pattern2);
        sleep(HZ/2);
        clear_leds (led_pattern2);
    }
}
