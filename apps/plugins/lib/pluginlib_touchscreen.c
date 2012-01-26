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
* Copyright (C) 2009 by Karl Kurbjun
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

#include "pluginlib_touchscreen.h"
#include "pluginlib_exit.h"

/*******************************************************************************
 * Touchbutton functions: These functions allow the plugin to specify a button
 *  location that, in turn gets mapped to a button press return value.
 ******************************************************************************/
 
/* touchbutton_check_button:
 *  This function checks the touchbutton structure passed to it for hits.  When
 *  one is found it returns action. It doesn't block because it doesn't actually
 *  call button_get. You need to call it before and pass its result.
 *  inputs:
 *      struct touchbutton *data: This is intended to be an array of
 *          touchbuttons of size num_buttons.  Each element in the array defines
 *          one button.
 *      int button: This is the button return value from a button_get() call.
 *          It is used to determine REPEAT/RELEASE events. This way
 *          this function can be mixed with other buttons
 *      int num_buttons: This tells touchbutton_get how many elements are in
 *          data.
 *  return:
 *      If a touch occured over one of the defined buttons, return action, else
 *          return 0.
 */
int touchbutton_check_button(int button, struct touchbutton *data, int num_buttons) {
    short x,y;
    
    /* Get the x/y location of the button press, this is set by button_get when
     *  a button is pulled from the queue.
     */
    x = rb->button_get_data() >> 16;
    y = (short) rb->button_get_data();
    struct viewport *v;

    int i;
    
    /* Loop over the data array to check if any of the buttons were pressed */
    for (i=0; i<num_buttons; i++) {
        v=&data[i].vp;
        /* See if the point is inside the button viewport */
        if(     x >= v->x && x < (v->x + v->width) &&
                y >= v->y && y < (v->y + v->height) ) {
            if(     ((button & BUTTON_REPEAT) && data[i].repeat) || 
                    ((button & BUTTON_REL) && !data[i].repeat) ) {
                return data[i].action;
            }
        }
    }
    return 0;
}

/* touchbutton_get_w_tmo:
 *  This function checks the touchbutton structure passed to it for hits.  When
 *  one is found it returns the corresponding action.
 *  inputs:
 *      struct touchbutton *data: This is intended to be an array of
 *          touchbuttons of size num_buttons.  Each element in the array defines
 *          one button.
 *      int tmo: Timeout when waiting for input.
 *      int num_buttons: This tells touchbutton_get how many elements are in
 *          data.
 *  return:
 *      If a touch occured over one of the defined buttons, return action, else
 *          return 0.
 */
int touchbutton_get_w_tmo(int tmo, struct touchbutton *data, int num_buttons)
{
    int btn = rb->button_get_w_tmo(tmo);
    int result = touchbutton_check_button(btn, data, num_buttons);
    exit_on_usb(result);
    return result;
}

/* touchbutton_get:
 *  This function checks the touchbutton structure passed to it for hits.  When
 *  one is found it returns the corresponding action.
 *  inputs:
 *      struct touchbutton *data: This is intended to be an array of
 *          touchbuttons of size num_buttons.  Each element in the array defines
 *          one button.
 *      int num_buttons: This tells touchbutton_get how many elements are in
 *          data.
 *  return:
 *      If a touch occured over one of the defined buttons, return action, else
 *          return 0.
 */
int touchbutton_get(struct touchbutton *data, int num_buttons)
{
    return touchbutton_get_w_tmo(TIMEOUT_BLOCK, data, num_buttons);
}

/* touchbutton_draw: 
 *  This function draws the button with the associated text as long as the
 *      invisible flag is not set.  Support for pixmaps needs to be added.
 *  inputs:
 *      struct touchbutton *data: This is intended to be an array of
 *          touchbuttons of size num_buttons.  Each element in the array defines
 *          one button.
 *      int num_buttons: This tells touchbutton_get how many elements are in
 *          data.
 *  return:
 *      If a touch occured over one of the defined buttons, return action, else
 *          return 0.
 */
void touchbutton_draw(struct touchbutton *data, int num_buttons) {
    int i;
    struct screen *lcd = rb->screens[SCREEN_MAIN];

    /* Loop over all the elements in data */
    for(i=0; i<num_buttons; i++) {
        /* Is this a visible button? */
        if(!data[i].invisible) {
            /* Set the current viewport to the button so that all drawing
             *  operations are within the button location.
             */
            lcd->set_viewport(&data[i].vp);

            /* Set line_height to height, then it'll center for us */
            data[i].vp.line_height = data[i].vp.height;
            data[i].vp.flags |= VP_FLAG_ALIGN_CENTER;
            
            /* If the width offset was 0, use a scrolling puts, else center and
             *  print the title.
             */
            lcd->puts_scroll_style(0, 0, data[i].title, STYLE_DEFAULT);
            /* Draw bounding box around the button location. */
            lcd->draw_border_viewport();
        }
    }
    lcd->set_viewport(NULL); /* Go back to the default viewport */
}

/*******************************************************************************
 * Touchmap functions: Not sure how exactly these functions are used, comments
 *  needed!!!
 ******************************************************************************/
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
