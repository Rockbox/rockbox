/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Rob Purchase
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



#if (CONFIG_KEYPAD == COWON_D2_PAD)
#define TOUCHSCREEN_QUIT   BUTTON_POWER
#define TOUCHSCREEN_TOGGLE BUTTON_MENU
#elif (CONFIG_KEYPAD == MROBE500_PAD)
#define TOUCHSCREEN_QUIT   BUTTON_POWER
#define TOUCHSCREEN_TOGGLE BUTTON_RC_MODE
#elif (CONFIG_KEYPAD == ONDAVX747_PAD)
#define TOUCHSCREEN_QUIT   BUTTON_POWER
#define TOUCHSCREEN_TOGGLE BUTTON_MENU
#elif (CONFIG_KEYPAD == ONDAVX777_PAD)
#define TOUCHSCREEN_QUIT   BUTTON_POWER
#define TOUCHSCREEN_TOGGLE BUTTON_MENU
#elif CONFIG_KEYPAD == CREATIVE_ZENXFI2_PAD
#define TOUCHSCREEN_QUIT   BUTTON_POWER
#define TOUCHSCREEN_TOGGLE BUTTON_MENU
#elif CONFIG_KEYPAD == SHANLING_Q1_PAD || CONFIG_KEYPAD == HIBY_R3PROII_PAD
#define TOUCHSCREEN_QUIT   BUTTON_POWER
#define TOUCHSCREEN_TOGGLE BUTTON_PLAY
#elif (CONFIG_KEYPAD == ANDROID_PAD)
#define TOUCHSCREEN_QUIT   BUTTON_BACK
#define TOUCHSCREEN_TOGGLE BUTTON_MENU
#elif (CONFIG_KEYPAD == SDL_PAD)
#define TOUCHSCREEN_QUIT   BUTTON_MIDLEFT
#define TOUCHSCREEN_TOGGLE BUTTON_CENTER
#else
# error "No keymap defined!"
#endif

static bool usb_exit = false;

static const struct button_mapping tstest_context[] = {
    {ACTION_STD_OK,     TOUCHSCREEN_TOGGLE, BUTTON_NONE},
    {ACTION_STD_CANCEL, TOUCHSCREEN_QUIT,   BUTTON_NONE},
    LAST_ITEM_IN_LIST,
};

static const struct button_mapping *get_context_map(int context)
{
    (void)context;
    return tstest_context;
}

static void draw_tsbutton_rect(int x, int y)
{
    int x0 = x*LCD_WIDTH/3;
    int y0 = y*LCD_HEIGHT/3;
    int x1 = (x+1)*LCD_WIDTH/3;
    int y1 = (y+1)*LCD_HEIGHT/3;

    rb->lcd_set_foreground(LCD_RGBPACK(0xc0, 0, 0));
    rb->lcd_fillrect(x0, y0, x1 - x0, y1 - y0);
}

static void draw_crosshair(int x, int y, unsigned fg)
{
    rb->lcd_set_foreground(fg);
    rb->lcd_hline(x-7, x+7, y);
    rb->lcd_vline(x, y-7, y+7);
}

static void tstest_button_mode(void)
{
    rb->splashf(HZ, "Button mode test");
    rb->touchscreen_set_mode(TOUCHSCREEN_BUTTON);

    int button = BUTTON_NONE;
    while (true)
    {
        rb->lcd_clear_display();

        if ((button & BUTTON_TOPLEFT) && !(button & BUTTON_REL))
            draw_tsbutton_rect(0, 0);
        if ((button & BUTTON_TOPMIDDLE) && !(button & BUTTON_REL))
            draw_tsbutton_rect(1, 0);
        if ((button & BUTTON_TOPRIGHT) && !(button & BUTTON_REL))
            draw_tsbutton_rect(2, 0);
        if ((button & BUTTON_MIDLEFT) && !(button & BUTTON_REL))
            draw_tsbutton_rect(0, 1);
        if ((button & BUTTON_CENTER) && !(button & BUTTON_REL))
            draw_tsbutton_rect(1, 1);
        if ((button & BUTTON_MIDRIGHT) && !(button & BUTTON_REL))
            draw_tsbutton_rect(2, 1);
        if ((button & BUTTON_BOTTOMLEFT) && !(button & BUTTON_REL))
            draw_tsbutton_rect(0, 2);
        if ((button & BUTTON_BOTTOMMIDDLE) && !(button & BUTTON_REL))
            draw_tsbutton_rect(1, 2);
        if ((button & BUTTON_BOTTOMRIGHT) && !(button & BUTTON_REL))
            draw_tsbutton_rect(2, 2);

        rb->lcd_update();

        if (button & TOUCHSCREEN_QUIT)
            break;
        if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
        {
            usb_exit = true;
            break;
        }

        button = rb->button_get(true);
    }
}

static void tstest_pointing_mode(void)
{
    struct touchevent tevent;

    rb->splashf(HZ, "Pointing mode test");
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);

    rb->lcd_clear_display();
    rb->lcd_update();

    while (true)
    {
        int action = rb->get_custom_action(CONTEXT_PLUGIN, TIMEOUT_BLOCK,
                                           get_context_map);
        if (action == ACTION_TOUCHSCREEN)
        {
            rb->lcd_clear_display();
            rb->action_get_touch_event(&tevent);
            draw_crosshair(tevent.x, tevent.y, LCD_RGBPACK(0, 0xc0, 0));
            rb->lcd_update();
        }
        else if (action == ACTION_STD_CANCEL)
        {
            break;
        }
        else if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
        {
            usb_exit = true;
            break;
        }
    }
}

static void tstest_gesture(void)
{
    static const char *const gesture_names[] = {
        [GESTURE_NONE]       = "none",
        [GESTURE_TAP]        = "tap",
        [GESTURE_LONG_PRESS] = "long press",
        [GESTURE_HOLD]       = "hold",
        [GESTURE_DRAGSTART]  = "dragstart",
        [GESTURE_DRAG]       = "drag",
        [GESTURE_RELEASE]    = "release",
    };

    struct gesture_event gevt;
    int gevent_array[10];
    int gevent_count = 0;

    rb->splashf(HZ, "Gesture test");
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);

    rb->lcd_clear_display();
    rb->lcd_update();

    while (true)
    {
        int action = rb->get_custom_action(CONTEXT_PLUGIN, TIMEOUT_BLOCK,
                                           get_context_map);
        if (action == ACTION_TOUCHSCREEN)
        {
            rb->lcd_clear_display();

            if (rb->action_gesture_get_event_in_vp(&gevt, NULL))
            {
                if (gevt.id == GESTURE_NONE)
                {
                    gevent_array[gevent_count] = GESTURE_NONE;
                    gevent_count = 1;
                }

                if (gevt.id != gevent_array[gevent_count-1])
                {
                    if (gevent_count < (int)ARRAYLEN(gevent_array))
                    {
                        gevent_array[gevent_count] = gevt.id;
                        gevent_count++;
                    }
                }

                rb->lcd_set_foreground(LCD_RGBPACK(0xff, 0xff, 0xff));
                for (int i = 0; i < gevent_count; ++i)
                    rb->lcd_puts(0, i, gesture_names[gevent_array[i]]);

                draw_crosshair(gevt.x, gevt.y, LCD_RGBPACK(0, 0xc0, 0));
                draw_crosshair(gevt.ox, gevt.oy, LCD_RGBPACK(0xc0, 0, 0));
            }

            rb->lcd_update();
        }
        else if (action == ACTION_STD_OK)
        {
            rb->splashf(HZ, "Gesture reset");
            rb->action_gesture_reset();
            rb->lcd_clear_display();
            rb->lcd_update();
        }
        else if (action == ACTION_STD_CANCEL)
        {
            break;
        }
        else if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
        {
            usb_exit = true;
            break;
        }
    }
}

static void tstest_velocity(void)
{
    struct touchevent tevent;
    struct gesture_vel gv;

    rb->splashf(HZ, "Velocity test");
    rb->touchscreen_set_mode(TOUCHSCREEN_POINT);

    rb->lcd_clear_display();
    rb->lcd_update();

    rb->gesture_vel_reset(&gv);

    while (true)
    {
        int action = rb->get_custom_action(CONTEXT_PLUGIN, TIMEOUT_BLOCK,
                                           get_context_map);
        if (action == ACTION_TOUCHSCREEN)
        {
            rb->lcd_clear_display();
            rb->action_get_touch_event(&tevent);
            rb->gesture_vel_process(&gv, &tevent);

            int xvel, yvel;
            rb->gesture_vel_get(&gv, &xvel, &yvel);

            rb->lcd_set_foreground(LCD_RGBPACK(0xff, 0xff, 0xff));
            rb->lcd_putsf(0, 0, "xvel=%4d yvel=%4d", xvel, yvel);

            draw_crosshair(tevent.x, tevent.y, LCD_RGBPACK(0, 0xc0, 0));
            rb->lcd_update();

            if (tevent.type == TOUCHEVENT_RELEASE)
                rb->gesture_vel_reset(&gv);
        }
        else if (action == ACTION_STD_OK)
        {
            rb->splashf(HZ, "Velocity reset");
            rb->gesture_vel_reset(&gv);
            rb->lcd_clear_display();
            rb->lcd_update();
        }
        else if (action == ACTION_STD_CANCEL)
        {
            break;
        }
        else if (rb->default_event_handler(action) == SYS_USB_CONNECTED)
        {
            usb_exit = true;
            break;
        }
    }
}

/* plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    int sel;
    MENUITEM_STRINGLIST(menu, "Touchscreen test",
                        NULL,
                        "Button mode test",
                        "Pointing mode test",
                        "Gesture test",
                        "Velocity test",
                        "Quit");

    while (true)
    {
        rb->touchscreen_set_mode(rb->global_settings->touch_mode);
        switch (rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            tstest_button_mode();
            break;

        case 1:
            tstest_pointing_mode();
            break;

        case 2:
            tstest_gesture();
            break;

        case 3:
            tstest_velocity();
            break;

        default:
            return PLUGIN_OK;
        }

        if (usb_exit)
            return PLUGIN_USB_CONNECTED;
    }
}
