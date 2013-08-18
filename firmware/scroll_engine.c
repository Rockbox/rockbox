/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include "config.h"
#include "gcc_extensions.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "usb.h"
#include "lcd.h"
#include "font.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "scroll_engine.h"

static const char scroll_tick_table[18] = {
 /* Hz values [f(x)=100.8/(x+.048)]:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33, 49.2, 96.2 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1
};

static struct scrollinfo lcd_scroll[LCD_SCROLLABLE_LINES];
static long next_scroll_tick;

#ifdef HAVE_REMOTE_LCD
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
static bool remote_enabled = false;
#else
#define remote_enabled = true;
#endif
static struct scrollinfo lcd_remote_scroll[LCD_REMOTE_SCROLLABLE_LINES];
#endif

struct scroll_screen_info lcd_scroll_info =
{
    .scroll       = lcd_scroll,
    .lines        = 0,
    .ticks        = 12,
    .delay        = HZ/2,
    .bidir_limit  = 50,
#ifdef HAVE_LCD_BITMAP
    .step         = 6,
#endif
#ifdef HAVE_LCD_CHARCELLS
    .jump_scroll_delay = HZ/4,
    .jump_scroll       = 0,
#endif
};

#ifdef HAVE_REMOTE_LCD
struct scroll_screen_info lcd_remote_scroll_info =
{
    .scroll       = lcd_remote_scroll,
    .lines        = 0,
    .ticks        = 12,
    .delay        = HZ/2,
    .bidir_limit  = 50,
    .step         = 6,
};
#endif /* HAVE_REMOTE_LCD */

void lcd_stop_scroll(void)
{
    lcd_scroll_info.lines = 0;
}

/* Stop scrolling line y in the specified viewport, or all lines if y < 0 */
void lcd_scroll_stop_line(const struct viewport* current_vp, int y)
{
    int i = 0;

    while (i < lcd_scroll_info.lines)
    {
        if ((lcd_scroll_info.scroll[i].vp == current_vp) && 
            ((y < 0) || (lcd_scroll_info.scroll[i].y == y)))
        {
            /* If i is not the last active line in the array, then move
               the last item to position i */
            if ((i + 1) != lcd_scroll_info.lines)
            {
                lcd_scroll_info.scroll[i] =
                    lcd_scroll_info.scroll[lcd_scroll_info.lines-1];
            }
            lcd_scroll_info.lines--;

            /* A line can only appear once, so we're done, 
             * unless we are clearing the whole viewport */
            if (y >= 0)
                return ;
        }
        else
        {
            i++;
        }
    }
}

/* Stop all scrolling lines in the specified viewport */
void lcd_scroll_stop(const struct viewport* vp)
{
    lcd_scroll_stop_line(vp, -1);
}

void lcd_scroll_speed(int speed)
{
    lcd_scroll_info.ticks = scroll_tick_table[speed];
}

#if defined(HAVE_LCD_BITMAP)
void lcd_scroll_step(int step)
{
    lcd_scroll_info.step = step;
}
#endif

void lcd_scroll_delay(int ms)
{
    lcd_scroll_info.delay = ms / (HZ / 10);
}

void lcd_bidir_scroll(int percent)
{
    lcd_scroll_info.bidir_limit = percent;
}

#ifdef HAVE_LCD_CHARCELLS
void lcd_jump_scroll(int mode) /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
{
    lcd_scroll_info.jump_scroll = mode;
}

void lcd_jump_scroll_delay(int ms)
{
    lcd_scroll_info.jump_scroll_delay = ms / (HZ / 10);
}
#endif

#ifdef HAVE_REMOTE_LCD
void lcd_remote_stop_scroll(void)
{
    lcd_remote_scroll_info.lines = 0;
}

/* Stop scrolling line y in the specified viewport, or all lines if y < 0 */
void lcd_remote_scroll_stop_line(const struct viewport* current_vp, int y)
{
    int i = 0;

    while (i < lcd_remote_scroll_info.lines)
    {
        if ((lcd_remote_scroll_info.scroll[i].vp == current_vp) && 
            ((y < 0) || (lcd_remote_scroll_info.scroll[i].y == y)))
        {
            /* If i is not the last active line in the array, then move
               the last item to position i */
            if ((i + 1) != lcd_remote_scroll_info.lines)
            {
                lcd_remote_scroll_info.scroll[i] = 
                    lcd_remote_scroll_info.scroll[lcd_remote_scroll_info.lines-1];
            }
            lcd_remote_scroll_info.lines--;

            /* A line can only appear once, so we're done, 
             * unless we are clearing the whole viewport */
            if (y >= 0)
                return ;
        }
        else
        {
            i++;
        }
    }
}

/* Stop all scrolling lines in the specified viewport */
void lcd_remote_scroll_stop(const struct viewport* vp)
{
    lcd_remote_scroll_stop_line(vp, -1);
}

void lcd_remote_scroll_speed(int speed)
{
    lcd_remote_scroll_info.ticks = scroll_tick_table[speed];
}

void lcd_remote_scroll_step(int step)
{
    lcd_remote_scroll_info.step = step;
}

void lcd_remote_scroll_delay(int ms)
{
    lcd_remote_scroll_info.delay = ms / (HZ / 10);
}

void lcd_remote_bidir_scroll(int percent)
{
    lcd_remote_scroll_info.bidir_limit = percent;
}

static void sync_display_ticks(void)
{
    lcd_scroll_info.next_scroll =
    lcd_remote_scroll_info.next_scroll = current_tick;
}

long scroll_do_step(void)
{
    long tick = current_tick;
    if (TIME_BEFORE(tick, next_scroll_tick))
        return next_scroll_tick;

#if CONFIG_PLATFORM & PLATFORM_NATIVE
    if (remote_enabled != remote_initialized)
    {
        remote_enabled = remote_initialized;
        sync_display_ticks();
    }
#endif

    if (!TIME_BEFORE(tick, lcd_scroll_info.next_scroll))
    {
        lcd_scroll_info.next_scroll = tick + lcd_scroll_info.ticks;
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
        if (lcd_active())
#endif
            lcd_scroll_fn();
    }

    if (remote_enabled &&
        !TIME_BEFORE(tick, lcd_remote_scroll_info.next_scroll))
    {
        lcd_remote_scroll_info.next_scroll =
            tick + lcd_remote_scroll_info.ticks;
        lcd_remote_scroll_fn();
    }

    /* return the tick of the earlier of the two */
    long next = lcd_scroll_info.next_scroll;
    if (remote_enabled &&
        TIME_BEFORE(lcd_remote_scroll_info.next_scroll, next))
    {
        next = lcd_remote_scroll_info.next_scroll;
    }

    next_scroll_tick = next;
    return next;
}
#else /* ndef HAVE_REMOTE_LCD */
static void sync_display_ticks(void)
{
    next_scroll_tick = current_tick;
}

long scroll_do_step(void)
{
    long tick = current_tick;

    if (TIME_BEFORE(tick, next_scroll_tick))
        return next_scroll_tick;

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    if (lcd_active())
#endif
        lcd_scroll_fn();

    long next = tick + lcd_scroll_info.ticks;
    next_scroll_tick = next;
    return next;
}
#endif /* HAVE_REMOTE_LCD */

void scroll_init(void)
{
    sync_display_ticks();
}
