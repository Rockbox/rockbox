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
#include <ctype.h>

/* global settings file
 * binary file, so dont use .cfg
 *
 * setting file format
 *
 * part                byte count
 * --------------------------------
 * 'TVGS'               4
 * version              1
 * word_mode            1
 * line_mode            1
 * view_mode            1
 * encoding             1
 * scrollbar_mode       1
 * need_scrollbar       1
 * page_mode            1
 * page_number_mode     1
 * title_mode           1
 * scroll_mode          1
 * autoscroll_speed     1
 * font name            MAX_PATH
 */
#define GLOBAL_SETTINGS_FILE     VIEWERS_DIR "/viewer.dat"

/* temporary file */
#define GLOBAL_SETTINGS_TMP_FILE VIEWERS_DIR "/viewer_file.tmp"

#define GLOBAL_SETTINGS_HEADER   "\x54\x56\x47\x53\x31" /* header="TVGS" version=1 */
#define GLOBAL_SETTINGS_H_SIZE   5

/* preferences and bookmarks at each file
 * binary file, so dont use .cfg
 *
 * setting file format
 *
 * part                byte count
 * --------------------------------
 * 'TVS'                3
 * version              1
 * file count           2
 * [1st file]
 *   file path          MAX_PATH
 *   next file pos      2
 *   [preferences]
 *     word_mode        1
 *     line_mode        1
 *     view_mode        1
 *     encoding         1
 *     scrollbar_mode   1
 *     need_scrollbar   1
 *     page_mode        1
 *     header_mode      1
 *     footer_mode      1
 *     scroll_mode      1
 *     autoscroll_speed 1
 *     font name        MAX_PATH
 *   bookmark count     1
 *   [1st bookmark]
 *     file_position    4
 *     page             2
 *     line             1
 *     flag             1
 *   [2nd bookmark]
 *   ...
 *   [last bookmark]
 * [2nd file]
 * ...
 * [last file]
 */
#define SETTINGS_FILE        VIEWERS_DIR "/viewer_file.dat"

/* temporary file */
#define SETTINGS_TMP_FILE    VIEWERS_DIR "/viewer_file.tmp"

#define SETTINGS_HEADER      "\x54\x56\x53\x32" /* header="TVS" version=2 */
#define SETTINGS_H_SIZE      4

#ifndef HAVE_LCD_BITMAP
#define BOOKMARK_ICON       "\xee\x84\x81\x00"
#endif

#define PREFERENCES_SIZE    (11 + MAX_PATH)


struct preferences prefs;
struct preferences old_prefs;

static int fd;
static const char *file_name;
static int cline = 1;
static int cpage = 1;
static int lpage = 0;

static void viewer_default_preferences(void)
{
    prefs.word_mode = WRAP;
    prefs.line_mode = NORMAL;
    prefs.view_mode = NARROW;
    prefs.scroll_mode = PAGE;
    prefs.page_mode = NO_OVERLAP;
    prefs.scrollbar_mode = SB_OFF;
    rb->memset(prefs.font, 0, MAX_PATH);
#ifdef HAVE_LCD_BITMAP
    prefs.header_mode = HD_BOTH;
    prefs.footer_mode = FT_BOTH;
    rb->snprintf(prefs.font, MAX_PATH, "%s", rb->global_settings->font_file);
#else
    prefs.header_mode = HD_NONE;
    prefs.footer_mode = FT_NONE;
#endif
    prefs.autoscroll_speed = 1;
    /* Set codepage to system default */
    prefs.encoding = rb->global_settings->default_codepage;
}

static bool viewer_read_preferences(int pfd)
{
    unsigned char buf[PREFERENCES_SIZE];
    unsigned char *p = buf;

    if (rb->read(pfd, buf, sizeof(buf)) != sizeof(buf))
        return false;

    prefs.word_mode        = *p++;
    prefs.line_mode        = *p++;
    prefs.view_mode        = *p++;
    prefs.encoding         = *p++;
    prefs.scrollbar_mode   = *p++;
    prefs.need_scrollbar   = *p++;
    prefs.page_mode        = *p++;
    prefs.header_mode      = *p++;
    prefs.footer_mode      = *p++;
    prefs.scroll_mode      = *p++;
    prefs.autoscroll_speed = *p++;
    rb->memcpy(prefs.font, p, MAX_PATH);

    return true;
}

static bool viewer_write_preferences(int pfd)
{
    unsigned char buf[PREFERENCES_SIZE];
    unsigned char *p = buf;

    *p++ = prefs.word_mode;
    *p++ = prefs.line_mode;
    *p++ = prefs.view_mode;
    *p++ = prefs.encoding;
    *p++ = prefs.scrollbar_mode;
    *p++ = prefs.need_scrollbar;
    *p++ = prefs.page_mode;
    *p++ = prefs.header_mode;
    *p++ = prefs.footer_mode;
    *p++ = prefs.scroll_mode;
    *p++ = prefs.autoscroll_speed;
    rb->memcpy(p, prefs.font, MAX_PATH);

    return (rb->write(pfd, buf, sizeof(buf)) == sizeof(buf));
}

static bool viewer_load_global_settings(void)
{
    unsigned buf[GLOBAL_SETTINGS_H_SIZE];
    int sfd = rb->open(GLOBAL_SETTINGS_FILE, O_RDONLY);

    if (sfd < 0)
        return false;

    if ((rb->read(sfd, buf, GLOBAL_SETTINGS_H_SIZE) != GLOBAL_SETTINGS_H_SIZE) ||
        rb->memcmp(buf, GLOBAL_SETTINGS_HEADER, GLOBAL_SETTINGS_H_SIZE) ||
        !viewer_read_preferences(sfd))
     {
        rb->close(sfd);
        return false;
    }
    rb->close(sfd);
    return true;
}

static bool viewer_save_global_settings(void)
{
    int sfd = rb->open(GLOBAL_SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC);

    if (sfd < 0)
         return false;

    if (rb->write(sfd, &GLOBAL_SETTINGS_HEADER, GLOBAL_SETTINGS_H_SIZE)
        != GLOBAL_SETTINGS_H_SIZE ||
        !viewer_write_preferences(sfd))
    {
        rb->close(sfd);
        rb->remove(GLOBAL_SETTINGS_TMP_FILE);
        return false;
    }
    rb->close(sfd);
    rb->remove(GLOBAL_SETTINGS_FILE);
    rb->rename(GLOBAL_SETTINGS_TMP_FILE, GLOBAL_SETTINGS_FILE);
    return true;
}

void viewer_load_settings(void)
{
    unsigned char buf[MAX_PATH+2];
    unsigned int fcount;
    unsigned int i;
    bool res = false;
    int sfd;
    unsigned int size;

    sfd = rb->open(SETTINGS_FILE, O_RDONLY);
    if (sfd < 0)
        goto read_end;

    if ((rb->read(sfd, buf, SETTINGS_H_SIZE+2) != SETTINGS_H_SIZE+2) ||
        rb->memcmp(buf, SETTINGS_HEADER, SETTINGS_H_SIZE))
    {
        /* illegal setting file */
        rb->close(sfd);

        if (rb->file_exists(SETTINGS_FILE))
            rb->remove(SETTINGS_FILE);

        goto read_end;
    }

    fcount = (buf[SETTINGS_H_SIZE] << 8) | buf[SETTINGS_H_SIZE+1];
    for (i = 0; i < fcount; i++)
    {
        if (rb->read(sfd, buf, MAX_PATH+2) != MAX_PATH+2)
            break;

        size = (buf[MAX_PATH] << 8) | buf[MAX_PATH+1];
        if (rb->strcmp(buf, file_name))
        {
            if (rb->lseek(sfd, size, SEEK_CUR) < 0)
                break;
            continue;
        }
        if (!viewer_read_preferences(sfd))
            break;

        res = viewer_read_bookmark_infos(sfd);
        break;
    }

    rb->close(sfd);

read_end:
    if (!res)
    {
        /* load global settings */
        if (!viewer_load_global_settings())
            viewer_default_preferences();

        file_pos = 0;
        screen_top_ptr = buffer;
        cpage = 1;
        cline = 1;
        bookmark_count = 0;
    }

    rb->memcpy(&old_prefs, &prefs, sizeof(struct preferences));
    calc_max_width();

    if (bookmark_count > 1)
        viewer_select_bookmark(-1);
    else if (bookmark_count == 1)
    {
        int screen_pos;
        int screen_top;

        screen_pos = bookmarks[0].file_position;
        screen_top = screen_pos % buffer_size;
        file_pos = screen_pos - screen_top;
        screen_top_ptr = buffer + screen_top;
        cpage = bookmarks[0].page;
        cline = bookmarks[0].line;
    }

    viewer_remove_last_read_bookmark();

    check_bom();
    get_filesize();

    buffer_end = BUFFER_END();  /* Update whenever file_pos changes */

    if (BUFFER_OOB(screen_top_ptr))
        screen_top_ptr = buffer;

    fill_buffer(file_pos, buffer, buffer_size);
    if (prefs.scroll_mode == PAGE && cline > 1)
        viewer_scroll_to_top_line();

    /* remember the current position */
    start_position = file_pos + screen_top_ptr - buffer;

#ifdef HAVE_LCD_BITMAP
    if (rb->strcmp(prefs.font, rb->global_settings->font_file))
        change_font(prefs.font);

    init_need_scrollbar();
    init_header_and_footer();
#endif
}

bool viewer_save_settings(void)
{
    unsigned char buf[MAX_PATH+2];
    unsigned int fcount = 0;
    unsigned int i;
    int idx;
    int ofd;
    int tfd;
    off_t first_copy_size = 0;
    off_t second_copy_start_pos = 0;
    off_t size;

    /* add reading page to bookmarks */
    idx = viewer_find_bookmark(cpage, cline);
    if (idx >= 0)
        bookmarks[idx].flag |= BOOKMARK_LAST;
    else
    {
        viewer_add_bookmark(cpage, cline);
        bookmarks[bookmark_count-1].flag = BOOKMARK_LAST;
    }
  
    tfd = rb->open(SETTINGS_TMP_FILE, O_WRONLY|O_CREAT|O_TRUNC);
    if (tfd < 0)
        return false;

    ofd = rb->open(SETTINGS_FILE, O_RDWR);
    if (ofd >= 0)
    {
        if ((rb->read(ofd, buf, SETTINGS_H_SIZE+2) != SETTINGS_H_SIZE+2) ||
            rb->memcmp(buf, SETTINGS_HEADER, SETTINGS_H_SIZE))
        {
            rb->close(ofd);
            goto save_err;
        }
        fcount = (buf[SETTINGS_H_SIZE] << 8) | buf[SETTINGS_H_SIZE+1];

        for (i = 0; i < fcount; i++)
        {
            if (rb->read(ofd, buf, MAX_PATH+2) != MAX_PATH+2)
            {
                rb->close(ofd);
                goto save_err;
            }
            size = (buf[MAX_PATH] << 8) | buf[MAX_PATH+1];
            if (rb->strcmp(buf, file_name))
            {
                if (rb->lseek(ofd, size, SEEK_CUR) < 0)
                {
                    rb->close(ofd);
                    goto save_err;
                }
            }
            else
            {
                first_copy_size = rb->lseek(ofd, 0, SEEK_CUR);
                if (first_copy_size < 0)
                {
                    rb->close(ofd);
                    goto save_err;
                }
                second_copy_start_pos = first_copy_size + size;
                first_copy_size -= MAX_PATH+2;
                fcount--;
                break;
            }
        }
        if (first_copy_size == 0)
            first_copy_size = rb->filesize(ofd);

        if (!copy_bookmark_file(ofd, tfd, 0, first_copy_size))
        {
            rb->close(ofd);
            goto save_err;
        }
        if (second_copy_start_pos > 0)
        {
            if (!copy_bookmark_file(ofd, tfd, second_copy_start_pos,
                                    rb->filesize(ofd) - second_copy_start_pos))
            {
                rb->close(ofd);
                goto save_err;
            }
        }
        rb->close(ofd);
    }
    else
    {
        rb->memcpy(buf, SETTINGS_HEADER, SETTINGS_H_SIZE);
        buf[SETTINGS_H_SIZE] = 0;
        buf[SETTINGS_H_SIZE+1] = 0;
        if (rb->write(tfd, buf, SETTINGS_H_SIZE+2) != SETTINGS_H_SIZE+2)
            goto save_err;
    }

    /* copy to current read file's bookmarks */
    rb->memset(buf, 0, MAX_PATH);
    rb->snprintf(buf, MAX_PATH, "%s", file_name);

    size = PREFERENCES_SIZE + bookmark_count * BOOKMARK_SIZE + 1;
    buf[MAX_PATH] = size >> 8;
    buf[MAX_PATH+1] = size;

    if (rb->write(tfd, buf, MAX_PATH+2) != MAX_PATH+2)
        goto save_err;

    if (!viewer_write_preferences(tfd))
        goto save_err;

    if (!viewer_write_bookmark_infos(tfd))
        goto save_err;

    if (rb->lseek(tfd, SETTINGS_H_SIZE, SEEK_SET) < 0)
        goto save_err;

    fcount++;
    buf[0] = fcount >> 8;
    buf[1] = fcount;

    if (rb->write(tfd, buf, 2) != 2)
        goto save_err;

    rb->close(tfd);

    rb->remove(SETTINGS_FILE);
    rb->rename(SETTINGS_TMP_FILE, SETTINGS_FILE);

    return true;

save_err:
    rb->close(tfd);
    rb->remove(SETTINGS_TMP_FILE);
    return false;
}
