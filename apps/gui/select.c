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
    const char * selected=option_select_get_text(&(select->options), buffer);
#ifdef HAVE_LCD_BITMAP
    screen_set_xmargin(display, 0);
#endif
    gui_textarea_clear(display);
    display->puts_scroll(0, 0, option_select_get_title(&(select->options)));

    if(gui_select_is_canceled(select))
        display->puts_scroll(0, 0, str(LANG_MENU_SETTING_CANCEL));
    display->puts_scroll(0, 1, selected);
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
    gui_select_limit_loop(select, false);
    switch(button)
    {
        case SELECT_INC | BUTTON_REPEAT :
#ifdef SELECT_RC_INC
        case SELECT_RC_INC | BUTTON_REPEAT :
#endif
            gui_select_limit_loop(select, true);
        case SELECT_INC :
#ifdef SELECT_RC_INC
        case SELECT_RC_INC :
#endif
            gui_select_next(select);
            return(true);

        case SELECT_DEC | BUTTON_REPEAT :
#ifdef SELECT_RC_DEC
        case SELECT_RC_DEC | BUTTON_REPEAT :
#endif
            gui_select_limit_loop(select, true);
        case SELECT_DEC :
#ifdef SELECT_RC_DEC
        case SELECT_RC_DEC :
#endif
            gui_select_prev(select);
            return(true);

        case SELECT_OK :
#ifdef SELECT_RC_OK
        case SELECT_RC_OK :
#endif
#ifdef SELECT_RC_OK2
        case SELECT_RC_OK2 :
#endif
#ifdef SELECT_OK2
        case SELECT_OK2 :
#endif
            gui_select_validate(select);
            return(false);

        case SELECT_CANCEL :
#ifdef SELECT_CANCEL2
        case SELECT_CANCEL2 :
#endif 
#ifdef SELECT_RC_CANCEL
        case SELECT_RC_CANCEL :
#endif
#ifdef SELECT_RC_CANCEL2
        case SELECT_RC_CANCEL2 :
#endif
            gui_select_cancel(select);
            gui_syncselect_draw(select);
            sleep(HZ/2);
            return(false);
    }
    return(false);
}

