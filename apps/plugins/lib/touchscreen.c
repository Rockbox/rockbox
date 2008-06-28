/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Maurus Cuelenaere
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

#ifdef HAVE_TOUCHPAD

#include "touchscreen.h"

unsigned int touchscreen_map(struct ts_mappings *map, int x, int y)
{
    int i;
    for(i=0; i < map->amount; i++)
    {
        #define _MAP(x) (map->mappings[x])
        if(x > _MAP(i).tl_x && x < (_MAP(i).tl_x+_MAP(i).width)
           && y > _MAP(i).tl_y && y < (_MAP(i).tl_y+_MAP(i).height))
            return i;
    }
    
    return -1;
}

unsigned int touchscreen_map_raster(struct ts_raster *map, int x, int y, struct ts_raster_result *result)
{
    int res1_x, res2_x, res1_y, res2_y;
    
    if((x - map->tl_x) < 0 ||
       (x - map->tl_x) > map->width)
        return -1;
    res1_x = (x - map->tl_x)/(map->raster_width);
    res2_x = (x - map->tl_x)%(map->raster_width);
    
    if((y - map->tl_y) < 0 ||
       (y - map->tl_y) > map->height)
        return -1;
    res1_y = (y - map->tl_y)/(map->raster_height);
    res2_y = (y - map->tl_y)%(map->raster_height);
    
    if(res2_x == 0 || res2_y == 0) /* pen hit a raster boundary */
        return -2;
    else
    {
        (*result).x = res1_x;
        (*result).y = res1_y;
        return 1;
    }
}

struct ts_raster_button_result touchscreen_raster_map_button(struct ts_raster_button_mapping *map, int x, int y, int button)
{
    struct ts_raster_button_result ret = {0, {0, 0}, {0, 0}};
    struct ts_raster_result tmp;
    
    ret.action = TS_ACTION_NONE;
    if(touchscreen_map_raster(map->raster, x, y, &tmp) != 1)
        return ret;
    
    #define NOT_HANDLED (ret.action == TS_ACTION_NONE)
    if((button == BUTTON_REPEAT) && (map->_prev_btn_state != BUTTON_REPEAT) && map->drag_drop_enable)
    {
        map->_prev_x = tmp.x;
        map->_prev_y = tmp.y;
    }
    if((button == BUTTON_REL) && (map->_prev_btn_state == BUTTON_REPEAT) && map->drag_drop_enable)
    {
        ret.action = TS_ACTION_DRAG_DROP;
        ret.from.x = map->_prev_x;
        ret.from.y = map->_prev_y;
        ret.to.x = tmp.x;
        ret.to.y = tmp.y;
    }
    if((button == BUTTON_REL) && map->double_click_enable && NOT_HANDLED)
    {
        if(map->_prev_x == tmp.x && map->_prev_y == tmp.y)
        {
            ret.action = TS_ACTION_DOUBLE_CLICK;
            ret.from.x = ret.to.x = tmp.x;
            ret.from.y = ret.to.y = tmp.y;
        }
        else
        {
            map->_prev_x = tmp.x;
            map->_prev_y = tmp.y;
        }
    }
    if((button & BUTTON_REL || button & BUTTON_REPEAT) && map->two_d_movement_enable && NOT_HANDLED)
    {
        if((map->two_d_from.x == tmp.x) ^ (map->two_d_from.y == tmp.y))
        {
            ret.action = TS_ACTION_TWO_D_MOVEMENT;
            ret.from.x = map->two_d_from.x;
            ret.from.y = map->two_d_from.y;
            ret.to.x = map->two_d_from.x + (map->two_d_from.x == tmp.x ? 0 : (tmp.x > map->two_d_from.x ? 1 : -1));
            ret.to.y = map->two_d_from.y + (map->two_d_from.y == tmp.y ? 0 : (tmp.y > map->two_d_from.y ? 1 : -1));
        }
        else
            ret.action = TS_ACTION_NONE;
    }
    if(map->click_enable && (button & BUTTON_REL) && NOT_HANDLED)
    {
        ret.action = TS_ACTION_CLICK;
        ret.from.x = ret.to.x = tmp.x;
        ret.from.y = ret.to.y = tmp.y;
    }
    if(map->move_progress_enable && NOT_HANDLED)
    {
        ret.action = TS_ACTION_MOVE;
        ret.from.x = ret.to.x = tmp.x;
        ret.from.y = ret.to.y = tmp.y;
    }
    
    map->_prev_btn_state = button;
    return ret;
}

#endif /* HAVE_TOUCHPAD */
