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
#include "core_alloc.h"
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

/* We dont actually do anything with these pointers,
   but they need to be grouped like this to save code
   so storing them as void* is ok. (stops compile warning) */
static const struct bitmap *inbuilt_iconset[NB_SCREENS] =
{
    &bm_default_icons,
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    &bm_remote_default_icons,
#endif
};

enum Iconset {
    Iconset_user,
    Iconset_viewers,
    Iconset_Count
};

static struct iconset {
    struct bitmap bmp;
    bool loaded;
    int handle;
    int handle_locked;
} iconsets[Iconset_Count][NB_SCREENS];

#define ICON_HEIGHT(screen) (!iconsets[Iconset_user][screen].loaded ?       \
                             (*(inbuilt_iconset[screen])) : iconsets[Iconset_user][screen].bmp).height \
                            / Icon_Last_Themeable

#define ICON_WIDTH(screen)  (!iconsets[Iconset_user][screen].loaded ?       \
                             (*(inbuilt_iconset[screen])) : iconsets[Iconset_user][screen].bmp).width

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
    const int screen = display->screen_type;
    const int width = ICON_WIDTH(screen);
    const int height = ICON_HEIGHT(screen);
    const int is_rtl = lang_is_rtl();
    const struct bitmap *iconset;
    
    if (icon == Icon_NOICON)
    {
        if (is_rtl)
            xpos = display->getwidth() - xpos - width;
        screen_clear_area(display, xpos, ypos, width, height);
        return;
    }
    else if (icon >= Icon_Last_Themeable)
    {
        iconset = &iconsets[Iconset_viewers][screen].bmp;
        icon -= Icon_Last_Themeable;
        if (!iconsets[Iconset_viewers][screen].loaded || 
           (global_status.viewer_icon_count * height > iconset->height) ||
           (icon * height + height > iconset->height))
        {
            screen_put_iconxy(display, xpos, ypos, Icon_Questionmark);
            return;
        }
    }
    else if (iconsets[Iconset_user][screen].loaded)
    {
        iconset = &iconsets[Iconset_user][screen].bmp;
    }
    else
    {
        iconset = inbuilt_iconset[screen];
    }
    /* add some left padding to the icons if they are on the edge */
    if (xpos == 0)
        xpos++;

    if (is_rtl)
        xpos = display->getwidth() - xpos - width;


    display->bmp_part(iconset, 0, height * icon, xpos, ypos, width, height);
}

void screen_put_cursorxy(struct screen * display, int x, int y, bool on)
{
#ifdef HAVE_LCD_BITMAP
    screen_put_icon(display, x, y, on?Icon_Cursor:0);
#else
    screen_put_icon(display, x, y, on?CURSOR_CHAR:-1);
#endif
}

static int buflib_move_callback(int handle, void* current, void* new)
{
    (void)handle;
    (void)new;
    int i;
    FOR_NB_SCREENS(j)
    {
        for (i=0; i<Iconset_Count; i++)
        {
            struct iconset *set = &iconsets[i][j];
            if (set->bmp.data == current)
            {
                if (set->handle_locked > 0)
                    return BUFLIB_CB_CANNOT_MOVE;
                set->bmp.data = new;
                return BUFLIB_CB_OK;
            }
        }
    }
    return BUFLIB_CB_OK;
}
static struct buflib_callbacks buflib_ops = {buflib_move_callback, NULL, NULL};

static void load_icons(const char* filename, enum Iconset iconset,
                        enum screen_type screen)
{
    int size_read;
    int bmpformat = (FORMAT_NATIVE|FORMAT_DITHER|FORMAT_TRANSPARENT);
    struct iconset *ic = &iconsets[iconset][screen];
    int fd;
    
    ic->loaded = false;
    if (filename[0] && filename[0] != '-')
    {
        char path[MAX_PATH];
        
        snprintf(path, sizeof(path), ICON_DIR "/%s.bmp", filename);
        fd = open(path, O_RDONLY);
        if (fd < 0)
            return;
        size_t buf_size = read_bmp_fd(fd, &ic->bmp, 0, 
                                        bmpformat|FORMAT_RETURN_SIZE, NULL);
        ic->handle = core_alloc_ex(filename, buf_size, &buflib_ops);
        if (ic->handle <= 0)
        {
            close(fd);
            return;
        }
        lseek(fd, 0, SEEK_SET);
        ic->bmp.data = core_get_data(ic->handle);

        ic->handle_locked = 1;
        size_read = read_bmp_fd(fd, &ic->bmp, buf_size, bmpformat, NULL);
        close(fd);
        ic->handle_locked = 0;

        /* free unused alpha channel, if any */
        core_shrink(ic->handle, ic->bmp.data, size_read);

        if (size_read <= 0)
            ic->handle = core_free(ic->handle);
        else
            ic->loaded = true;
    }
}

void icons_init(void)
{
    int i;
    FOR_NB_SCREENS(j)
    {
        for (i=0; i<Iconset_Count; i++)
        {
            struct iconset* set = &iconsets[i][j];
            if (set->loaded && set->handle > 0)
            {
                set->handle = core_free(set->handle);
                set->loaded = false;
            }
        }
    }

    if (global_settings.show_icons)
    {
        load_icons(global_settings.icon_file, Iconset_user, SCREEN_MAIN);

        if (global_settings.viewers_icon_file[0] &&
            global_settings.viewers_icon_file[0] != '-')
        {
            load_icons(global_settings.viewers_icon_file,
                    Iconset_viewers, SCREEN_MAIN);
            read_viewer_theme_file();
        }
        else
        {
            load_icons(DEFAULT_VIEWER_BMP, Iconset_viewers, SCREEN_MAIN);
        }

#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
        load_icons(global_settings.remote_icon_file, 
                Iconset_user, SCREEN_REMOTE);
        
        if (global_settings.remote_viewers_icon_file[0] &&
            global_settings.remote_viewers_icon_file[0] != '-')
        {
            load_icons(global_settings.remote_viewers_icon_file,
                    Iconset_viewers, SCREEN_REMOTE);
        }
        else
        {
            load_icons(DEFAULT_REMOTE_VIEWER_BMP,
                    Iconset_viewers, SCREEN_REMOTE);
        }
#endif
    }
}

int get_icon_width(enum screen_type screen_type)
{
    return ICON_WIDTH(screen_type);
}

int get_icon_height(enum screen_type screen_type)
{
    return ICON_HEIGHT(screen_type);
}
