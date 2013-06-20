/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"
#include "generic_i2c.h"
#include "synaptics-rmi.h"
#include "lcd.h"
#include "string.h"
#include "usb.h"
#include "power-imx233.h"
#include "adc.h"
#include "lradc-imx233.h"
#include "backlight.h"
#ifndef BOOTLOADER
#include "touchscreen.h"
#include "touchscreen-imx233.h"
#include "button.h"
#include "font.h"
#include "action.h"
#endif

/* physical channel */
#define CHAN    0
/* number of irq per second */
#define RATE    HZ
/* number of samples per irq */
#define SAMPLES 10
/* delay's delay */
#define DELAY   (LRADC_DELAY_FREQ / RATE / SAMPLES)

static int button_delay;
static int button_chan;
static int button_val;
#ifndef BOOTLOADER
static int last_x = 0;
static int last_y = 0;
static bool touching = false;
//static bool old_touching = false;
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
    //imx233_pinctrl_set_gpio(0, 25, false);
    
    /* TY+ */
    imx233_pinctrl_acquire(0, 26, "touchpad Y+ power high");
    imx233_pinctrl_set_function(0, 26, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(0, 26, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(0, 26, true);
    //imx233_pinctrl_set_gpio(0, 26, false);

    /* TY- */
    imx233_pinctrl_acquire(1, 21, "touchpad Y- power low");
    imx233_pinctrl_set_function(1, 21, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(1, 21, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(1, 21, true);
    //imx233_pinctrl_set_gpio(1, 21, false);

    /* TX- */
    imx233_pinctrl_acquire(3, 15, "touchpad X- power high");
    imx233_pinctrl_set_function(3, 15, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(3, 15, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(3, 15, true);
    //imx233_pinctrl_set_gpio(3, 15, false);

}
#endif /* BOOTLOADER */

void button_lradc_irq(int chan)
{
    (void) chan;
    button_val = imx233_lradc_read_channel(button_chan) / SAMPLES;
    imx233_lradc_clear_channel(button_chan);
    imx233_lradc_setup_channel(button_chan, true, true, SAMPLES - 1, LRADC_SRC(CHAN));
    imx233_lradc_setup_delay(button_delay, 1 << button_chan, 0, SAMPLES - 1, DELAY);
    imx233_lradc_kick_delay(button_delay);
}

void button_init_device(void)
{
    /* hold button */
    imx233_pinctrl_acquire(0, 13, "hold");
    imx233_pinctrl_set_function(0, 13, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 13, false);

    /* all the other buttons */
    button_chan = imx233_lradc_acquire_channel(LRADC_SRC(CHAN), TIMEOUT_NOBLOCK);
    if(button_chan < 0)
        panicf("Cannot get channel for button");
    button_delay = imx233_lradc_acquire_delay(TIMEOUT_NOBLOCK);
    if(button_delay < 0)
        panicf("Cannot get delay for button");
    imx233_lradc_setup_channel(button_chan, true, true, SAMPLES - 1, LRADC_SRC(CHAN));
    imx233_lradc_setup_delay(button_delay, 1 << button_chan, 0, SAMPLES - 1, DELAY);
    imx233_lradc_enable_channel_irq(button_chan, true);
    imx233_lradc_set_channel_irq_callback(button_chan, button_lradc_irq);
    imx233_lradc_kick_delay(button_delay);
    
#ifndef BOOTLOADER
    touchpad_pin_setup();
    /* Now that is powered up, proceed with touchpad initialization */
    imx233_touchscreen_init();
    imx233_touchscreen_enable(true);
#endif /* BOOTLOADER */
}

bool button_hold(void)
{
    return !!imx233_pinctrl_get_gpio(0, 13);
}

/*  Buttons raw values
    2311 rew
    1868 ff
    1417 back
    900 vol down
    455 vol up
 */

/*
  Coordinates -> button mapping
  X, Y, RadiusX, RadiusY
 */
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
    static bool old_hold;

    /* light handling */
    bool hold = button_hold();
    if(hold != old_hold)
    {
        old_hold = hold;
#ifndef BOOTLOADER
        backlight_hold_changed(hold);
#endif /* BOOTLOADER */
    }
    /* Center button pressed when PSWITCH bits [2:3] are both high
       Play button pressed when PSWITCH bit 2 is high and bit 3 is low
    */
    switch(BF_RD(DIGCTL_STATUS, PSWITCH))
    {
        case 1: res |= BUTTON_POWER; break;
        case 3: res |= BUTTON_SELECT; break;
    }

#ifndef BOOTLOADER
    touching = imx233_touchscreen_get_touch(&last_x, &last_y);
    if(touching)
    {
        // TODO some kind of debouncing may also help center button 
        //if (!(res & BUTTON_SELECT || coord_in_radius(last_x, last_y, TOUCH_CENTER)))
        //{
            /* Any direction that is not center nor right
             * Why right? The movement makes the right side to be pressed
             */
            if (coord_in_radius(last_x, last_y, TOUCH_LEFT))
            {
                res |= BUTTON_LEFT;
                goto end;
            }
            if (coord_in_radius(last_x, last_y, TOUCH_RIGHT))
            {
                res |= BUTTON_RIGHT;
                goto end;
            }
            if (coord_in_radius(last_x, last_y, TOUCH_DOWN))
            {
                res |= BUTTON_DOWN;
                goto end;
            }
            if (coord_in_radius(last_x, last_y, TOUCH_UP))
            {
                res |= BUTTON_UP;
                goto end;
            }
        //}
     }
     end:
#endif /* BOOTLOADER */

    /* Values below or above certain values are garbage */
    if(button_val < 200 || button_val > 2600)
        return res;
    if(button_val < 1700)
    {
        if(button_val > 1300)
        {
            res |= BUTTON_BACK;
        }
        else if(button_val < 1100)
        {
            if (button_val > 700)
                res |= BUTTON_VOL_DOWN;
            else
                res |= BUTTON_VOL_UP;
        }
    }
    else
    {
        if(button_val > 2350)
            res |= BUTTON_REW;
        else
            res |= BUTTON_FF;
    }
    return res;    
}
#ifndef BOOTLOADER

#define MAX_ENTRIES     100
#define VIEWPORT_HEIGHT 100
#define VIEWPORT_WIDTH 100
#define MAX_X   3700
#define MAX_Y   3700
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
//        lcd_putsf(0, 3, "Buttons ADC %d", button_val);
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
                //lcd_putsf(0, 17, "c(%d,%d)m(%d,%d)", max_x - ((max_x - min_x) / 2), max_y - ((max_y - min_y) / 2), median_x, median_y);
                //lcd_putsf(0, 18, "r(%d,%d)", (max_x - min_x) / 2, (max_y - min_y) / 2);
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