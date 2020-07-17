/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Michael Sevakis
 *
 * LCD scrolling driver and scheduler
 *
 * Much collected and combined from the various Rockbox LCD drivers.
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
#ifndef __SCROLL_ENGINE_H__
#define __SCROLL_ENGINE_H__

#include <stdbool.h>
#include "config.h"
#include "file.h"

struct viewport;
struct scrollinfo;

extern void scroll_init(void) INIT_ATTR;

extern void lcd_bidir_scroll(int threshold);
extern void lcd_scroll_speed(int speed);
extern void lcd_scroll_delay(int ms);

extern void lcd_scroll_stop(void);
extern void lcd_scroll_stop_viewport(const struct viewport *vp);
extern void lcd_scroll_stop_viewport_rect(const struct viewport *vp, int x, int y, int width, int height);
extern bool lcd_scroll_now(struct scrollinfo *scroll);
#ifdef HAVE_REMOTE_LCD
extern void lcd_remote_scroll_speed(int speed);
extern void lcd_remote_scroll_delay(int ms);

extern void lcd_remote_scroll_stop(void);
extern void lcd_remote_scroll_stop_viewport(const struct viewport *vp);
extern void lcd_remote_scroll_stop_viewport_rect(const struct viewport *vp, int x, int y, int width, int height);
extern bool lcd_remote_scroll_now(struct scrollinfo *scroll);
#endif



/* internal usage, but in multiple drivers
 * larger than the normal linebuffer since it holds the line a second
 * time (+3 spaces) for non-bidir scrolling */
#define SCROLL_SPACING   3
#define SCROLL_LINE_SIZE (MAX_PATH + SCROLL_SPACING + 3*LCD_WIDTH/2 + 2)

struct scrollinfo
{
    struct viewport* vp;
    char linebuffer[(SCROLL_LINE_SIZE / 2) - SCROLL_SPACING];
    const char *line;
    /* rectangle for the line */
    int x, y; /* relative to the viewort */
    int width, height;
    /* pixel to skip from the beginning of the string, increments as the text scrolls */
    int offset;
    /* scroll presently forward or backward? */
    bool backward;
    bool bidir;
    long start_tick;

    /* support for custom scrolling functions,
     * must be called with ::line == NULL to indicate that the line
     * stops scrolling or when the userdata pointer is going to be changed
     * (the custom scroller can release the userdata then) */
    void (*scroll_func)(struct scrollinfo *s);
    void *userdata;
};

struct scroll_screen_info
{
    struct scrollinfo * const scroll;
    const int num_scroll; /* number of scrollable lines (also number of scroll structs) */
    int lines;  /* Number of currently scrolling lines */
    long ticks; /* # of ticks between updates*/
    long delay; /* ticks delay before start */
    int bidir_limit;  /* percent */
    int step;  /* pixels per scroll step */
#if defined(HAVE_REMOTE_LCD)
    long last_scroll;
#endif
};

/** main lcd **/
#define LCD_SCROLLABLE_LINES ((LCD_HEIGHT+4)/5 < 32 ? (LCD_HEIGHT+4)/5 : 32)

extern struct scroll_screen_info lcd_scroll_info;

/** remote lcd **/
#ifdef HAVE_REMOTE_LCD
#define LCD_REMOTE_SCROLLABLE_LINES \
    (((LCD_REMOTE_HEIGHT+4)/5 < 32) ? (LCD_REMOTE_HEIGHT+4)/5 : 32)
extern struct scroll_screen_info lcd_remote_scroll_info;
#endif

#endif /* __SCROLL_ENGINE_H__ */
