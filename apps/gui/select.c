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

#include "select.h"

#include "lang.h"
#include "textarea.h"
#include "screen_access.h"
#include "kernel.h"
#include "action.h"


void gui_select_init_numeric(struct gui_select * select,
        const char * title,
        int init_value,
        int min_value,
        int max_value,
        int step,
        const char * unit,
        option_formatter *formatter)
{
    select->canceled=false;
    select->validated=false;
    option_select_init_numeric(&select->options, title, init_value,
                               min_value, max_value, step, unit, formatter);
}

void gui_select_init_items(struct gui_select * select,
        const char * title,
        int selected,
        const struct opt_items * items,
        int nb_items)
{
    select->canceled=false;
    select->validated=false;
    option_select_init_items(&select->options, title, selected, items, nb_items);
}

void gui_select_draw(struct gui_select * select, struct screen * display)
{
    char buffer[30];
    const char * selected=option_select_get_text(&(select->options), buffer,
                                                 sizeof buffer);
#ifdef HAVE_LCD_BITMAP
    screen_set_xmargin(display, 0);
#endif
    gui_textarea_clear(display);
    display->puts_scroll(0, 0, (unsigned char *)select->options.title);

    if(select->canceled)
        display->puts_scroll(0, 0, str(LANG_MENU_SETTING_CANCEL));
    display->puts_scroll(0, 1, (unsigned char *)selected);
    gui_textarea_update(display);
}

void gui_syncselect_draw(struct gui_select * select)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_select_draw(select, &(screens[i]));
}

bool gui_syncselect_do_button(struct gui_select * select, int button)
{
    switch(button)
    {
        case ACTION_SETTINGS_INCREPEAT:
            select->options.limit_loop = true;
        case ACTION_SETTINGS_INC:
            option_select_next(&select->options);
            return(true);

        case ACTION_SETTINGS_DECREPEAT:
            select->options.limit_loop = true;
        case ACTION_SETTINGS_DEC:
            option_select_prev(&select->options);
            return(true);

        case ACTION_STD_OK:
        case ACTION_STD_PREV: /*NOTE: this is in CONTEXT_SETTINGS ! */
            select->validated=true;
            return(false);

        case ACTION_STD_CANCEL:
            select->canceled = true;
            gui_syncselect_draw(select);
            sleep(HZ/2);
            action_signalscreenchange();
            return(false);
    }
    return(false);
}

