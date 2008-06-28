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

#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

#ifdef HAVE_LCD_BITMAP

#define MAXLINES  (LCD_HEIGHT/6)
#define MAXBUFFER 512

#else /* HAVE_LCD_CHARCELLS */

#define MAXLINES  2
#define MAXBUFFER 64

#endif

static void splash(struct screen * screen, const char *fmt, va_list ap)
{
    char splash_buf[MAXBUFFER];
    short widths[MAXLINES];
    char *lines[MAXLINES];

    char *next;
    char *lastbreak = NULL;
    char *store = NULL;
    int line = 0;
    int x = 0;
    int y, i;
    int space_w, w, h;
#ifdef HAVE_LCD_BITMAP
    int maxw = 0;
#if LCD_DEPTH > 1
    unsigned prevfg = 0;
#endif

    screen->getstringsize(" ", &space_w, &h);
#else /* HAVE_LCD_CHARCELLS */

    space_w = h = 1;
    screen->double_height (false);
#endif
    y = h;

    vsnprintf(splash_buf, sizeof(splash_buf), fmt, ap);
    va_end(ap);

    /* break splash string into display lines, doing proper word wrap */

    next = strtok_r(splash_buf, " ", &store);
    if (!next)
        return;  /* nothing to display */

    lines[0] = next;
    while (true)
    {
#ifdef HAVE_LCD_BITMAP
        screen->getstringsize(next, &w, NULL);
#else
        w = utf8length(next);
#endif
        if (lastbreak)
        {
            if (x + (next - lastbreak) * space_w + w > screen->width)
            {   /* too wide, wrap */
                widths[line] = x;
#ifdef HAVE_LCD_BITMAP
                if (x > maxw)
                    maxw = x;
#endif
                if ((y + h > screen->height) || (line >= (MAXLINES-1)))
                    break;  /* screen full or out of lines */
                x = 0;
                y += h;
                lines[++line] = next;
            }
            else
            {
                /*  restore & calculate spacing */
                *lastbreak = ' ';
                x += (next - lastbreak) * space_w;
            }
        }
        x += w;
        lastbreak = next + strlen(next);
        next = strtok_r(NULL, " ", &store);
        if (!next)
        {   /* no more words */
            widths[line] = x;
#ifdef HAVE_LCD_BITMAP
            if (x > maxw)
                maxw = x;
#endif
            break;
        }
    }

    /* prepare screen */

    screen->stop_scroll();

#ifdef HAVE_LCD_BITMAP
    /* If we center the display, then just clear the box we need and put
       a nice little frame and put the text in there! */
    y = (screen->height - y) / 2;  /* height => y start position */
    x = (screen->width - maxw) / 2 - 2;

#if LCD_DEPTH > 1
    if (screen->depth > 1)
    {
        prevfg = screen->get_foreground();
        screen->set_drawmode(DRMODE_FG);
        screen->set_foreground(
            SCREEN_COLOR_TO_NATIVE(screen, LCD_LIGHTGRAY));
    }
    else
#endif
        screen->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);

    screen->fillrect(x, y-2, maxw+4, screen->height-y*2+4);

#if LCD_DEPTH > 1
    if (screen->depth > 1)
        screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, LCD_BLACK));
    else
#endif
        screen->set_drawmode(DRMODE_SOLID);

    screen->drawrect(x, y-2, maxw+4, screen->height-y*2+4);
#else /* HAVE_LCD_CHARCELLS */
    y = 0;    /* vertical centering on 2 lines would be silly */
    x = 0;
    screen->clear_display();
#endif

    /* print the message to screen */

    for (i = 0; i <= line; i++)
    {
        x = MAX((screen->width - widths[i]) / 2, 0);

#ifdef HAVE_LCD_BITMAP
        screen->putsxy(x, y, lines[i]);
#else
        screen->puts(x, y, lines[i]);
#endif
        y += h;
    }

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH > 1)
    if (screen->depth > 1)
    {
        screen->set_foreground(prevfg);
        screen->set_drawmode(DRMODE_SOLID);
    }
#endif
    screen->update();
}

void gui_splash(struct screen * screen, int ticks, 
                const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    splash(screen, fmt, ap);
    va_end( ap );

    if(ticks)
        sleep(ticks);
}

void gui_syncsplash(int ticks, const char *fmt, ...)
{
    va_list ap;
    int i;
#if !defined(SIMULATOR) || CONFIG_CODEC == SWCODEC
    long id;
    /* fmt may be a so called virtual pointer. See settings.h. */
    if((id = P2ID((unsigned char *)fmt)) >= 0)
        /* If fmt specifies a voicefont ID, and voice menus are
           enabled, then speak it. */
        cond_talk_ids_fq(id);
#endif
    /* If fmt is a lang ID then get the corresponding string (which
       still might contain % place holders). */
    fmt = P2STR((unsigned char *)fmt);
    va_start( ap, fmt );
    FOR_NB_SCREENS(i)
    {
        screens[i].set_viewport(NULL);
        splash(&(screens[i]), fmt, ap);
    }
    va_end( ap );

    if(ticks)
        sleep(ticks);
}
