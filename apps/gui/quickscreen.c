/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jonathan Gordon
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
#include "settings_list.h"
#include "lang.h"
#include "option_select.h"

static struct viewport vps[NB_SCREENS][QUICKSCREEN_ITEM_COUNT];

static void quickscreen_fix_viewports(struct gui_quickscreen *qs,
                                      struct screen *display,
                                      struct viewport *parent)
{
    int height, i, count=0, top;
    int nb_lines = parent->height/display->char_height;
    bool single_line_bottom = false;
    
    for(i=0; i<QUICKSCREEN_ITEM_COUNT; i++)
    {
        if (qs->items[i])
            count++;
        vps[display->screen_type][i] = *parent;
    }
    
    /* special handling when there is only enough room for 2 items.
       discard the top and bottom items, so only show the 2 side ones */
    if (nb_lines < 4)
    {
        qs->items[QUICKSCREEN_TOP] = NULL;
        qs->items[QUICKSCREEN_BOTTOM] = NULL;
        vps[display->screen_type][QUICKSCREEN_RIGHT].y = parent->y;
        vps[display->screen_type][QUICKSCREEN_LEFT].height = parent->height/2;
        vps[display->screen_type][QUICKSCREEN_RIGHT].y = parent->y+parent->height/2;
        vps[display->screen_type][QUICKSCREEN_RIGHT].height = parent->height/2;
        return;
    }
    else if (nb_lines < 5 && count == 4) /* drop the top item */
    {
        qs->items[QUICKSCREEN_TOP] = NULL;
        single_line_bottom = true;
    }
    
    height = display->char_height*2;
    if (nb_lines > 8 ||
        (nb_lines > 5 && qs->items[QUICKSCREEN_TOP] == NULL))
        height += display->char_height;
    /* Top item */
    if (qs->items[QUICKSCREEN_TOP])
    {
        vps[display->screen_type][QUICKSCREEN_TOP].y = parent->y;
        vps[display->screen_type][QUICKSCREEN_TOP].height = height;
    }
    else
    {
        vps[display->screen_type][QUICKSCREEN_TOP].height = 0;
    }
    /* bottom item */
    if (qs->items[QUICKSCREEN_BOTTOM])
    {
        if (single_line_bottom)
            height = display->char_height;
        vps[display->screen_type][QUICKSCREEN_BOTTOM].y = parent->y+parent->height - height;
        vps[display->screen_type][QUICKSCREEN_BOTTOM].height = height;
    }
    else
    {
        vps[display->screen_type][QUICKSCREEN_BOTTOM].height = 0;
    }
    
    /* side items */
    height = parent->height -
             vps[display->screen_type][QUICKSCREEN_BOTTOM].height -
             vps[display->screen_type][QUICKSCREEN_TOP].height ;
    top = parent->y+vps[display->screen_type][QUICKSCREEN_TOP].height;
    vps[display->screen_type][QUICKSCREEN_LEFT].y = top;
    vps[display->screen_type][QUICKSCREEN_RIGHT].y = top;
    vps[display->screen_type][QUICKSCREEN_LEFT].height = height;
    vps[display->screen_type][QUICKSCREEN_RIGHT].height = height;
            
    vps[display->screen_type][QUICKSCREEN_RIGHT].x = parent->x+parent->width/2;
    
    vps[display->screen_type][QUICKSCREEN_LEFT].width = parent->width/2;
    vps[display->screen_type][QUICKSCREEN_RIGHT].width = parent->width/2;
}

static void quickscreen_draw_text(char *s, int item, bool title,
                                  struct screen *display, struct viewport *vp)
{
    int nb_lines = vp->height/display->char_height;
    int w, line = 0, x=0;
    display->getstringsize(s, &w, NULL);
    switch (item)
    {
        case QUICKSCREEN_TOP:
            if (nb_lines > 2)
            {
                if (title)
                {
                    display->mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                                     (vp->width/2)-4, 0, 7, 8);
                    line = 1;
                }
                else
                    line = 2;
            }
            else
                line = title?0:1;
            x = (vp->width - w)/2;
            break;
        case QUICKSCREEN_BOTTOM:
            if (title && nb_lines > 2 && item == QUICKSCREEN_BOTTOM)
            {
                display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                                     (vp->width/2)-4, vp->height-8, 7, 8);
            }
            line = title?0:1;
            x = (vp->width - w)/2;
            break;
        case QUICKSCREEN_LEFT:
            if (nb_lines > 1)
            {
                line = nb_lines/2;
                if (title)
                    line--;
            }
            else
                line = 0;
            if (title)
                display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward], 1,
                                     line*display->char_height+display->char_height/2, 7, 8);
            x = 12;
            break;
        case QUICKSCREEN_RIGHT:
            line = nb_lines/2;
            if (title == false)
                line++;
            if (title)
                display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                                     vp->width - 8,
                                     line*display->char_height+display->char_height/2, 7, 8);
            x = vp->width - w - 12;
            break;
    }
    if (w>vp->width)
        display->puts_scroll(0,line,s);
    else
        display->putsxy(x, line*display->char_height, s);
}

static void gui_quickscreen_draw(struct gui_quickscreen *qs,
                                 struct screen *display,
                                 struct viewport *parent)
{
    int i;
    char buf[MAX_PATH];
    unsigned char *title, *value;
    void *setting;
    int temp;
    display->set_viewport(parent);
    display->clear_viewport();
    for (i=0; i<QUICKSCREEN_ITEM_COUNT; i++)
    {
        
        if (!qs->items[i])
            continue;
        display->set_viewport(&vps[display->screen_type][i]);
        display->scroll_stop(&vps[display->screen_type][i]);
        
        title = P2STR(ID2P(qs->items[i]->lang_id));
        setting = qs->items[i]->setting;
        if (qs->items[i]->flags&F_T_BOOL)
            temp = *(bool*)setting?1:0;
        else
            temp = *(int*)setting;
        value = option_get_valuestring((struct settings_list*)qs->items[i], buf, MAX_PATH, temp);
        
        if (vps[display->screen_type][i].height < display->char_height*2)
        {
            char text[MAX_PATH];
            snprintf(text, MAX_PATH, "%s: %s", title, value);
            quickscreen_draw_text(text, i, true, display, &vps[display->screen_type][i]);
        }
        else
        {
            quickscreen_draw_text(title, i, true, display, &vps[display->screen_type][i]);
            quickscreen_draw_text(value, i, false, display, &vps[display->screen_type][i]);
        }
        display->update_viewport();
    }
    display->set_viewport(parent);
    display->update_viewport();
    display->set_viewport(NULL);
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
            if (qs->items[QUICKSCREEN_LEFT])
                option_select_next_val((struct settings_list *)qs->items[QUICKSCREEN_LEFT]);
            return(true);

        case ACTION_QS_DOWN:
            if (qs->items[QUICKSCREEN_BOTTOM])
                option_select_next_val((struct settings_list *)qs->items[QUICKSCREEN_BOTTOM]);
            return(true);

        case ACTION_QS_RIGHT:
            if (qs->items[QUICKSCREEN_RIGHT])
                option_select_next_val((struct settings_list *)qs->items[QUICKSCREEN_RIGHT]);
            return(true);

        case ACTION_QS_DOWNINV:
            if (qs->items[QUICKSCREEN_TOP])
                option_select_next_val((struct settings_list *)qs->items[QUICKSCREEN_TOP]);
            return(true);
    }
    return(false);
}

bool gui_syncquickscreen_run(struct gui_quickscreen * qs, int button_enter)
{
    int button, i;
    bool changed = false;
    struct viewport vp[NB_SCREENS];
    /* To quit we need either :
     *  - a second press on the button that made us enter
     *  - an action taken while pressing the enter button,
     *    then release the enter button*/
    bool can_quit=false;
    gui_syncstatusbar_draw(&statusbars, true);
    FOR_NB_SCREENS(i)
    {
        screens[i].set_viewport(NULL);
        screens[i].scroll_stop(NULL);
        vp[i].x = 0;
        vp[i].width = screens[i].width;
        vp[i].y = STATUSBAR_HEIGHT;
        vp[i].height = screens[i].height - STATUSBAR_HEIGHT;
#ifdef HAVE_LCD_COLOR
        if (screens[i].is_color)
        {
            vp[i].fg_pattern = global_settings.fg_color;
            vp[i].bg_pattern = global_settings.bg_color;
        }
#endif
        vp[i].xmargin = 0;
        vp[i].ymargin = 0;
        vp[i].font = FONT_UI;
        vp[i].drawmode = STYLE_DEFAULT;
        quickscreen_fix_viewports(qs, &screens[i], &vp[i]);
        gui_quickscreen_draw(qs, &screens[i], &vp[i]);
    }
    while (true) {
        button = get_action(CONTEXT_QUICKSCREEN,TIMEOUT_BLOCK);
        if(default_event_handler(button) == SYS_USB_CONNECTED)
            return(true);
        if(gui_quickscreen_do_button(qs, button))
        {
            changed = true;
            can_quit=true;
            if (button == ACTION_QS_DOWNINV && 
                !qs->items[QUICKSCREEN_TOP])
                break;
            FOR_NB_SCREENS(i)
                gui_quickscreen_draw(qs, &screens[i], &vp[i]);
        }
        else if(button==button_enter)
            can_quit=true;
            
        if((button == button_enter) && can_quit)
            break;
            
        if(button==ACTION_STD_CANCEL)
            break;
            
        gui_syncstatusbar_draw(&statusbars, false);
    }
    if (changed)
        settings_apply();
    return false;
}

bool quick_screen_quick(int button_enter)
{
    struct gui_quickscreen qs;
    qs.items[QUICKSCREEN_LEFT] = find_setting_from_string(global_settings.quickscreen_left, NULL);
    qs.items[QUICKSCREEN_RIGHT] = find_setting_from_string(global_settings.quickscreen_right,NULL);
    qs.items[QUICKSCREEN_BOTTOM] = find_setting_from_string(global_settings.quickscreen_bottom, NULL);
    qs.items[QUICKSCREEN_TOP] = find_setting_from_string(global_settings.quickscreen_top,NULL);
    qs.callback = NULL;
    gui_syncquickscreen_run(&qs, button_enter);
    return(0);
}

#ifdef BUTTON_F3
bool quick_screen_f3(int button_enter)
{
    struct gui_quickscreen qs;
    qs.items[QUICKSCREEN_LEFT] = find_setting(&global_settings.scrollbar, NULL);
    qs.items[QUICKSCREEN_RIGHT] = find_setting(&global_settings.statusbar, NULL);
    qs.items[QUICKSCREEN_BOTTOM] = find_setting(&global_settings.flip_display, NULL);
    qs.items[QUICKSCREEN_TOP] = NULL;
    qs.callback = NULL;
    gui_syncquickscreen_run(&qs, button_enter);
    return(0);
}
#endif /* BUTTON_F3 */

#endif /* HAVE_QUICKSCREEN */

