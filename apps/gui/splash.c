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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

static void splash(struct screen * screen,  bool center,  
                   const char *fmt, va_list ap)
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
    unsigned prevbg = 0;
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
    if (center)
    {
        y = (screen->height - y) / 2;  /* height => y start position */
        x = (screen->width - maxw) / 2 - 2;
#if LCD_DEPTH > 1
        if (screen->depth > 1)
        {
            prevbg = screen->get_background();
            prevfg = screen->get_foreground();
            screen->set_background(LCD_LIGHTGRAY);
            screen->set_foreground(LCD_BLACK);
        }
#endif
        screen->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        screen->fillrect(x, y-2, maxw+4, screen->height-y*2+4);
        screen->set_drawmode(DRMODE_SOLID);
        screen->drawrect(x, y-2, maxw+4, screen->height-y*2+4);
    }
    else
    {
        y = 0;
        x = 0;
        screen->clear_display();
    }
#else /* HAVE_LCD_CHARCELLS */
    y = 0;    /* vertical center on 2 lines would be silly */
    x = 0;
    screen->clear_display();
#endif

    /* print the message to screen */

    for (i = 0; i <= line; i++)
    {
        if (center)
            x = MAX((screen->width - widths[i]) / 2, 0);

#ifdef HAVE_LCD_BITMAP
        screen->putsxy(x, y, lines[i]);
#else
        screen->puts(x, y, lines[i]);
#endif
        y += h;
    }

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH > 1)
    if (center && (screen->depth > 1))
    {
        screen->set_background(prevbg);
        screen->set_foreground(prevfg);
    }
#endif
#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
    screen->update();
#endif
}

void gui_splash(struct screen * screen, int ticks,
                    bool center,  const unsigned char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    splash(screen, center, fmt, ap);
    va_end( ap );

    if(ticks)
        sleep(ticks);
}

void gui_syncsplash(int ticks, bool center,  const unsigned char *fmt, ...)
{
    va_list ap;
    int i;
    va_start( ap, fmt );
    FOR_NB_SCREENS(i)
        splash(&(screens[i]), center, fmt, ap);
    va_end( ap );

    if(ticks)
        sleep(ticks);
}
