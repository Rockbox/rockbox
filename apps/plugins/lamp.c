/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Vuong Minh Hiep (vmh)
 * Copyright (C) 2008 Thomas Martitz (kugel.)
 * Copyright (C) 2008 Alexander Papst
 * Copyright (C) 2008 Peter D'Hoye
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

#include "plugin.h"
#include "lib/helper.h"
#include "lib/pluginlib_actions.h"

/* this set the context to use with PLA */
static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* variable button definitions.
 - only targets with a colour display
    LAMP_LEFT / LAMP_RIGHT: change the color
    LAMP_NEXT / LAMP_PREV:  (optional) change the color
 - only targets which can set brightness
    LAMP_UP / LAMP_DOWN:    change the brightness
*/

/* we use PLA */
#ifdef HAVE_SCROLLWHEEL
#   define LAMP_LEFT              PLA_LEFT
#   define LAMP_RIGHT             PLA_RIGHT
#   define LAMP_UP                PLA_SCROLL_FWD
#   define LAMP_DOWN              PLA_SCROLL_BACK
#   define LAMP_UP_REPEAT         PLA_SCROLL_FWD_REPEAT
#   define LAMP_DOWN_REPEAT       PLA_SCROLL_BACK_REPEAT
#else
#   define LAMP_LEFT              PLA_LEFT
#   define LAMP_RIGHT             PLA_RIGHT
#   define LAMP_UP                PLA_UP
#   define LAMP_DOWN              PLA_DOWN
#   define LAMP_UP_REPEAT         PLA_UP_REPEAT
#   define LAMP_DOWN_REPEAT       PLA_DOWN_REPEAT
#endif/* HAVE_SCROLLWHEEL */


#define LAMP_EXIT        PLA_EXIT
#define LAMP_EXIT2       PLA_CANCEL


#ifdef HAVE_LCD_COLOR
/* RGB color sets */
#define NUM_COLORSETS   2
static unsigned colorset[NUM_COLORSETS] = {
    LCD_RGBPACK(255, 255, 255),    /* white */
    LCD_RGBPACK(255,   0,   0),    /* red */
};
#endif /* HAVE_LCD_COLOR */

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    enum plugin_status status = PLUGIN_OK;
    long button;
    bool quit = false;
    (void)parameter;

#ifdef HAVE_LCD_COLOR
    int cs = 0;
    bool update = false;
#endif /* HAVE_LCD_COLOR */

#if LCD_DEPTH > 1
    unsigned bg_color = rb->lcd_get_background();
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background(LCD_WHITE);
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    int current_brightness = MAX_BRIGHTNESS_SETTING;
    backlight_brightness_set(MAX_BRIGHTNESS_SETTING);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    buttonlight_brightness_set(MAX_BRIGHTNESS_SETTING);
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */

#ifdef HAVE_LCD_INVERT
#ifdef HAVE_NEGATIVE_LCD
    rb->lcd_set_invert_display(true);
#else
    rb->lcd_set_invert_display(false);
#endif /* HAVE_NEGATIVE_LCD */
#endif /* HAVE_LCD_INVERT */

    backlight_force_on();
#ifdef HAVE_BUTTON_LIGHT
    buttonlight_force_on();
#endif /* HAVE_BUTTON_LIGHT */

    rb->lcd_clear_display();
    rb->lcd_update();

    do
    {
#ifdef HAVE_LCD_COLOR
        if(update)
        {
            if(cs < 0)
                cs = NUM_COLORSETS-1;
            if(cs >= NUM_COLORSETS)
                cs = 0;
            rb->lcd_set_background(colorset[cs]);
            rb->lcd_clear_display();
            rb->lcd_update();
            update = false;
        }
#endif /* HAVE_LCD_COLOR */
        button = pluginlib_getaction(HZ*30, plugin_contexts,
                               ARRAYLEN(plugin_contexts));

        switch(button)
        {
#ifdef HAVE_LCD_COLOR
            case LAMP_RIGHT:
#ifdef LAMP_NEXT
            case LAMP_NEXT:
#endif /* LAMP_NEXT */
                cs++;
                update = true;
                break;

            case LAMP_LEFT:
#ifdef LAMP_PREV
            case LAMP_PREV:
#endif /* LAMP_PREV */
                cs--;
                update = true;
                break;
#endif /* HAVE_LCD_COLOR */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
            case LAMP_UP:
            case (LAMP_UP_REPEAT):
                if (current_brightness < MAX_BRIGHTNESS_SETTING)
                    backlight_brightness_set(++current_brightness);
                break;

            case LAMP_DOWN:
            case (LAMP_DOWN_REPEAT):
                if (current_brightness > MIN_BRIGHTNESS_SETTING)
                    backlight_brightness_set(--current_brightness);
                break;
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
            case LAMP_EXIT:
            case LAMP_EXIT2:
                quit = true;
                break;
            case BUTTON_NONE:
                /* time out */
                break;
            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    status = PLUGIN_USB_CONNECTED;
                    quit = true;
                }
        }
        rb->reset_poweroff_timer();
    } while (!quit);

    /* restore */
    backlight_use_settings();
#ifdef HAVE_BUTTON_LIGHT
    buttonlight_use_settings();
#endif /* HAVE_BUTTON_LIGHT */

#ifdef HAVE_LCD_INVERT
    rb->lcd_set_invert_display(rb->global_settings->invert);
#endif /* HAVE_LCD_INVERT */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    backlight_brightness_use_setting();
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    buttonlight_brightness_use_setting();
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */

#if LCD_DEPTH > 1
    rb->lcd_set_background(bg_color);
#endif
    return status;
}
