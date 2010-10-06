/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Gilles Roux
 *               2003 Garrett Derner
 *               2010 Yoshihisa Uchida
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
#include "plugin.h"
#include "tv_pager.h"
#include "tv_preferences.h"
#include "tv_reader.h"
#include "tv_window.h"

#if PLUGIN_BUFFER_SIZE < 0x13000
#define TV_MAX_PAGE 999
#else
#define TV_MAX_PAGE 9999
#endif

static unsigned char pager_buffer[4 * TV_MAX_PAGE];

static struct tv_screen_pos cur_pos;

static int parse_page;
static int last_page;
static int max_page;

static int lines_per_page;

static int line_pos[LCD_HEIGHT / 2];
static int parse_page;
static int parse_top_line;
static int parse_lines;

static void set_uint32(unsigned char *p, unsigned int val)
{
    *p++ =  val        & 0xff;
    *p++ = (val >> 8)  & 0xff;
    *p++ = (val >> 16) & 0xff;
    *p   = (val >> 24) & 0xff;
}

static unsigned int get_uint32(const unsigned char *p)
{
    return (((((p[3] << 8) | p[2]) << 8) | p[1]) << 8) | p[0];
}

static void tv_set_fpos(int page, off_t pos)
{
    if (page >= 0 && page <= max_page)
        set_uint32(pager_buffer + page * 4, pos);
}

static off_t tv_get_fpos(int page)
{
    if (page >= 0 && page <= max_page)
        return get_uint32(pager_buffer + page * 4);
    return 0;
}

static int tv_change_preferences(const struct tv_preferences *oldp)
{
    (void)oldp;

    cur_pos.page = 0;
    cur_pos.line = 0;
    last_page = 0;
    max_page  = TV_MAX_PAGE - 1;
    tv_set_fpos(cur_pos.page, 0);
    tv_seek(0, SEEK_SET);
    return TV_CALLBACK_OK;
}

bool tv_init_pager(unsigned char **buf, size_t *size)
{
    tv_set_screen_pos(&cur_pos);
    tv_add_preferences_change_listner(tv_change_preferences);

    /* valid page: 0, ..., max_page. */
    max_page = TV_MAX_PAGE - 1;

    line_pos[0] = 0;

    return tv_init_reader(buf, size);
}

void tv_finalize_pager(void)
{
    tv_finalize_reader();
}

void tv_reset_line_positions(void)
{
    parse_page = cur_pos.page;
    parse_top_line = cur_pos.line;
    parse_lines = 0;
}

void tv_move_next_line(int size)
{
    if (!tv_is_eof())
    {
        tv_seek(size, SEEK_CUR);
        cur_pos.file_pos = tv_get_current_file_pos();
        cur_pos.line++;
        parse_lines++;
        line_pos[cur_pos.line] = line_pos[cur_pos.line - 1] + size;
    }
}

int tv_get_line_positions(int offset)
{
   int line = cur_pos.line + offset;

   if (line < 0)
       line = 0;
   else if (line > parse_lines)
       line = parse_lines;

   return line_pos[parse_top_line + line] - line_pos[parse_top_line];
}

void tv_new_page(void)
{
   parse_page = cur_pos.page;
   if (cur_pos.page == last_page && last_page < max_page)
   {
       if (!tv_is_eof())
           tv_set_fpos(++last_page, tv_get_current_file_pos());
       else
           max_page = last_page;
   }

   if (++cur_pos.page > max_page)
       cur_pos.page = max_page;

   lines_per_page = cur_pos.line;
   cur_pos.line = 0;
}

static void tv_seek_page(int offset, int whence)
{
    int new_page = offset;

    switch (whence)
    {
        case SEEK_CUR:
            new_page += cur_pos.page;
            break;
        case SEEK_SET:
            break;
        case SEEK_END:
            new_page += last_page;
            whence = SEEK_SET;
            break;
        default:
            return;
            break;
    }
    if (new_page < 0)
        new_page = 0;
    else if (new_page >= last_page)
        new_page = last_page;

    cur_pos.page = new_page;
    cur_pos.line = 0;
    cur_pos.file_pos = tv_get_fpos(new_page);
    tv_seek(cur_pos.file_pos, SEEK_SET);
}

static bool tv_create_line_positions(void)
{
    bool res;

    if (tv_is_eof())
        return false;

    cur_pos.line = 0;
    tv_reset_line_positions();
    res = tv_traverse_lines();
    lines_per_page = cur_pos.line;
    tv_new_page();

    return res;
}

void tv_convert_fpos(off_t fpos, struct tv_screen_pos *pos)
{
    int i;

    for (i = 0; i < last_page; i++)
    {
        if (tv_get_fpos(i) <= fpos && tv_get_fpos(i + 1) > fpos)
            break;
    }

    pos->page     = i;
    pos->line     = 0;
    pos->file_pos = fpos;

    if (tv_get_fpos(i) == fpos)
        return;

    tv_seek_page(i, SEEK_SET);
    while (tv_create_line_positions() && cur_pos.file_pos < fpos)
        rb->splashf(0, "converting %ld%%...", 100 * cur_pos.file_pos / fpos);

    if (i < max_page)
        cur_pos.page--;
    tv_seek_page(cur_pos.page, SEEK_SET);
    for (i = 0; i < lines_per_page; i++)
    {
        if (cur_pos.file_pos + tv_get_line_positions(i) >= fpos)
            break;
    }

    pos->page = cur_pos.page;
    pos->line = i;
}

static void tv_seek_to_bottom_line(void)
{
    off_t total_size = tv_get_file_size();

    tv_seek_page(0, SEEK_END);
    while (tv_create_line_positions())
        rb->splashf(0, "loading %ld%%...", 100 * cur_pos.file_pos / total_size);

    cur_pos.line = lines_per_page - 1;
}

void tv_move_screen(int page_offset, int line_offset, int whence)
{
    struct tv_screen_pos new_pos;
    int i;

    switch (whence)
    {
        case SEEK_CUR:
             cur_pos.page += page_offset;
             cur_pos.line += line_offset;
             break;
        case SEEK_SET:
             cur_pos.page = page_offset;
             cur_pos.line = line_offset;
             break;
        case SEEK_END:
             tv_seek_to_bottom_line();
             cur_pos.page += page_offset;
             cur_pos.line += line_offset;
             break;
        default:
             return;
             break;
    }

    if (cur_pos.page < 0 || (cur_pos.page == 0 && cur_pos.line < 0))
    {
        tv_seek_page(0, SEEK_SET);
        return;
    }
    else if (cur_pos.page > max_page)
    {
        tv_seek_page(max_page, SEEK_SET);
        return;
    }

    new_pos = cur_pos;
    if (cur_pos.line < 0)
        new_pos.page--;

    tv_seek_page(new_pos.page, SEEK_SET);
    while (cur_pos.page < new_pos.page && tv_create_line_positions())
        rb->splashf(0, "loading %d%%...", 100 * cur_pos.page / new_pos.page);

    if (new_pos.line == 0)
        return;

    if (parse_page == cur_pos.page)
    {
        if (cur_pos.page < max_page && new_pos.line == lines_per_page)
        {
            tv_seek(line_pos[lines_per_page], SEEK_CUR);
            cur_pos.file_pos += line_pos[lines_per_page];

            for (i = 0; i < parse_lines; i++)
                line_pos[i] = line_pos[i + lines_per_page] - line_pos[lines_per_page];

            cur_pos.page++;
            cur_pos.line = 0;
            parse_top_line = 0;
            new_pos.line = 0;
        }
    }
    else
    {
        tv_create_line_positions();
        tv_seek_page(new_pos.page, SEEK_SET);
    }

    cur_pos.line = new_pos.line;
    if (cur_pos.line >= lines_per_page)
        cur_pos.line = lines_per_page - 1;
    else if (cur_pos.line < 0)
    {
        cur_pos.line += lines_per_page;
        if (cur_pos.line < 0)
            cur_pos.line = 0;
    }
    tv_seek(line_pos[cur_pos.line], SEEK_CUR);
    cur_pos.file_pos += line_pos[cur_pos.line];
}
