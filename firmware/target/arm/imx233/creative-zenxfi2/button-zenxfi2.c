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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"
#include "string.h"
#include "usb.h"
#include "touchscreen.h"
#include "touchscreen-imx233.h"
#include "button-imx233.h"

struct imx233_button_map_t imx233_button_map[] =
{
    IMX233_BUTTON(POWER, GPIO(0, 11), "power", INVERTED),
    IMX233_BUTTON(MENU, GPIO(0, 14), "menu", INVERTED),
    IMX233_BUTTON_(JACK, GPIO(2, 7), "jack"),
    IMX233_BUTTON_(END, END(), "")
};

struct touch_calibration_point
{
    short px_x; /* known pixel value */
    short px_y;
    short val_x; /* touchscreen value at the known pixel */
    short val_y;
};

static struct touch_calibration_point topleft, bottomright;

/* Amaury Pouly: values on my device
 * Portait: (x and y are swapped)
 *   (0,0) = 200, 300
 *   (240,400) = 3900, 3800
 * Landscape:
 *   (0,0) = 200, 3800
 *   (400,240) = 3900, 300
*/
void button_init_device(void)
{
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    topleft.val_x = 300;
    topleft.val_y = 200;

    bottomright.val_x = 3800;
    bottomright.val_y = 3900;
#else
    topleft.val_x = 300;
    topleft.val_y = 3900;

    bottomright.val_x = 3800;
    bottomright.val_y = 200;
#endif
    topleft.px_x = 0;
    topleft.px_y = 0;

    bottomright.px_x = LCD_WIDTH;
    bottomright.px_y = LCD_HEIGHT;

    imx233_touchscreen_init();
    imx233_touchscreen_enable(true);
    /* generic */
    imx233_button_init();
}

static void fix_pixels(int *val_x, int *val_y)
{
    short x,y;

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    x = *val_y;
    y = *val_x;
#else
    x = *val_x;
    y = *val_y;
#endif

    x = (x - topleft.val_x) * (bottomright.px_x - topleft.px_x) / (bottomright.val_x - topleft.val_x) + topleft.px_x;
    y = (y - topleft.val_y) * (bottomright.px_y - topleft.px_y) / (bottomright.val_y - topleft.val_y) + topleft.px_y;

    x = MAX(0, MIN(x, LCD_WIDTH - 1));
    y = MAX(0, MIN(y, LCD_HEIGHT - 1));

    *val_x = x;
    *val_y = y;;
}

void touchscreen_enable_device(bool en)
{
    imx233_touchscreen_enable(en);
}

static int touchscreen_read_device(int *data)
{
    int x, y;
    bool touch = imx233_touchscreen_get_touch(&x, &y);
    fix_pixels(&x, &y);
    int btns = touchscreen_to_pixels(x, y, data);
    return touch ? btns : 0;
}

int button_read_device(int *data)
{
    return imx233_button_read(touchscreen_read_device(data));
}
