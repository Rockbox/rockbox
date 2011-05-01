/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "lib/pluginlib_exit.h"
#include "tv_action.h"
#include "tv_button.h"
#include "tv_preferences.h"



enum plugin_status plugin_start(const void* file)
{
    int button;
#if defined(TV_AUTOSCROLL_PRE) 
    int lastbutton = BUTTON_NONE;
#endif
    bool autoscroll = false;
    long old_tick;
    bool done = false;
    bool display_update = true;
    size_t size;
    unsigned char *plugin_buf;

    old_tick = *rb->current_tick;

    if (!file)
        return PLUGIN_ERROR;

    /* get the plugin buffer */
    plugin_buf = rb->plugin_get_buffer(&size);

    if (!tv_init_action(&plugin_buf, &size)) {
        rb->splash(HZ, "Error initialize");
        return PLUGIN_ERROR;
    }

    if (!tv_load_file(file)) {
        rb->splash(HZ, "Error opening file");
        return PLUGIN_ERROR;
    }

    atexit(tv_exit);
    while (!done) {
#ifdef HAVE_LCD_BITMAP
        if (preferences->statusbar)
            rb->send_event(GUI_EVENT_ACTIONUPDATE, NULL);
#endif

        if (display_update)
            tv_draw();

        display_update = true;

        button = rb->button_get_w_tmo(HZ/10);

        switch (button) {
            case TV_MENU:
#ifdef TV_MENU2
            case TV_MENU2:
#endif
                {
                    unsigned res = tv_menu();

                    if (res != TV_MENU_RESULT_EXIT_MENU)
                    {
                        if (res == TV_MENU_RESULT_ATTACHED_USB)
                            return PLUGIN_USB_CONNECTED;
                        else if (res == TV_MENU_RESULT_ERROR)
                            return PLUGIN_ERROR;
                        else
                            done = true;
                    }
                }
                break;

            case TV_AUTOSCROLL:
#ifdef TV_AUTOSCROLL_PRE
                if (lastbutton != TV_AUTOSCROLL_PRE)
                    break;
#endif
                autoscroll = !autoscroll;
                break;

            case TV_SCROLL_UP:
            case TV_SCROLL_UP | BUTTON_REPEAT:
#ifdef TV_SCROLL_UP2
            case TV_SCROLL_UP2:
            case TV_SCROLL_UP2 | BUTTON_REPEAT:
#endif
                tv_scroll_up(TV_VERTICAL_SCROLL_PREFS);
                old_tick = *rb->current_tick;
                break;

            case TV_SCROLL_DOWN:
            case TV_SCROLL_DOWN | BUTTON_REPEAT:
#ifdef TV_PAGE_DOWN2
            case TV_SCROLL_DOWN2:
            case TV_SCROLL_DOWN2 | BUTTON_REPEAT:
#endif
                tv_scroll_down(TV_VERTICAL_SCROLL_PREFS);
                old_tick = *rb->current_tick;
                break;

            case TV_SCREEN_LEFT:
            case TV_SCREEN_LEFT | BUTTON_REPEAT:
                if (preferences->windows > 1)
                {
                    /* Screen left */
                    tv_scroll_left(TV_HORIZONTAL_SCROLL_PREFS);
                }
                else {   /* prefs->windows == 1 */
                    if (preferences->narrow_mode == NM_PAGE)
                    {
                        /* scroll to previous page */
                        tv_scroll_up(TV_VERTICAL_SCROLL_PAGE);
                    }
                    else
                    {
                        /* Top of file */
                        tv_top();
                    }
                }
                break;

            case TV_SCREEN_RIGHT:
            case TV_SCREEN_RIGHT | BUTTON_REPEAT:
                if (preferences->windows > 1)
                {
                    /* Screen right */
                    tv_scroll_right(TV_HORIZONTAL_SCROLL_PREFS);
                }
                else {   /* prefs->windows == 1 */
                    if (preferences->narrow_mode == NM_PAGE)
                    {
                        /* scroll to next page */
                        tv_scroll_down(TV_VERTICAL_SCROLL_PAGE);
                    }
                    else
                    {
                        /* Bottom of file */
                        tv_bottom();
                    }
                }
                break;

#ifdef TV_LINE_UP
            case TV_LINE_UP:
            case TV_LINE_UP | BUTTON_REPEAT:
                /* Scroll up one line */
                tv_scroll_up(TV_VERTICAL_SCROLL_LINE);
                old_tick = *rb->current_tick;
                break;

            case TV_LINE_DOWN:
            case TV_LINE_DOWN | BUTTON_REPEAT:
                /* Scroll down one line */
                tv_scroll_down(TV_VERTICAL_SCROLL_LINE);
                old_tick = *rb->current_tick;
                break;
#endif
#ifdef TV_COLUMN_LEFT
            case TV_COLUMN_LEFT:
            case TV_COLUMN_LEFT | BUTTON_REPEAT:
                /* Scroll left one column */
                tv_scroll_left(TV_HORIZONTAL_SCROLL_COLUMN);
                break;

            case TV_COLUMN_RIGHT:
            case TV_COLUMN_RIGHT | BUTTON_REPEAT:
                /* Scroll right one column */
                tv_scroll_right(TV_HORIZONTAL_SCROLL_COLUMN);
                break;
#endif

#ifdef TV_RC_QUIT
            case TV_RC_QUIT:
#endif
            case TV_QUIT:
#ifdef TV_QUIT2
            case TV_QUIT2:
#endif
                done = true;
                break;

            case TV_BOOKMARK:
                tv_add_or_remove_bookmark();
                break;

            default:
                exit_on_usb(button);
                display_update = false;
                break;
        }
        if (button != BUTTON_NONE)
        {
#if defined(TV_AUTOSCROLL_PRE) 
            lastbutton = button;
#endif
            rb->yield();
        }
        if (autoscroll)
        {
            if(old_tick <= *rb->current_tick - (110 - preferences->autoscroll_speed * 10))
            {
                tv_scroll_down(TV_VERTICAL_SCROLL_PREFS);
                old_tick = *rb->current_tick;
                display_update = true;
            }
        }
    }
    return PLUGIN_OK;
}
