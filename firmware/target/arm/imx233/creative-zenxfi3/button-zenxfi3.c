/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "system.h"
#include "tick.h"
#include "button-imx233.h"
#include "mpr121-zenxfi3.h"

#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3640), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON_(HOLD, GPIO(0, 4), "hold", INVERTED),
    IMX233_BUTTON(VOL_DOWN, GPIO(2, 7), "vol_down", INVERTED),
    IMX233_BUTTON(VOL_UP, GPIO(2, 8), "vol_up", INVERTED),
    IMX233_BUTTON_(JACK, LRADC_EX(5, GT, 2000, I_VDDIO, 0), "jack"),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON_(END, END(), "")
};

/* MPR121 configuration, mostly extracted from OF */
static struct mpr121_config_t mpr121_config =
{
    .ele =
    {
        [0] = {.tth = 5, .rth = 4 },
        [1] = {.tth = 5, .rth = 4 },
        [2] = {.tth = 5, .rth = 4 },
        [3] = {.tth = 5, .rth = 4 },
        [4] = {.tth = 4, .rth = 3 },
        [5] = {.tth = 4, .rth = 3 },
        [6] = {.tth = 5, .rth = 4 },
        [7] = {.tth = 5, .rth = 4 },
        [8] = {.gpio = ELE_GPIO_OUTPUT_OPEN_LED },
    },
    .filters =
    {
        .ele =
        {
            .rising = {.mhd = 1, .nhd = 1, .ncl = 1, .fdl = 1 },
            .falling = {.mhd = 1, .nhd = 1, .ncl = 0xff, .fdl = 2 },
        }
    },
    .autoconf =
    {
        .en = true, .ren = true, .retry = RETRY_NEVER,
        .usl = 0xc4, .lsl = 0x7f, .tl = 0xb0
    },
    .ele_en = ELE_EN0_x(7),
    .cal_lock = CL_TRACK
};

/* B0P18 is #IRQ line of the touchpad */
void button_init_device(void)
{
    mpr121_init();
    mpr121_set_config(&mpr121_config);
    /* generic part */
    imx233_button_init();
}

int button_read_device(void)
{
    /* since sliding hold will usually trigger power, ignore power button
     * for one second after hold is released */
    static int power_ignore_counter = 0;
    static bool old_hold;
    bool hold = button_hold();
    if(hold != old_hold)
    {
        old_hold = hold;
        if(!hold)
            power_ignore_counter = HZ;
    }
    /* interpret touchpad status */
    unsigned status = mpr121_get_touch_status();
    unsigned touchpad_btns = 0;
    /* ELE3: up
     * ELE4: back
     * ELE5: menu
     * ELE6: down
     * ELE7: play */
    if(status & 0x8) touchpad_btns |= BUTTON_UP;
    if(status & 0x10) touchpad_btns |= BUTTON_BACK;
    if(status & 0x20) touchpad_btns |= BUTTON_MENU;
    if(status & 0x40) touchpad_btns |= BUTTON_DOWN;
    if(status & 0x80) touchpad_btns |= BUTTON_PLAY;
    /* feed it to generic code */
    int res = imx233_button_read(touchpad_btns);
    if(power_ignore_counter > 0)
    {
        power_ignore_counter--;
        res &= ~BUTTON_POWER;
    }
    return res;
}
