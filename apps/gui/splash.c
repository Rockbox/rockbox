/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Daniel Stenberg (2002)
 * Copyright (C) William Wilgus (2026)
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
#include "stdarg.h"
#include "string.h"
#include "rbunicode.h"
#include "stdio.h"
#include "kernel.h"
#include "screen_access.h"
#include "lang.h"
#include "settings.h"
#include "talk.h"
#include "splash.h"
#include "viewport.h"
#include "strptokspn_r.h"
#include "scrollbar.h"
#include "font.h"
#ifndef BOOTLOADER
#include "misc.h" /* get_current_activity */
#endif

static long progress_next_tick, talked_tick;

#define MAXLINES  (LCD_HEIGHT/6)

#if MEMORYSIZE > 8
    #define MAXBUFFER MAX(1024, (LCD_WIDTH/SYSFONT_WIDTH) * MAXLINES + 1)
#else
    #define MAXBUFFER MAX(512, (LCD_WIDTH/SYSFONT_WIDTH) * MAXLINES + 1)
#endif

#define RECT_SPACING 3
#define SPLASH_MEMORY_INTERVAL (HZ)

static bool splash_internal(struct screen * screen, const char *fmt, va_list ap,
                            struct viewport *vp, int addl_lines)
{
    static const char matchstr[] = "\r\n\f\v\t";
    static int max_width[NB_SCREENS] = {0};
    static int max_height[NB_SCREENS] = {0};
    static char splash_buf[MAXBUFFER] = {0};

    struct splash_lines {
        const char *str;
        uint16_t x;
        uint16_t len;
    } lines[MAXLINES];
    memset(lines, 0, sizeof(lines));

#ifndef BOOTLOADER
    static enum current_activity last_act = ACTIVITY_UNKNOWN;
    enum current_activity act = get_current_activity();

    if (last_act != act) /* changed activities reset width / height */
    {
        FOR_NB_SCREENS(i)
        {
            max_width[i] = 0;
            max_height[i] = 0;
        }
        last_act = act;
    }
#endif

    char *buf = splash_buf;
    const char *next, *store;

    bool has_tabs = false;
    int line = 0;
    int x = 0, w = 0;
    int y;
    int space_w, chr_h;
    int width = vp->width - RECT_SPACING*2;
    int height = vp->height;

    /* prevent screen artifacts by keeping the max width / height seen */
    int maxw = max_width[screen->screen_type];
    int maxh = max_height[screen->screen_type];
    int fontnum = vp->font;

    size_t next_len;

    font_getstringsize(" ", &space_w, &chr_h, fontnum);
    y = (addl_lines * chr_h);

    int res = vsnprintf(splash_buf, sizeof(splash_buf), fmt, ap);
    va_end(ap);

    if (res <= 0 || width < space_w || height < chr_h)
    {
#ifdef SIMULATOR
        if (res <= 0)
            printf("ERROR: %s %d\n", __func__, res);
        else
            printf("WARNING: %s vp too small\n", __func__);
#endif
        return false; /* nothing to display */
    }

    const char *lastbreak = splash_buf;
    if (*lastbreak == '\f') /* only as first character - Reset splash box size */
    {
        maxw = 0;
        maxh = 0;
    }

    while(true) /* break splash string into display lines, doing proper word wrap */
    {
        while (*lastbreak != '\0') /* handle escape chars*/
        {
            switch (*lastbreak) /* all chars in matchstr */
            {
                case '\t': /*fallthrough*/
                    has_tabs = true; /* left justify (right justify for RTL)*/
                case '\n':
                {
                    if (lines[line].len > 0 || *lastbreak != '\t')
                    {
                        x += w;
                        if (x > maxw)
                            maxw = x;

                        x = 0;
                        y += chr_h;
                        if (y >= height || line >= MAXLINES-1)
                        {
                            break;  /* out of lines */
                        }
                        line++;
                    }
                    if (*lastbreak == '\t')
                    {
                        /* add offset for tab */
                        lines[line].x += space_w * 2;
                        x += space_w * 4;
                    }
                    break;
                }
                case '\f': /*fallthrough*/
                case '\v': /*fallthrough*/
                case '\r':
                    break; /* acts the same as a space character */
                default: /* No valid break chars */
                    lastbreak = " "; /* exit */
                    break;
            }
            lastbreak++;
        }

        if (lines[line].len > 0)
        {
            x += w;
            if (x > maxw)
                maxw = x;
            x = 0;
            y += chr_h;
            if (y >= height || line >= MAXLINES-1)
            {
                if (y > maxh)
                    maxh = y;
                break;  /* out of lines */
            }
            line++;
        }

        next = strptokspn_r(buf, matchstr, &next_len, &store);

        if (!next)
        {
            x += w;
            if (x > maxw)
                maxw = x;
            if (y > maxh)
                maxh = y;
            break;
        }
        buf = NULL; /* uses store for continuation */

        lines[line].str = next;
        lines[line].len = next_len;

        w = font_getstringnsize(next, next_len, NULL, NULL, fontnum);
        if (w > width)
        {
            const char *nxp, *nx = next;
            int nw, newlen;
            int oldlen = next_len;

            while (nx - next < oldlen) /* try to split at a space char */
            {
                nxp = nx;
                nx++;
                if (*nxp != ' ' && *nx != '\0') /* split on space or EOL */
                    continue;
                newlen = nxp - next;
                nw = font_getstringnsize(next, newlen, NULL, NULL, fontnum);

                if (nw > width)
                {
                    /* is next word larger than max width & room left on this line? */
                    if (nw - w > width && w + space_w * 8 < width)
                        w = nw;
                    break;
                }
                w = nw;
                next_len = newlen;
                store = nx; /* we want to skip the space char */
            }
            if (w > width) /* split when it fits */
            {
                w = width;
                next_len = font_measurestring(next, oldlen, &w, fontnum);
                store = next + next_len;
            }

            lines[line].len = next_len;
        }
        lastbreak = next + next_len;
    }

    /* prepare viewport
     * First boundaries, then the background filling, then the border and finally
     * the text*/

    screen->scroll_stop();

    width = maxw + 2*RECT_SPACING;
    height = maxh + 2*RECT_SPACING;

    if (width > vp->width)
        width = vp->width;
    if (height > vp->height)
        height = vp->height;

    /* center the vp in the screen area */
    vp->x += (vp->width - width) / 2;
    vp->y += (vp->height - height) / 2;
    vp->width = width;
    vp->height = height;
    if (!has_tabs)
        vp->flags |=  VP_FLAG_ALIGN_CENTER;

    /* prevent artifacts by locking to max width & height observed on repeated calls */
    max_width[screen->screen_type] = width - 2*RECT_SPACING;
    max_height[screen->screen_type] = height - 2*RECT_SPACING;

#if LCD_DEPTH > 1
    unsigned fg = 0, bg = 0;
    bool broken = false;

    if (screen->depth > 1)
    {
        fg = screen->get_foreground();
        bg = screen->get_background();

        broken = (fg == bg) ||
                 (bg == 63422 && fg == 65535); /* -> iPod reFresh themes from '22 */

        vp->drawmode = DRMODE_FG;
        /* can't do vp->fg_pattern here, since set_foreground does a bit more on
         * greyscale */
        screen->set_foreground(broken ? SCREEN_COLOR_TO_NATIVE(screen, LCD_LIGHTGRAY) :
                               bg);     /* gray as fallback for broken themes */
    }
    else
#endif
        vp->drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);

    screen->fill_viewport();

#if LCD_DEPTH > 1
    if (screen->depth > 1)
        /* can't do vp->fg_pattern here, since set_foreground does a bit more on
         * greyscale */
        screen->set_foreground(broken ? SCREEN_COLOR_TO_NATIVE(screen, LCD_BLACK) :
                               fg);     /* black as fallback for broken themes */
    else
#endif
        vp->drawmode = DRMODE_SOLID;

    screen->draw_border_viewport();

    /* center y within the available splash box */
    if (y < maxh)
        y = (maxh - y) / 2;
    else
        y = RECT_SPACING;

    /* print the message to screen */
    for(int i = 0; i < line && y < height; i++, y+= chr_h)
    {
        if (lines[i].len > 0)
        {
            x = lines[i].x;
            screen->putsxyf(x, y, "%.*s", lines[i].len, lines[i].str);
        }
    }

    return true; /* needs update */
}

void splashf(int ticks, const char *fmt, ...)
{
    va_list ap;

    /* fmt may be a so called virtual pointer. See settings.h. */
    long id;
    if((id = P2ID((const unsigned char*)fmt)) >= 0)
        /* If fmt specifies a voicefont ID, and voice menus are
           enabled, then speak it. */
        cond_talk_ids_fq(id);

    /* If fmt is a lang ID then get the corresponding string (which
       still might contain % place holders). */
    fmt = P2STR((unsigned char *)fmt);
    FOR_NB_SCREENS(i)
    {
        struct screen * screen = &(screens[i]);
        struct viewport vp;
        viewport_set_defaults(&vp, screen->screen_type);
        struct viewport *last_vp = screen->set_viewport(&vp);

        va_start(ap, fmt);
        if (splash_internal(screen, fmt, ap, &vp, 0))
            screen->update_viewport();
        va_end(ap);

        screen->set_viewport(last_vp);
    }
    if (ticks)
        sleep(ticks);
}

/* set delay before progress meter is shown */
void splash_progress_set_delay(long delay_ticks)
{
    progress_next_tick = current_tick + delay_ticks;
    talked_tick = 0;
}

/* splash a progress meter */
void splash_progress(int current, int total, const char *fmt, ...)
{
    va_list ap;
    int vp_flag = VP_FLAG_VP_DIRTY;
    /* progress update tick */
    long now = current_tick;

    if (current < total)
    {
        if(TIME_BEFORE(now, progress_next_tick))
            return;
        /* limit to 20fps */
        progress_next_tick = now + HZ/20;
        vp_flag = 0; /* don't mark vp dirty to prevent flashing */
    }

    if (global_settings.talk_menu &&
        total > 0 &&
        TIME_AFTER(current_tick, talked_tick + HZ*5))
    {
        talked_tick = current_tick;
        talk_ids(false, LANG_LOADING_PERCENT,
                 TALK_ID(current * 100 / total, UNIT_PERCENT));
    }

    /* If fmt is a lang ID then get the corresponding string (which
       still might contain % place holders). */
    fmt = P2STR((unsigned char *)fmt);
    FOR_NB_SCREENS(i)
    {
        struct screen * screen = &(screens[i]);
        struct viewport vp;
        viewport_set_defaults(&vp, screen->screen_type);
        struct viewport *last_vp = screen->set_viewport_ex(&vp, vp_flag);

        va_start(ap, fmt);
        if (splash_internal(screen, fmt, ap, &vp, 1))
        {
            int size = screen->getcharheight();
            int x = RECT_SPACING;
            int y = vp.height - size - RECT_SPACING;
            int w = vp.width - RECT_SPACING * 2;
            int h = size;
#ifdef HAVE_LCD_COLOR
            const int sb_flags = HORIZONTAL | FOREGROUND;
#else
            const int sb_flags = HORIZONTAL;
#endif
            gui_scrollbar_draw(screen, x, y, w, h, total, 0, current, sb_flags);

            screen->update_viewport();
        }
        va_end(ap);

        screen->set_viewport(last_vp);
    }
}
