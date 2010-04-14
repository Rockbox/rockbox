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
#include "textviewer.h"
#include "tv_bookmark.h"
#include "tv_button.h"
#include "tv_menu.h"
#include "tv_readtext.h"
#include "tv_screen.h"
#include "tv_settings.h"

PLUGIN_HEADER

static bool viewer_init(const unsigned char *file)
{
    viewer_init_screen();
    viewer_init_buffer();

    if (!viewer_open(file))
        return false;

    /* Init mac_text value used in processing buffer */
    mac_text = false;

    return true;
}

static void viewer_exit(void *parameter)
{
    (void)parameter;

    /* save preference and bookmarks */
    if (!viewer_save_settings())
        rb->splash(HZ, "Can't save preference and bookmarks.");

    viewer_close();
    viewer_finalize_screen();
}

enum plugin_status plugin_start(const void* file)
{
    int button, i, ok;
    int lastbutton = BUTTON_NONE;
    bool autoscroll = false;
    long old_tick;

    old_tick = *rb->current_tick;

    if (!file)
        return PLUGIN_ERROR;

    ok = viewer_init(file);
    if (!ok) {
        rb->splash(HZ, "Error opening file.");
        return PLUGIN_ERROR;
    }

    viewer_load_settings(); /* load the preferences and bookmark */

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    viewer_draw();

    while (!done) {

        if (autoscroll)
        {
            if(old_tick <= *rb->current_tick - (110-prefs.autoscroll_speed*10))
            {
                viewer_scroll_down(VIEWER_SCROLL_LINE);
                viewer_draw();
                old_tick = *rb->current_tick;
            }
        }

        button = rb->button_get_w_tmo(HZ/10);

        switch (button) {
            case VIEWER_MENU:
#ifdef VIEWER_MENU2
            case VIEWER_MENU2:
#endif
                viewer_menu();
                break;

            case VIEWER_AUTOSCROLL:
#ifdef VIEWER_AUTOSCROLL_PRE
                if (lastbutton != VIEWER_AUTOSCROLL_PRE)
                    break;
#endif
                autoscroll = !autoscroll;
                break;

            case VIEWER_SCROLL_UP:
            case VIEWER_SCROLL_UP | BUTTON_REPEAT:
#ifdef VIEWER_SCROLL_UP2
            case VIEWER_SCROLL_UP2:
            case VIEWER_SCROLL_UP2 | BUTTON_REPEAT:
#endif
                viewer_scroll_up(VIEWER_SCROLL_PREFS);
                viewer_draw();
                old_tick = *rb->current_tick;
                break;

            case VIEWER_SCROLL_DOWN:
            case VIEWER_SCROLL_DOWN | BUTTON_REPEAT:
#ifdef VIEWER_PAGE_DOWN2
            case VIEWER_SCROLL_DOWN2:
            case VIEWER_SCROLL_DOWN2 | BUTTON_REPEAT:
#endif
                viewer_scroll_down(VIEWER_SCROLL_PREFS);
                viewer_draw();
                old_tick = *rb->current_tick;
                break;

            case VIEWER_SCREEN_LEFT:
            case VIEWER_SCREEN_LEFT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE)
                {
                    /* Screen left */
                    viewer_scroll_left(VIEWER_SCROLL_SCREEN);
                }
                else {   /* prefs.view_mode == NARROW */
                    /* Top of file */
                    viewer_top();
                }
                viewer_draw();
                break;

            case VIEWER_SCREEN_RIGHT:
            case VIEWER_SCREEN_RIGHT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE)
                {
                    /* Screen right */
                    viewer_scrol_right(VIEWER_SCROLL_SCREEN);
                }
                else {   /* prefs.view_mode == NARROW */
                    /* Bottom of file */
                    viewer_bottom();
                }
                viewer_draw();
                break;

#ifdef VIEWER_LINE_UP
            case VIEWER_LINE_UP:
            case VIEWER_LINE_UP | BUTTON_REPEAT:
                /* Scroll up one line */
                viewer_scroll_up(VIEWER_SCROLL_LINE);
                viewer_draw();
                old_tick = *rb->current_tick;
                break;

            case VIEWER_LINE_DOWN:
            case VIEWER_LINE_DOWN | BUTTON_REPEAT:
                /* Scroll down one line */
                viewer_scroll_down(VIEWER_SCROLL_LINE);
                viewer_draw();
                old_tick = *rb->current_tick;
                break;
#endif
#ifdef VIEWER_COLUMN_LEFT
            case VIEWER_COLUMN_LEFT:
            case VIEWER_COLUMN_LEFT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE)
                {
                    viewer_scroll_left(VIEWER_SCROLL_COLUMN);
                    viewer_draw();
                }
                break;

            case VIEWER_COLUMN_RIGHT:
            case VIEWER_COLUMN_RIGHT | BUTTON_REPEAT:
                if (prefs.view_mode == WIDE)
                {
                    viewer_scroll_right(VIEWER_SCROLL_COLUMN);
                    viewer_draw();
                }
                break;
#endif

#ifdef VIEWER_RC_QUIT
            case VIEWER_RC_QUIT:
#endif
            case VIEWER_QUIT:
#ifdef VIEWER_QUIT2
            case VIEWER_QUIT2:
#endif
                viewer_exit(NULL);
                done = true;
                break;

            case VIEWER_BOOKMARK:
                viewer_add_remove_bookmark(cpage, cline);
                viewer_draw();
                break;

            default:
                if (rb->default_event_handler_ex(button, viewer_exit, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
        {
            lastbutton = button;
            rb->yield();
        }
    }
    return PLUGIN_OK;
}
