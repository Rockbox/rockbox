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
    struct font* pf;
    struct viewport scrollbar_vp; /* viewport for scrollbar */
    struct viewport vp;
    const char *title;
    const char *text;   /* displayed text */
    int display_lines;  /* number of lines can be displayed */
    int line_count;     /* number of lines */
    int line;           /* current first line */
    int start;          /* possition of first line in text  */
};

static bool isbrchr(const unsigned char *str, int len)
{
    const unsigned char *p = "!,-.:;?　、。！，．：；？―";
    if (isspace(*str))
        return true;

    while(*p)
    {
        int n = rb->utf8seek(p, 1);
        if (len == n && !rb->strncmp(p, str, len))
            return true;
        p += n;
    }
    return false;
}

static const char* get_next_line(const char *text, struct view_info *info)
{
    const char *ptr = text;
    const char *space = NULL;
    int total, n, w;
    total = 0;
    while(*ptr)
    {
        unsigned short ch;
        n = ((intptr_t)rb->utf8decode(ptr, &ch) - (intptr_t)ptr);
        if (rb->is_diacritic(ch, NULL))
            w = 0;
        else
            w = rb->font_get_width(info->pf, ch);
        if (isbrchr(ptr, n))
            space = ptr+(isspace(*ptr) || total + w <= info->vp.width? n: 0);
        if (*ptr == '\n')
        {
            ptr += n;
            break;
        }
        if (total + w > info->vp.width)
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
    bool scrollbar = false;

    while (*ptr)
    {
        ptr = get_next_line(ptr, info);
        i++;
        if (!scrollbar && i > info->display_lines)
        {
            ptr = info->text;
            i = 0;
            info->scrollbar_vp = info->vp;
            info->scrollbar_vp.width = rb->global_settings->scrollbar_width;
            info->vp.width -= info->scrollbar_vp.width;
            if (rb->global_settings->scrollbar != SCROLLBAR_RIGHT)
                info->vp.x = info->scrollbar_vp.width;
            else
                info->scrollbar_vp.x = info->vp.width;
            scrollbar = true;
        }
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
}

static int init_view(struct view_info *info,
                     const char *title, const char *text)
{
    rb->viewport_set_defaults(&info->vp, SCREEN_MAIN);
    info->pf = rb->font_get(rb->screens[SCREEN_MAIN]->getuifont());
    info->display_lines = info->vp.height / info->pf->height;

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
        info->vp.y += info->pf->height;
        info->vp.height -= info->pf->height;
    }

    calc_line_count(info);
    return 0;
}

static void draw_text(struct view_info *info)
{
#define OUTPUT_SIZE LCD_WIDTH+1
    static char output[OUTPUT_SIZE];
    const char *text, *ptr;
    int max_show, line;
    struct screen* display = rb->screens[SCREEN_MAIN];

    /* clear screen */
    display->clear_display();

    /* display title. */
    if(info->title)
    {
        display->set_viewport(NULL);
        display->puts(0, 0, info->title);
    }

    max_show = MIN(info->line_count - info->line, info->display_lines);
    text = info->text + info->start;

    display->set_viewport(&info->vp);
    for (line = 0; line < max_show; line++)
    {
        int len;
        ptr = get_next_line(text, info);
        len = ptr-text;
        while(len > 0 && isspace(text[len-1]))
            len--;
        rb->memcpy(output, text, len);
        output[len] = 0;
        display->puts(0, line, output);
        text = ptr;
    }
    if (info->line_count > info->display_lines)
    {
        display->set_viewport(&info->scrollbar_vp);
        rb->gui_scrollbar_draw(display, (info->scrollbar_vp.width? 0: 1), 0,
                info->scrollbar_vp.width - 1, info->scrollbar_vp.height,
                info->line_count, info->line, info->line + max_show,
                VERTICAL);
    }

    display->set_viewport(NULL);
    display->update();
}

static void scroll_up(struct view_info *info, int n)
{
    if (info->line <= 0)
        return;

    calc_first_line(info, info->line-n);
    draw_text(info);
}

static void scroll_down(struct view_info *info, int n)
{
    if (info->line + info->display_lines >= info->line_count)
        return;

    calc_first_line(info, info->line+n);
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
#ifdef HAVE_SCROLLWHEEL
        case PLA_SCROLL_BACK:
        case PLA_SCROLL_BACK_REPEAT:
#endif
            scroll_up(&info, 1);
            break;
        case PLA_DOWN:
        case PLA_DOWN_REPEAT:
#ifdef HAVE_SCROLLWHEEL
        case PLA_SCROLL_FWD:
        case PLA_SCROLL_FWD_REPEAT:
#endif
            scroll_down(&info, 1);
            break;
        case PLA_LEFT:
            scroll_up(&info, info.display_lines);
            break;
        case PLA_RIGHT:
            scroll_down(&info, info.display_lines);
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
