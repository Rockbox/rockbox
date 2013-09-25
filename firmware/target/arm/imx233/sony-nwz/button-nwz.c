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
#include "button-lradc-imx233.h"

struct imx233_button_lradc_mapping_t imx233_button_lradc_mapping[] =
{
#ifdef SONY_NWZE360
    {195, BUTTON_VOL_DOWN},
    {810, BUTTON_VOL_UP},
#endif
    {1095, BUTTON_BACK},
    {1470, BUTTON_PLAY},
    {1845, BUTTON_RIGHT},
    {2185, BUTTON_LEFT},
    {2525, BUTTON_UP},
    {2870, BUTTON_DOWN},
    {3400, 0},
    {0, IMX233_BUTTON_LRADC_END},
};

void button_init_device(void)
{
    imx233_button_lradc_init();
}

#ifdef HAS_BUTTON_HOLD
bool button_hold(void)
{
    return imx233_button_lradc_hold();
}
#endif

int button_read_device(void)
{
    int res = 0;
    if(BF_RD(POWER_STS, PSWITCH) == 3)
        res |= BUTTON_POWER;
    return imx233_button_lradc_read(res);
}
