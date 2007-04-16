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

/* Quick and Dirty hack untill lcd bitmap drawing is fixed */
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif


#include <default_icons.h>
#ifdef HAVE_REMOTE_LCD
#include <remote_default_icons.h>
#endif

#define DEFAULT_VIEWER_BMP          ICON_DIR "/viewers.bmp"
#define DEFAULT_REMOTE_VIEWER_BMP   ICON_DIR "/remote_viewers.bmp"

/* These should robably be moved to config-<target>.h */
#define MAX_ICON_HEIGHT 24
#define MAX_ICON_WIDTH 24


/* We dont actually do anything with these pointers,
   but they need to be grouped like this to save code
   so storing them as void* is ok. (stops compile warning) */
static const void * inbuilt_icons[NB_SCREENS] = {
        (void*)default_icons
#ifdef HAVE_REMOTE_LCD
      , (void*)remote_default_icons
#endif
};

static const int default_width[NB_SCREENS] = {
      BMPWIDTH_default_icons
#ifdef HAVE_REMOTE_LCD
    , BMPWIDTH_remote_default_icons
#endif
};

/* height of whole file */
static const int default_height[NB_SCREENS] = {
      BMPHEIGHT_default_icons
#ifdef HAVE_REMOTE_LCD
    , BMPHEIGHT_remote_default_icons
#endif
};

#define IMG_BUFSIZE (MAX_ICON_HEIGHT * MAX_ICON_WIDTH * \
                     Icon_Last_Themeable *LCD_DEPTH/8)
static unsigned char icon_buffer[IMG_BUFSIZE][NB_SCREENS];
static bool custom_icons_loaded[NB_SCREENS] = {false};
static struct bitmap user_iconset[NB_SCREENS];

static unsigned char viewer_icon_buffer[IMG_BUFSIZE][NB_SCREENS];
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
    ypos = y*height + display->getymargin() + off_y;

    if ( height > ICON_HEIGHT(screen) )/* center the cursor */
        ypos += (height - ICON_HEIGHT(screen)) / 2;
    screen_put_iconxy(display, xpos, ypos, icon);
}

/* x,y in pixels */
typedef void (*lcd_draw_func)(const fb_data *src, int src_x, int src_y,
                      int stride, int x, int y, int width, int height);
void screen_put_iconxy(struct screen * display, 
                       int xpos, int ypos, enum themable_icons icon)
{
    fb_data *data;
    int screen = display->screen_type;
    lcd_draw_func draw_func = NULL;
    
    if (icon == Icon_NOICON)
    {
        screen_clear_area(display, xpos, ypos,
                          ICON_WIDTH(screen), ICON_HEIGHT(screen));
        return;
    }
    else if (icon >= Icon_Last_Themeable)
    {
        icon -= Icon_Last_Themeable;
        if (!viewer_icons_loaded[screen] || 
           (icon*ICON_HEIGHT(screen) > viewer_iconset[screen].height))
        {
            screen_clear_area(display, xpos, ypos,
                          ICON_WIDTH(screen), ICON_HEIGHT(screen));
            return;
        }
        data = (fb_data *)viewer_iconset[screen].data;
    }
    else if (custom_icons_loaded[screen])
    {
        data = (fb_data *)user_iconset[screen].data;
    }
    else
    {
        data = (fb_data *)inbuilt_icons[screen];
    }
    /* add some left padding to the icons if they are on the edge */
    if (xpos == 0)
        xpos++;
    
#ifdef HAVE_REMOTE_LCD
    if (display->screen_type == SCREEN_REMOTE)
    {
        /* Quick and Dirty hack untill lcd bitmap drawing is fixed */
        draw_func = (lcd_draw_func)lcd_remote_bitmap_part;
    }
    else
#endif
#if LCD_DEPTH == 16
        draw_func = display->transparent_bitmap_part;
#else /* LCD_DEPTH < 16 */
        draw_func = display->bitmap_part;
#endif /* LCD_DEPTH == 16 */
    
    draw_func(  (const fb_data *)data,
                0, ICON_HEIGHT(screen)*icon, 
                ICON_WIDTH(screen), xpos, ypos, 
                ICON_WIDTH(screen), ICON_HEIGHT(screen));
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
#ifdef HAVE_REMOTE_LCD
    Iconset_Remotescreen,
    Iconset_Remotescreen_viewers,
#endif
};

static void load_icons(const char* filename, enum Iconset iconset)
{
    int size_read;
    bool *loaded_ok = NULL;
    struct bitmap *bmp = NULL;
    
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
#ifdef HAVE_REMOTE_LCD
        case Iconset_Remotescreen:
            loaded_ok = &custom_icons_loaded[SCREEN_MAIN];
            bmp = &user_iconset[SCREEN_MAIN];
            bmp->data = icon_buffer[SCREEN_MAIN];
            break;
        case Iconset_Remotescreen_viewers:
            loaded_ok = &viewer_icons_loaded[SCREEN_REMOTE];
            bmp = &viewer_iconset[SCREEN_REMOTE];
            bmp->data = viewer_icon_buffer[SCREEN_REMOTE];
            break;
#endif
    }
    
    *loaded_ok = false;
    if (filename != NULL)
    {
        size_read = read_bmp_file((char*)filename, bmp, IMG_BUFSIZE,
                                  FORMAT_NATIVE | FORMAT_DITHER);
        if (size_read > 0)
        {
            *loaded_ok = true;
        }
    }
}


void icons_init(void)
{
    char path[MAX_PATH];
    if (global_settings.icon_file[0])
    {
        snprintf(path, MAX_PATH, "%s/%s.bmp",
                 ICON_DIR, global_settings.icon_file);
        load_icons(path, Iconset_Mainscreen);
    }
    if (global_settings.viewers_icon_file[0])
    {
        snprintf(path, MAX_PATH, "%s/%s.bmp",
                 ICON_DIR, global_settings.viewers_icon_file);
        load_icons(path, Iconset_Mainscreen_viewers);
        read_viewer_theme_file();
    }
    else
        load_icons(DEFAULT_VIEWER_BMP, Iconset_Mainscreen_viewers);
#ifdef HAVE_REMOTE_LCD
    if (global_settings.remote_icon_file[0])
    {
        snprintf(path, MAX_PATH, "%s/%s.bmp",
                 ICON_DIR, global_settings.remote_icon_file);
        load_icons(path, Iconset_Remotescreen);
    }
    if (global_settings.remote_viewers_icon_file[0])
    {
        snprintf(path, MAX_PATH, "%s/%s.bmp",
                 ICON_DIR, global_settings.remote_viewers_icon_file);
        load_icons(path, Iconset_Remotescreen_viewers);
    }
    else
        load_icons(DEFAULT_REMOTE_VIEWER_BMP, Iconset_Mainscreen_viewers);
#endif
}

int get_icon_width(enum screen_type screen_type)
{
    return ICON_WIDTH(screen_type);
}
