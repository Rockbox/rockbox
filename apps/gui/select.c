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
#include "sprintf.h"
#include "kernel.h"
#include "screen_access.h"

void gui_select_init_numeric(struct gui_select * select,
        const char * title,
        int init_value,
        int min_value,
        int max_value,
        int step,
        const char * unit,
        void (*formatter)(char* dest,
                          int dest_length,
                          int variable,
                          const char* unit)
                          )
{
    select->canceled=false;
    select->validated=false;
    select->title=title;
    select->min_value=min_value;
    select->max_value=max_value;
    select->option=init_value;
    select->step=step;
    select->extra_string=unit;
    select->formatter=formatter;
    select->items=NULL;
}

void gui_select_init_items(struct gui_select * select,
        const char * title,
        int selected,
        const struct opt_items * items,
        int nb_items)
{
    select->canceled=false;
    select->validated=false;
    select->title=title;
    select->min_value=0;
    select->max_value=nb_items;
    select->option=selected;
    select->step=1;
    select->formatter=NULL;
    select->items=items;
}
//FIXME: si step>1, mettre d'abord au plus grand, ensuite passerau plus petit
void gui_select_next(struct gui_select * select)
{
    select->option+=select->step;
    if(select->option >= select->max_value)
        select->option=select->min_value;
}

void gui_select_prev(struct gui_select * select)
{
    select->option-=select->step;
    if(select->option < select->min_value)
        select->option=select->max_value-1;
}

void gui_select_draw(struct gui_select * select, struct screen * display)
{
#ifdef HAVE_LCD_BITMAP
    screen_set_xmargin(display, 0);
#endif
    gui_textarea_clear(display);
    display->puts_scroll(0, 0, select->title);

    if(gui_select_is_canceled(select))
        display->puts_scroll(0, 0, str(LANG_MENU_SETTING_CANCEL));
    if(select->items)
        display->puts_scroll(0, 1, P2STR(select->items[select->option].string));
    else
    {
        char buffer[30];
        if(!select->formatter)
            snprintf(buffer, sizeof buffer,"%d %s", select->option, select->extra_string);
        else
            select->formatter(buffer, sizeof buffer, select->option, select->extra_string);
        display->puts_scroll(0, 1, buffer);
    }
    gui_textarea_update(display);
}

void gui_syncselect_draw(struct gui_select * select)
{
    int i;
    for(i=0;i<NB_SCREENS;i++)
        gui_select_draw(select, &(screens[i]));
}

bool gui_syncselect_do_button(struct gui_select * select, int button)
{
    bool moved=false;
    switch(button)
    {
        case SELECT_INC :
        case SELECT_INC | BUTTON_REPEAT :
            gui_select_next(select);
            moved=true;
            break;
        case SELECT_DEC :
        case SELECT_DEC | BUTTON_REPEAT :
            gui_select_prev(select);
            moved=true;
            break;
        case SELECT_OK :
#ifdef SELECT_OK2
        case SELECT_OK2 :
#endif
            gui_select_validate(select);
            break;
        case SELECT_CANCEL :
        case SELECT_CANCEL2 :
            gui_select_cancel(select);
            gui_syncselect_draw(select);
            sleep(HZ/2);
            break;
    }
    return(moved);
}

