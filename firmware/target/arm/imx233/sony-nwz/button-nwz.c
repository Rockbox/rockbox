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
#include "button-imx233.h"

#define CHAN    0
#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3630), "vddio"), /* we need VDDIO for relative */
#ifdef SONY_NWZE360
    IMX233_BUTTON(VOL_DOWN, LRADC_REL(CHAN, 180, I_VDDIO), "vol_down"),
    IMX233_BUTTON(VOL_UP, LRADC_REL(CHAN, 830, I_VDDIO), "vol_up"),
#endif
    IMX233_BUTTON(BACK, LRADC_REL(CHAN, 1135, I_VDDIO), "back"),
    IMX233_BUTTON(PLAY, LRADC_REL(CHAN, 1536, I_VDDIO), "play"),
    IMX233_BUTTON(RIGHT, LRADC_REL(CHAN, 1930, I_VDDIO), "right"),
    IMX233_BUTTON(LEFT, LRADC_REL(CHAN, 2290, I_VDDIO), "left"),
    IMX233_BUTTON(UP, LRADC_REL(CHAN, 2655, I_VDDIO), "up"),
    IMX233_BUTTON(DOWN, LRADC_REL(CHAN, 3020, I_VDDIO), "down"),
    IMX233_BUTTON(POWER, PSWITCH(3), "power"),
#ifdef HAS_BUTTON_HOLD
    IMX233_BUTTON_(HOLD, GPIO(0, 9), "hold", INVERTED, PULLUP),
#endif
    IMX233_BUTTON_(END, END(), "")
};

void button_init_device(void)
{
    imx233_button_init();
}

int button_read_device(void)
{
    return imx233_button_read(0);
}
