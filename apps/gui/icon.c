/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inttypes.h"
#include "config.h"
#include "icon.h"
#include "screen_access.h"
#include "icons.h"
#include "settings.h"
#include "bmp.h"
#include "filetypes.h"
#include "language.h"

#include "bitmaps/default_icons.h"
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
#include "bitmaps/remote_default_icons.h"
#endif

/* These are just the file names, the full path is snprint'ed when used */
#define DEFAULT_VIEWER_BMP          "viewers"
#define DEFAULT_REMOTE_VIEWER_BMP   "remote_viewers"

/* These can be defined in config-<target>.h, if it is not provide defaults */
#if !defined(MAX_ICON_HEIGHT)
#define MAX_ICON_HEIGHT 24
#endif

#if !defined(MAX_ICON_WIDTH)
#define MAX_ICON_WIDTH 24
#endif

/* We dont actually do anything with these pointers,
   but they need to be grouped like this to save code
   so storing them as void* is ok. (stops compile warning) */
static const struct bitmap inbuilt_iconset[NB_SCREENS] =
{
    {
        .width = BMPWIDTH_default_icons,
        .height = BMPHEIGHT_default_icons,
        .data = (unsigned char*)default_icons,
    },
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    {
        .width = BMPWIDTH_remote_default_icons,
        .height = BMPHEIGHT_remote_default_icons,
        .data = (unsigned char*)remote_default_icons,
    },
#endif
};

#define IMG_BUFSIZE (MAX_ICON_HEIGHT * MAX_ICON_WIDTH * \
                     Icon_Last_Themeable *LCD_DEPTH/8)
static unsigned char icon_buffer[NB_SCREENS][IMG_BUFSIZE];
static bool custom_icons_loaded[NB_SCREENS] = {false};
static struct bitmap user_iconset[NB_SCREENS];

static unsigned char viewer_icon_buffer[NB_SCREENS][IMG_BUFSIZE];
static bool viewer_icons_loaded[NB_SCREENS] = {false};
static struct bitmap viewer_iconset[NB_SCREENS];


#define ICON_HEIGHT(screen) (!custom_icons_loaded[screen]?       \
                             inbuilt_iconset : user_iconset)[screen].height \
                            / Icon_Last_Themeable

#define ICON_WIDTH(screen)  (!custom_icons_loaded[screen]?       \
                             inbuilt_iconset : user_iconset)[screen].width

/* x,y in letters, not pixles */
void screen_put_icon(struct screen * display, 
                       int x, int y, enum themable_icons icon)
{
    screen_put_icon_with_offset(display, x, y, 0, 0, icon);
}

void screen_put_icon_with_offset(struct screen * display, 
                       int x, int y, int off_x, int off_y,
                       enum themable_icons icon)
{
    const int screen = display->screen_type;
    const int icon_width = ICON_WIDTH(screen);
    const int icon_height = ICON_HEIGHT(screen);
    int xpos, ypos;
    int width, height;
    display->getstringsize((unsigned char *)"M", &width, &height);
    xpos = x*icon_width + off_x;
    ypos = y*height + off_y;

    if ( height > icon_height )/* center the cursor */
        ypos += (height - icon_height) / 2;
    screen_put_iconxy(display, xpos, ypos, icon);
}

/* x,y in pixels */
void screen_put_iconxy(struct screen * display,
                       int xpos, int ypos, enum themable_icons icon)
{
    const void *data;
    const int screen = display->screen_type;
    const int width = ICON_WIDTH(screen);
    const int height = ICON_HEIGHT(screen);
    const int is_rtl = lang_is_rtl();
    int stride;
    const struct bitmap *iconset;
    screen_bitmap_part_func *draw_func = NULL;
    
    if (icon == Icon_NOICON)
    {
        if (is_rtl)
            xpos = display->getwidth() - xpos - width;
        screen_clear_area(display, xpos, ypos, width, height);
        return;
    }
    else if (icon >= Icon_Last_Themeable)
    {
        iconset = &viewer_iconset[screen];
        icon -= Icon_Last_Themeable;
        if (!viewer_icons_loaded[screen] || 
           (global_status.viewer_icon_count * height > iconset->height) ||
           (icon * height + height > iconset->height))
        {
            screen_put_iconxy(display, xpos, ypos, Icon_Questionmark);
            return;
        }
    }
    else if (custom_icons_loaded[screen])
    {
        iconset = &user_iconset[screen];
    }
    else
    {
        iconset = &inbuilt_iconset[screen];
    }
    data = iconset->data;
    stride = STRIDE(display->screen_type, iconset->width, iconset->height);

    /* add some left padding to the icons if they are on the edge */
    if (xpos == 0)
        xpos++;

    if (is_rtl)
        xpos = display->getwidth() - xpos - width;

#if (LCD_DEPTH == 16) || defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH == 16)
    if (display->depth == 16)
        draw_func   = display->transparent_bitmap_part;
    else
#endif
        draw_func   = display->bitmap_part;

    draw_func(data, 0, height * icon, stride, xpos, ypos, width, height);
}

void screen_put_cursorxy(struct screen * display, int x, int y, bool on)
{
#ifdef HAVE_LCD_BITMAP
    screen_put_icon(display, x, y, on?Icon_Cursor:0);
#else
    screen_put_icon(display, x, y, on?CURSOR_CHAR:-1);
#endif
}

enum Iconset {
    Iconset_Mainscreen,
    Iconset_Mainscreen_viewers,
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    Iconset_Remotescreen,
    Iconset_Remotescreen_viewers,
#endif
};

static void load_icons(const char* filename, enum Iconset iconset)
{
    int size_read;
    bool *loaded_ok = NULL;
    struct bitmap *bmp = NULL;
    int bmpformat = (FORMAT_NATIVE|FORMAT_DITHER);
    
    switch (iconset)
    {
        case Iconset_Mainscreen:
            loaded_ok = &custom_icons_loaded[SCREEN_MAIN];
            bmp = &user_iconset[SCREEN_MAIN];
            bmp->data = icon_buffer[SCREEN_MAIN];
            break;
        case Iconset_Mainscreen_viewers:
            loaded_ok = &viewer_icons_loaded[SCREEN_MAIN];
            bmp = &viewer_iconset[SCREEN_MAIN];
            bmp->data = viewer_icon_buffer[SCREEN_MAIN];
            break;
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
        case Iconset_Remotescreen:
            loaded_ok = &custom_icons_loaded[SCREEN_REMOTE];
            bmp = &user_iconset[SCREEN_REMOTE];
            bmp->data = icon_buffer[SCREEN_REMOTE];
            bmpformat |= FORMAT_REMOTE;
            break;
        case Iconset_Remotescreen_viewers:
            loaded_ok = &viewer_icons_loaded[SCREEN_REMOTE];
            bmp = &viewer_iconset[SCREEN_REMOTE];
            bmp->data = viewer_icon_buffer[SCREEN_REMOTE];
            bmpformat |= FORMAT_REMOTE;
            break;
#endif
    }
    
    *loaded_ok = false;
    if (filename[0] && filename[0] != '-')
    {
        char path[MAX_PATH];
        
        snprintf(path, sizeof(path), ICON_DIR "/%s.bmp", filename);
        size_read = read_bmp_file(path, bmp, IMG_BUFSIZE, bmpformat, NULL);
        if (size_read > 0)
        {
            *loaded_ok = true;
        }
    }
}


void icons_init(void)
{
    load_icons(global_settings.icon_file, Iconset_Mainscreen);
    
    if (global_settings.viewers_icon_file[0] &&
        global_settings.viewers_icon_file[0] != '-')
    {
        load_icons(global_settings.viewers_icon_file,
                   Iconset_Mainscreen_viewers);
        read_viewer_theme_file();
    }
    else
    {
        load_icons(DEFAULT_VIEWER_BMP, Iconset_Mainscreen_viewers);
    }

#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    load_icons(global_settings.remote_icon_file, 
               Iconset_Remotescreen);
    
    if (global_settings.remote_viewers_icon_file[0] &&
        global_settings.remote_viewers_icon_file[0] != '-')
    {
        load_icons(global_settings.remote_viewers_icon_file,
                   Iconset_Remotescreen_viewers);
    }
    else
    {
        load_icons(DEFAULT_REMOTE_VIEWER_BMP,
                   Iconset_Remotescreen_viewers);
    }
#endif
}

int get_icon_width(enum screen_type screen_type)
{
    return ICON_WIDTH(screen_type);
}
