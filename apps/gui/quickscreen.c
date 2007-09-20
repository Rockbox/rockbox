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
#include "action.h"

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

/*
 * Draws the quickscreen on a given screen
 *  - qs : the quickscreen
 *  - display : the screen to draw on
 */
static void gui_quickscreen_draw(struct gui_quickscreen * qs, struct screen * display)
{
    const unsigned char *option;
    const unsigned char *title;
    int w, font_h;
    bool statusbar = global_settings.statusbar;
#ifdef HAS_BUTTONBAR
    display->has_buttonbar=false;
#endif
    gui_textarea_clear(display);
    if (display->height / display->char_height < 7) /* we need at leats 7 lines */
    {
        display->setfont(FONT_SYSFIXED);
    }
    display->getstringsize("A", NULL, &font_h);

    /* do these calculations once */
    const unsigned int puts_center = display->height/2/font_h;
    const unsigned int puts_bottom = display->height/font_h;
    const unsigned int putsxy_center = display->height/2;
    const unsigned int putsxy_bottom = display->height;

    /* Displays the first line of text */
    option=(unsigned char *)option_select_get_text(qs->left_option);
    title=(unsigned char *)qs->left_option->title;
    display->puts_scroll(2, puts_center-4+!statusbar, title);
    display->puts_scroll(2, puts_center-3+!statusbar, option);
    display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 1,
                         putsxy_center-(font_h*3), 7, 8);

    /* Displays the second line of text */
    option=(unsigned char *)option_select_get_text(qs->right_option);
    title=(unsigned char *)qs->right_option->title;
    display->getstringsize(title, &w, NULL);
    if(w > display->width - 8)
    {
        display->puts_scroll(2, puts_center-2+!statusbar, title);
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward], 1,
                             putsxy_center-font_h, 7, 8);
    }
    else
    {
        display->putsxy(display->width - w - 12, putsxy_center-font_h, title);
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                        display->width - 8, putsxy_center-font_h, 7, 8);
    }
    display->getstringsize(option, &w, NULL);
    if(w > display->width)
        display->puts_scroll(0, puts_center-1+!statusbar, option);
    else
        display->putsxy(display->width -w-12, putsxy_center, option);

    /* Displays the third line of text */
    option=(unsigned char *)option_select_get_text(qs->bottom_option);
    title=(unsigned char *)qs->bottom_option->title;

    display->getstringsize(title, &w, NULL);
    if(w > display->width)
        display->puts_scroll(0, puts_bottom-4+!statusbar, title);
    else
        display->putsxy(display->width/2-w/2, putsxy_bottom-(font_h*3), title);

    display->getstringsize(option, &w, NULL);
    if(w > display->width)
        display->puts_scroll(0, puts_bottom-3+!statusbar, option);
    else
        display->putsxy(display->width/2-w/2, putsxy_bottom-(font_h*2), option);
    display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow], display->width/2-4,
                         putsxy_bottom-font_h, 7, 8);

    gui_textarea_update(display);
    display->setfont(FONT_UI);
}

/*
 * Draws the quickscreen on all available screens
 *  - qs : the quickscreen
 */
static void gui_syncquickscreen_draw(struct gui_quickscreen * qs)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_quickscreen_draw(qs, &screens[i]);
}

/*
 * Does the actions associated to the given button if any
 *  - qs : the quickscreen
 *  - button : the key we are going to analyse
 * returns : true if the button corresponded to an action, false otherwise
 */
static bool gui_quickscreen_do_button(struct gui_quickscreen * qs, int button)
{

    switch(button)
    {
        case ACTION_QS_LEFT:
            option_select_next(qs->left_option);
            return(true);

        case ACTION_QS_DOWN:
            option_select_next(qs->bottom_option);
            return(true);

        case ACTION_QS_RIGHT:
            option_select_next(qs->right_option);
            return(true);

        case ACTION_QS_DOWNINV:
            option_select_prev(qs->bottom_option);
            return(true);
    }
    return(false);
}

bool gui_syncquickscreen_run(struct gui_quickscreen * qs, int button_enter)
{
    int button;
    /* To quit we need either :
     *  - a second press on the button that made us enter
     *  - an action taken while pressing the enter button,
     *    then release the enter button*/
    bool can_quit=false;
    gui_syncquickscreen_draw(qs);
    gui_syncstatusbar_draw(&statusbars, true);
    while (true) {
        button = get_action(CONTEXT_QUICKSCREEN,TIMEOUT_BLOCK);
        if(default_event_handler(button) == SYS_USB_CONNECTED)
            return(true);
        if(gui_quickscreen_do_button(qs, button))
        {
            can_quit=true;
            if(qs->callback)
                qs->callback(qs);
            gui_syncquickscreen_draw(qs);
        }
        else if(button==button_enter)
            can_quit=true;
            
        if((button == button_enter) && can_quit)
            break;
            
        if(button==ACTION_STD_CANCEL)
            break;
            
        gui_syncstatusbar_draw(&statusbars, false);
    }
    return false;
}

#endif /* HAVE_QUICKSCREEN */

