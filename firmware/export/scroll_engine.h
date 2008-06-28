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

#include <lcd.h>

void scroll_init(void);
void lcd_scroll_stop(struct viewport* vp);
void lcd_scroll_stop_line(struct viewport* vp, int y);
void lcd_scroll_fn(void);
#ifdef HAVE_REMOTE_LCD
void lcd_remote_scroll_fn(void);
void lcd_remote_scroll_stop(struct viewport* vp);
void lcd_remote_scroll_stop_line(struct viewport* vp, int y);
#endif

/* internal usage, but in multiple drivers */
#define SCROLL_SPACING   3
#ifdef HAVE_LCD_BITMAP
#define SCROLL_LINE_SIZE (MAX_PATH + SCROLL_SPACING + 3*LCD_WIDTH/2 + 2)
#else
#define SCROLL_LINE_SIZE (MAX_PATH + SCROLL_SPACING + 3*LCD_WIDTH + 2)
#endif

struct scrollinfo
{
    struct viewport* vp;
    char line[SCROLL_LINE_SIZE];
    int len;    /* length of line in chars */
    int y;      /* Position of the line on the screen (char co-ordinates) */
    int offset;
    int startx;
#ifdef HAVE_LCD_BITMAP
    int width;  /* length of line in pixels */
    int style; /* line style */
#endif/* HAVE_LCD_BITMAP */
    bool backward; /* scroll presently forward or backward? */
    bool bidir;
    long start_tick;
};

struct scroll_screen_info
{
    struct scrollinfo * const scroll;
    const int num_scroll; /* number of scrollable lines (also number of scroll structs) */
    int lines;  /* Number of currently scrolling lines */
    long ticks; /* # of ticks between updates*/
    long delay; /* ticks delay before start */
    int bidir_limit;  /* percent */
#ifdef HAVE_LCD_CHARCELLS
    long jump_scroll_delay; /* delay between jump scroll jumps */
    int jump_scroll; /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
#endif
#if defined(HAVE_LCD_BITMAP) || defined(HAVE_REMOTE_LCD)
    int step;  /* pixels per scroll step */
#endif
#if defined(HAVE_REMOTE_LCD)
    long last_scroll;
#endif
};

/** main lcd **/
#ifdef HAVE_LCD_BITMAP
#define LCD_SCROLLABLE_LINES ((LCD_HEIGHT+4)/5 < 32 ? (LCD_HEIGHT+4)/5 : 32)
#else
#define LCD_SCROLLABLE_LINES LCD_HEIGHT * 2
#endif

extern struct scroll_screen_info lcd_scroll_info;

/** remote lcd **/
#ifdef HAVE_REMOTE_LCD
#define LCD_REMOTE_SCROLLABLE_LINES \
    (((LCD_REMOTE_HEIGHT+4)/5 < 32) ? (LCD_REMOTE_HEIGHT+4)/5 : 32)
extern struct scroll_screen_info lcd_remote_scroll_info;
#endif

#endif /* __SCROLL_ENGINE_H__ */
