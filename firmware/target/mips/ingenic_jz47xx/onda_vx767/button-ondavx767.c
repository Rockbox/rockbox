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

#define BTN_VOL_DOWN (1 << 27)
#define BTN_VOL_UP   (1 << 0)
#define BTN_MENU     (1 << 1)
#define BTN_OFF      (1 << 29)
#define BTN_HOLD     (1 << 16)
#define BTN_MASK     (BTN_VOL_DOWN | BTN_VOL_UP \
                      | BTN_MENU | BTN_OFF )

bool button_hold(void)
{
    return (~REG_GPIO_PXPIN(3) & BTN_HOLD ? 1 : 0);
}

void button_init_device(void)
{  
    __gpio_port_as_input(3, 29);
    __gpio_port_as_input(3, 27);
    __gpio_port_as_input(3, 16);
    __gpio_port_as_input(3, 1);
    __gpio_port_as_input(3, 0);
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
        if(key & BTN_MENU)
            ret |= BUTTON_MENU;
        if(key & BTN_OFF)
            ret |= BUTTON_POWER;
    }

    return ret;
}
