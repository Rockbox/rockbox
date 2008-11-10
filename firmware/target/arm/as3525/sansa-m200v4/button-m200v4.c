/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by ??
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

void button_init_device(void)
{
    GPIOA_DIR &= ~((1<<3) | (1<<2) | (1<<1) | (1<<0));  /* A3-A0 is input */
    GPIOA_DIR |= ((1<<6) | (1<<5) | (1<<4)); /* A4-A6 row outputs */
}

int button_read_device(void)
{
    int result = BUTTON_NONE;

    /* direct GPIO connections */
    if (GPIOA_PIN(3))
        result |= BUTTON_MENU;

    /* This is a keypad using A4-A6 as columns and A0-A2 as rows */
    GPIOA_PIN(4) = (1<<4);

    /* A4A0 is unused */

    if (GPIOA_PIN(1))
        result |= BUTTON_VOLDOWN;

    if (GPIOA_PIN(2))
        result |= BUTTON_PLAYPAUSE;

    GPIOA_PIN(4) = 0x00;  
    
    GPIOA_PIN(5) = (1<<5);

    if (GPIOA_PIN(0))
        result |= BUTTON_LEFT;

    if (GPIOA_PIN(1))
        result |= BUTTON_SELECT;

    if (GPIOA_PIN(2))
        result |= BUTTON_RIGHT;

    GPIOA_PIN(5) = 0x00;


    GPIOA_PIN(6) = (1<<6);

    if (GPIOA_PIN(0))
        result |= BUTTON_REPEATAB;

    if (GPIOA_PIN(1))
        result |= BUTTON_VOLUP;

    if (GPIOA_PIN(2))
        result |= BUTTON_HOLD;
    GPIOA_PIN(6) = 0x00;

    return result;
}

bool button_hold(void)
{
    bool ret = false;
    
    GPIOA_PIN(6) = (1<<6);
    if (GPIOA_PIN(2))
        ret = true;
    GPIOA_PIN(6) = 0x00; 
    
    return ret;
}
