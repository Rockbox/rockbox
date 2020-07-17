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
#include "viewport.h"
#include "debug.h"

#ifdef HAVE_REMOTE_LCD
#define MAX_LINES (LCD_SCROLLABLE_LINES + LCD_REMOTE_SCROLLABLE_LINES)
#else
#define MAX_LINES  LCD_SCROLLABLE_LINES
#endif


static void style_line(struct screen *display, int x, int y, struct line_desc *line);

static void put_text(struct screen *display, int x, int y, struct line_desc *line,
                      const char *text, bool prevent_scroll, int text_skip_pixels);

struct line_desc_scroll {
    struct line_desc desc;
    bool used;
};

/* Allocate MAX_LINES+1 because at the time get_line_desc() is called
 * the scroll engine did not yet determine that it ran out of lines
 * (because puts_scroll_func() wasn't called yet. Therefore we can
 * run out of lines before setting the used field. By allocating
 * one item more we can survive that point and set used to false
 * if the scroll engine runs out of lines */
static struct line_desc_scroll lines[MAX_LINES+1];

static struct line_desc_scroll *get_line_desc(void)
{
    static unsigned line_index;
    struct line_desc_scroll *this;

    do
    {
        this = &lines[line_index++];
        if (line_index >= ARRAYLEN(lines))
            line_index = 0;
    } while (this->used);

    return this;
}

static void scroller(struct scrollinfo *s, struct screen *display)
{
    /* style_line() expects the entire line rect, including padding, to
     * draw selector properly across the text+padding. however struct scrollinfo
     * has only the rect for the text itself, which is off depending on the
     * line padding. this needs to be corrected for calling style_line().
     * The alternative would be to really redraw only the text area,
     * but that would complicate the code a lot */ 
    struct line_desc_scroll *line = s->userdata;
    if (!s->line)
    {
        line->used = false;
    }
    else
    {
        style_line(display, s->x, s->y - (line->desc.height/2 - display->getcharheight()/2), &line->desc);
        put_text(display, s->x, s->y, &line->desc, s->line, true, s->offset);
    }
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

static void put_icon(struct screen *display, int x, int y,
                     struct line_desc *line,
                     enum themable_icons icon)
{
    unsigned drmode = DRMODE_FG;
    /* Need to change the drawmode:
     * mono icons should behave like text, inverted on the selector bar
     * native (colored) icons should be drawn as-is */
    if (get_icon_format(display->screen_type) == FORMAT_MONO && (line->style & STYLE_INVERT))
        drmode = DRMODE_SOLID | DRMODE_INVERSEVID;

    display->set_drawmode(drmode);
    screen_put_iconxy(display, x, y, icon);
}


static void put_text(struct screen *display,
                      int x, int y, struct line_desc *line,
                      const char *text, bool prevent_scroll,
                      int text_skip_pixels)
{
    /* set drawmode because put_icon() might have changed it */
    unsigned drmode = DRMODE_FG;
    if (line->style & STYLE_INVERT)
        drmode = DRMODE_SOLID | DRMODE_INVERSEVID;

    display->set_drawmode(drmode);

    if (line->scroll && !prevent_scroll)
    {
        bool scrolls;
        struct line_desc_scroll *line_data = get_line_desc();
        line_data->desc = *line;
        /* precalculate to avoid doing it in the scroller, it's save to
         * do this on the copy of the original line_desc*/
        if (line_data->desc.height == -1)
            line_data->desc.height = display->getcharheight();
        scrolls = display->putsxy_scroll_func(x, y, text,
            scrollers[display->screen_type], line_data, text_skip_pixels);
        line_data->used = scrolls;
    }
    else
        display->putsxy_scroll_func(x, y, text, NULL, NULL, text_skip_pixels);
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
    bool num_is_valid;
    int ch, num, height;
    int xpos = x;
    int icon_y, icon_h, icon_w;
    enum themable_icons icon;
    char tempbuf[MAX_PATH+32];
    unsigned int tempbuf_idx;
    int max_width = display->getwidth();

    height = line->height == -1 ? display->getcharheight() : line->height;
    icon_h = get_icon_height(display->screen_type);
    icon_w = get_icon_width(display->screen_type);
    tempbuf_idx = 0;
    /* For the icon use a differnet calculation which to round down instead.
     * This tends to align better with the font's baseline for small heights.
     * A proper aligorithm would take the baseline into account but this has
     * worked sufficiently well for us (fix this if needed) */
    icon_y = y + (height - icon_h)/2;
    /* vertically center string on the line
     * x/2 - y/2 rounds up compared to (x-y)/2 if one of x and y is odd */
    y += height/2 - display->getcharheight()/2;

    /* parse format string */
    while (xpos < max_width)
    {
        ch = *fmt++;
        /* need to check for escaped '$' */
        if (ch == '$' && *fmt != '$')
        {
            /* extra flag as num == 0 can be valid */
            num_is_valid = false;
            num = 0;
            if (tempbuf_idx)
            {   /* flush pending inline text */
                tempbuf_idx = tempbuf[tempbuf_idx] = 0;
                put_text(display, xpos, y, line, tempbuf, false, 0);
                xpos += display->getstringsize(tempbuf, NULL, NULL);
                if (xpos >= max_width)
                    return;
            }
next:
            ch = *fmt++;
            switch(ch)
            {
                case '*': /* num from parameter list */
                    num = va_arg(ap, int);
                    num_is_valid = true;
                    goto next;

                case 'i': /* icon (without pad) */
                case 'I': /* icon with pad */
                    if (ch == 'i')
                        num = 0;
                    else /* 'I' */
                        if (!num_is_valid)
                            num = 1;
                    icon = va_arg(ap, int);
                    /* draw it, then skip over */
                    if (icon != Icon_NOICON)
                        put_icon(display, xpos + num, icon_y, line, icon);
                    xpos += icon_w + num*2;
                    break;

                case 'S':
                    if (!num_is_valid)
                        num = 1;
                    xpos += num * display->getcharwidth();
                    break;

                case 's':
                    if (!num_is_valid)
                        num = 1;
                    xpos += num;
                    break;

                case 't':
                    str = va_arg(ap, const char *);
                    put_text(display, xpos, y, line, str, false, num);
                    xpos += display->getstringsize(str, NULL, NULL);
                    break;

                default:
                    if (LIKELY(isdigit(ch)))
                    {
                        num_is_valid = true;
                        num = 10*num + ch - '0';
                        goto next;
                    }
                    else
                    {
                        /* any other character here is an erroneous format string */
                        snprintf(tempbuf, sizeof(tempbuf), "<E:%c>", ch);
                        display->putsxy(xpos, y, tempbuf);
                        /* Don't consider going forward, fix the caller */
                        return;
                    }
            }
        }
        else
        {   /* handle string constant in format string */
            if (tempbuf_idx < sizeof(tempbuf)-1)
            {
                tempbuf[tempbuf_idx++] = ch;
            }
            else if (tempbuf_idx == sizeof(tempbuf)-1)
            {
                tempbuf[tempbuf_idx++] = '\0';
                DEBUGF("%s ", ch ? "put_line: String truncated" : "");
            }
            if (!ch)
            {   /* end of format string. flush pending inline string, if any */
                if (tempbuf[0])
                    put_text(display, xpos, y, line, tempbuf, false, 0);
                return;
            }
            else if (ch == '$')
                fmt++; /* escaped '$', display just once */
        }
    }
}

static void style_line(struct screen *display,
                       int x, int y, struct line_desc *line)
{
    int style = line->style;
    int width = display->getwidth();
    int height = line->height == -1 ? display->getcharheight() : line->height;
    int bar_height = height;

    /* mask out gradient and colorbar styles for non-color displays */
    if (display->depth < 16 && (style & (STYLE_COLORBAR|STYLE_GRADIENT)))
    {
        style &= ~(STYLE_COLORBAR|STYLE_GRADIENT);
        style |= STYLE_INVERT;
    }

    if (line->separator_height > 0 && (line->line == line->nlines-1))
    {
        int sep_height = MIN(line->separator_height, height);
        display->set_drawmode(DRMODE_FG);
#ifdef HAVE_LCD_COLOR
        display->set_foreground(global_settings.list_separator_color);
#endif
        display->fillrect(x, y + height - sep_height, width, sep_height);
        bar_height -= sep_height;
#ifdef HAVE_LCD_COLOR
        display->set_foreground(global_settings.fg_color);
#endif
    }

    /* mask out gradient and colorbar styles for non-color displays */
    if (display->depth < 16)
    {
        if (style & (STYLE_COLORBAR|STYLE_GRADIENT))
        {
            style &= ~(STYLE_COLORBAR|STYLE_GRADIENT);
            style |= STYLE_INVERT;
        }
        style &= ~STYLE_COLORED;
    }

    switch (style & _STYLE_DECO_MASK)
    {
#ifdef HAVE_LCD_COLOR
        case STYLE_GRADIENT:
            display->set_drawmode(DRMODE_FG);
            display->gradient_fillrect_part(x, y, width, bar_height,
                                            line->line_color,
                                            line->line_end_color,
                                            height*line->nlines,
                                            height*line->line);
            break;
        case STYLE_COLORBAR:
            display->set_drawmode(DRMODE_FG);
            display->set_foreground(line->line_color);
            display->fillrect(x, y, width - x, bar_height);
            break;
#endif
        case STYLE_INVERT:
            display->set_drawmode(DRMODE_FG);
            display->fillrect(x, y, width - x, bar_height);
            break;
        case STYLE_DEFAULT: default:
            display->set_drawmode(DRMODE_BG | DRMODE_INVERSEVID);
            display->fillrect(x, y, width - x, bar_height);
            break;
        case STYLE_NONE:
            break;
    }
#if (LCD_DEPTH > 1 || (defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1))
    /* prepare fg and bg colors for text drawing, be careful to not
     * override any previously set colors unless mandated by the style */
    if (display->depth > 1)
    {
        if (style & STYLE_COLORED)
        {
            if (style & STYLE_INVERT)
                display->set_background(line->text_color);
            else
                display->set_foreground(line->text_color);
        }
        else if (style & (STYLE_GRADIENT|STYLE_COLORBAR))
            display->set_foreground(line->text_color);
    }
#endif
}

void vput_line(struct screen *display,
              int x, int y, struct line_desc *line,
              const char *fmt, va_list ap)
{
#if (LCD_DEPTH > 1 || (defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1))
    /* push and pop fg and bg colors as to not compromise unrelated lines */
    unsigned fg = 0, bg = 0; /* shut up gcc */
    if (display->depth > 1 && line->style > STYLE_INVERT)
    {
        fg = display->get_foreground();
        bg = display->get_background();
    }
#endif
    style_line(display, x, y, line);
    print_line(display, x, y, line, fmt, ap);
#if (LCD_DEPTH > 1 || (defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1))
    if (display->depth > 1 && line->style > STYLE_INVERT)
    {    
        display->set_foreground(fg);
        display->set_background(bg);
    }
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
