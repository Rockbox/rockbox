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

#if !defined(BOOTLOADER)
static struct scrollinfo LCDFN(scroll)[LCDM(SCROLLABLE_LINES)];

struct scroll_screen_info LCDFN(scroll_info) =
{
    .scroll       = LCDFN(scroll),
    .lines        = 0,
    .ticks        = 12,
    .delay        = HZ/2,
    .bidir_limit  = 50,
    .step         = 6,
};


void LCDFN(scroll_stop)(void)
{
    for (int i = 0; i < LCDFN(scroll_info).lines; i++)
    {
        /* inform scroller about end of scrolling */
        struct scrollinfo *s = &LCDFN(scroll_info).scroll[i];
        s->line = NULL;
        s->scroll_func(s);
    }
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
            && (x < (s->x+s->width) && (x+width) > s->x)
            && (y < (s->y+s->height) && (y+height) > s->y))
        {
            /* inform scroller about end of scrolling */
            s->line = NULL;
            s->scroll_func(s);
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

void LCDFN(scroll_step)(int step)
{
    LCDFN(scroll_info).step = step;
}

void LCDFN(scroll_delay)(int ms)
{
    LCDFN(scroll_info).delay = ms / (HZ / 10);
}

void LCDFN(bidir_scroll)(int percent)
{
    LCDFN(scroll_info).bidir_limit = percent;
}


/* This renders the scrolling line described by s immediatly.
 * This can be called to update a scrolling line if the text has changed
 * without waiting for the next scroll tick
 *
 * Returns true if the text scrolled to the end */
bool LCDFN(scroll_now)(struct scrollinfo *s)
{
    int width = s->line_stringsize; /* Calculated by LCDFN puts_scroll_worker() */
    /*int width = font_getstringsize(s->linebuffer, NULL, NULL, s->vp->font);*/

    bool ended = false;
    /* assume s->scroll_func() don't yield; otherwise this buffer might need
     * to be mutex'd (the worst case would be minor glitches though) */
    static char line_buf[SCROLL_LINE_SIZE];

    if (s->bidir)
    {   /* scroll bidirectional */
        s->line = s->linebuffer;
        if (s->offset <= 0) {
            /* at beginning of line */
            s->offset = 0;
            s->backward = false;
            ended = true;
        }
        else if (s->offset >= width - s->width) {
            /* at end of line */
            s->offset = width - s->width;
            s->backward = true;
            ended = true;
        }
    }
    else
    {
        s->backward = false; /* bugfix we only account for forward scrolling here */
        snprintf(line_buf, sizeof(line_buf)-1, "%s%s%s",
            s->linebuffer, "   ", s->linebuffer);
        s->line = line_buf;
        width += font_getstringsize(" ", NULL, NULL, s->vp->font) * 3;
        /* scroll forward the whole time */
        if (s->offset >= width) {
            s->offset = 0;
            ended = true;
        }
    }

    /* Stash and restore these four, so that the scroll_func
     * can do whatever it likes without destroying the state */
    struct frame_buffer_t *framebuf = s->vp->buffer;
    unsigned drawmode;
#if LCD_DEPTH > 1
    unsigned fg_pattern, bg_pattern;
    fg_pattern = s->vp->fg_pattern;
    bg_pattern = s->vp->bg_pattern;
#endif
    drawmode   = s->vp->drawmode;
    s->scroll_func(s);

    LCDFN(update_viewport_rect)(s->x, s->y, s->width, s->height);

#if LCD_DEPTH > 1
    s->vp->fg_pattern = fg_pattern;
    s->vp->bg_pattern = bg_pattern;
#endif
    s->vp->drawmode = drawmode;
    s->vp->buffer = framebuf;

    return ended;
}

static void LCDFN(scroll_worker)(void)
{
    int index;
    struct scroll_screen_info *si = &LCDFN(scroll_info);
    struct viewport *oldvp;

    if (global_settings.disable_mainmenu_scrolling
        && get_current_activity() == ACTIVITY_MAINMENU) {
        /* No scrolling on the main menu if disabled
         (to not break themes with lockscreens) */
        return;
    }

    for ( index = 0; index < si->lines; index++ )
    {
        struct scrollinfo *s = &si->scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick)) {
            continue;
        }

        s->start_tick = current_tick;

        if (s->backward)
            s->offset -= si->step;
        else
            s->offset += si->step;

        /* this runs out of the ui thread, thus we need to
         * save and restore the current viewport since the
         * ui thread is unaware of the swapped viewports. */
        oldvp = LCDFN(set_viewport_ex)(s->vp, 0); /* don't mark the last vp as dirty */

        /* put the line onto the display now */
        bool makedelay = LCDFN(scroll_now(s));
        if (makedelay)
        {
#if 0 /* haven't found a case in which this matters .. yet?? --Bilgus */
            /* re-calculate stringsize in case the font changed */
            s->line_stringsize = font_getstringsize(s->linebuffer, NULL, NULL, s->vp->font);
#endif
            s->start_tick += si->delay + si->ticks;
        }

#ifdef SIMULATOR /* Bugfix sim won't update screen unless called from active thread */
        LCDFN(set_viewport)(oldvp);
#else
        LCDFN(set_viewport_ex)(oldvp, 0); /* don't mark the last vp as dirty */
#endif
    }
}
#endif /*!BOOTLOADER*/
