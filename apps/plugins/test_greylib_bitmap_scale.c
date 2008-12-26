/*****************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Andrew Mahone
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
#include "lib/grey.h"

PLUGIN_HEADER
GREY_INFO_STRUCT
static unsigned char grey_bm_buf[LCD_WIDTH * LCD_HEIGHT + 
                   BM_SCALED_SIZE(LCD_WIDTH,0,FORMAT_NATIVE,0)];
static unsigned char grey_display_buf[2*LCD_WIDTH * LCD_HEIGHT];

static const struct plugin_api* rb; /* global api struct pointer */

/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    static char filename[MAX_PATH];
    struct bitmap grey_bm = {
        .width = LCD_WIDTH,
        .height = LCD_HEIGHT,
        .data = grey_bm_buf
    };
    int ret, x, y;

    if(!parameter) return PLUGIN_ERROR;

    rb = api;

    rb->strcpy(filename, parameter);

    ret = rb->read_bmp_file(filename, &grey_bm, sizeof(grey_bm_buf),
                            FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT,
                            &format_grey);
    
    if(ret < 1)
    {
        rb->splash(HZ*2, "failed to load bitmap");
        return PLUGIN_ERROR;
    }

    long buf_taken;
    if(!grey_init(rb, &grey_display_buf[0], sizeof(grey_display_buf), 0,
                  LCD_WIDTH, LCD_HEIGHT, &buf_taken))
    {
        rb->splash(HZ*2,"grey init failed");
        return PLUGIN_ERROR;
    }
    grey_ub_clear_display();
    x = (LCD_WIDTH - grey_bm.width) / 2;
    y = (LCD_HEIGHT - grey_bm.height) / 2;
    grey_ub_gray_bitmap(grey_bm_buf, x, y, grey_bm.width, grey_bm.height);
    grey_show(true);

    rb->button_get(true);

    grey_release();
    
    return PLUGIN_OK;
}
