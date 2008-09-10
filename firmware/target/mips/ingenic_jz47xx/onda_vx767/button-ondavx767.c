/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "button-target.h"

#define BTN_VOL_DOWN ((1 << 27) | (1 << 21) | (1 << 30))
#define BTN_VOL_UP   (1 << 0)
#define BTN_MENU     (1 << 3)
#define BTN_BACK     (1 << 4)
#define BTN_SELECT   (1 << 29)
#define BTN_REWIND   (1 << 1)
#define BTN_FAST_FWD (1 << 2)
#define BTN_HOLD     (1 << 31) /* Unknown currently */
#define BTN_MASK     (BTN_VOL_DOWN | BTN_VOL_UP | BTN_MENU \
                      | BTN_BACK | BTN_SELECT | BTN_REWIND \
                      | BTN_FAST_FWD)

bool button_hold(void)
{
    return (~__gpio_get_port(3) & BTN_HOLD ? 1 : 0);
}

void button_init_device(void)
{  
    __gpio_port_as_input(3, 30);
    __gpio_port_as_input(3, 21);
    __gpio_port_as_input(3, 27);
    __gpio_port_as_input(3, 0);
    __gpio_port_as_input(3, 3);
    __gpio_port_as_input(3, 4);
    __gpio_port_as_input(3, 29);
    __gpio_port_as_input(3, 1);
    __gpio_port_as_input(3, 2);
    /* __gpio_port_as_input(3, 31); */
}

int button_read_device(void)
{
    if(button_hold())
        return 0;
    
    unsigned int key = ~(__gpio_get_port(3));
    int ret = 0;
    
    if(key & BTN_MASK)
    {
        if(key & BTN_VOL_DOWN)
            ret |= BUTTON_VOL_DOWN;
        if(key & BTN_VOL_UP)
            ret |= BUTTON_VOL_UP;
        if(key & BTN_SELECT)
            ret |= BUTTON_SELECT;
        if(key & BTN_MENU)
            ret |= BUTTON_MENU;
        if(key & BTN_BACK)
            ret |= BUTTON_BACK;
        if(key & BTN_REWIND)
            ret |= BUTTON_REWIND;
        if(key & BTN_FAST_FWD)
            ret |= BUTTON_FAST_FWD;
    }

    return ret;
}
