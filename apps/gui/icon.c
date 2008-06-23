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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#include <default_icons.h>
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
#include <remote_default_icons.h>
#endif

/* These are just the file names, the full path is snprint'ed when used */
#define DEFAULT_VIEWER_BMP          "viewers"
#define DEFAULT_REMOTE_VIEWER_BMP   "remote_viewers"

/* These should probably be moved to config-<target>.h */
#define MAX_ICON_HEIGHT 24
#define MAX_ICON_WIDTH 24


/* We dont actually do anything with these pointers,
   but they need to be grouped like this to save code
   so storing them as void* is ok. (stops compile warning) */
static const void * inbuilt_icons[NB_SCREENS] = {
        (void*)default_icons
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
      , (void*)remote_default_icons
#endif
};

static const int default_width[NB_SCREENS] = {
      BMPWIDTH_default_icons
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    , BMPWIDTH_remote_default_icons
#endif
};

/* height of whole file */
static const int default_height[NB_SCREENS] = {
      BMPHEIGHT_default_icons
#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    , BMPHEIGHT_remote_default_icons
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
                             default_height[screen] :   \
                             user_iconset[screen].height) \
                            / Icon_Last_Themeable
                            
#define ICON_WIDTH(screen)  (!custom_icons_loaded[screen]?       \
                             default_width[screen] :   \
                             user_iconset[screen].width)
                            
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
    int xpos, ypos;
    int width, height;
    int screen = display->screen_type;
    display->getstringsize((unsigned char *)"M", &width, &height);
    xpos = x*ICON_WIDTH(screen) + off_x;
    ypos = y*height + off_y;

    if ( height > ICON_HEIGHT(screen) )/* center the cursor */
        ypos += (height - ICON_HEIGHT(screen)) / 2;
    screen_put_iconxy(display, xpos, ypos, icon);
}

/* x,y in pixels */
void screen_put_iconxy(struct screen * display,
                       int xpos, int ypos, enum themable_icons icon)
{
    const void *data;
    int screen = display->screen_type;
    int width = ICON_WIDTH(screen);
    int height = ICON_HEIGHT(screen);
    screen_bitmap_part_func *draw_func = NULL;
    
    if (icon == Icon_NOICON)
    {
        screen_clear_area(display, xpos, ypos, width, height);
        return;
    }
    else if (icon >= Icon_Last_Themeable)
    {
        icon -= Icon_Last_Themeable;
        if (!viewer_icons_loaded[screen] || 
           (global_status.viewer_icon_count*height
             > viewer_iconset[screen].height) ||
           (icon * height > viewer_iconset[screen].height))
        {
            screen_put_iconxy(display, xpos, ypos, Icon_Questionmark);
            return;
        }
        data = viewer_iconset[screen].data;
    }
    else if (custom_icons_loaded[screen])
    {
        data = user_iconset[screen].data;
    }
    else
    {
        data = inbuilt_icons[screen];
    }
    /* add some left padding to the icons if they are on the edge */
    if (xpos == 0)
        xpos++;
    
#if (LCD_DEPTH == 16) || defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH == 16)
    if (display->depth == 16)
        draw_func = display->transparent_bitmap_part;
    else
#endif
        draw_func = display->bitmap_part;

    draw_func(data, 0, height * icon, width, xpos, ypos, width, height);
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

static void load_icons(const char* filename, enum Iconset iconset, 
    bool allow_disable)
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
    if (!allow_disable || (filename[0] && filename[0] != '-'))
    {
        char path[MAX_PATH];
        
        snprintf(path, sizeof(path), "%s/%s.bmp", ICON_DIR, filename);
        size_read = read_bmp_file(path, bmp, IMG_BUFSIZE, bmpformat);
        if (size_read > 0)
        {
            *loaded_ok = true;
        }
    }
}


void icons_init(void)
{
    load_icons(global_settings.icon_file, Iconset_Mainscreen, true);
    
    if (*global_settings.viewers_icon_file)
    {
        load_icons(global_settings.viewers_icon_file, 
                   Iconset_Mainscreen_viewers, true);
        read_viewer_theme_file();
    }
    else
    {
        load_icons(DEFAULT_VIEWER_BMP, Iconset_Mainscreen_viewers, false);
    }

#if defined(HAVE_REMOTE_LCD) && (NB_SCREENS > 1)
    load_icons(global_settings.remote_icon_file, 
               Iconset_Remotescreen, true);
    
    if (*global_settings.remote_viewers_icon_file)
    {
        load_icons(global_settings.remote_viewers_icon_file,
                   Iconset_Remotescreen_viewers, true);
    }
    else
    {
        load_icons(DEFAULT_REMOTE_VIEWER_BMP,
                   Iconset_Remotescreen_viewers, false);
    }
#endif
}

int get_icon_width(enum screen_type screen_type)
{
    return ICON_WIDTH(screen_type);
}
