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
#include "stdio.h"
#include "kernel.h"
#include "screen_access.h"


#ifdef HAVE_LCD_BITMAP

#define SPACE 3 /* pixels between words */
#define MAXLETTERS 128 /* 16*8 */
#define MAXLINES 10

#else

#define SPACE 1 /* one letter space */
#define MAXLETTERS 22 /* 11 * 2 */
#define MAXLINES 2

#endif

static void splash(struct screen * screen,
                   bool center,  const char *fmt, va_list ap)
{
    char *next;
    char *store=NULL;
    int x=0;
    int y=0;
    int w, h;
    unsigned char splash_buf[MAXLETTERS];
    unsigned char widths[MAXLINES];
    int line=0;
    bool first=true;
#ifdef HAVE_LCD_BITMAP
    int maxw=0;
#endif

#ifdef HAVE_LCD_CHARCELLS
    screen->double_height (false);
#endif
    vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );

    if(center) {
        /* first a pass to measure sizes */
        next = strtok_r(splash_buf, " ", &store);
        while (next) {
#ifdef HAVE_LCD_BITMAP
            screen->getstringsize(next, &w, &h);
#else
            w = strlen(next);
            h = 1; /* store height in characters */
#endif
            if(!first) {
                if(x+w> screen->width) {
                    /* Too wide, wrap */
                    y+=h;
                    line++;
                    if((y > (screen->height-h)) || (line > screen->nb_lines))
                        /* STOP */
                        break;
                    x=0;
                    first=true;
                }
            }
            else
                first = false;

            /* think of it as if the text was written here at position x,y
               being w pixels/chars wide and h high */

            x += w+SPACE;
            widths[line]=x-SPACE; /* don't count the trailing space */
#ifdef HAVE_LCD_BITMAP
            /* store the widest line */
            if(widths[line]>maxw)
                maxw = widths[line];
#endif
            next = strtok_r(NULL, " ", &store);
        }

#ifdef HAVE_LCD_BITMAP
        /* Start displaying the message at position y. The reason for the
           added h here is that it isn't added until the end of lines in the
           loop above and we always break the loop in the middle of a line. */
        y = (screen->height - (y+h) )/2;
#else
        y = 0; /* vertical center on 2 lines would be silly */
#endif
        first=true;

        /* Now recreate the string again since the strtok_r() above has ruined
           the one we already have! Here's room for improvements! */
        vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );
    }
    va_end( ap );

    if(center)
    {
        x = (screen->width-widths[0])/2;
        if(x < 0)
            x = 0;
    }

#ifdef HAVE_LCD_BITMAP
    /* If we center the display, then just clear the box we need and put
       a nice little frame and put the text in there! */
    if(center && (y > 2)) {
        int xx = (screen->width-maxw)/2 - 2;
        /* The new graphics routines handle clipping, so no need to check */
#if LCD_DEPTH > 1
        if(screen->depth>1)
            screen->set_background(LCD_LIGHTGRAY);
#endif
        screen->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        screen->fillrect(xx, y-2, maxw+4, screen->height-y*2+4);
        screen->set_drawmode(DRMODE_SOLID);
        screen->drawrect(xx, y-2, maxw+4, screen->height-y*2+4);
    }
    else
#endif
        screen->clear_display();
    line=0;
    next = strtok_r(splash_buf, " ", &store);
    while (next) {
#ifdef HAVE_LCD_BITMAP
        screen->getstringsize(next, &w, &h);
#else
        w = strlen(next);
        h = 1;
#endif
        if(!first) {
            if(x+w> screen->width) {
                /* too wide */
                y+=h;
                line++; /* goto next line */
                first=true;
                if(y > (screen->height-h))
                    /* STOP */
                    break;
                if(center) {
                    x = (screen->width-widths[line])/2;
                    if(x < 0)
                       x = 0;
                }
                else
                    x=0;
            }
        }
        else
            first=false;
#ifdef HAVE_LCD_BITMAP
        screen->putsxy(x, y, next);
#else
        screen->puts(x, y, next);
#endif
        x += w+SPACE; /*  pixels space! */
        next = strtok_r(NULL, " ", &store);
    }

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH > 1)
    if(screen->depth > 1)
        screen->set_background(LCD_DEFAULT_BG);
#endif
#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
    screen->update();
#endif
}

void gui_splash(struct screen * screen, int ticks,
                    bool center,  const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    splash(screen, center, fmt, ap);
    va_end( ap );

    if(ticks)
        sleep(ticks);
}

void gui_syncsplash(int ticks, bool center,  const char *fmt, ...)
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
