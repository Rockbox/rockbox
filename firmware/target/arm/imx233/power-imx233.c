/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "power.h"
#include "system-target.h"
#include "usb-target.h"

void INT_VDD5V(void)
{
    if(HW_POWER_CTRL & HW_POWER_CTRL__VBUSVALID_IRQ)
    {
        if(HW_POWER_STS & HW_POWER_STS__VBUSVALID)
            usb_insert_int();
        else
            usb_remove_int();
        /* reverse polarity */
        __REG_TOG(HW_POWER_CTRL) = HW_POWER_CTRL__POLARITY_VBUSVALID;
        /* enable int */
        __REG_CLR(HW_POWER_CTRL) = HW_POWER_CTRL__VBUSVALID_IRQ;
    }
}

void power_init(void)
{
    /* clear vbusvalid irq and set correct polarity */
    __REG_CLR(HW_POWER_CTRL) = HW_POWER_CTRL__VBUSVALID_IRQ;
    if(HW_POWER_STS & HW_POWER_STS__VBUSVALID)
        __REG_CLR(HW_POWER_CTRL) = HW_POWER_CTRL__POLARITY_VBUSVALID;
    else
        __REG_SET(HW_POWER_CTRL) = HW_POWER_CTRL__POLARITY_VBUSVALID;
    __REG_SET(HW_POWER_CTRL) = HW_POWER_CTRL__ENIRQ_VBUS_VALID;
    imx233_enable_interrupt(INT_SRC_VDD5V, true);
}

void power_off(void)
{
    /* power down */
    HW_POWER_RESET = HW_POWER_RESET__UNLOCK | HW_POWER_RESET__PWD;
    while(1);
}

unsigned int power_input_status(void)
{
    return POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return false;
}
