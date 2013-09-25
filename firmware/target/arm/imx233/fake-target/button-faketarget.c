/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"
#include "string.h"
#include "usb.h"

void button_init_device(void)
{
#warning do some init ?
}

#ifdef HAS_BUTTON_HOLD
bool button_hold(void)
{
#warning implement button hold or remove #define HAS_BUTTON_HOLD in button-target.h
    return false;
}
#endif

int button_read_device(void)
{
#warning Power button is typically wired to PSWITCH, other buttons use other ways
    int res = 0;
    if(BF_RD(POWER_STS, PSWITCH) == 3)
        res |= BUTTON_POWER;
    return res;
}
