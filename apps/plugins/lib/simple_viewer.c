/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2010 Teruaki Kawashima
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

#include "config.h"
#include "plugin.h"
#include "pluginlib_actions.h"
#include "simple_viewer.h"
#include <ctype.h>

struct view_info {
#ifdef HAVE_LCD_BITMAP
    struct font* pf;
#endif
    const char *title;
    const char *text;   /* displayed text */
    int display_lines;  /* number of lines can be displayed */
    int line_count;     /* number of lines */
    int line;           /* current fisrt line */
    int start;          /* possition of first line in text  */
};

static const char* get_next_line(const char *text, struct view_info *info)
{
    (void) info;
    const char *ptr = text;
    const char *space = NULL;
    int total, n, w;
    total = 0;
    while(*ptr)
    {
#ifdef HAVE_LCD_CHARCELLS
        n = rb->utf8seek(ptr, 1);
        w = 1;
#else
        unsigned short ch;
        n = ((long)rb->utf8decode(ptr, &ch) - (long)ptr);
        w = rb->font_get_width(info->pf, ch);
#endif
        if (isspace(*ptr))
            space = ptr+n;
        if (*ptr == '\n')
        {
            ptr += n;
            break;
        }
        if (total + w > LCD_WIDTH)
            break;
        ptr += n;
        total += w;
    }
    return *ptr && space? space: ptr;
}

static void calc_line_count(struct view_info *info)
{
    const char *ptr = info->text;
    int i = 0;

    while (*ptr)
    {
        ptr = get_next_line(ptr, info);
        i++;
    }
    info->line_count = i;
}

static void calc_first_line(struct view_info *info, int line)
{
    const char *ptr = info->text;
    int i = 0;

    if (line > info->line_count - info->display_lines)
        line = info->line_count - info->display_lines;
    if (line < 0)
        line = 0;

    DEBUGF("%d -> %d\n", info->line, line);
    if (info->line <= line)
    {
        ptr += info->start;
        i = info->line;
    }
    while (*ptr && i < line)
    {
        ptr = get_next_line(ptr, info);
        i++;
    }
    info->start = ptr - info->text;
    info->line = i;
    DEBUGF("%d: %d\n", info->line, info->start);
}

static int init_view(struct view_info *info,
                     const char *title, const char *text)
{
#ifdef HAVE_LCD_BITMAP
    info->pf = rb->font_get(FONT_UI);
    info->display_lines = LCD_HEIGHT / info->pf->height;
#else

    info->display_lines = LCD_HEIGHT;
#endif

    info->title = title;
    info->text = text;
    info->line_count = 0;
    info->line = 0;
    info->start = 0;

    /* no title for small screens. */
    if (info->display_lines < 4)
    {
        info->title = NULL;
    }
    else
    {
        info->display_lines--;
    }

    calc_line_count(info);
    return 0;
}

static void draw_text(struct view_info *info)
{
#ifdef HAVE_LCD_BITMAP
#define OUTPUT_SIZE LCD_WIDTH+1
#else
#define OUTPUT_SIZE LCD_WIDTH*3+1
#endif
    static char output[OUTPUT_SIZE];
    const char *text, *ptr;
    int i, max_show, lines = 0;

    /* clear screen */
    rb->lcd_clear_display();

    /* display title. */
    if(info->title)
    {
        rb->lcd_puts(0, lines, info->title);
        lines++;
    }

    max_show = MIN(info->line_count - info->line, info->display_lines);
    DEBUGF("draw_text: %d-%d/%d\n",
           info->line, info->line + max_show, info->line_count);
    text = info->text + info->start;

    for (i = 0; i < max_show; i++, lines++)
    {
        ptr = get_next_line(text, info);
        DEBUGF("%d>%d-%d, ", i, text-info->text, ptr-text);
        rb->strlcpy(output, text, ptr-text+1);
        rb->lcd_puts(0, lines, output);
        text = ptr;
    }
    DEBUGF("\n");

    rb->lcd_update();
}

static void scroll_up(struct view_info *info)
{
    if (info->line <= 0)
        return;
    calc_first_line(info, info->line-1);
    draw_text(info);
    return;
}

static void scroll_down(struct view_info *info)
{
    if (info->line + info->display_lines >= info->line_count)
        return;

    calc_first_line(info, info->line+1);
    draw_text(info);
}

static void scroll_to_top(struct view_info *info)
{
    if (info->line <= 0)
        return;

    calc_first_line(info, 0);
    draw_text(info);
}

static void scroll_to_bottom(struct view_info *info)
{
    if (info->line + info->display_lines >= info->line_count)
        return;

    calc_first_line(info, info->line_count - info->display_lines);
    draw_text(info);
}

int view_text(const char *title, const char *text)
{
    struct view_info info;
    const struct button_mapping *view_contexts[] = {
        pla_main_ctx,
    };
    int button;

    init_view(&info, title, text);
    draw_text(&info);

    /* wait for keypress */
    while(1)
    {
        button = pluginlib_getaction(TIMEOUT_BLOCK, view_contexts,
                                     ARRAYLEN(view_contexts));
        switch (button)
        {
        case PLA_UP:
        case PLA_UP_REPEAT:
            scroll_up(&info);
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
            scroll_down(&info);
            break;
        case PLA_LEFT_REPEAT:
            scroll_to_top(&info);
            break;
        case PLA_RIGHT_REPEAT:
            scroll_to_bottom(&info);
            break;
        case PLA_EXIT:
        case PLA_CANCEL:
            return PLUGIN_OK;
        default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            break;
        }
   }

    return PLUGIN_OK;
}
