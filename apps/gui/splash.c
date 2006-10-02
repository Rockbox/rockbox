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

#define MAXLETTERS 128 /* 16*8 */
#define MAXLINES 10

#else

#define MAXLETTERS 22 /* 11 * 2 */
#define MAXLINES 2

#endif

static void splash(struct screen * screen,
                   bool center,  const char *fmt, va_list ap)
{
    char *next;
    char *store=NULL;
    int x = 0;
    int w, h;
    unsigned char splash_buf[MAXLETTERS];
    unsigned short widths[MAXLINES];
    unsigned char *text = splash_buf;
    int line = 0, i_line = 0;
    bool first=true;
#ifdef HAVE_LCD_BITMAP
    int maxw=0;
    int space;
    int y;
    screen->getstringsize(" ", &space, &y);
#if LCD_DEPTH > 1
    unsigned prevbg = LCD_DEFAULT_BG;
    unsigned prevfg = LCD_DEFAULT_FG;
#endif
#else
    screen->double_height (false);  /* HAVE_LCD_CHARCELLS */
    int space = 1;
    int y = 0;
#endif
    screen->stop_scroll();
    vsnprintf( splash_buf, sizeof(splash_buf), fmt, ap );
    va_end( ap );
    
    /* measure sizes & concatenates tokenise words into lines. */
    next = strtok_r(splash_buf, " ", &store);
    while (next) {
#ifdef HAVE_LCD_BITMAP
        screen->getstringsize(next, &w, &h);
#else
        w = strlen(next);
        h = 1; /* store height in characters */
#endif
        if(!first) {
            if(x + w > screen->width) { /* Too wide, wrap */
                y += h;
                line++;
                x = 0;
                if((y > (screen->height-h)) || (line > screen->nb_lines))
                    /* STOP */
                    break;
            }
            else
                next[-1] = ' ';  /* re-concatenate string */
        }
        else
            first = false;

        x += w + space;
        widths[line] = x - space; /* don't count the trailing space */
#ifdef HAVE_LCD_BITMAP
        /* store the widest line */
        if(widths[line]>maxw)
            maxw = widths[line];
#endif
        next = strtok_r(NULL, " ", &store);
    }

    if(center) {
#ifdef HAVE_LCD_BITMAP
    /* Start displaying the message at position y. */
        y = (screen->height - y)/2;
#else
        y = 0; /* vertical center on 2 lines would be silly */
#endif
    } else
        y = 0;

    /* If we center the display, then just clear the box we need and put
       a nice little frame and put the text in there! */
#ifdef HAVE_LCD_BITMAP
    if(center && (y > 2)) {
        int xx = (screen->width - maxw)/2 - 2;
        /* The new graphics routines handle clipping, so no need to check */
#if LCD_DEPTH > 1
        if(screen->depth>1) {
            prevbg = screen->get_background();
            prevfg = screen->get_foreground();
            screen->set_background(LCD_LIGHTGRAY);
            screen->set_foreground(LCD_BLACK);
        }
#endif
        screen->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        screen->fillrect(xx, y-2, maxw+4, screen->height-y*2+4);
        screen->set_drawmode(DRMODE_SOLID);
        screen->drawrect(xx, y-2, maxw+4, screen->height-y*2+4);
    }
    else
#endif
        screen->clear_display();

        /* print the message to screen */
        while(line-- >= 0) {
            if (center) {
                x = (screen->width-widths[i_line++])/2;
                if(x < 0)
                    x = 0;
            } else
                x = 0;
#ifdef HAVE_LCD_BITMAP
            screen->putsxy(x, y, text);
#else
            screen->puts(x, y, text);
#endif
            text += strlen(text) + 1;
            y +=h;
        }

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH > 1)
    if(screen->depth > 1) {
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
