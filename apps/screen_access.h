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

#ifndef _SCREEN_ACCESS_H_
#define _SCREEN_ACCESS_H_

#include "lcd.h"
#include "buttonbar.h"

enum screen_type {
    SCREEN_MAIN
#ifdef HAVE_REMOTE_LCD
    ,SCREEN_REMOTE
#endif
};

#if defined(HAVE_REMOTE_LCD) && !defined (ROCKBOX_HAS_LOGF)
#define NB_SCREENS 2
#else
#define NB_SCREENS 1
#endif

#if NB_SCREENS == 1
#define FOR_NB_SCREENS(i) i = 0;
#else
#define FOR_NB_SCREENS(i) for(i = 0; i < NB_SCREENS; i++)
#endif

#ifdef HAVE_LCD_CHARCELLS
#define MAX_LINES_ON_SCREEN 2
#endif

typedef void screen_bitmap_part_func(const void *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height);
typedef void screen_bitmap_func(const void *src, int x, int y, int width,
                              int height);

/* if this struct is changed the plugin api may break so bump the api
   versions in plugin.h */
struct screen
{
    enum screen_type screen_type;
    int width, height;
    int depth;
    int nb_lines;
#ifdef HAVE_LCD_BITMAP
    int pixel_format;
#endif
    int char_width;
    int char_height;
    bool is_color;
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    bool has_disk_led;
#endif
#ifdef HAVE_BUTTONBAR
    bool has_buttonbar;
#endif
    void (*set_viewport)(struct viewport* vp);
    void (*setmargins)(int x, int y);
    int (*getwidth)(void);
    int (*getheight)(void);
    int (*getxmargin)(void);
    int (*getymargin)(void);
    int (*getstringsize)(const unsigned char *str, int *w, int *h);
#if defined(HAVE_LCD_BITMAP) || defined(HAVE_REMOTE_LCD) /* always bitmap */
    void (*setfont)(int newfont);
    int (*getfont)(void);

    void (*scroll_step)(int pixels);
    void (*puts_style_offset)(int x, int y, const unsigned char *str,
                              int style, int offset);
    void (*puts_scroll_style)(int x, int y, const unsigned char *string,
                                 int style);
    void (*puts_scroll_style_offset)(int x, int y, const unsigned char *string,
                                     int style, int offset);
    void (*mono_bitmap)(const unsigned char *src,
                        int x, int y, int width, int height);
    void (*mono_bitmap_part)(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height);
    void (*bitmap)(const void *src,
                   int x, int y, int width, int height);
    void (*bitmap_part)(const void *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height);
    void (*transparent_bitmap)(const void *src,
                               int x, int y, int width, int height);
    void (*transparent_bitmap_part)(const void *src, int src_x, int src_y,
                                    int stride, int x, int y, int width, int height);
    void (*set_drawmode)(int mode);
#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1
    unsigned (*color_to_native)(unsigned color);
#endif
#if (LCD_DEPTH > 1) || (defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH > 1))
    unsigned (*get_background)(void);
    unsigned (*get_foreground)(void);
    void (*set_background)(unsigned background);
    void (*set_foreground)(unsigned foreground);
#endif /* (LCD_DEPTH > 1) || (LCD_REMOTE_DEPTH > 1) */
#if defined(HAVE_LCD_COLOR)
    void (*set_selector_start)(unsigned selector);
    void (*set_selector_end)(unsigned selector);
    void (*set_selector_text)(unsigned selector_text);
#endif
    void (*update_rect)(int x, int y, int width, int height);
    void (*update_viewport_rect)(int x, int y, int width, int height);
    void (*fillrect)(int x, int y, int width, int height);
    void (*drawrect)(int x, int y, int width, int height);
    void (*drawpixel)(int x, int y);
    void (*drawline)(int x1, int y1, int x2, int y2);
    void (*vline)(int x, int y1, int y2);
    void (*hline)(int x1, int x2, int y);
#endif /* HAVE_LCD_BITMAP || HAVE_REMOTE_LCD */

#ifdef HAVE_LCD_CHARCELLS  /* no charcell remote LCDs so far */
    void (*double_height)(bool on);
    void (*putc)(int x, int y, unsigned long ucs);
    void (*icon)(int icon, bool enable);
    unsigned long (*get_locked_pattern)(void);
    void (*define_pattern)(unsigned long ucs, const char *pattern);
    void (*unlock_pattern)(unsigned long ucs);
#endif
    void (*putsxy)(int x, int y, const unsigned char *str);
    void (*puts)(int x, int y, const unsigned char *str);
    void (*puts_offset)(int x, int y, const unsigned char *str, int offset);
    void (*puts_scroll)(int x, int y, const unsigned char *string);
    void (*puts_scroll_offset)(int x, int y, const unsigned char *string,
                                 int offset);
    void (*scroll_speed)(int speed);
    void (*scroll_delay)(int ms);
    void (*stop_scroll)(void);
    void (*clear_display)(void);
    void (*clear_viewport)(void);
    void (*scroll_stop)(struct viewport* vp);
    void (*scroll_stop_line)(struct viewport* vp, int y);
    void (*update)(void);
    void (*update_viewport)(void);
    void (*backlight_on)(void);
    void (*backlight_off)(void);
    bool (*is_backlight_on)(bool ignore_always_off);
    void (*backlight_set_timeout)(int index);
};

#ifdef HAVE_BUTTONBAR
/*
 * Sets if the given screen has a buttonbar or not
 * - screen : the screen structure
 * - has : a boolean telling wether the current screen will have a buttonbar or not
 */
#define screen_has_buttonbar(screen, has_btnb) \
    (screen)->has_buttonbar=has_btnb;
#endif

/*
 * Sets the x margin in pixels for the given screen
 * - screen : the screen structure
 * - xmargin : the number of pixels to the left of the screen
 */
#define screen_set_xmargin(screen, xmargin) \
    (screen)->setmargins(xmargin, (screen)->getymargin());

/*
 * Sets the y margin in pixels for the given screen
 * - screen : the screen structure
 * - xmargin : the number of pixels to the top of the screen
 */
#define screen_set_ymargin(screen, ymargin) \
    (screen)->setmargins((screen)->getxmargin(), ymargin);

#if defined(HAVE_LCD_BITMAP) || defined(HAVE_REMOTE_LCD)
/*
 * Clear only a given area of the screen
 * - screen : the screen structure
 * - xstart, ystart : where the area starts
 * - width, height : size of the area
 */
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height);
#endif

/*
 * Initializes the whole screen_access api
 */
extern void screen_access_init(void);

/*
 * exported screens array that should be used
 * by each app that wants to write to access display
 */
extern struct screen screens[NB_SCREENS];

#endif /*_SCREEN_ACCESS_H_*/
