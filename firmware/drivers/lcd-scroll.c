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
 * LCD scroll control functions (API to apps).
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

/* This file is meant to be #included by scroll_engine.c (twice if a remote
 * is present) */

#ifndef LCDFN /* Not compiling for remote - define macros for main LCD. */
#define LCDFN(fn) lcd_ ## fn
#define LCDM(ma) LCD_ ## ma
#define MAIN_LCD
#endif

static struct scrollinfo LCDFN(scroll)[LCD_SCROLLABLE_LINES];

struct scroll_screen_info LCDFN(scroll_info) =
{
    .scroll       = LCDFN(scroll),
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


void LCDFN(scroll_stop)(void)
{
    LCDFN(scroll_info).lines = 0;
}

/* Stop scrolling line y in the specified viewport, or all lines if y < 0 */
void LCDFN(scroll_stop_viewport_line)(const struct viewport *current_vp, int line)
{
    int i = 0;

    while (i < LCDFN(scroll_info).lines)
    {
        struct viewport *vp = LCDFN(scroll_info).scroll[i].vp;
        if (((vp == current_vp)) && 
            ((line < 0) || (LCDFN(scroll_info).scroll[i].y == line)))
        {
            /* If i is not the last active line in the array, then move
               the last item to position i */
            if ((i + 1) != LCDFN(scroll_info).lines)
            {
                LCDFN(scroll_info).scroll[i] =
                    LCDFN(scroll_info).scroll[LCDFN(scroll_info).lines-1];
            }
            LCDFN(scroll_info).lines--;

            /* A line can only appear once, so we're done, 
             * unless we are clearing the whole viewport */
            if (line >= 0)
                return ;
        }
        else
        {
            i++;
        }
    }
}

/* Stop all scrolling lines in the specified viewport */
void LCDFN(scroll_stop_viewport)(const struct viewport *current_vp)
{
    LCDFN(scroll_stop_viewport_line)(current_vp, -1);
}

void LCDFN(scroll_speed)(int speed)
{
    LCDFN(scroll_info).ticks = scroll_tick_table[speed];
}

#if defined(HAVE_LCD_BITMAP)
void LCDFN(scroll_step)(int step)
{
    LCDFN(scroll_info).step = step;
}
#endif

void LCDFN(scroll_delay)(int ms)
{
    LCDFN(scroll_info).delay = ms / (HZ / 10);
}

void LCDFN(bidir_scroll)(int percent)
{
    LCDFN(scroll_info).bidir_limit = percent;
}

#ifdef HAVE_LCD_CHARCELLS
void LCDFN(jump_scroll)(int mode) /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
{
    LCDFN(scroll_info).jump_scroll = mode;
}

void LCDFN(jump_scroll_delay)(int ms)
{
    LCDFN(scroll_info).jump_scroll_delay = ms / (HZ / 10);
}
#endif
