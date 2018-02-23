/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2018 by Amaury Pouly
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
#include "gpio-jz4760b.h"


void button_init_device(void)
{
    jz_gpio_setup_std_in(0, 30); /* PA30: power */
    jz_gpio_setup_std_in(2, 29); /* PC29: vol down */
    jz_gpio_setup_std_in(2, 31); /* PC31: vol up */
    jz_gpio_setup_std_in(3, 17); /* PD17: select */
    jz_gpio_setup_std_in(3, 27); /* PD27: left */
    jz_gpio_setup_std_in(4, 8); /* PE8: back */
    jz_gpio_setup_std_in(4, 9); /* PE9: right */
    jz_gpio_setup_std_in(5, 11); /* PF11: menu */
}

int button_read_device(void)
{
    int btn = 0;
    if(jz_gpio_get_input(0, 30))
        btn |= BUTTON_POWER;
    unsigned mask = jz_gpio_get_input_mask(2, 0xffffffff);
    if(!(mask & (1 << 29)))
        btn |= BUTTON_VOL_DOWN;
    if(!(mask & (1 << 31)))
        btn |= BUTTON_VOL_UP;
    mask = jz_gpio_get_input_mask(3, 0xffffffff);
    if(!(mask & (1 << 17)))
        btn |= BUTTON_SELECT;
    if(!(mask & (1 << 27)))
        btn |= BUTTON_LEFT;
    mask = jz_gpio_get_input_mask(4, 0xffffffff);
    if(!(mask & (1 << 8)))
        btn |= BUTTON_BACK;
    if(!(mask & (1 << 9)))
        btn |= BUTTON_RIGHT;
    if(!jz_gpio_get_input(5, 11))
        btn |= BUTTON_MENU;
    return btn;
}
