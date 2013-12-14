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

/* Clears scrolling lines that intersect with the area */
void LCDFN(scroll_stop_viewport_rect)(const struct viewport *vp, int x, int y, int width, int height)
{
    int i = 0;
    while (i < LCDFN(scroll_info).lines)
    {
        struct scrollinfo *s = &LCDFN(scroll_info).scroll[i];
        /* check if the specified area crosses the viewport in some way */
        if (s->vp == vp
            && (x < (s->x+s->width) && (x+width) >= s->x)
            && (y < (s->y+s->height) && (y+height) >= s->y))
        {
            /* If i is not the last active line in the array, then move
               the last item to position i. This compacts
               the scroll array at the same time of removing the line */
            if ((i + 1) != LCDFN(scroll_info).lines)
            {
                LCDFN(scroll_info).scroll[i] =
                    LCDFN(scroll_info).scroll[LCDFN(scroll_info).lines-1];
            }
            LCDFN(scroll_info).lines--;
        }
        else
        {
            i++;
        }
    }
}

/* Stop all scrolling lines in the specified viewport */
void LCDFN(scroll_stop_viewport)(const struct viewport *vp)
{
    LCDFN(scroll_stop_viewport_rect)(vp, 0, 0, vp->width, vp->height);
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

static void LCDFN(scroll_worker)(void)
{
    int index, width;
    bool makedelay;
    static char line_buf[SCROLL_LINE_SIZE];
    bool is_default;
    struct scroll_screen_info *si = &LCDFN(scroll_info);
    struct scrollinfo *s;
    struct viewport *vp;
    int step;

    for ( index = 0; index < si->lines; index++ )
    {
        s = &si->scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick))
            continue;

        s->start_tick = current_tick;

        /* this runs out of the ui thread, thus we need to
         * save and restore the current viewport since the ui thread
         * is unaware of the swapped viewports. the vp must
         * be switched early so that lcd_getstringsize() picks the
         * correct font */
        vp = LCDFN(get_viewport)(&is_default);
        LCDFN(set_viewport)(s->vp);

        width = LCDFN(getstringsize)(s->linebuffer, NULL, NULL);
        makedelay = false;
#ifdef HAVE_LCD_BITMAP
        step = si->step;
#else
        step = 1;
#endif

        if (s->backward)
            s->offset -= step;
        else
            s->offset += step;

        if (s->bidir)
        {   /* scroll bidirectional */
            s->line = s->linebuffer;
            if (s->offset <= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->backward = false;
                makedelay = true;
            }
            else if (s->offset >= width - s->width) {
                /* at end of line */
                s->offset = width - s->width;
                s->backward = true;
                makedelay = true;
            }
        }
        else
        {
            snprintf(line_buf, sizeof(line_buf)-1, "%s%s%s",
                s->linebuffer, "   ", s->linebuffer);
            s->line = line_buf;
            width += LCDFN(getstringsize)("   ", NULL, NULL);
            /* scroll forward the whole time */
            if (s->offset >= width) {
                s->offset = 0;
                makedelay = true;
            }
        }

        /* Stash and restore these three, so that the scroll_func
         * can do whatever it likes without destroying the state */
#ifdef HAVE_LCD_BITMAP
        unsigned drawmode;
#if LCD_DEPTH > 1
        unsigned fg_pattern, bg_pattern;
        fg_pattern = s->vp->fg_pattern;
        bg_pattern = s->vp->bg_pattern;
#endif
        drawmode   = s->vp->drawmode;
#endif
        s->scroll_func(s);

        LCDFN(update_viewport_rect)(s->x, s->y, s->width, s->height);

#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
        s->vp->fg_pattern = fg_pattern;
        s->vp->bg_pattern = bg_pattern;
#endif
        s->vp->drawmode = drawmode;
#endif
        LCDFN(set_viewport)(vp);

        if (makedelay)
            s->start_tick += si->delay + si->ticks;
    }
}
