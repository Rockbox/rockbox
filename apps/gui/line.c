/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Thomas Martitz
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

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "scroll_engine.h"
#include "system.h"
#include "line.h"
#include "gcc_extensions.h"
#include "icon.h"
#include "screens.h"
#include "settings.h"
#include "debug.h"

#ifdef HAVE_REMOTE_LCD
#define MAX_LINES (LCD_SCROLLABLE_LINES + LCD_REMOTE_SCROLLABLE_LINES)
#else
#define MAX_LINES  LCD_SCROLLABLE_LINES
#endif


static void style_line(struct screen *display, int x, int y, struct line_desc *line);


static struct line_desc *get_line_desc(void)
{
    static struct line_desc lines[MAX_LINES];
    static unsigned line_index;
    struct line_desc *ret;

    ret = &lines[line_index++];
    if (line_index >= ARRAYLEN(lines))
        line_index = 0;

    return ret;
}

static void scroller(struct scrollinfo *s, struct screen *display)
{
    /* style_line() expects the entire line rect, including padding, to
     * draw selector properly across the text+padding. however struct scrollinfo
     * has only the rect for the text itself, which is off depending on the
     * line padding. this needs to be corrected for calling style_line().
     * The alternative would be to really redraw only the text area,
     * but that would complicate the code a lot */ 
    struct line_desc *line = s->userdata;
    int y = s->y - (line->height/2 - display->getcharheight()/2);
    style_line(display, s->x, y, line);
    display->putsxy_scroll_func(s->x, s->y, s->line, NULL, NULL, s->offset);
}

static void scroller_main(struct scrollinfo *s)
{
    scroller(s, &screens[SCREEN_MAIN]);
}

#ifdef HAVE_REMOTE_LCD
static void scroller_remote(struct scrollinfo *s)
{
    scroller(s, &screens[SCREEN_REMOTE]);
}
#endif

static void (*scrollers[NB_SCREENS])(struct scrollinfo *s) = {
    scroller_main,
#ifdef HAVE_REMOTE_LCD
    scroller_remote,
#endif
};

static void put_text(struct screen *display,
                      int x, int y, struct line_desc *line,
                      const char *text)
{
    if (line->scroll)
    {
        struct line_desc *line_data = get_line_desc();
        *line_data = *line;
        /* precalculate to avoid doing it in the scroller, it's save to
         * do this on the copy of the original line_desc*/
        if (line_data->height == -1)
            line_data->height = display->getcharheight();
        display->putsxy_scroll_func(x, y, text,
            scrollers[display->screen_type], line_data, /* TODO: x_offset*/ 0);
    }
    else
        display->putsxy_scroll_func(x, y, text, NULL, NULL, /* TODO: x_offset*/ 0);
}

/* A line consists of:
 * |[Ss]|[i]|[Ss]|[t]|, where s is empty space (pixels), S is empty space
 * (n space characters), i is an icon and t is the text.
 *
 * All components are optional. However, even if none are specified the whole
 * line will be cleared and redrawn.
 *
 * For empty space with the width of an icon use i and pass Icon_NOICON as
 * corresponding argument.
 */
static void print_line(struct screen *display,
                      int x, int y, struct line_desc *line,
                      const char *fmt, va_list ap)
{
    const char *str;
    int ch, num, height;
    int xpos = x;
    int icon_y, icon_h, icon_w;
    enum themable_icons icon;
    char tempbuf[128];
    int tempbuf_idx;

    height = line->height == -1 ? display->getcharheight() : line->height;
    icon_h = get_icon_height(display->screen_type);
    icon_w = get_icon_width(display->screen_type);
    tempbuf_idx = 0;
    /* vertically center string on the line
     * x/2 - y/2 rounds up compared to (x-y)/2 if one of x and y is odd */
    icon_y = y + height/2 - icon_h/2;
    y += height/2 - display->getcharheight()/2;

    /* parse format string */
    while (1)
    {
        ch = *fmt++;
        if (ch == '$')
        {
            num = 0;
            if (tempbuf_idx)
            {   /* flush pending inline text */
                tempbuf_idx = tempbuf[tempbuf_idx] = 0;
                put_text(display, xpos, y, line, tempbuf);
                xpos += display->getstringsize(tempbuf, NULL, NULL);
            }
next:
            ch = *fmt++;
            switch(ch)
            {
                case '*': /* num from parameter list */
                    num = va_arg(ap, int);
                    goto next;

                case 'i': /* icon */
                    icon = va_arg(ap, int);
                    /* draw it, then skip over */
                    if (icon != Icon_NOICON)
                        screen_put_iconxy(display, xpos, icon_y, icon);
                    xpos += icon_w;
                    break;

                case 'S':
                    DEBUGF("skip %d chars (%d px)\n", num, display->getcharwidth());
                    xpos += MAX(num,1) * display->getcharwidth();
                    break;

                case 's':
                    xpos += MAX(num,1);
                    break;

                case 't':
                    str = va_arg(ap, const char *);
                    put_text(display, xpos, y, line, str);
                    xpos += display->getstringsize(str, NULL, NULL);
                    break;

                default:
                    if (LIKELY(isdigit(ch)))
                    {
                        num = 10*num + ch - '0';
                        goto next;
                    }
                    else
                    {
                        /* any other character here is an erroneous format string */
                        sprintf(tempbuf, "<E:%c>", ch);
                        display->putsxy(xpos, y, tempbuf);
                        /* Don't consider going forward, fix the caller */
                        return;
                    }
            }
        }
        else
        {   /* handle string constant in format string */
            tempbuf[tempbuf_idx++] = ch;
            if (!ch)
            {   /* end of string. put it online */
                put_text(display, xpos, y, line, tempbuf);
                return;
            }
        }
    }
}

static void style_line(struct screen *display,
                       int x, int y, struct line_desc *line)
{
    int style = line->style;
    int width = display->getwidth();
    int height = line->height == -1 ? display->getcharheight() : line->height;

#if (LCD_DEPTH > 1 || (defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1))
    if (style & STYLE_COLORED)
    {
        if (style & STYLE_INVERT)
            display->set_background(style & STYLE_COLOR_MASK);
        else
            display->set_foreground(style & STYLE_COLOR_MASK);
    }
#endif

    /* mask out gradient and colorbar styles for non-color displays */
    if (display->depth < 16 && (style & (STYLE_COLORBAR|STYLE_GRADIENT)))
    {
        style &= ~(STYLE_COLORBAR|STYLE_GRADIENT);
        style |= STYLE_INVERT;
    }
    
    switch (style & STYLE_MODE_MASK)
    {
        case STYLE_NONE:
            break;
#if (LCD_DEPTH > 1 || (defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1))
        case STYLE_GRADIENT:
            display->set_drawmode(DRMODE_FG);
            display->gradient_fillrect_part(x, y, width, height,
                                            global_settings.lss_color,
                                            global_settings.lse_color,
                                            height*line->nlines,
                                            height*line->line);
            display->set_foreground(global_settings.lst_color);
            break;
        case STYLE_COLORBAR:
            display->set_drawmode(DRMODE_FG);
            display->set_foreground(global_settings.lss_color);
            display->fillrect(x, y, width - x, height);
            display->set_foreground(global_settings.lst_color);
            break;
#endif
        case STYLE_INVERT:
            display->set_drawmode(DRMODE_SOLID);
            display->fillrect(x, y, width - x, height);
            display->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
            break;
        default:
            display->set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
            display->fillrect(x, y, width - x, height);
            display->set_drawmode(DRMODE_SOLID);
            break;
    }
    /* fg color, bg color and drawmode are left as-is for text drawing */
}

void vput_line(struct screen *display,
              int x, int y, struct line_desc *line,
              const char *fmt, va_list ap)
{
    style_line(display, x, y, line);
    print_line(display, x, y, line, fmt, ap);
#if (LCD_DEPTH > 1 || (defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1))
    if (display->depth > 1)
        display->set_foreground(global_settings.fg_color);
#endif
    display->set_drawmode(DRMODE_SOLID);
}

void put_line(struct screen *display,
              int x, int y, struct line_desc *line,
              const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vput_line(display, x, y, line, fmt, ap);
    va_end(ap);
}
