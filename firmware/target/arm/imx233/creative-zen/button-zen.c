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
#include "button-imx233.h"

#if defined(CREATIVE_ZENXFISTYLE)
#define CHAN    2
#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3660), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 230, I_VDDIO), "menu"),
    IMX233_BUTTON(SHORTCUT, LRADC_REL(CHAN, 480, I_VDDIO), "shortcut"),
    IMX233_BUTTON(UP, LRADC_REL(CHAN, 690, I_VDDIO), "up"),
    IMX233_BUTTON(LEFT, LRADC_REL(CHAN, 920, I_VDDIO), "left"),
    IMX233_BUTTON(RIGHT, LRADC_REL(CHAN, 1120, I_VDDIO), "right"),
    IMX233_BUTTON(DOWN, LRADC_REL(CHAN, 1335, I_VDDIO), "down"),
    IMX233_BUTTON(SELECT, LRADC_REL(CHAN, 1565, I_VDDIO), "select"),
    IMX233_BUTTON(BACK, LRADC_REL(CHAN, 2850, I_VDDIO), "back"),
    IMX233_BUTTON(PLAYPAUSE, LRADC_REL(CHAN, 3110, I_VDDIO), "play"),
    IMX233_BUTTON_(JACK, GPIO(2, 7), "jack"),
    IMX233_BUTTON(POWER, GPIO(0, 11), "power", INVERTED),
    IMX233_BUTTON_(END, END(), "")
};
#elif defined(CREATIVE_ZEN)
#define CHAN    0
#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3480), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON_(HOLD, LRADC_REL(CHAN, 200, I_VDDIO), "hold"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 250, I_VDDIO), "menu"),
    IMX233_BUTTON(SHORTCUT, LRADC_REL(CHAN, 520, I_VDDIO), "shortcut"),
    IMX233_BUTTON(UP, LRADC_REL(CHAN, 1490, I_VDDIO), "up"),
    IMX233_BUTTON(SELECT, LRADC_REL(CHAN, 1740, I_VDDIO), "select"),
    IMX233_BUTTON(LEFT, LRADC_REL(CHAN, 2015, I_VDDIO), "left"),
    IMX233_BUTTON(RIGHT, LRADC_REL(CHAN, 2255, I_VDDIO), "right"),
    IMX233_BUTTON(DOWN, LRADC_REL(CHAN, 2485, I_VDDIO), "down"),
    IMX233_BUTTON(BACK, LRADC_REL(CHAN, 2700, I_VDDIO), "back"),
    IMX233_BUTTON(PLAYPAUSE, LRADC_REL(CHAN, 2945, I_VDDIO), "play"),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON_(END, END(), "")
};
#elif defined(CREATIVE_ZENXFI)
#define CHAN    0
#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3460), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON_(HOLD, LRADC_REL(CHAN, 0, I_VDDIO), "hold"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 200, I_VDDIO), "menu"),
    IMX233_BUTTON(SHORTCUT, LRADC_REL(CHAN, 445, I_VDDIO), "shortcut"),
    IMX233_BUTTON(UP, LRADC_REL(CHAN, 645, I_VDDIO), "up"),
    IMX233_BUTTON(LEFT, LRADC_REL(CHAN, 860, I_VDDIO), "left"),
    IMX233_BUTTON(RIGHT, LRADC_REL(CHAN, 1060, I_VDDIO), "right"),
    IMX233_BUTTON(DOWN, LRADC_REL(CHAN, 1260, I_VDDIO), "down"),
    IMX233_BUTTON(SELECT, LRADC_REL(CHAN, 1480, I_VDDIO), "select"),
    IMX233_BUTTON(TOPRIGHT, LRADC_REL(CHAN, 1700, I_VDDIO), "topright"),
    IMX233_BUTTON(BOTTOMLEFT, LRADC_REL(CHAN, 1920, I_VDDIO), "bottomleft"),
    IMX233_BUTTON(TOPLEFT, LRADC_REL(CHAN, 2145, I_VDDIO), "topleft"),
    IMX233_BUTTON(BOTTOMRIGHT, LRADC_REL(CHAN, 2460, I_VDDIO), "bottomright"),
    IMX233_BUTTON(BACK, LRADC_REL(CHAN, 2700, I_VDDIO), "back"),
    IMX233_BUTTON(PLAYPAUSE, LRADC_REL(CHAN, 2945, I_VDDIO), "play"),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON_(JACK, GPIO(2, 8), "jack", INVERTED),
    IMX233_BUTTON_(END, END(), "")
};
#elif defined(CREATIVE_ZENMOZAIC)
#define CHAN    0
#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3500), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON_(HOLD, LRADC_REL(CHAN, 0, I_VDDIO), "hold"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 200, I_VDDIO), "menu"),
    IMX233_BUTTON(SHORTCUT, LRADC_REL(CHAN, 445, I_VDDIO), "shortcut"),
    IMX233_BUTTON(UP, LRADC_REL(CHAN, 645, I_VDDIO), "up"),
    IMX233_BUTTON(LEFT, LRADC_REL(CHAN, 860, I_VDDIO), "left"),
    IMX233_BUTTON(RIGHT, LRADC_REL(CHAN, 1060, I_VDDIO), "right"),
    IMX233_BUTTON(DOWN, LRADC_REL(CHAN, 1260, I_VDDIO), "down"),
    IMX233_BUTTON(SELECT, LRADC_REL(CHAN, 1480, I_VDDIO), "select"),
    IMX233_BUTTON(BACK, LRADC_REL(CHAN, 2700, I_VDDIO), "back"),
    IMX233_BUTTON(PLAYPAUSE, LRADC_REL(CHAN, 2945, I_VDDIO), "play"),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON_(JACK, GPIO(2, 8), "jack"),
    IMX233_BUTTON_(END, END(), "")
};
#elif defined(CREATIVE_ZENV)
#define CHAN    0
#define I_VDDIO 0 /* index in the table */

struct imx233_button_map_t imx233_button_map[] =
{
    [I_VDDIO] = IMX233_BUTTON_(VDDIO, VDDIO(3500), "vddio"), /* we need VDDIO for relative */
    IMX233_BUTTON_(HOLD, LRADC_REL(CHAN, 190, I_VDDIO), "hold"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 250, I_VDDIO), "play"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 530, I_VDDIO), "back"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 785, I_VDDIO), "vol_up"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 1040, I_VDDIO), "vol_down"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 1295, I_VDDIO), "menu"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 1540, I_VDDIO), "up"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 1800, I_VDDIO), "select"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 2070, I_VDDIO), "left"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 2315, I_VDDIO), "right"),
    IMX233_BUTTON(MENU, LRADC_REL(CHAN, 2550, I_VDDIO), "down"),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON_(END, END(), "")
};
#else
#error wrong target
#endif

void button_init_device(void)
{
    imx233_button_init();
}

int button_read_device(void)
{
    int btn = 0;
#ifdef CREATIVE_ZENXFISTYLE
#else
    if(imx233_power_read_pswitch() == 1)
        btn |= BUTTON_POWER;
#endif
    return imx233_button_read(btn);
}
