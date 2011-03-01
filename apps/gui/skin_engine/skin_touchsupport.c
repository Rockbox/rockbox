/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 - Jonathan Gordon
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
 
#include "config.h"
#include <stdio.h>
#include "action.h"
#include "skin_engine.h"
#include "wps_internals.h"
#include "misc.h"
#include "option_select.h"
#include "sound.h"
#include "settings_list.h"


/** Disarms all touchregions. */
void skin_disarm_touchregions(struct wps_data *data)
{
    struct skin_token_list *regions = data->touchregions;
    while (regions)
    {
        ((struct touchregion *)regions->token->value.data)->armed = false;
        regions = regions->next;
    }
}

/* Get the touched action.
 * egde_offset is a percentage value for the position of the touch
 * inside the bar for regions which arnt WPS_TOUCHREGION_ACTION type.
 */
int skin_get_touchaction(struct wps_data *data, int* edge_offset,
                         struct touchregion **retregion)
{
    int returncode = ACTION_NONE;
    short x,y;
    short vx, vy;
    int type = action_get_touchscreen_press(&x, &y);
    static int last_action = ACTION_NONE;
    struct touchregion *r, *temp = NULL;
    bool repeated = (type == BUTTON_REPEAT);
    bool released = (type == BUTTON_REL);
    bool pressed = (type == BUTTON_TOUCHSCREEN);
    struct skin_token_list *regions = data->touchregions;

    while (regions)
    {
        r = (struct touchregion *)regions->token->value.data;
        /* make sure this region's viewport is visible */
        if (r->wvp->hidden_flags&VP_DRAW_HIDDEN)
        {
            regions = regions->next;
            continue;
        }
        /* check if it's inside this viewport */
        if (viewport_point_within_vp(&(r->wvp->vp), x, y))
        {   /* reposition the touch inside the viewport since touchregions
             * are relative to a preceding viewport */
            vx = x - r->wvp->vp.x;
            vy = y - r->wvp->vp.y;
            /* now see if the point is inside this region */
            if (vx >= r->x && vx < r->x+r->width &&
                vy >= r->y && vy < r->y+r->height)
            {
                /* reposition the touch within the area */
                vx -= r->x;
                vy -= r->y;
                

                switch(r->type)
                {
                    case WPS_TOUCHREGION_ACTION:
                        if (r->armed && ((repeated && r->repeat) || (released && !r->repeat)))
                        {
                            last_action = r->action;
                            returncode = r->action;
                            temp = r;
                        }
                        if (pressed)
                        {
                            r->armed = true;
                            r->last_press = current_tick;
                        }
                        break;
                    default:
                        if (edge_offset)
                        {
                            if(r->width > r->height)
                                *edge_offset = vx*100/r->width;
                            else
                                *edge_offset = vy*100/r->height;
                            if (r->reverse_bar)
                                *edge_offset = 100 - *edge_offset;
                        }
                        returncode = r->type;
                        temp = r;
                        break;
                }
            }
        }
        regions = regions->next;
    }

    /* On release, all regions are disarmed. */
    if (released)
        skin_disarm_touchregions(data);
    if (retregion && temp)
        *retregion = temp;
    
    if (returncode != ACTION_NONE)
    {
        switch (returncode)
        {
            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_DEC:
            {
                const struct settings_list *setting = 
                                            temp->setting_data.setting;
                option_select_next_val(setting, 
                                       returncode == ACTION_SETTINGS_DEC,
                                       true);
                returncode = ACTION_REDRAW;
            }
            break;
            case ACTION_SETTINGS_SET:
            {
                struct touchsetting *data = &temp->setting_data;
                const struct settings_list *s = data->setting;
                void (*f)(int) = NULL;
                switch (s->flags&F_T_MASK)
                {
                    case F_T_CUSTOM:
                        s->custom_setting
                            ->load_from_cfg(s->setting, data->value.text);
                        break;                          
                    case F_T_INT:
                    case F_T_UINT:
                        *(int*)s->setting = data->value.number;
                        if (s->flags&F_CHOICE_SETTING)
                            f = s->choice_setting->option_callback;
                        else if (s->flags&F_TABLE_SETTING)
                            f = s->table_setting->option_callback;
                        else
                            f = s->int_setting->option_callback;

                        if (f)
                            f(data->value.number);
                        break;
                    case F_T_BOOL:
                        *(bool*)s->setting = data->value.number ? true : false;
                        if (s->bool_setting->option_callback)
                            s->bool_setting
                                ->option_callback(data->value.number ? true : false);
                        break;
                }
                returncode = ACTION_REDRAW;
            }
            break;
            case ACTION_TOUCH_MUTE:
            {
                const int min_vol = sound_min(SOUND_VOLUME);
                if (global_settings.volume == min_vol)
                    global_settings.volume = temp->value;
                else
                {
                    temp->value = global_settings.volume;
                    global_settings.volume = min_vol;
                }
                setvol();
                returncode = ACTION_REDRAW;
            }
            break;
        }
        return returncode;
    }
        
    last_action = ACTION_TOUCHSCREEN;
    return ACTION_TOUCHSCREEN;
}
