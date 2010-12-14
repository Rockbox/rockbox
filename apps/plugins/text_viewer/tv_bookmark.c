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
#include "tv_bookmark.h"
#include "tv_pager.h"
#include "tv_preferences.h"

/* text viewer bookmark functions */

enum {
    TV_BOOKMARK_SYSTEM = 1,
    TV_BOOKMARK_USER   = 2,
};


struct tv_bookmark_info {
    struct tv_screen_pos pos;
    unsigned char flag;
};

/* bookmark stored array */
struct tv_bookmark_info bookmarks[TV_MAX_BOOKMARKS + 1];
static unsigned char bookmark_count;

static int tv_compare_screen_pos(const struct tv_screen_pos *p1, const struct tv_screen_pos *p2)
{
    if (p1->page != p2->page)
        return p1->page - p2->page;

    return p1->line - p2->line;
}

static int bm_comp(const void *a, const void *b)
{
    struct tv_bookmark_info *pa;
    struct tv_bookmark_info *pb;

    pa = (struct tv_bookmark_info*)a;
    pb = (struct tv_bookmark_info*)b;

    return tv_compare_screen_pos(&pa->pos, &pb->pos);
}

static int tv_add_bookmark(const struct tv_screen_pos *pos)
{
    if (bookmark_count >= TV_MAX_BOOKMARKS)
        return -1;

    bookmarks[bookmark_count].pos  = *pos;
    bookmarks[bookmark_count].flag = TV_BOOKMARK_USER;

    return bookmark_count++;
}

static void tv_remove_bookmark(int idx)
{
    int k;

    if (idx >= 0 && idx < bookmark_count)
    {
        for (k = idx + 1; k < bookmark_count; k++)
            bookmarks[k-1] = bookmarks[k];

        bookmark_count--;
    }
}

static int tv_find_bookmark(const struct tv_screen_pos *pos)
{
    int i;

    for (i = 0; i < bookmark_count; i++)
    {
        if (tv_compare_screen_pos(&bookmarks[i].pos, pos) == 0)
            return i;
    }
    return -1;
}

static int tv_change_preferences(const struct tv_preferences *oldp)
{
    int i;

    if (oldp)
    {
        for (i = 0; i < bookmark_count; i++)
            tv_convert_fpos(bookmarks[i].pos.file_pos, &bookmarks[i].pos);
    }
    return TV_CALLBACK_OK;
}

bool tv_init_bookmark(unsigned char **buf, size_t *size)
{
    (void)buf;
    (void)size;

    tv_add_preferences_change_listner(tv_change_preferences);
    return true;
}

void tv_finalize_bookmark(void)
{
    /* no-operation function */
}

int tv_get_bookmark_positions(struct tv_screen_pos *pos_array)
{
    int i;

    for(i = 0; i < bookmark_count; i++)
        *pos_array++ = bookmarks[i].pos;

    return bookmark_count;
}

void tv_toggle_bookmark(void)
{
    const struct tv_screen_pos *pos = tv_get_screen_pos();
    int idx = tv_find_bookmark(pos);

    if (idx < 0)
    {
        if (tv_add_bookmark(pos) >= 0)
            rb->splash(HZ/2, "Bookmark add");
        else
            rb->splash(HZ/2, "No more add bookmark");
        return;
    }
    tv_remove_bookmark(idx);
    rb->splash(HZ/2, "Bookmark remove");
}

void tv_create_system_bookmark(void)
{
    const struct tv_screen_pos *pos = tv_get_screen_pos();
    int idx = tv_find_bookmark(pos);

    if (idx >= 0)
        bookmarks[idx].flag |= TV_BOOKMARK_SYSTEM;
    else
    {
        bookmarks[bookmark_count].pos  = *pos;
        bookmarks[bookmark_count].flag = TV_BOOKMARK_SYSTEM;
        bookmark_count++;
    }
}

static const char* get_bookmark_name(int selected, void * data,
                                     char * buffer, size_t buffer_len)
{
    (void)data;
    struct tv_bookmark_info *bookmark = &bookmarks[selected];
    rb->snprintf(buffer, buffer_len,
#ifdef HAVE_LCD_BITMAP
                 "%cPage: %d  Line: %d",
#else
                 "%cP:%d  L:%d",
#endif
                 (bookmark->flag & TV_BOOKMARK_SYSTEM)? '*' : ' ',
                 bookmark->pos.page + 1, bookmark->pos.line + 1);
    return buffer;
}

static int list_action_callback(int action, struct gui_synclist *lists)
{
    (void) lists;
    if (action == ACTION_STD_OK)
        return ACTION_STD_CANCEL;
    return action;
}

void tv_select_bookmark(void)
{
    int i;
    struct tv_screen_pos select_pos;

    for (i = 0; i < bookmark_count; i++)
    {
        if (bookmarks[i].flag & TV_BOOKMARK_SYSTEM)
            break;
    }

    /* if does not find the system bookmark, add the system bookmark. */
    if (i >= bookmark_count)
        tv_create_system_bookmark();

    if (bookmark_count == 1)
        select_pos = bookmarks[0].pos;
    else
    {
        struct simplelist_info info;

        rb->qsort(bookmarks, bookmark_count, sizeof(struct tv_bookmark_info), bm_comp);

        rb->simplelist_info_init(&info, "Select bookmark",
                                 bookmark_count, bookmarks);
        info.get_name = get_bookmark_name;
        info.action_callback = list_action_callback;
        rb->simplelist_show_list(&info);

        if (info.selection >= 0 && info.selection < bookmark_count)
            select_pos = bookmarks[info.selection].pos;
        else
        {
            /* when does not select any bookmarks, move to the current page */
            tv_copy_screen_pos(&select_pos);

            if (select_pos.file_pos == 0)
                rb->splash(HZ, "Start the first page");
            else
                rb->splash(HZ, "Return to the current page");
        }
    }

    /* deletes the system bookmark */
    for (i = 0; i < bookmark_count; i++)
    {
        if ((bookmarks[i].flag &= TV_BOOKMARK_USER) == 0)
        {
            tv_remove_bookmark(i);
            break;
        }
    }

    /* move to the select position */
    if (preferences->vertical_scroll_mode == VS_PAGE)
        select_pos.line = 0;

    tv_move_screen(select_pos.page, select_pos.line, SEEK_SET);
}

/* serialize or deserialize of the bookmark array */
static bool tv_read_bookmark_info(int fd, struct tv_bookmark_info *b)
{
    unsigned char buf[SERIALIZE_BOOKMARK_SIZE];

    if (rb->read(fd, buf, sizeof(buf)) < 0)
        return false;

    b->pos.file_pos = (buf[0] << 24)|(buf[1] << 16)|(buf[2] << 8)|buf[3];
    b->pos.page     = (buf[4] << 8)|buf[5];
    b->pos.line     = buf[6];
    b->flag         = buf[7];

    return true;
}

bool tv_deserialize_bookmarks(int fd)
{
    int i;
    bool res = true;

    if (rb->read(fd, &bookmark_count, 1) < 0)
        return false;

    for (i = 0; i < bookmark_count; i++)
    {
        if (!tv_read_bookmark_info(fd, &bookmarks[i]))
        {
            res = false;
            break;
        }
    }

    bookmark_count = i;
    return res;
}

static void tv_write_bookmark_info(unsigned char *p, const struct tv_bookmark_info *b)
{
    *p++ = b->pos.file_pos >> 24;
    *p++ = b->pos.file_pos >> 16;
    *p++ = b->pos.file_pos >> 8;
    *p++ = b->pos.file_pos;

    *p++ = b->pos.page >> 8;
    *p++ = b->pos.page;

    *p++ = b->pos.line;
    *p   = b->flag;
}

int tv_serialize_bookmarks(unsigned char *buf)
{
    int i;

    buf[0] = bookmark_count;

    for (i = 0; i < bookmark_count; i++)
    {
        tv_write_bookmark_info(buf + i * SERIALIZE_BOOKMARK_SIZE + 1, &bookmarks[i]);
    }
    return i * SERIALIZE_BOOKMARK_SIZE + 1;
}
