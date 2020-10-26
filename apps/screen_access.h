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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _SCREEN_ACCESS_H_
#define _SCREEN_ACCESS_H_

#include "lcd.h"
#include "scroll_engine.h"
#include "backdrop.h"
#include "line.h"

#if defined(HAVE_REMOTE_LCD) && !defined (ROCKBOX_HAS_LOGF)
#define NB_SCREENS 2
void screen_helper_remote_setfont(int font);
#else
#define NB_SCREENS 1
#endif
void screen_helper_setfont(int font);

#define FOR_NB_SCREENS(i) for(int i = 0; i < NB_SCREENS; i++)

typedef void screen_bitmap_part_func(const void *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height);
typedef void screen_bitmap_func(const void *src, int x, int y, int width,
                              int height);

/* if this struct is changed the plugin api may break so bump the api
   versions in plugin.h */
struct screen
{
    enum screen_type screen_type;
    int lcdwidth, lcdheight;
    int depth;
    int (*getnblines)(void);
    int pixel_format;
    int (*getcharwidth)(void);
    int (*getcharheight)(void);
    bool is_color;
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    bool has_disk_led;
#endif
    void (*set_drawmode)(int mode);
    struct viewport* (*init_viewport)(struct viewport* vp);
    struct viewport* (*set_viewport)(struct viewport* vp);
    struct viewport* (*set_viewport_ex)(struct viewport* vp, int flags);
    void (*viewport_set_buffer)(struct viewport *vp, struct frame_buffer_t *buffer);
    struct viewport** current_viewport;
    int (*getwidth)(void);
    int (*getheight)(void);
    int (*getstringsize)(const unsigned char *str, int *w, int *h);
    void (*setfont)(int newfont);
    int (*getuifont)(void);
    void (*setuifont)(int newfont);

    void (*scroll_step)(int pixels);
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
    void (*bmp)(const struct bitmap *bm, int x, int y);
    void (*bmp_part)(const struct bitmap* bm, int src_x, int src_y,
                                int x, int y, int width, int height);
#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1
    unsigned (*color_to_native)(unsigned color);
#endif
#if (LCD_DEPTH > 1) || (defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH > 1))
    unsigned (*get_background)(void);
    unsigned (*get_foreground)(void);
    void (*set_background)(unsigned background);
    void (*set_foreground)(unsigned foreground);
#endif /* (LCD_DEPTH > 1) || (LCD_REMOTE_DEPTH > 1) */
    void (*update_rect)(int x, int y, int width, int height);
    void (*update_viewport_rect)(int x, int y, int width, int height);
    void (*fillrect)(int x, int y, int width, int height);
    void (*drawrect)(int x, int y, int width, int height);
    void (*fill_viewport)(void);
    void (*draw_border_viewport)(void);
    void (*drawpixel)(int x, int y);
    void (*drawline)(int x1, int y1, int x2, int y2);
    void (*vline)(int x, int y1, int y2);
    void (*hline)(int x1, int x2, int y);

    void (*putsxy)(int x, int y, const unsigned char *str);
    void (*puts)(int x, int y, const unsigned char *str);
    void (*putsf)(int x, int y, const unsigned char *str, ...);
    bool (*puts_scroll)(int x, int y, const unsigned char *string);
    bool (*putsxy_scroll_func)(int x, int y, const unsigned char *string,
                               void (*scroll_func)(struct scrollinfo *),
                               void *data, int x_offset);
    void (*scroll_speed)(int speed);
    void (*scroll_delay)(int ms);
    void (*clear_display)(void);
    void (*clear_viewport)(void);
    void (*scroll_stop)(void);
    void (*scroll_stop_viewport)(const struct viewport *vp);
    void (*scroll_stop_viewport_rect)(const struct viewport* vp, int x, int y, int width, int height);
    void (*update)(void);
    void (*update_viewport)(void);
    void (*backlight_on)(void);
    void (*backlight_off)(void);
    bool (*is_backlight_on)(bool ignore_always_off);
    void (*backlight_set_timeout)(int index);
#if LCD_DEPTH > 1
    bool (*backdrop_load)(const char *filename, char* backdrop_buffer);
    void (*backdrop_show)(char* backdrop_buffer);
#endif
#if defined(HAVE_LCD_COLOR)
    void (*gradient_fillrect)(int x, int y, int width, int height,
            unsigned start, unsigned end);
    void (*gradient_fillrect_part)(int x, int y, int width, int height,
            unsigned start, unsigned end, int src_height, int row_skip);
#endif
    void (*nine_segment_bmp)(const struct bitmap* bm, int x, int y,
                                int width, int height);
    void (*put_line)(int x, int y, struct line_desc *line, const char *fmt, ...);
};

/*
 * Clear only a given area of the screen
 * - screen : the screen structure
 * - xstart, ystart : where the area starts
 * - width, height : size of the area
 */
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height);

/*
 * exported screens array that should be used
 * by each app that wants to write to access display
 */
extern struct screen screens[NB_SCREENS];

#endif /*_SCREEN_ACCESS_H_*/
