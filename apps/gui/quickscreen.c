/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "quickscreen.h"

#ifdef HAVE_QUICKSCREEN

#include <stdio.h>
#include "system.h"
#include "icons.h"
#include "textarea.h"
#include "font.h"
#include "kernel.h"
#include "misc.h"
#include "statusbar.h"

void gui_quickscreen_init(struct gui_quickscreen * qs,
                          struct option_select *left_option,
                          struct option_select *bottom_option,
                          struct option_select *right_option,
                          quickscreen_callback callback)
{
    qs->left_option=left_option;
    qs->bottom_option=bottom_option;
    qs->right_option=right_option;
    qs->callback=callback;
}

void gui_quickscreen_draw(struct gui_quickscreen * qs, struct screen * display)
{
    #define PUTS_CENTER (display->height/2/font_h)
    #define PUTS_BOTTOM (display->height/font_h)
    #define PUTSXY_CENTER (display->height/2)
    #define PUTSXY_BOTTOM (display->height)

    char buffer[30];
    const unsigned char *option;
    const unsigned char *title;
    int w, font_h;
    bool statusbar = global_settings.statusbar;
#ifdef HAS_BUTTONBAR
    display->has_buttonbar=false;
#endif
    gui_textarea_clear(display);
    display->setfont(FONT_SYSFIXED);
    display->getstringsize("A", NULL, &font_h);

    /* Displays the first line of text */
    option=(unsigned char *)option_select_get_text(qs->left_option, buffer,
                                                   sizeof buffer);
    title=(unsigned char *)qs->left_option->title;
    display->puts_scroll(2, PUTS_CENTER-4+!statusbar, title);
    display->puts_scroll(2, PUTS_CENTER-3+!statusbar, option);
    display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 1,
                         PUTSXY_CENTER-(font_h*3), 7, 8);

    /* Displays the second line of text */
    option=(unsigned char *)option_select_get_text(qs->right_option, buffer,
                                                   sizeof buffer);
    title=(unsigned char *)qs->right_option->title;
    display->getstringsize(title, &w, NULL);
    if(w > display->width - 8)
    {
        display->puts_scroll(2, PUTS_CENTER-2+!statusbar, title);
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward], 1,
                             PUTSXY_CENTER-font_h, 7, 8);
    }
    else
    {
        display->putsxy(display->width - w - 12, PUTSXY_CENTER-font_h, title);
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                        display->width - 8, PUTSXY_CENTER-font_h, 7, 8);
    }
    display->getstringsize(option, &w, NULL);
    if(w > display->width)
        display->puts_scroll(0, PUTS_CENTER-1+!statusbar, option);
    else
        display->putsxy(display->width -w-12, PUTSXY_CENTER, option);

    /* Displays the third line of text */
    option=(unsigned char *)option_select_get_text(qs->bottom_option, buffer,
                                                   sizeof buffer);
    title=(unsigned char *)qs->bottom_option->title;

    display->getstringsize(title, &w, NULL);
    if(w > display->width)
        display->puts_scroll(0, PUTS_BOTTOM-4+!statusbar, title);
    else
        display->putsxy(display->width/2-w/2, PUTSXY_BOTTOM-(font_h*3), title);

    display->getstringsize(option, &w, NULL);
    if(w > display->width)
        display->puts_scroll(0, PUTS_BOTTOM-3+!statusbar, option);
    else
        display->putsxy(display->width/2-w/2, PUTSXY_BOTTOM-(font_h*2), option);
    display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow], display->width/2-4,
                         PUTSXY_BOTTOM-font_h, 7, 8);

    gui_textarea_update(display);
    display->setfont(FONT_UI);
}

void gui_syncquickscreen_draw(struct gui_quickscreen * qs)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_quickscreen_draw(qs, &screens[i]);
}

bool gui_quickscreen_do_button(struct gui_quickscreen * qs, int button)
{

    switch(button)
    {
        case QUICKSCREEN_LEFT :
        case QUICKSCREEN_LEFT | BUTTON_REPEAT :
#ifdef QUICKSCREEN_RC_LEFT
        case QUICKSCREEN_RC_LEFT :
        case QUICKSCREEN_RC_LEFT | BUTTON_REPEAT :
#endif
            option_select_next(qs->left_option);
            return(true);

        case QUICKSCREEN_BOTTOM :
        case QUICKSCREEN_BOTTOM | BUTTON_REPEAT :
#ifdef QUICKSCREEN_RC_BOTTOM
        case QUICKSCREEN_RC_BOTTOM :
        case QUICKSCREEN_RC_BOTTOM | BUTTON_REPEAT :
#endif
            option_select_next(qs->bottom_option);
            return(true);

        case QUICKSCREEN_RIGHT :
        case QUICKSCREEN_RIGHT | BUTTON_REPEAT :
#ifdef QUICKSCREEN_RC_RIGHT
        case QUICKSCREEN_RC_RIGHT :
        case QUICKSCREEN_RC_RIGHT | BUTTON_REPEAT :
#endif
            option_select_next(qs->right_option);
            return(true);

#ifdef QUICKSCREEN_BOTTOM_INV
        case QUICKSCREEN_BOTTOM_INV :
        case QUICKSCREEN_BOTTOM_INV | BUTTON_REPEAT :
#endif
#ifdef QUICKSCREEN_RC_BOTTOM_INV
        case QUICKSCREEN_RC_BOTTOM_INV :
        case QUICKSCREEN_RC_BOTTOM_INV | BUTTON_REPEAT :
#endif
#if defined(QUICKSCREEN_RC_BOTTOM_INV) || defined(QUICKSCREEN_BOTTOM_INV)
            option_select_prev(qs->bottom_option);
            return(true);
#endif
    }
    return(false);
}
#ifdef BUTTON_REMOTE
#define uncombine_button(key_read, combined_button) \
    key_read & ~(combined_button & ~BUTTON_REMOTE)
#else
#define uncombine_button(key_read, combined_button) \
    key_read & ~combined_button
#endif

bool gui_syncquickscreen_run(struct gui_quickscreen * qs, int button_enter)
{
    int raw_key, button;
    /* To quit we need either :
     *  - a second press on the button that made us enter
     *  - an action taken while pressing the enter button,
     *    then release the enter button*/
    bool can_quit=false;
    gui_syncquickscreen_draw(qs);
    gui_syncstatusbar_draw(&statusbars, true);
    while (true) {
        raw_key = button_get(true);
        button=uncombine_button(raw_key, button_enter);
        if(default_event_handler(button) == SYS_USB_CONNECTED)
            return(true);
        if(gui_quickscreen_do_button(qs, button))
        {
            can_quit=true;
            if(qs->callback)
                qs->callback(qs);
            gui_syncquickscreen_draw(qs);
        }
        else if(raw_key==button_enter)
            can_quit=true;
        if(raw_key==(button_enter | BUTTON_REL) && can_quit)
            return(false);
#ifdef QUICKSCREEN_QUIT
        if(raw_key==QUICKSCREEN_QUIT
#ifdef QUICKSCREEN_QUIT2
           || raw_key==QUICKSCREEN_QUIT2
#endif
#if QUICKSCREEN_RC_QUIT
           || raw_key==QUICKSCREEN_RC_QUIT
#endif
        )
            return(false);
#endif /* QUICKSCREEN_QUIT */
        gui_syncstatusbar_draw(&statusbars, false);
    }
}

#endif /* HAVE_QUICKSCREEN */

