/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Lorenzo Miori
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
#include "lcd.h"
#include "string.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"
#include "button-lradc-imx233.h"
#include "button-target.h"

#ifndef BOOTLOADER
#include "touchscreen.h"
#include "touchscreen-imx233.h"
#include "button.h"
#include "font.h"
#include "action.h"
#endif

struct imx233_button_lradc_mapping_t imx233_button_lradc_mapping[] =
{
    {455, BUTTON_VOL_UP},
    {900, BUTTON_VOL_DOWN},
    {1410, BUTTON_BACK},
    {1868, BUTTON_FF},
    {2311, BUTTON_REW},
    {2700, 0},
    {3300, IMX233_BUTTON_LRADC_END},
};

#ifndef BOOTLOADER
static int last_x = 0;
static int last_y = 0;
static bool touching = false;
#endif /* BOOTLOADER */

#ifndef BOOTLOADER

/* Touchpad extra pin initialization
 * Strange facts:
 * 1. In the fully working sample I have, it seems that pins
 * must be all set to low
 * 2. In the other sample without LCD, it seems (by measurement) that
 * not all the pins are set to low! Actually, I still need to see if
 * touchpad works in this other sample.
*/
void touchpad_pin_setup(void)
{
    /* TX+ */
    imx233_pinctrl_acquire(0, 25, "touchpad X+ power low");
    imx233_pinctrl_set_function(0, 25, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(0, 25, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(0, 25, true);

    /* TY+ */
    imx233_pinctrl_acquire(0, 26, "touchpad Y+ power high");
    imx233_pinctrl_set_function(0, 26, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(0, 26, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(0, 26, true);

    /* TY- */
    imx233_pinctrl_acquire(1, 21, "touchpad Y- power low");
    imx233_pinctrl_set_function(1, 21, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(1, 21, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(1, 21, true);

    /* TX- */
    imx233_pinctrl_acquire(3, 15, "touchpad X- power high");
    imx233_pinctrl_set_function(3, 15, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(3, 15, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(3, 15, true);

}
#endif /* BOOTLOADER */

void button_init_device(void)
{
    imx233_button_lradc_init();
#ifndef BOOTLOADER
    touchpad_pin_setup();
    /* Now that is powered up, proceed with touchpad initialization */
    imx233_touchscreen_init();
    imx233_touchscreen_enable(true);
#endif /* BOOTLOADER */
}

bool button_hold(void)
{
    return imx233_button_lradc_hold();
}

/* X, Y, RadiusX, RadiusY */
#define TOUCH_UP        2400, 1050, 650, 250
#define TOUCH_DOWN      2057, 3320, 500, 350
#define TOUCH_LEFT      3581, 2297, 300, 350
#define TOUCH_RIGHT     1000, 2100, 400, 700
#define TOUCH_CENTER    2682, 2167, 335, 276

bool coord_in_radius(int x, int y, int cx, int cy, int rx, int ry)
{
    return ((x >= cx - rx && x <= cx + rx) && (y >= cy - ry && y <= cy + ry));
}

int button_read_device(void)
{
    int res = 0;

    switch(imx233_power_read_pswitch())
    {
        case 1: res |= BUTTON_POWER; break;
        case 3: res |= BUTTON_SELECT; break;
    }

#ifndef BOOTLOADER
    touching = imx233_touchscreen_get_touch(&last_x, &last_y);
    if(touching)
    {
        if (coord_in_radius(last_x, last_y, TOUCH_LEFT))
        {
            res |= BUTTON_LEFT;
        }
        else if (coord_in_radius(last_x, last_y, TOUCH_RIGHT))
        {
            res |= BUTTON_RIGHT;
        }
        else if (coord_in_radius(last_x, last_y, TOUCH_DOWN))
        {
            res |= BUTTON_DOWN;
        }
        else if (coord_in_radius(last_x, last_y, TOUCH_UP))
        {
            res |= BUTTON_UP;
        }
    }
#endif /* BOOTLOADER */
    return imx233_button_lradc_read(res);
}

#ifndef BOOTLOADER

#define MAX_ENTRIES         100
#define VIEWPORT_HEIGHT     100
#define VIEWPORT_WIDTH      100
#define MAX_X               3700
#define MAX_Y               3700
#define ADAPT_TO_VIEWPORT(cx, cy, rx, ry)       ((float)(cx) / MAX_X) * VIEWPORT_WIDTH, \
                                                ((float)(cy) / MAX_Y) * VIEWPORT_HEIGHT, \
                                                ((float)(rx) / MAX_X) * VIEWPORT_WIDTH, \
                                                ((float)(ry) / MAX_Y) * VIEWPORT_HEIGHT
static void draw_calibration_rect(int cx, int cy, int rx, int ry)
{
    if (coord_in_radius(last_x, last_y, cx, cy, rx, ry))
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0xff, 0xff), LCD_BLACK);
    else
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0, 0), LCD_BLACK);
    lcd_drawrect(ADAPT_TO_VIEWPORT(cx - rx, cy - ry, 2 * rx, 2 * ry));
}

bool button_debug_screen(void)
{
    int last = 0;
    struct point_t
    {
        int x;
        int y;
    };
    struct point_t last_entries[MAX_ENTRIES];
    struct viewport report_vp;

    memset(&report_vp, 0, sizeof(report_vp));
    report_vp.x = (LCD_WIDTH - VIEWPORT_WIDTH) / 2;
    report_vp.y = (LCD_HEIGHT - VIEWPORT_HEIGHT) / 2;
    report_vp.width = VIEWPORT_WIDTH;
    report_vp.height = VIEWPORT_HEIGHT;

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    while(1)
    {
        int button = get_action(CONTEXT_STD, HZ / 10);
        switch(button)
        {
            case ACTION_STD_OK:
            case ACTION_STD_MENU:
                lcd_set_viewport(NULL);
                lcd_setfont(FONT_UI);
                lcd_clear_display();
                return true;
            case ACTION_STD_CANCEL:
                lcd_set_viewport(NULL);
                lcd_setfont(FONT_UI);
                lcd_clear_display();
                return false;
        }

        lcd_set_viewport(NULL);
        lcd_putsf(0, 1, "(%d,%d) %s", last_x, last_y, touching ? "touching!" : "");
        lcd_putsf(0, 0, "Type %s", imx233_pinctrl_get_gpio(0, 31) ? "CAP" : "REG");
        lcd_set_viewport(&report_vp);
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0, 0, 0xff), LCD_BLACK);
        lcd_drawrect(0, 0, 100, 100);
        float percent_x = ((float)(last_x) / MAX_X);
        float percent_y = ((float)(last_y) / MAX_Y);
        if (touching)
        {
            lcd_set_viewport(NULL);
            if (last < MAX_ENTRIES)
            {
                last_entries[last].x = last_x;
                last_entries[last].y = last_y;
                last++;
                lcd_putsf(0, 17, "Recording: %d captures left", MAX_ENTRIES - last);
            }
            else
            {
                int min_x = 9999;
                int min_y = 9999;
                int max_x = -1;
                int max_y = -1;
                int median_x = 0;
                int median_y = 0;
                for (int i = 0; i < MAX_ENTRIES; i++)
                {
                    min_x = MIN(min_x, last_entries[i].x);
                    min_y = MIN(min_y, last_entries[i].y);
                    max_x = MAX(max_x, last_entries[i].x);
                    max_y = MAX(max_y, last_entries[i].y);
                    median_x += last_entries[i].x;
                    median_y += last_entries[i].y;
                }
                median_x /= MAX_ENTRIES;
                median_y /= MAX_ENTRIES;
                lcd_putsf(0, 17, "center(%d,%d)", median_x, median_y);
                lcd_putsf(0, 18, "radius(%d,%d)", median_x / 2, median_y / 2);
            }
            lcd_set_viewport(&report_vp);
            lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0x8c, 0), LCD_BLACK);
            lcd_fillrect(VIEWPORT_WIDTH * percent_x, VIEWPORT_HEIGHT * percent_y, 2, 2);
        }

        /* Draw current calibration settings */
        lcd_set_viewport(&report_vp);
        draw_calibration_rect(TOUCH_UP);
        draw_calibration_rect(TOUCH_DOWN);
        draw_calibration_rect(TOUCH_CENTER);
        draw_calibration_rect(TOUCH_LEFT);
        draw_calibration_rect(TOUCH_RIGHT);

        lcd_update();
        yield();
    }

    return true;
}
#endif
